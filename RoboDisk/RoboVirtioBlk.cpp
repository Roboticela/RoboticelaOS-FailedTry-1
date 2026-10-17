#include "RoboPci.hpp"
#include "RoboMemory.hpp"
#include "RoboPartition.hpp"

namespace {

constexpr uint32_t ROBO_VIRTIO_VENDOR = 0x1AF4u;
constexpr uint32_t ROBO_VIRTIO_QUEUE = 256u;
constexpr uint32_t ROBO_VIRTIO_DEVICE_LIMIT = 4u;
constexpr uint32_t ROBO_VIRTIO_SECTOR_LIMIT = 16u;

struct RoboVqDesc {
    uint64_t address;
    uint32_t length;
    uint16_t flags;
    uint16_t next;
};

struct RoboVqUsedElem {
    uint32_t id;
    uint32_t length;
};

struct RoboBlkWhere {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint8_t commonBar;
    uint8_t notifyBar;
    uint32_t commonOffset;
    uint32_t notifyOffset;
    uint32_t notifyMultiply;
};

struct RoboVqAvail {
    uint16_t flags;
    uint16_t index;
    uint16_t ring[ROBO_VIRTIO_QUEUE];
};

struct RoboVqUsed {
    uint16_t flags;
    uint16_t index;
    RoboVqUsedElem ring[ROBO_VIRTIO_QUEUE];
};

alignas(16) RoboVqDesc gDesc[8];
alignas(2) RoboVqAvail gAvail{};
alignas(4) RoboVqUsed gUsed{};
alignas(8) uint8_t gHeader[16];
uint8_t gStatus = 0xFF;
RoboBlkWhere gWhere[ROBO_VIRTIO_DEVICE_LIMIT];
uint32_t gWhereCount = 0;
int gOpen = -1;
uint8_t* gCommon = nullptr;
uint8_t* gNotify = nullptr;
uint32_t gNotifyMultiply = 0;
uint16_t gQueueSize = 0;
uint16_t gNotifyOff = 0;
uint16_t gUsedSeen = 0;

void RoboDiskPause(void) {
#if defined(__aarch64__)
    __asm__ volatile("yield");
#else
    __asm__ volatile("pause");
#endif
}

void RoboDiskFence(void) {
#if defined(__aarch64__)
    __asm__ volatile("dsb sy" ::: "memory");
#else
    __asm__ volatile("mfence" ::: "memory");
#endif
}

void RoboDiskClean(const void* address, uint32_t bytes) {
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
    RoboDiskFence();
}

uint8_t* RoboVirtioBar(const RoboBlkWhere* where, uint8_t barIndex, uint32_t offset) {
    uint32_t bar = RoboPciRead32(where->bus, where->device, where->function, static_cast<uint8_t>(0x10u + barIndex * 4u));
    if ((bar & 1u) != 0) {
        return nullptr;
    }
    uint64_t address = bar & ~0xFull;
    if ((bar & 4u) != 0) {
        uint32_t high = RoboPciRead32(where->bus, where->device, where->function, static_cast<uint8_t>(0x14u + barIndex * 4u));
        address |= static_cast<uint64_t>(high) << 32;
    }
    if (address == 0 || address >= 0x100000000ull) {
        return nullptr;
    }
    RoboMemoryMarkUncached(address, 0x4000ull);
    return reinterpret_cast<uint8_t*>(address + offset);
}

bool RoboVirtioOpen(uint32_t index) {
    if (index >= gWhereCount) {
        return false;
    }
    if (gOpen == static_cast<int>(index) && gCommon != nullptr) {
        return true;
    }
    const RoboBlkWhere* where = &gWhere[index];
    uint32_t command = RoboPciRead32(where->bus, where->device, where->function, 0x04);
    RoboPciWrite32(where->bus, where->device, where->function, 0x04, command | 0x06u);
    gCommon = RoboVirtioBar(where, where->commonBar, where->commonOffset);
    gNotify = RoboVirtioBar(where, where->notifyBar, where->notifyOffset);
    gNotifyMultiply = where->notifyMultiply;
    if (gCommon == nullptr || gNotify == nullptr) {
        gOpen = -1;
        return false;
    }
    gCommon[20] = 0;
    for (uint32_t spin = 0; spin < 4000u && gCommon[20] != 0; ++spin) {
        RoboDiskPause();
    }
    if (gCommon[20] != 0) {
        gOpen = -1;
        return false;
    }
    gCommon[20] = 1;
    gCommon[20] = 3;
    *reinterpret_cast<volatile uint32_t*>(gCommon) = 1;
    uint32_t high = *reinterpret_cast<volatile uint32_t*>(gCommon + 4);
    *reinterpret_cast<volatile uint32_t*>(gCommon + 8) = 0;
    *reinterpret_cast<volatile uint32_t*>(gCommon + 12) = 0;
    *reinterpret_cast<volatile uint32_t*>(gCommon + 8) = 1;
    *reinterpret_cast<volatile uint32_t*>(gCommon + 12) = high & 1u;
    gCommon[20] = 11;
    if ((gCommon[20] & 8u) == 0) {
        gOpen = -1;
        return false;
    }
    *reinterpret_cast<volatile uint16_t*>(gCommon + 22) = 0;
    gQueueSize = *reinterpret_cast<volatile uint16_t*>(gCommon + 24);
    if (gQueueSize < 4u || gQueueSize > ROBO_VIRTIO_QUEUE) {
        gOpen = -1;
        return false;
    }
    gNotifyOff = *reinterpret_cast<volatile uint16_t*>(gCommon + 30);
    gAvail.flags = 0;
    gAvail.index = 0;
    gUsed.flags = 0;
    gUsed.index = 0;
    gUsedSeen = 0;
    uint64_t descAddress = reinterpret_cast<uint64_t>(gDesc);
    uint64_t availAddress = reinterpret_cast<uint64_t>(&gAvail);
    uint64_t usedAddress = reinterpret_cast<uint64_t>(&gUsed);
    RoboDiskClean(gDesc, sizeof(gDesc));
    RoboDiskClean(&gAvail, sizeof(gAvail));
    RoboDiskClean(&gUsed, sizeof(gUsed));
    *reinterpret_cast<volatile uint32_t*>(gCommon + 32) = static_cast<uint32_t>(descAddress);
    *reinterpret_cast<volatile uint32_t*>(gCommon + 36) = static_cast<uint32_t>(descAddress >> 32);
    *reinterpret_cast<volatile uint32_t*>(gCommon + 40) = static_cast<uint32_t>(availAddress);
    *reinterpret_cast<volatile uint32_t*>(gCommon + 44) = static_cast<uint32_t>(availAddress >> 32);
    *reinterpret_cast<volatile uint32_t*>(gCommon + 48) = static_cast<uint32_t>(usedAddress);
    *reinterpret_cast<volatile uint32_t*>(gCommon + 52) = static_cast<uint32_t>(usedAddress >> 32);
    *reinterpret_cast<volatile uint16_t*>(gCommon + 28) = 1;
    gCommon[20] = 15;
    gOpen = static_cast<int>(index);
    return true;
}

}  // namespace

