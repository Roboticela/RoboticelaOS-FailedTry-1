#include "Robo/RoboScene.hpp"
#include "Robo/TitleBar.hpp"
#include "Robo/Heading.hpp"
#include "Robo/Paragraph.hpp"
#include "Robo/Text.hpp"
#include "RoboBootInfo.hpp"
#include "RoboFS.hpp"
#include "RoboShell.hpp"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

namespace {

typedef void (*RoboAppBuildFn)(Robo::RoboScene&);
typedef int (*RoboAppBuildStepFn)(Robo::RoboScene&, int);

struct RoboAppExports {
    RoboAppBuildFn build;
    RoboAppBuildStepFn step;
};

struct RoboBoundApp {
    char id[48]{};
    RoboAppBuildFn build = nullptr;
    RoboAppBuildStepFn step = nullptr;
};

RoboBoundApp gBoundApp{};
bool gDesktopAppsInstalled = false;

constexpr uint32_t ROBO_ELF_PT_LOAD = 1u;
constexpr uint16_t ROBO_ELF_MACHINE_X64 = 62u;
// TestApp.elf is ~0.5 MiB; keep the read buffer bounded so open cannot stall on a giant alloc.
constexpr uint32_t ROBO_APP_FILE_MAX = 2u * 1024u * 1024u;

struct RoboElf64Ehdr {
    uint8_t ident[16];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint64_t entry;
    uint64_t phOff;
    uint64_t shOff;
    uint32_t flags;
    uint16_t ehSize;
    uint16_t phEntSize;
    uint16_t phNum;
    uint16_t shEntSize;
    uint16_t shNum;
    uint16_t shStrNdx;
};

struct RoboElf64Phdr {
    uint32_t type;
    uint32_t flags;
    uint64_t offset;
    uint64_t vAddr;
    uint64_t pAddr;
    uint64_t fileSize;
    uint64_t memSize;
    uint64_t align;
};

void RoboAppPump() {
    RoboShellPumpPaint();
}

bool RoboEndsWithIgnoreCase(const char* name, const char* suffix) {
    if (name == nullptr || suffix == nullptr) {
        return false;
    }
    std::size_t nameLen = 0;
    std::size_t suffixLen = 0;
    while (name[nameLen] != '\0' && nameLen < 64u) {
        ++nameLen;
    }
    while (suffix[suffixLen] != '\0' && suffixLen < 16u) {
        ++suffixLen;
    }
    if (suffixLen == 0 || nameLen < suffixLen) {
        return false;
    }
    for (std::size_t i = 0; i < suffixLen; ++i) {
        char a = name[nameLen - suffixLen + i];
        char b = suffix[i];
        if (a >= 'A' && a <= 'Z') {
            a = static_cast<char>(a - 'A' + 'a');
        }
        if (b >= 'A' && b <= 'Z') {
            b = static_cast<char>(b - 'A' + 'a');
        }
        if (a != b) {
            return false;
        }
    }
    return true;
}

void RoboCopyId(char* dest, std::size_t destSize, const char* src) {
    if (dest == nullptr || destSize == 0) {
        return;
    }
    dest[0] = '\0';
    if (src == nullptr) {
        return;
    }
    for (std::size_t i = 0; i + 1u < destSize && src[i] != '\0'; ++i) {
        dest[i] = src[i];
        dest[i + 1u] = '\0';
    }
}

void RoboIdFromFileName(char* dest, std::size_t destSize, const char* fileName) {
    if (dest == nullptr || destSize == 0) {
        return;
    }
    dest[0] = '\0';
    if (fileName == nullptr) {
        return;
    }
    std::size_t length = 0;
    while (fileName[length] != '\0' && length < 40u) {
        ++length;
    }
    if (length >= 4u && RoboEndsWithIgnoreCase(fileName, ".elf")) {
        length -= 4u;
    }
    for (std::size_t i = 0; i < length && i + 1u < destSize; ++i) {
        dest[i] = fileName[i];
        dest[i + 1u] = '\0';
    }
}

bool RoboIdsMatch(const char* left, const char* right) {
    if (left == nullptr || right == nullptr) {
        return false;
    }
    for (std::size_t i = 0; i < 48u; ++i) {
        char a = left[i];
        char b = right[i];
        if (a >= 'A' && a <= 'Z') {
            a = static_cast<char>(a - 'A' + 'a');
        }
        if (b >= 'A' && b <= 'Z') {
            b = static_cast<char>(b - 'A' + 'a');
        }
        if (a != b) {
            return false;
        }
        if (a == '\0') {
            return true;
        }
    }
    return true;
}

void RoboAppStubPage(Robo::RoboScene& scene, const char* appId) {
    scene.SetPagePadding(0.0f);
    scene.SetFoundation("RoboFoundation");
    Robo::TitleBar titleBar;
    titleBar.SetTitle(appId != nullptr ? appId : "App");
    scene.AddTitleBar(std::move(titleBar));
    Robo::Heading heading(1);
    heading.AddPlain(appId != nullptr ? appId : "App");
    scene.AddText(std::move(heading));
    Robo::Paragraph body;
    body.AddPlain("App image was not loaded from Applications.");
    scene.AddText(std::move(body));
}

void RoboCopyChunked(uint8_t* dest, const uint8_t* src, uint64_t bytes) {
    constexpr uint64_t kPumpChunk = 64u * 1024u;
    uint64_t written = 0;
    while (written < bytes) {
        uint64_t chunk = bytes - written;
        if (chunk > kPumpChunk) {
            chunk = kPumpChunk;
        }
        memcpy(dest + written, src + written, static_cast<size_t>(chunk));
        written += chunk;
        RoboAppPump();
    }
}

bool RoboAppMapElf(const uint8_t* image, uint32_t imageSize) {
    if (image == nullptr || imageSize < sizeof(RoboElf64Ehdr)) {
        return false;
    }
    const RoboElf64Ehdr* header = reinterpret_cast<const RoboElf64Ehdr*>(image);
    if (header->ident[0] != 0x7F || header->ident[1] != 'E' || header->ident[2] != 'L' || header->ident[3] != 'F') {
        return false;
    }
    if (header->ident[4] != 2 || header->machine != ROBO_ELF_MACHINE_X64) {
        return false;
    }
    if (header->phOff + static_cast<uint64_t>(header->phNum) * header->phEntSize > imageSize ||
        header->phEntSize < sizeof(RoboElf64Phdr)) {
        return false;
    }
    for (uint16_t index = 0; index < header->phNum; ++index) {
        const RoboElf64Phdr* program = reinterpret_cast<const RoboElf64Phdr*>(
            image + header->phOff + static_cast<uint64_t>(index) * header->phEntSize);
        if (program->type != ROBO_ELF_PT_LOAD || program->memSize == 0) {
            continue;
        }
        if (program->offset + program->fileSize > imageSize || program->fileSize > program->memSize) {
            return false;
        }
        if (program->vAddr < ROBO_APP_LOAD_ADDRESS ||
            program->vAddr + program->memSize > ROBO_APP_LOAD_ADDRESS + ROBO_APP_BYTES) {
            return false;
        }
        uint8_t* dest = reinterpret_cast<uint8_t*>(static_cast<uintptr_t>(program->vAddr));
        RoboCopyChunked(dest, image + program->offset, program->fileSize);
        if (program->memSize > program->fileSize) {
            memset(dest + program->fileSize, 0, static_cast<size_t>(program->memSize - program->fileSize));
        }
        RoboAppPump();
    }
    return true;
}

bool RoboAppBindMapped(const char* appId) {
    const RoboAppExports* exports =
        reinterpret_cast<const RoboAppExports*>(static_cast<uintptr_t>(ROBO_APP_LOAD_ADDRESS));
    if (exports->build == nullptr && exports->step == nullptr) {
        return false;
    }
    gBoundApp = {};
    RoboCopyId(gBoundApp.id, sizeof(gBoundApp.id), appId);
    gBoundApp.build = exports->build;
    gBoundApp.step = exports->step;
    return true;
}

bool RoboAppLoadFromApplications(const char* appId) {
    if (appId == nullptr || appId[0] == '\0') {
        return false;
    }
    if (gBoundApp.step != nullptr && RoboIdsMatch(gBoundApp.id, appId)) {
        return true;
    }
    char path[96]{};
    const char* prefix = "Applications/";
    std::size_t at = 0;
    for (; prefix[at] != '\0' && at + 1u < sizeof(path); ++at) {
        path[at] = prefix[at];
    }
    for (std::size_t i = 0; appId[i] != '\0' && at + 5u < sizeof(path); ++i) {
        path[at++] = appId[i];
    }
    path[at++] = '.';
    path[at++] = 'e';
    path[at++] = 'l';
    path[at++] = 'f';
    path[at] = '\0';

    uint8_t* file = static_cast<uint8_t*>(malloc(ROBO_APP_FILE_MAX));
    if (file == nullptr) {
        return false;
    }
    RoboAppPump();
    uint32_t size = 0;
    bool ok = RoboFsReadFile(path, file, ROBO_APP_FILE_MAX, &size) && size > 0;
    RoboAppPump();
    if (ok) {
        ok = RoboAppMapElf(file, size) && RoboAppBindMapped(appId);
    }
    free(file);
    RoboAppPump();
    return ok;
}

}  // namespace

