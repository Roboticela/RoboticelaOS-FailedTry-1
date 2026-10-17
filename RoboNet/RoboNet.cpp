#include "RoboNet.hpp"

#include "RoboMemory.hpp"
#include "RoboPci.hpp"

namespace {

constexpr uint32_t ROBO_VIRTIO_VENDOR = 0x1AF4u;
constexpr uint32_t ROBO_NET_QUEUE = 16u;

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

struct RoboVqAvail {
    uint16_t flags;
    uint16_t index;
    uint16_t ring[ROBO_NET_QUEUE];
};

struct RoboVqUsed {
    uint16_t flags;
    uint16_t index;
    RoboVqUsedElem ring[ROBO_NET_QUEUE];
};

alignas(16) RoboVqDesc gTxDesc[4];
alignas(2) RoboVqAvail gTxAvail{};
alignas(4) RoboVqUsed gTxUsed{};
alignas(16) uint8_t gTxPacket[2048];
uint8_t* gCommon = nullptr;
uint8_t* gNotify = nullptr;
uint32_t gNotifyMultiply = 0;
uint16_t gTxNotify = 0;
uint16_t gTxSeen = 0;
bool gNetReady = false;
uint8_t gBus = 0;
uint8_t gSlot = 0;

void RoboNetPause(void) {
#if defined(__aarch64__)
    __asm__ volatile("yield");
#else
    __asm__ volatile("pause");
#endif
}

void RoboNetFence(void) {
#if defined(__aarch64__)
    __asm__ volatile("dsb sy" ::: "memory");
#else
    __asm__ volatile("mfence" ::: "memory");
#endif
}

void RoboNetClean(const void* address, uint32_t bytes) {
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
    RoboNetFence();
}

uint8_t* RoboNetBar(uint8_t barIndex, uint32_t offset) {
    uint32_t bar = RoboPciRead32(gBus, gSlot, 0, static_cast<uint8_t>(0x10u + barIndex * 4u));
    if ((bar & 1u) != 0) {
        return nullptr;
    }
    uint64_t address = bar & ~0xFull;
    if ((bar & 4u) != 0) {
        uint32_t high = RoboPciRead32(gBus, gSlot, 0, static_cast<uint8_t>(0x14u + barIndex * 4u));
        address |= static_cast<uint64_t>(high) << 32;
    }
    if (address == 0 || address >= 0x100000000ull) {
        return nullptr;
    }
    RoboMemoryMarkUncached(address, 0x4000ull);
    return reinterpret_cast<uint8_t*>(address + offset);
}

bool RoboNetOpen(uint8_t commonBar, uint32_t commonOffset, uint8_t notifyBar, uint32_t notifyOffset, uint32_t notifyMultiply) {
    uint32_t command = RoboPciRead32(gBus, gSlot, 0, 0x04);
    RoboPciWrite32(gBus, gSlot, 0, 0x04, command | 0x06u);
    gCommon = RoboNetBar(commonBar, commonOffset);
    gNotify = RoboNetBar(notifyBar, notifyOffset);
    gNotifyMultiply = notifyMultiply;
    if (gCommon == nullptr || gNotify == nullptr) {
        return false;
    }
    gCommon[20] = 0;
    for (uint32_t spin = 0; spin < 2000u && gCommon[20] != 0; ++spin) {
        RoboNetPause();
    }
    if (gCommon[20] != 0) {
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
        return false;
    }
    *reinterpret_cast<volatile uint16_t*>(gCommon + 22) = 1;
    uint16_t queueSize = *reinterpret_cast<volatile uint16_t*>(gCommon + 24);
    if (queueSize < 2u) {
        return false;
    }
    gTxNotify = *reinterpret_cast<volatile uint16_t*>(gCommon + 30);
    gTxAvail.flags = 0;
    gTxAvail.index = 0;
    gTxUsed.flags = 0;
    gTxUsed.index = 0;
    gTxSeen = 0;
    uint64_t descAddress = reinterpret_cast<uint64_t>(gTxDesc);
    uint64_t availAddress = reinterpret_cast<uint64_t>(&gTxAvail);
    uint64_t usedAddress = reinterpret_cast<uint64_t>(&gTxUsed);
    RoboNetClean(gTxDesc, sizeof(gTxDesc));
    RoboNetClean(&gTxAvail, sizeof(gTxAvail));
    RoboNetClean(&gTxUsed, sizeof(gTxUsed));
    *reinterpret_cast<volatile uint32_t*>(gCommon + 32) = static_cast<uint32_t>(descAddress);
    *reinterpret_cast<volatile uint32_t*>(gCommon + 36) = static_cast<uint32_t>(descAddress >> 32);
    *reinterpret_cast<volatile uint32_t*>(gCommon + 40) = static_cast<uint32_t>(availAddress);
    *reinterpret_cast<volatile uint32_t*>(gCommon + 44) = static_cast<uint32_t>(availAddress >> 32);
    *reinterpret_cast<volatile uint32_t*>(gCommon + 48) = static_cast<uint32_t>(usedAddress);
    *reinterpret_cast<volatile uint32_t*>(gCommon + 52) = static_cast<uint32_t>(usedAddress >> 32);
    *reinterpret_cast<volatile uint16_t*>(gCommon + 28) = 1;
    gCommon[20] = 15;
    return (gCommon[20] & 8u) != 0;
}

int RoboNetTransmit(const uint8_t* packet, uint32_t bytes) {
    if (!gNetReady || packet == nullptr || bytes == 0 || bytes > 1514u || gNotify == nullptr) {
        return -1;
    }
    for (uint32_t index = 0; index < 12u; ++index) {
        gTxPacket[index] = 0;
    }
    for (uint32_t index = 0; index < bytes; ++index) {
        gTxPacket[12u + index] = packet[index];
    }
    uint32_t length = bytes + 12u;
    RoboNetClean(gTxPacket, length);
    gTxDesc[0].address = reinterpret_cast<uint64_t>(gTxPacket);
    gTxDesc[0].length = length;
    gTxDesc[0].flags = 0;
    gTxDesc[0].next = 0;
    uint16_t slot = gTxAvail.index % ROBO_NET_QUEUE;
    gTxAvail.ring[slot] = 0;
    RoboNetClean(&gTxAvail, sizeof(gTxAvail));
    gTxAvail.index = static_cast<uint16_t>(gTxAvail.index + 1u);
    RoboNetFence();
    uint32_t notifyAt = gTxNotify * gNotifyMultiply;
    *reinterpret_cast<volatile uint16_t*>(gNotify + notifyAt) = 1;
    for (uint32_t spin = 0; spin < 2000u; ++spin) {
        RoboNetFence();
        if (gTxUsed.index != gTxSeen) {
            gTxSeen = gTxUsed.index;
            return static_cast<int>(bytes);
        }
        RoboNetPause();
    }
    return -1;
}

}  // namespace

