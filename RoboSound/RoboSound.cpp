#include "RoboSound.hpp"

#include "RoboMemory.hpp"
#include "RoboPci.hpp"

namespace {

constexpr uint32_t ROBO_SOUND_SAMPLES = 4096u;

struct RoboSoundBufferEntry {
    uint64_t address;
    uint32_t length;
    uint32_t flags;
};

uint8_t* gRegisters = nullptr;
alignas(128) int16_t gSamples[ROBO_SOUND_SAMPLES];
alignas(128) RoboSoundBufferEntry gBufferList[1];
bool gSoundReady = false;
uint32_t gSampleCount = 0;

uint8_t* RoboSoundBar(uint8_t bus, uint8_t slot) {
    uint32_t bar = RoboPciRead32(bus, slot, 0, 0x10);
    if ((bar & 1u) != 0) {
        return nullptr;
    }
    uint64_t address = bar & ~0xFull;
    if ((bar & 4u) != 0) {
        uint32_t high = RoboPciRead32(bus, slot, 0, 0x14);
        address |= static_cast<uint64_t>(high) << 32;
    }
    if (address == 0 || address >= 0x100000000ull) {
        return nullptr;
    }
    RoboMemoryMarkUncached(address, 0x4000ull);
    return reinterpret_cast<uint8_t*>(address);
}

}  // namespace

bool RoboSoundStart(void) {
    gSoundReady = false;
    gRegisters = nullptr;
    gSampleCount = 0;
    for (uint32_t bus = 0; bus < 8u; ++bus) {
        for (uint32_t slot = 0; slot < 32u; ++slot) {
            uint32_t identity = RoboPciRead32(static_cast<uint8_t>(bus), static_cast<uint8_t>(slot), 0, 0);
            if (identity == 0 || identity == 0xFFFFFFFFu) {
                continue;
            }
            uint32_t classCode = RoboPciRead32(static_cast<uint8_t>(bus), static_cast<uint8_t>(slot), 0, 0x08);
            if (((classCode >> 24) & 0xFFu) != 0x04u) {
                continue;
            }
            uint32_t command = RoboPciRead32(static_cast<uint8_t>(bus), static_cast<uint8_t>(slot), 0, 0x04);
            RoboPciWrite32(static_cast<uint8_t>(bus), static_cast<uint8_t>(slot), 0, 0x04, command | 0x06u);
            uint8_t* registers = RoboSoundBar(static_cast<uint8_t>(bus), static_cast<uint8_t>(slot));
            if (registers == nullptr) {
                return false;
            }
            volatile uint32_t* control = reinterpret_cast<volatile uint32_t*>(registers + 0x08);
            *control = 0;
            for (uint32_t spin = 0; spin < 2000u && (*control & 1u) != 0; ++spin) {
            }
            *control = 1;
            bool awake = false;
            for (uint32_t spin = 0; spin < 2000u; ++spin) {
                if ((*control & 1u) != 0) {
                    awake = true;
                    break;
                }
            }
            if (!awake) {
                return false;
            }
            gBufferList[0].address = reinterpret_cast<uint64_t>(gSamples);
            gBufferList[0].length = sizeof(gSamples);
            gBufferList[0].flags = 0;
            volatile uint32_t* stream = reinterpret_cast<volatile uint32_t*>(registers + 0x80);
            stream[2] = sizeof(gSamples);
            stream[6] = static_cast<uint32_t>(reinterpret_cast<uint64_t>(gBufferList));
            stream[7] = static_cast<uint32_t>(reinterpret_cast<uint64_t>(gBufferList) >> 32);
            *reinterpret_cast<volatile uint16_t*>(registers + 0x92) = 0x0011u;
            stream[0] = 0x00100002u;
            gRegisters = registers;
            gSoundReady = true;
            return true;
        }
    }
    return false;
}

bool RoboSoundWrite(const int16_t* samples, uint32_t count) {
    if (!gSoundReady || samples == nullptr || count == 0 || gRegisters == nullptr) {
        return false;
    }
    uint32_t copy = count;
    if (copy > ROBO_SOUND_SAMPLES) {
        copy = ROBO_SOUND_SAMPLES;
    }
    for (uint32_t index = 0; index < copy; ++index) {
        gSamples[index] = samples[index];
    }
    gSampleCount = copy;
    volatile uint32_t* stream = reinterpret_cast<volatile uint32_t*>(gRegisters + 0x80);
    stream[2] = copy * static_cast<uint32_t>(sizeof(int16_t));
    stream[0] = 0x00100002u;
    return true;
}