extern "C" void RoboAppBind(const RoboBootInfo* info) {
    gBoundApp = {};
    gDesktopAppsInstalled = false;
    if (info == nullptr) {
        return;
    }
    for (uint32_t index = 0; index < info->fileCount; ++index) {
        const RoboBootFile* file = &info->files[index];
        if (file->address == 0 || !RoboEndsWithIgnoreCase(file->name, ".elf")) {
            continue;
        }
        const RoboAppExports* exports =
            reinterpret_cast<const RoboAppExports*>(static_cast<uintptr_t>(file->address));
        if (exports->build == nullptr && exports->step == nullptr) {
            continue;
        }
        RoboIdFromFileName(gBoundApp.id, sizeof(gBoundApp.id), file->name);
        gBoundApp.build = exports->build;
        gBoundApp.step = exports->step;
        return;
    }
}

void RoboAppInstallDesktopIcons(Robo::RoboScene& scene) {
    if (gDesktopAppsInstalled) {
        return;
    }
    char names[16][48]{};
    const int count = RoboFsList("Applications", &names[0][0], 48, 16);
    int placed = 0;
    for (int index = 0; index < count; ++index) {
        const char* name = names[index];
        if (!RoboEndsWithIgnoreCase(name, ".elf")) {
            continue;
        }
        char id[48]{};
        RoboIdFromFileName(id, sizeof(id), name);
        if (id[0] == '\0') {
            continue;
        }
        const float x = 24.0f + static_cast<float>(placed % 6) * 120.0f;
        const float y = 24.0f + static_cast<float>(placed / 6) * 124.0f;
        scene.InstallAppIcon(id, id, x, y);
        ++placed;
    }
    gDesktopAppsInstalled = true;
}

int RoboAppBuildStep(Robo::RoboScene& scene, int step, const char* appId) {
    if (appId == nullptr || appId[0] == '\0') {
        return -1;
    }
    if ((gBoundApp.step == nullptr && gBoundApp.build == nullptr) || !RoboIdsMatch(gBoundApp.id, appId)) {
        if (!RoboAppLoadFromApplications(appId)) {
            if (step == 0) {
                RoboAppStubPage(scene, appId);
            }
            return -1;
        }
    }
    if (gBoundApp.step != nullptr) {
        return gBoundApp.step(scene, step);
    }
    if (gBoundApp.build != nullptr) {
        if (step == 0) {
            gBoundApp.build(scene);
        }
        return -1;
    }
    if (step == 0) {
        RoboAppStubPage(scene, appId);
    }
    return -1;
}
