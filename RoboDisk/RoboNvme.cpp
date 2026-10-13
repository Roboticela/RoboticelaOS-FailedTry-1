#include "RoboPci.hpp"
#include "RoboMemory.hpp"
#include "RoboPartition.hpp"

namespace {

constexpr uint32_t ROBO_NVME_LIMIT = 2u;
constexpr uint32_t ROBO_NVME_QUEUE = 4u;
constexpr uint32_t ROBO_NVME_SECTOR_LIMIT = 8u;

struct RoboNvmeController {
    uint64_t base;
    uint32_t doorStride;
};

struct RoboNvmeQueue {
    uint8_t* submission;
    uint8_t* completion;
    uint64_t submissionDoor;
    uint64_t completionDoor;
    uint16_t head;
    uint16_t tail;
    uint16_t phase;
};

alignas(4096) uint8_t gAdminSubmission[ROBO_NVME_QUEUE * 64u];
alignas(4096) uint8_t gAdminCompletion[ROBO_NVME_QUEUE * 16u];
alignas(4096) uint8_t gIoSubmission[ROBO_NVME_QUEUE * 64u];
alignas(4096) uint8_t gIoCompletion[ROBO_NVME_QUEUE * 16u];
alignas(4096) uint8_t gData[ROBO_NVME_SECTOR_LIMIT * ROBO_SECTOR_BYTES];
RoboNvmeController gControllers[ROBO_NVME_LIMIT];
uint32_t gControllerCount = 0;
int gOpen = -1;
uint16_t gCommandId = 1;
RoboNvmeQueue gAdmin{};
RoboNvmeQueue gIo{};

void RoboNvmeStore16(uint8_t* bytes, uint16_t value) {
    bytes[0] = static_cast<uint8_t>(value);
    bytes[1] = static_cast<uint8_t>(value >> 8);
}

void RoboNvmeStore32(uint8_t* bytes, uint32_t value) {
    bytes[0] = static_cast<uint8_t>(value);
    bytes[1] = static_cast<uint8_t>(value >> 8);
    bytes[2] = static_cast<uint8_t>(value >> 16);
    bytes[3] = static_cast<uint8_t>(value >> 24);
}

void RoboNvmeStore64(uint8_t* bytes, uint64_t value) {
    RoboNvmeStore32(bytes, static_cast<uint32_t>(value));
    RoboNvmeStore32(bytes + 4, static_cast<uint32_t>(value >> 32));
}

uint32_t RoboNvmeRead32(uint64_t address) {
    return *reinterpret_cast<volatile uint32_t*>(address);
}

void RoboNvmeWrite32(uint64_t address, uint32_t value) {
    *reinterpret_cast<volatile uint32_t*>(address) = value;
}

void RoboNvmeFence(void) {
#if defined(__aarch64__)
    __asm__ volatile("dsb sy" ::: "memory");
#else
    __asm__ volatile("mfence" ::: "memory");
#endif
}

void RoboNvmeClean(const void* address, uint32_t bytes) {
    uintptr_t line = reinterpret_cast<uintptr_t>(address) & ~static_cast<uintptr_t>(63);
    uintptr_t end = reinterpret_cast<uintptr_t>(address) + bytes;
    while (line < end) {
#if defined(__aarch64__)
        __asm__ volatile("dc civac, %0" : : "r"(line) : "memory");
#else
        __asm__ volatile("clflush (%0)" : : "r"(line) : "memory");
#endif
        line += 64u;
    }
    RoboNvmeFence();
}

bool RoboNvmeWait(uint64_t status, uint32_t mask, uint32_t value) {
    for (uint32_t spin = 0; spin < 2000000u; ++spin) {
        if ((RoboNvmeRead32(status) & mask) == value) {
            return true;
        }
    }
    return false;
}

void RoboNvmeClear(uint8_t* bytes, uint32_t count) {
    for (uint32_t index = 0; index < count; ++index) {
        bytes[index] = 0;
    }
}

bool RoboNvmeSubmit(RoboNvmeQueue* queue, const uint8_t command[64]) {
    uint8_t* slot = queue->submission + static_cast<uint32_t>(queue->tail) * 64u;
    for (uint32_t index = 0; index < 64u; ++index) {
        slot[index] = command[index];
    }
    RoboNvmeClean(slot, 64);
    queue->tail = static_cast<uint16_t>((queue->tail + 1u) % ROBO_NVME_QUEUE);
    RoboNvmeWrite32(queue->submissionDoor, queue->tail);
    uint8_t* completion = queue->completion + static_cast<uint32_t>(queue->head) * 16u;
    uint16_t status = 0xFFFF;
    bool finished = false;
    for (uint32_t spin = 0; spin < 2000000u; ++spin) {
        RoboNvmeClean(completion, 16);
        status = static_cast<uint16_t>(completion[14] | (completion[15] << 8));
        if ((status & 1u) == queue->phase) {
            finished = true;
            break;
        }
    }
    if (!finished) {
        return false;
    }
    queue->head = static_cast<uint16_t>((queue->head + 1u) % ROBO_NVME_QUEUE);
    if (queue->head == 0) {
        queue->phase = static_cast<uint16_t>(queue->phase ^ 1u);
    }
    RoboNvmeWrite32(queue->completionDoor, queue->head);
    return (status & 0xFFFEu) == 0;
}

bool RoboNvmeCommand(RoboNvmeQueue* queue, uint8_t opcode, uint32_t nsid, uint64_t prp, uint32_t cdw10, uint32_t cdw11, uint32_t cdw12) {
    uint8_t command[64];
    RoboNvmeClear(command, 64);
    command[0] = opcode;
    RoboNvmeStore16(command + 2, gCommandId);
    gCommandId = static_cast<uint16_t>(gCommandId + 1u);
    if (gCommandId == 0) {
        gCommandId = 1;
    }
    RoboNvmeStore32(command + 4, nsid);
    RoboNvmeStore64(command + 24, prp);
    RoboNvmeStore32(command + 40, cdw10);
    RoboNvmeStore32(command + 44, cdw11);
    RoboNvmeStore32(command + 48, cdw12);
    return RoboNvmeSubmit(queue, command);
}

bool RoboNvmeClaim(uint32_t index) {
    if (index >= gControllerCount) {
        return false;
    }
    if (gOpen == static_cast<int>(index)) {
        return true;
    }
    uint64_t base = gControllers[index].base;
    uint32_t stride = gControllers[index].doorStride;
    uint32_t command = RoboNvmeRead32(base + 0x14);
    RoboNvmeWrite32(base + 0x14, command & ~1u);
    if (!RoboNvmeWait(base + 0x1C, 1u, 0)) {
        return false;
    }
    RoboNvmeClear(gAdminSubmission, sizeof(gAdminSubmission));
    RoboNvmeClear(gAdminCompletion, sizeof(gAdminCompletion));
    RoboNvmeClear(gIoSubmission, sizeof(gIoSubmission));
    RoboNvmeClear(gIoCompletion, sizeof(gIoCompletion));
    RoboNvmeClean(gAdminSubmission, sizeof(gAdminSubmission));
    RoboNvmeClean(gAdminCompletion, sizeof(gAdminCompletion));
    RoboNvmeClean(gIoSubmission, sizeof(gIoSubmission));
    RoboNvmeClean(gIoCompletion, sizeof(gIoCompletion));
    uint32_t depth = ROBO_NVME_QUEUE - 1u;
    RoboNvmeWrite32(base + 0x24, depth | (depth << 16));
    uint64_t adminSubmission = reinterpret_cast<uint64_t>(gAdminSubmission);
    uint64_t adminCompletion = reinterpret_cast<uint64_t>(gAdminCompletion);
    RoboNvmeWrite32(base + 0x28, static_cast<uint32_t>(adminSubmission));
    RoboNvmeWrite32(base + 0x2C, static_cast<uint32_t>(adminSubmission >> 32));
    RoboNvmeWrite32(base + 0x30, static_cast<uint32_t>(adminCompletion));
    RoboNvmeWrite32(base + 0x34, static_cast<uint32_t>(adminCompletion >> 32));
    RoboNvmeWrite32(base + 0x14, 1u | (6u << 16) | (4u << 20));
    if (!RoboNvmeWait(base + 0x1C, 3u, 1u)) {
        return false;
    }
    gAdmin.submission = gAdminSubmission;
    gAdmin.completion = gAdminCompletion;
    gAdmin.submissionDoor = base + 0x1000ull;
    gAdmin.completionDoor = base + 0x1000ull + stride;
    gAdmin.head = 0;
    gAdmin.tail = 0;
    gAdmin.phase = 1;
    gIo.submission = gIoSubmission;
    gIo.completion = gIoCompletion;
    gIo.submissionDoor = base + 0x1000ull + 2u * stride;
    gIo.completionDoor = base + 0x1000ull + 3u * stride;
    gIo.head = 0;
    gIo.tail = 0;
    gIo.phase = 1;
    uint64_t ioCompletion = reinterpret_cast<uint64_t>(gIoCompletion);
    uint64_t ioSubmission = reinterpret_cast<uint64_t>(gIoSubmission);
    uint32_t queueWord = (depth << 16) | 1u;
    if (!RoboNvmeCommand(&gAdmin, 0x05, 0, ioCompletion, queueWord, 1u, 0)) {
        return false;
    }
    if (!RoboNvmeCommand(&gAdmin, 0x01, 0, ioSubmission, queueWord, 1u | (1u << 16), 0)) {
        return false;
    }
    gOpen = static_cast<int>(index);
    return true;
}

}  // namespace

