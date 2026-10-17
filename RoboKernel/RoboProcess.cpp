#include "RoboProcess.hpp"

extern "C" volatile uint8_t gRoboDiskReady = 0;
extern "C" volatile uint8_t gRoboLaunchChosen = 0;

#include "RoboCall.hpp"
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
char gLaunchNames[8][48];
int gLaunchCount = 0;
int gLaunchIndex = 0;
char gLaunchPath[96];
bool gLaunchPathReady = false;

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

void RoboLauncherScan(void) {
    char listed[8][48]{};
    const int rawCount = RoboFsList("Applications", &listed[0][0], 48, 8);
    gLaunchCount = 0;
    gLaunchIndex = 0;
    gLaunchPathReady = false;
    gLaunchPath[0] = 0;
    // Native programs only — Compiler *.elf apps open from the desktop icon path.
    for (int index = 0; index < rawCount && gLaunchCount < 8; ++index) {
        const char* name = listed[index];
        bool elf = false;
        for (int ch = 0; name[ch] != 0; ++ch) {
            if (name[ch] == '.' && (name[ch + 1] == 'e' || name[ch + 1] == 'E') &&
                (name[ch + 2] == 'l' || name[ch + 2] == 'L') && (name[ch + 3] == 'f' || name[ch + 3] == 'F') &&
                name[ch + 4] == 0) {
                elf = true;
                break;
            }
        }
        if (elf) {
            continue;
        }
        for (int ch = 0; ch < 48; ++ch) {
            gLaunchNames[gLaunchCount][ch] = name[ch];
        }
        gLaunchCount += 1;
    }
    if (gLaunchCount <= 0) {
        return;
    }
    const char* prefer =
#if defined(__aarch64__)
        "ARM.bin";
#else
        "X64.bin";
#endif
    for (int index = 0; index < gLaunchCount; ++index) {
        const char* name = gLaunchNames[index];
        bool match = true;
        for (int ch = 0; prefer[ch] != 0; ++ch) {
            if (name[ch] != prefer[ch]) {
                match = false;
                break;
            }
        }
        if (match) {
            gLaunchIndex = index;
            break;
        }
    }
    const char* chosen = gLaunchNames[gLaunchIndex];
    const char* prefix = "Applications/";
    int at = 0;
    for (int index = 0; prefix[index] != 0 && at < 90; ++index) {
        gLaunchPath[at++] = prefix[index];
    }
    for (int index = 0; index < 40 && chosen[index] != 0 && at < 95; ++index) {
        gLaunchPath[at++] = chosen[index];
    }
    gLaunchPath[at] = 0;
    gLaunchPathReady = at > 13;
}

bool RoboLauncherSelect(int index) {
    if (gLaunchCount <= 0) {
        RoboLauncherScan();
    }
    if (index < 0 || index >= gLaunchCount) {
        return false;
    }
    gLaunchIndex = index;
    const char* chosen = gLaunchNames[gLaunchIndex];
    const char* prefix = "Applications/";
    int at = 0;
    for (int cursor = 0; prefix[cursor] != 0 && at < 90; ++cursor) {
        gLaunchPath[at++] = prefix[cursor];
    }
    for (int cursor = 0; cursor < 40 && chosen[cursor] != 0 && at < 95; ++cursor) {
        gLaunchPath[at++] = chosen[cursor];
    }
    gLaunchPath[at] = 0;
    gLaunchPathReady = at > 13;
    return gLaunchPathReady;
}

int RoboLauncherCount(void) {
    return gLaunchCount;
}

const char* RoboLauncherName(int index) {
    if (index < 0 || index >= gLaunchCount) {
        return "";
    }
    return gLaunchNames[index];
}

bool RoboProcessLoad(RoboUserMap* map) {
    if (map == nullptr) {
        return false;
    }
    const char* path = gLaunchPathReady ? gLaunchPath : ROBO_PROGRAM_PATH;
    uint32_t size = 0;
    if (!RoboFsReadFile(path, gProgram, sizeof(gProgram), &size) || size < ROBO_PROGRAM_HEADER) {
        if (path != ROBO_PROGRAM_PATH &&
            (!RoboFsReadFile(ROBO_PROGRAM_PATH, gProgram, sizeof(gProgram), &size) || size < ROBO_PROGRAM_HEADER)) {
            return false;
        }
        if (path == ROBO_PROGRAM_PATH) {
            return false;
        }
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
    RoboCallFocus(codeBytes, heapBytes);
    return true;
}

void RoboProcessNote(uint32_t reason) {
    if (reason == 2u) {
        gExitCount += 1u;
    } else if (reason == 3u) {
        gFaultCount += 1u;
    }
}

