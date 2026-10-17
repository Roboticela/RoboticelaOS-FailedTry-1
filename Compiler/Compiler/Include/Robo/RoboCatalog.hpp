#pragma once

#include "Sidebar.hpp"

namespace Robo {

// One catalog row. The page file holds the table. The runtime builds the rails.
struct RoboCatalogEntry {
    const char* id = nullptr;
    const char* label = nullptr;
    const char* icon = nullptr;
    int parent = -1;
    bool section = false;
    bool selected = false;
};

int RoboCountCatalogRails(const RoboCatalogEntry* rows, int count);
int RoboDefaultCatalogRail(const RoboCatalogEntry* rows, int count);
int RoboBuildCatalogRails(Sidebar& sidebar, const RoboCatalogEntry* rows, int count, int railStart, int railBudget);
bool RoboEnsureCatalogChildren(Sidebar& sidebar, int railIndex, const RoboCatalogEntry* rows, int count);

}  // namespace Robo