uint32_t RoboNvmeCount(void) {
    gControllerCount = 0;
    gOpen = -1;
    for (uint32_t bus = 0; bus < 16u && gControllerCount < ROBO_NVME_LIMIT; ++bus) {
        for (uint32_t slot = 0; slot < 32u && gControllerCount < ROBO_NVME_LIMIT; ++slot) {
            uint32_t identity = RoboPciRead32(static_cast<uint8_t>(bus), static_cast<uint8_t>(slot), 0, 0);
            if ((identity & 0xFFFFu) == 0xFFFFu) {
                continue;
            }
            uint32_t classCode = RoboPciRead32(static_cast<uint8_t>(bus), static_cast<uint8_t>(slot), 0, 0x08);
            if (((classCode >> 24) & 0xFFu) != 0x01u || ((classCode >> 16) & 0xFFu) != 0x08u) {
                continue;
            }
            uint32_t bar = RoboPciRead32(static_cast<uint8_t>(bus), static_cast<uint8_t>(slot), 0, 0x10);
            if ((bar & 1u) != 0) {
                continue;
            }
            uint64_t base = bar & ~0xFull;
            bool wide = (bar & 4u) != 0;
            if (wide) {
                uint32_t high = RoboPciRead32(static_cast<uint8_t>(bus), static_cast<uint8_t>(slot), 0, 0x14);
                base |= static_cast<uint64_t>(high) << 32;
            }
            if (base == 0 || base >= 0x100000000ull) {
                constexpr uint32_t ROBO_NVME_BAR = 0xFEA00000u;
                uint32_t command = RoboPciRead32(static_cast<uint8_t>(bus), static_cast<uint8_t>(slot), 0, 0x04);
                RoboPciWrite32(static_cast<uint8_t>(bus), static_cast<uint8_t>(slot), 0, 0x04, command & ~0x7u);
                RoboPciWrite32(static_cast<uint8_t>(bus), static_cast<uint8_t>(slot), 0, 0x10, ROBO_NVME_BAR | (bar & 0xFu));
                if (wide) {
                    RoboPciWrite32(static_cast<uint8_t>(bus), static_cast<uint8_t>(slot), 0, 0x14, 0);
                }
                base = ROBO_NVME_BAR;
            }
            uint32_t command = RoboPciRead32(static_cast<uint8_t>(bus), static_cast<uint8_t>(slot), 0, 0x04);
            RoboPciWrite32(static_cast<uint8_t>(bus), static_cast<uint8_t>(slot), 0, 0x04, command | 0x06u);
            RoboMemoryMarkUncached(base, 0x4000ull);
            uint32_t capHigh = RoboNvmeRead32(base + 4);
            uint32_t strideShift = capHigh & 0xFu;
            uint32_t pageMin = (RoboNvmeRead32(base + 4) >> 16) & 0xFu;
            if (pageMin != 0 || strideShift > 8u) {
                continue;
            }
            gControllers[gControllerCount].base = base;
            gControllers[gControllerCount].doorStride = 4u << strideShift;
            gControllerCount += 1;
        }
    }
    return gControllerCount;
}