uint32_t RoboVirtioBlkCount(void) {
    gWhereCount = 0;
    gOpen = -1;
    for (uint32_t bus = 0; bus < 16u && gWhereCount < ROBO_VIRTIO_DEVICE_LIMIT; ++bus) {
        for (uint32_t slot = 0; slot < 32u && gWhereCount < ROBO_VIRTIO_DEVICE_LIMIT; ++slot) {
            uint32_t identity = RoboPciRead32(static_cast<uint8_t>(bus), static_cast<uint8_t>(slot), 0, 0);
            if ((identity & 0xFFFFu) != ROBO_VIRTIO_VENDOR) {
                continue;
            }
            uint32_t classCode = RoboPciRead32(static_cast<uint8_t>(bus), static_cast<uint8_t>(slot), 0, 0x08);
            if (((classCode >> 24) & 0xFFu) != 0x01u) {
                continue;
            }
            RoboBlkWhere* where = &gWhere[gWhereCount];
            where->bus = static_cast<uint8_t>(bus);
            where->device = static_cast<uint8_t>(slot);
            where->function = 0;
            where->commonBar = 0xFF;
            where->notifyBar = 0xFF;
            where->commonOffset = 0;
            where->notifyOffset = 0;
            where->notifyMultiply = 0;
            uint8_t next = static_cast<uint8_t>(RoboPciRead32(where->bus, where->device, 0, 0x34) & 0xFFu);
            while (next >= 0x40u) {
                uint32_t first = RoboPciRead32(where->bus, where->device, 0, next);
                uint32_t second = RoboPciRead32(where->bus, where->device, 0, static_cast<uint8_t>(next + 4));
                uint32_t third = RoboPciRead32(where->bus, where->device, 0, static_cast<uint8_t>(next + 8));
                uint8_t vendorCap = static_cast<uint8_t>(first);
                uint8_t capNext = static_cast<uint8_t>(first >> 8);
                uint8_t capKind = static_cast<uint8_t>(first >> 24);
                if (vendorCap == 0x09u && capKind == 1u) {
                    where->commonBar = static_cast<uint8_t>(second);
                    where->commonOffset = third;
                } else if (vendorCap == 0x09u && capKind == 2u) {
                    where->notifyBar = static_cast<uint8_t>(second);
                    where->notifyOffset = third;
                    where->notifyMultiply = RoboPciRead32(where->bus, where->device, 0, static_cast<uint8_t>(next + 16));
                }
                next = capNext;
            }
            if (where->commonBar != 0xFFu && where->notifyBar != 0xFFu) {
                gWhereCount += 1;
            }
        }
    }
    return gWhereCount;
}

