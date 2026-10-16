#include "Robo/RoboScene.hpp"
#include "Robo/TitleBar.hpp"
#include "Robo/Heading.hpp"
#include "Robo/Paragraph.hpp"
#include "Robo/Text.hpp"
#include "RoboBootInfo.hpp"

#include <stdint.h>
#include <string.h>

namespace {

typedef void (*RoboDevKitBuildFn)(Robo::RoboScene&);

RoboDevKitBuildFn gDevKitBuild = nullptr;

bool RoboNameMatch(const char* name, const char* want) {
    if (name == nullptr || want == nullptr) {
        return false;
    }
    for (uint32_t index = 0; index < ROBO_BOOT_FILE_NAME; ++index) {
        if (name[index] != want[index]) {
            return false;
        }
        if (want[index] == 0) {
            return true;
        }
    }
    return true;
}

bool RoboNameIsDevKit(const char* name) {
    return RoboNameMatch(name, "DEVKIT.ELF") || RoboNameMatch(name, "DevKit.elf");
}

}  // namespace

extern "C" void RoboDevKitBind(const RoboBootInfo* info) {
    gDevKitBuild = nullptr;
    if (info == nullptr) {
        return;
    }
    for (uint32_t index = 0; index < info->fileCount; ++index) {
        const RoboBootFile* file = &info->files[index];
        if (!RoboNameIsDevKit(file->name) || file->address == 0) {
            continue;
        }
        gDevKitBuild = reinterpret_cast<RoboDevKitBuildFn>(static_cast<uintptr_t>(file->address));
        return;
    }
}

void RoboDevKitOpenPage(Robo::RoboScene& scene) {
    if (gDevKitBuild != nullptr) {
        gDevKitBuild(scene);
        return;
    }
    scene.SetPagePadding(0.0f);
    scene.SetFoundation("RoboFoundation");
    Robo::TitleBar titleBar;
    titleBar.SetTitle("DevKit");
    scene.AddTitleBar(std::move(titleBar));
    Robo::Heading heading(1);
    heading.AddPlain("DevKit");
    scene.AddText(std::move(heading));
    Robo::Paragraph body;
    body.AddPlain("DevKit.elf was not loaded. Rebuild and copy DevKit.elf onto the ESP.");
    scene.AddText(std::move(body));
}