bool RoboNetStart(void) {
    gNetReady = false;
    gCommon = nullptr;
    for (uint32_t bus = 0; bus < 8u; ++bus) {
        for (uint32_t slot = 0; slot < 32u; ++slot) {
            uint32_t identity = RoboPciRead32(static_cast<uint8_t>(bus), static_cast<uint8_t>(slot), 0, 0);
            if ((identity & 0xFFFFu) != ROBO_VIRTIO_VENDOR) {
                continue;
            }
            uint32_t classCode = RoboPciRead32(static_cast<uint8_t>(bus), static_cast<uint8_t>(slot), 0, 0x08);
            if (((classCode >> 24) & 0xFFu) != 0x02u) {
                continue;
            }
            gBus = static_cast<uint8_t>(bus);
            gSlot = static_cast<uint8_t>(slot);
            uint8_t commonBar = 0xFF;
            uint8_t notifyBar = 0xFF;
            uint32_t commonOffset = 0;
            uint32_t notifyOffset = 0;
            uint32_t notifyMultiply = 0;
            uint8_t next = static_cast<uint8_t>(RoboPciRead32(gBus, gSlot, 0, 0x34) & 0xFFu);
            while (next >= 0x40u) {
                uint32_t first = RoboPciRead32(gBus, gSlot, 0, next);
                uint32_t second = RoboPciRead32(gBus, gSlot, 0, static_cast<uint8_t>(next + 4));
                uint32_t third = RoboPciRead32(gBus, gSlot, 0, static_cast<uint8_t>(next + 8));
                uint8_t vendorCap = static_cast<uint8_t>(first);
                uint8_t capNext = static_cast<uint8_t>(first >> 8);
                uint8_t capKind = static_cast<uint8_t>(first >> 24);
                if (vendorCap == 0x09u && capKind == 1u) {
                    commonBar = static_cast<uint8_t>(second);
                    commonOffset = third;
                } else if (vendorCap == 0x09u && capKind == 2u) {
                    notifyBar = static_cast<uint8_t>(second);
                    notifyOffset = third;
                    notifyMultiply = RoboPciRead32(gBus, gSlot, 0, static_cast<uint8_t>(next + 16));
                }
                next = capNext;
            }
            if (commonBar == 0xFFu || notifyBar == 0xFFu) {
                continue;
            }
            gNetReady = RoboNetOpen(commonBar, commonOffset, notifyBar, notifyOffset, notifyMultiply);
            return gNetReady;
        }
    }
    return false;
}

int RoboUdpSend(const uint8_t* packet, uint32_t bytes) {
    return RoboNetTransmit(packet, bytes);
}

int RoboUdpReceive(uint8_t* packet, uint32_t capacity) {
    (void)packet;
    (void)capacity;
    return gNetReady ? 0 : -1;
}

int RoboTcpSend(const uint8_t* packet, uint32_t bytes) {
    return RoboNetTransmit(packet, bytes);
}

int RoboTcpReceive(uint8_t* packet, uint32_t capacity) {
    (void)packet;
    (void)capacity;
    return gNetReady ? 0 : -1;
}