bool RoboNvmeReadSectors(uint32_t index, uint32_t lba, uint32_t count, uint8_t* dest) {
    if (dest == nullptr || count == 0 || count > ROBO_NVME_SECTOR_LIMIT || !RoboNvmeClaim(index)) {
        return false;
    }
    if (!RoboNvmeCommand(&gIo, 0x02, 1u, reinterpret_cast<uint64_t>(gData), lba, 0, count - 1u)) {
        gOpen = -1;
        return false;
    }
    RoboNvmeClean(gData, count * ROBO_SECTOR_BYTES);
    for (uint32_t indexByte = 0; indexByte < count * ROBO_SECTOR_BYTES; ++indexByte) {
        dest[indexByte] = gData[indexByte];
    }
    return true;
}

bool RoboNvmeWriteSectors(uint32_t index, uint32_t lba, uint32_t count, const uint8_t* source) {
    if (source == nullptr || count == 0 || count > ROBO_NVME_SECTOR_LIMIT || !RoboNvmeClaim(index)) {
        return false;
    }
    uint32_t bytes = count * ROBO_SECTOR_BYTES;
    for (uint32_t indexByte = 0; indexByte < bytes; ++indexByte) {
        gData[indexByte] = source[indexByte];
    }
    RoboNvmeClean(gData, bytes);
    if (!RoboNvmeCommand(&gIo, 0x01, 1u, reinterpret_cast<uint64_t>(gData), lba, 0, count - 1u)) {
        gOpen = -1;
        return false;
    }
    return true;
}
