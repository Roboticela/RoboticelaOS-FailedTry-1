#include "RoboPci.hpp"
#include "RoboMemory.hpp"
#include "RoboPartition.hpp"

namespace {

constexpr uint32_t ROBO_AHCI_PORT_LIMIT = 8u;
constexpr uint32_t ROBO_AHCI_SECTOR_LIMIT = 32u;

struct RoboAhciPort {
    uint64_t base;
    uint32_t index;
};

alignas(1024) uint8_t gCommandList[1024];
alignas(256) uint8_t gReceived[256];
alignas(128) uint8_t gCommand[256];
RoboAhciPort gPorts[ROBO_AHCI_PORT_LIMIT];
uint32_t gPortCount = 0;
int gOpen = -1;

uint32_t RoboAhciRead(uint64_t address) {
    return *reinterpret_cast<volatile uint32_t*>(address);
}

void RoboAhciWrite(uint64_t address, uint32_t value) {
    *reinterpret_cast<volatile uint32_t*>(address) = value;
}

void RoboAhciFence(void) {
#if defined(__aarch64__)
    __asm__ volatile("dsb sy" ::: "memory");
#else
    __asm__ volatile("mfence" ::: "memory");
#endif
}

void RoboAhciClean(const void* address, uint32_t bytes) {
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
    RoboAhciFence();
}

bool RoboAhciWait(uint64_t command, uint32_t bit, bool clear) {
    for (uint32_t spin = 0; spin < 1000000u; ++spin) {
        uint32_t value = RoboAhciRead(command);
        bool set = (value & bit) != 0;
        if (set != clear) {
            return true;
        }
    }
    return false;
}

bool RoboAhciClaim(uint32_t index) {
    if (index >= gPortCount) {
        return false;
    }
    if (gOpen == static_cast<int>(index)) {
        return true;
    }
    uint64_t port = gPorts[index].base;
    uint64_t command = port + 0x18;
    uint32_t cmd = RoboAhciRead(command);
    if ((cmd & 1u) != 0) {
        RoboAhciWrite(command, cmd & ~1u);
        if (!RoboAhciWait(command, 1u << 15, true)) {
            return false;
        }
        cmd = RoboAhciRead(command);
    }
    if ((cmd & (1u << 4)) != 0) {
        RoboAhciWrite(command, cmd & ~(1u << 4));
        if (!RoboAhciWait(command, 1u << 14, true)) {
            return false;
        }
    }
    for (uint32_t byte = 0; byte < sizeof(gCommandList); ++byte) {
        gCommandList[byte] = 0;
    }
    for (uint32_t byte = 0; byte < sizeof(gReceived); ++byte) {
        gReceived[byte] = 0;
    }
    uint64_t list = reinterpret_cast<uint64_t>(gCommandList);
    uint64_t fis = reinterpret_cast<uint64_t>(gReceived);
    RoboAhciWrite(port + 0x00, static_cast<uint32_t>(list));
    RoboAhciWrite(port + 0x04, static_cast<uint32_t>(list >> 32));
    RoboAhciWrite(port + 0x08, static_cast<uint32_t>(fis));
    RoboAhciWrite(port + 0x0C, static_cast<uint32_t>(fis >> 32));
    RoboAhciWrite(port + 0x10, 0xFFFFFFFFu);
    cmd = RoboAhciRead(command);
    RoboAhciWrite(command, cmd | (1u << 4) | 1u);
    gOpen = static_cast<int>(index);
    return true;
}

}  // namespace

