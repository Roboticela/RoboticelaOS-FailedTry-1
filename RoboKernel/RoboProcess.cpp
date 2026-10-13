#include "RoboProcess.hpp"

#include "RoboFS.hpp"
#include "RoboMemory.hpp"
#include "RoboProgram.hpp"

namespace {

#if defined(__aarch64__)
constexpr uint32_t ROBO_PROGRAM_ARCH = ROBO_PROGRAM_ARM64;
constexpr const char* ROBO_PROGRAM_PATH = "Applications/ARM.bin";
#else
constexpr uint32_t ROBO_PROGRAM_ARCH = ROBO_PROGRAM_X64;
constexpr const char* ROBO_PROGRAM_PATH = "Applications/X64.bin";
#endif

alignas(16) uint8_t gProgram[ROBO_PROGRAM_CODE_PAGES * ROBO_PROGRAM_PAGE];
uint32_t gExitCount = 0;
uint32_t gFaultCount = 0;

uint32_t RoboLoad32(const uint8_t* bytes) {
    return static_cast<uint32_t>(bytes[0])
        | (static_cast<uint32_t>(bytes[1]) << 8)
        | (static_cast<uint32_t>(bytes[2]) << 16)
        | (static_cast<uint32_t>(bytes[3]) << 24);
}

bool RoboMagic(const uint8_t* bytes) {
    for (uint32_t index = 0; index < 8u; ++index) {
        if (bytes[index] != static_cast<uint8_t>(ROBO_PROGRAM_MAGIC[index])) {
            return false;
        }
    }
    return true;
}

}  // namespace

bool RoboProcessLoad(RoboUserMap* map) {
    if (map == nullptr) {
        return false;
    }
    uint32_t size = 0;
    if (!RoboFsReadFile(ROBO_PROGRAM_PATH, gProgram, sizeof(gProgram), &size) || size < ROBO_PROGRAM_HEADER) {
        return false;
    }
    if (!RoboMagic(gProgram) || RoboLoad32(gProgram + 8) != ROBO_PROGRAM_ARCH) {
        return false;
    }
    uint32_t entry = RoboLoad32(gProgram + 12);
    uint32_t codeBytes = RoboLoad32(gProgram + 16);
    uint32_t heapBytes = RoboLoad32(gProgram + 20);
    if (codeBytes == 0 || entry >= codeBytes || heapBytes == 0) {
        return false;
    }
    if (ROBO_PROGRAM_HEADER + codeBytes > size) {
        return false;
    }
    if (!RoboMemoryMapUser(gProgram + ROBO_PROGRAM_HEADER, codeBytes, heapBytes, map)) {
        return false;
    }
    map->rip += entry;
    return true;
}

void RoboProcessNote(uint32_t reason) {
    if (reason == 2u) {
        gExitCount += 1u;
    } else if (reason == 3u) {
        gFaultCount += 1u;
    }
}