bool RoboVirtioBlkRead(uint32_t index, uint32_t lba, uint32_t count, uint8_t* dest) {
    if (dest == nullptr || count == 0 || count > ROBO_VIRTIO_SECTOR_LIMIT || !RoboVirtioOpen(index)) {
        return false;
    }
    gHeader[0] = 0;
    gHeader[1] = 0;
    gHeader[2] = 0;
    gHeader[3] = 0;
    gHeader[4] = 0;
    gHeader[5] = 0;
    gHeader[6] = 0;
    gHeader[7] = 0;
    gHeader[8] = static_cast<uint8_t>(lba);
    gHeader[9] = static_cast<uint8_t>(lba >> 8);
    gHeader[10] = static_cast<uint8_t>(lba >> 16);
    gHeader[11] = static_cast<uint8_t>(lba >> 24);
    gHeader[12] = 0;
    gHeader[13] = 0;
    gHeader[14] = 0;
    gHeader[15] = 0;
    gStatus = 0xFF;
    RoboDiskClean(gHeader, sizeof(gHeader));
    RoboDiskClean(dest, count * ROBO_SECTOR_BYTES);
    RoboDiskClean(&gStatus, 1);
    gDesc[0].address = reinterpret_cast<uint64_t>(gHeader);
    gDesc[0].length = sizeof(gHeader);
    gDesc[0].flags = 1;
    gDesc[0].next = 1;
    gDesc[1].address = reinterpret_cast<uint64_t>(dest);
    gDesc[1].length = count * ROBO_SECTOR_BYTES;
    gDesc[1].flags = 1u | 2u;
    gDesc[1].next = 2;
    gDesc[2].address = reinterpret_cast<uint64_t>(&gStatus);
    gDesc[2].length = 1;
    gDesc[2].flags = 2;
    gDesc[2].next = 0;
    uint16_t slot = static_cast<uint16_t>(gAvail.index % gQueueSize);
    gAvail.ring[slot] = 0;
    RoboDiskClean(gDesc, sizeof(gDesc));
    RoboDiskFence();
    gAvail.index = static_cast<uint16_t>(gAvail.index + 1u);
    RoboDiskClean(&gAvail, sizeof(gAvail));
    *reinterpret_cast<volatile uint16_t*>(gNotify + static_cast<uint32_t>(gNotifyOff) * gNotifyMultiply) = 0;
    for (uint32_t spin = 0; spin < 4000u; ++spin) {
        RoboDiskFence();
        if (gUsed.index != gUsedSeen) {
            gUsedSeen = gUsed.index;
            RoboDiskClean(dest, count * ROBO_SECTOR_BYTES);
            RoboDiskClean(&gStatus, 1);
            return gStatus == 0;
        }
        RoboDiskPause();
    }
    gOpen = -1;
    return false;
}

bool RoboVirtioBlkWrite(uint32_t index, uint32_t lba, uint32_t count, const uint8_t* source) {
    if (source == nullptr || count == 0 || count > ROBO_VIRTIO_SECTOR_LIMIT || !RoboVirtioOpen(index)) {
        return false;
    }
    gHeader[0] = 1;
    gHeader[1] = 0;
    gHeader[2] = 0;
    gHeader[3] = 0;
    gHeader[4] = 0;
    gHeader[5] = 0;
    gHeader[6] = 0;
    gHeader[7] = 0;
    gHeader[8] = static_cast<uint8_t>(lba);
    gHeader[9] = static_cast<uint8_t>(lba >> 8);
    gHeader[10] = static_cast<uint8_t>(lba >> 16);
    gHeader[11] = static_cast<uint8_t>(lba >> 24);
    gHeader[12] = 0;
    gHeader[13] = 0;
    gHeader[14] = 0;
    gHeader[15] = 0;
    gStatus = 0xFF;
    RoboDiskClean(gHeader, sizeof(gHeader));
    RoboDiskClean(source, count * ROBO_SECTOR_BYTES);
    RoboDiskClean(&gStatus, 1);
    gDesc[0].address = reinterpret_cast<uint64_t>(gHeader);
    gDesc[0].length = sizeof(gHeader);
    gDesc[0].flags = 1;
    gDesc[0].next = 1;
    gDesc[1].address = reinterpret_cast<uint64_t>(source);
    gDesc[1].length = count * ROBO_SECTOR_BYTES;
    gDesc[1].flags = 1;
    gDesc[1].next = 2;
    gDesc[2].address = reinterpret_cast<uint64_t>(&gStatus);
    gDesc[2].length = 1;
    gDesc[2].flags = 2;
    gDesc[2].next = 0;
    uint16_t slot = static_cast<uint16_t>(gAvail.index % gQueueSize);
    gAvail.ring[slot] = 0;
    RoboDiskClean(gDesc, sizeof(gDesc));
    RoboDiskFence();
    gAvail.index = static_cast<uint16_t>(gAvail.index + 1u);
    RoboDiskClean(&gAvail, sizeof(gAvail));
    *reinterpret_cast<volatile uint16_t*>(gNotify + static_cast<uint32_t>(gNotifyOff) * gNotifyMultiply) = 0;
    for (uint32_t spin = 0; spin < 4000u; ++spin) {
        RoboDiskFence();
        if (gUsed.index != gUsedSeen) {
            gUsedSeen = gUsed.index;
            RoboDiskClean(&gStatus, 1);
            return gStatus == 0;
        }
        RoboDiskPause();
    }
    gOpen = -1;
    return false;
}