uint32_t RoboAhciCount(void) {
    gPortCount = 0;
    gOpen = -1;
    for (uint32_t bus = 0; bus < 16u && gPortCount == 0; ++bus) {
        for (uint32_t slot = 0; slot < 32u && gPortCount == 0; ++slot) {
            uint32_t identity = RoboPciRead32(static_cast<uint8_t>(bus), static_cast<uint8_t>(slot), 0, 0);
            if ((identity & 0xFFFFu) == 0xFFFFu) {
                continue;
            }
            uint32_t classCode = RoboPciRead32(static_cast<uint8_t>(bus), static_cast<uint8_t>(slot), 0, 0x08);
            if (((classCode >> 24) & 0xFFu) != 0x01u || ((classCode >> 16) & 0xFFu) != 0x06u) {
                continue;
            }
            uint32_t bar = RoboPciRead32(static_cast<uint8_t>(bus), static_cast<uint8_t>(slot), 0, 0x24);
            if ((bar & 1u) != 0) {
                continue;
            }
            uint64_t abar = bar & ~0xFull;
            if ((bar & 4u) != 0) {
                uint32_t high = RoboPciRead32(static_cast<uint8_t>(bus), static_cast<uint8_t>(slot), 0, 0x28);
                abar |= static_cast<uint64_t>(high) << 32;
            }
            if (abar == 0 || abar >= 0x100000000ull) {
                continue;
            }
            uint32_t command = RoboPciRead32(static_cast<uint8_t>(bus), static_cast<uint8_t>(slot), 0, 0x04);
            RoboPciWrite32(static_cast<uint8_t>(bus), static_cast<uint8_t>(slot), 0, 0x04, command | 0x06u);
            RoboMemoryMarkUncached(abar, 0x2000ull);
            uint32_t ghc = RoboAhciRead(abar + 4);
            RoboAhciWrite(abar + 4, ghc | 0x80000000u);
            uint32_t implemented = RoboAhciRead(abar + 0x0C);
            for (uint32_t port = 0; port < 32u && gPortCount < ROBO_AHCI_PORT_LIMIT; ++port) {
                if ((implemented & (1u << port)) == 0) {
                    continue;
                }
                uint64_t base = abar + 0x100ull + port * 0x80ull;
                uint32_t status = RoboAhciRead(base + 0x28);
                if ((status & 0x0Fu) != 3u) {
                    continue;
                }
                gPorts[gPortCount].base = base;
                gPorts[gPortCount].index = port;
                gPortCount += 1;
            }
        }
    }
    return gPortCount;
}

bool RoboAhciReadSectors(uint32_t index, uint32_t lba, uint32_t count, uint8_t* dest) {
    if (dest == nullptr || count == 0 || count > ROBO_AHCI_SECTOR_LIMIT || !RoboAhciClaim(index)) {
        return false;
    }
    uint64_t port = gPorts[index].base;
    if (RoboAhciRead(port + 0x38) != 0) {
        return false;
    }
    for (uint32_t byte = 0; byte < sizeof(gCommand); ++byte) {
        gCommand[byte] = 0;
    }
    gCommand[0x00] = 0x27;
    gCommand[0x01] = 0x80;
    gCommand[0x02] = 0x25;
    gCommand[0x04] = static_cast<uint8_t>(lba);
    gCommand[0x05] = static_cast<uint8_t>(lba >> 8);
    gCommand[0x06] = static_cast<uint8_t>(lba >> 16);
    gCommand[0x07] = 0x40;
    gCommand[0x08] = static_cast<uint8_t>(lba >> 24);
    gCommand[0x0C] = static_cast<uint8_t>(count);
    gCommand[0x0D] = static_cast<uint8_t>(count >> 8);
    uint64_t buffer = reinterpret_cast<uint64_t>(dest);
    gCommand[0x80] = static_cast<uint8_t>(buffer);
    gCommand[0x81] = static_cast<uint8_t>(buffer >> 8);
    gCommand[0x82] = static_cast<uint8_t>(buffer >> 16);
    gCommand[0x83] = static_cast<uint8_t>(buffer >> 24);
    gCommand[0x84] = static_cast<uint8_t>(buffer >> 32);
    gCommand[0x85] = static_cast<uint8_t>(buffer >> 40);
    gCommand[0x86] = static_cast<uint8_t>(buffer >> 48);
    gCommand[0x87] = static_cast<uint8_t>(buffer >> 56);
    uint32_t bytes = count * ROBO_SECTOR_BYTES;
    uint32_t dbc = (bytes - 1u) | 0x80000000u;
    gCommand[0x8C] = static_cast<uint8_t>(dbc);
    gCommand[0x8D] = static_cast<uint8_t>(dbc >> 8);
    gCommand[0x8E] = static_cast<uint8_t>(dbc >> 16);
    gCommand[0x8F] = static_cast<uint8_t>(dbc >> 24);
    uint32_t* header = reinterpret_cast<uint32_t*>(gCommandList);
    header[0] = (1u << 16) | 5u;
    header[1] = 0;
    uint64_t table = reinterpret_cast<uint64_t>(gCommand);
    header[2] = static_cast<uint32_t>(table);
    header[3] = static_cast<uint32_t>(table >> 32);
    RoboAhciClean(dest, bytes);
    RoboAhciClean(gCommand, sizeof(gCommand));
    RoboAhciClean(gCommandList, 32);
    RoboAhciWrite(port + 0x38, 1u);
    for (uint32_t spin = 0; spin < 1000000u; ++spin) {
        if (RoboAhciRead(port + 0x38) == 0) {
            uint32_t task = RoboAhciRead(port + 0x20);
            return (task & 0x81u) == 0;
        }
    }
    gOpen = -1;
    return false;
}
