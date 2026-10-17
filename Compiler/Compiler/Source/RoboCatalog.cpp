#include "Robo/RoboCatalog.hpp"

#include "Robo/RoboIconPack.hpp"

namespace Robo {

namespace {

void FillCatalogItem(SidebarItem& item, const RoboCatalogEntry& entry) {
    if (entry.label != nullptr) {
        item.SetLabel(entry.label);
    }
    if (entry.id != nullptr && entry.id[0] != '\0') {
        item.SetId(entry.id);
    }
    if (entry.section) {
        item.SetSection(true);
    }
    if (entry.selected) {
        item.SetSelected(true);
    }
    if (entry.icon != nullptr && entry.icon[0] != '\0' && RoboIconPackContains(entry.icon)) {
        item.SetIcon(RoboBakeNamedIcon(entry.icon, 20.0f));
    }
}

int CatalogRailEntry(const RoboCatalogEntry* rows, int count, int railIndex) {
    int seen = 0;
    for (int index = 0; index < count; ++index) {
        if (rows[index].parent >= 0) {
            continue;
        }
        if (seen == railIndex) {
            return index;
        }
        ++seen;
    }
    return -1;
}

}  // namespace

int RoboCountCatalogRails(const RoboCatalogEntry* rows, int count) {
    int total = 0;
    if (rows == nullptr || count <= 0) {
        return 0;
    }
    for (int index = 0; index < count; ++index) {
        if (rows[index].parent < 0) {
            ++total;
        }
    }
    return total;
}

int RoboDefaultCatalogRail(const RoboCatalogEntry* rows, int count) {
    if (rows == nullptr || count <= 0) {
        return 0;
    }
    int selected = -1;
    for (int index = 0; index < count; ++index) {
        if (rows[index].selected) {
            selected = index;
            break;
        }
    }
    if (selected < 0) {
        return 0;
    }
    int railCatalog = selected;
    int guard = 0;
    while (railCatalog >= 0 && railCatalog < count && rows[railCatalog].parent >= 0 && guard < count) {
        railCatalog = rows[railCatalog].parent;
        ++guard;
    }
    int seen = 0;
    for (int index = 0; index < count; ++index) {
        if (rows[index].parent >= 0) {
            continue;
        }
        if (index == railCatalog) {
            return seen;
        }
        ++seen;
    }
    return 0;
}

int RoboBuildCatalogRails(Sidebar& sidebar, const RoboCatalogEntry* rows, int count, int railStart, int railBudget) {
    const int total = RoboCountCatalogRails(rows, count);
    if (rows == nullptr || railBudget <= 0) {
        return total;
    }
    int railIndex = 0;
    int added = 0;
    for (int index = 0; index < count; ++index) {
        if (rows[index].parent >= 0) {
            continue;
        }
        if (railIndex < railStart) {
            ++railIndex;
            continue;
        }
        if (railIndex < static_cast<int>(sidebar.Items().size())) {
            ++railIndex;
            continue;
        }
        SidebarItem item;
        FillCatalogItem(item, rows[index]);
        sidebar.AddItem(std::move(item));
        ++railIndex;
        ++added;
        if (added >= railBudget) {
            return railIndex;
        }
    }
    if (!sidebar.Items().empty() && sidebar.Selected() < 0) {
        sidebar.Select(RoboDefaultCatalogRail(rows, count));
    }
    return railIndex;
}

bool RoboEnsureCatalogChildren(Sidebar& sidebar, int railIndex, const RoboCatalogEntry* rows, int count) {
    if (rows == nullptr || railIndex < 0 || railIndex >= static_cast<int>(sidebar.Items().size())) {
        return false;
    }
    SidebarItem& rail = sidebar.Items()[static_cast<std::size_t>(railIndex)];
    const int catalogIndex = CatalogRailEntry(rows, count, railIndex);
    if (catalogIndex < 0) {
        return false;
    }
    const int have = static_cast<int>(rail.Children().size());
    int seen = 0;
    int added = 0;
    constexpr int kBudget = 8;
    for (int index = 0; index < count; ++index) {
        if (rows[index].parent != catalogIndex) {
            continue;
        }
        if (seen < have) {
            ++seen;
            continue;
        }
        SidebarItem child;
        FillCatalogItem(child, rows[index]);
        rail.AddChild(std::move(child));
        ++added;
        if (added >= kBudget) {
            for (int next = index + 1; next < count; ++next) {
                if (rows[next].parent == catalogIndex) {
                    return true;
                }
            }
            return false;
        }
    }
    return false;
}

}  // namespace Robo
