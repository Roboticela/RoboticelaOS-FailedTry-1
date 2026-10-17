#pragma once

#include "IPlatform.hpp"
#include "RoboScene.hpp"
#include "Sidebar.hpp"

#include <cstdint>

namespace Robo {

// One catalog row. Loaded from ROBOCAT.PAK or passed as a page table.
struct RoboCatalogEntry {
    const char* id = nullptr;
    const char* label = nullptr;
    const char* icon = nullptr;
    int parent = -1;
    bool section = false;
    bool selected = false;
};

// Loads ROBOCAT.PAK through the platform asset path. Safe to call more than once.
void RoboCatalogLoad(const IPlatform& platform);

// Copies one catalog image into the row table (boot-held or asset bytes).
void RoboCatalogAdd(const std::uint8_t* data, std::uint32_t size);

const RoboCatalogEntry* RoboCatalogRows();
int RoboCatalogCount();

int RoboCountCatalogRails(const RoboCatalogEntry* rows, int count);
int RoboDefaultCatalogRail(const RoboCatalogEntry* rows, int count);
int RoboBuildCatalogRails(Sidebar& sidebar, const RoboCatalogEntry* rows, int count, int railStart, int railBudget);
bool RoboEnsureCatalogChildren(Sidebar& sidebar, int railIndex, const RoboCatalogEntry* rows, int count);

// Shared DevKit / catalog page helpers (keeps generated pages chrome-only).
Sidebar* RoboFindSidebar(RoboScene& scene);
void RoboEnsureActivitySidebar(RoboScene& scene);
int RoboBuildSidebarRailsStep(RoboScene& scene, int railStart, int railBudget);
void RoboBuildSidebarFromCatalog(RoboScene& scene);
void RoboApplyDevKitChrome(RoboScene& scene);

}  // namespace Robo
