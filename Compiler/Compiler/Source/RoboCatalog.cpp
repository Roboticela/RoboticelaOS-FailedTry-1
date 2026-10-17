#include "Robo/RoboCatalog.hpp"

#include "Robo/Foundation.hpp"
#include "Robo/RoboIconPack.hpp"

#include <cstring>
#include <string>
#include <vector>

namespace Robo {

namespace {

struct StoredRow {
    std::string id;
    std::string label;
    std::string icon;
    int parent = -1;
    bool section = false;
    bool selected = false;
};

std::vector<std::uint8_t> gPack;
std::vector<StoredRow> gStored;
std::vector<RoboCatalogEntry> gRows;
bool gLoaded = false;

std::uint16_t Read16(const std::uint8_t* data, std::uint32_t size, std::uint32_t offset) {
    if (offset + 2u > size) {
        return 0;
    }
    return static_cast<std::uint16_t>(data[offset] | (static_cast<std::uint16_t>(data[offset + 1u]) << 8));
}

std::uint32_t Read32(const std::uint8_t* data, std::uint32_t size, std::uint32_t offset) {
    if (offset + 4u > size) {
        return 0;
    }
    return static_cast<std::uint32_t>(data[offset]) | (static_cast<std::uint32_t>(data[offset + 1u]) << 8) |
           (static_cast<std::uint32_t>(data[offset + 2u]) << 16) | (static_cast<std::uint32_t>(data[offset + 3u]) << 24);
}

std::int32_t ReadI32(const std::uint8_t* data, std::uint32_t size, std::uint32_t offset) {
    return static_cast<std::int32_t>(Read32(data, size, offset));
}

void RebuildViews() {
    gRows.clear();
    gRows.reserve(gStored.size());
    for (StoredRow& row : gStored) {
        RoboCatalogEntry entry;
        entry.id = row.id.c_str();
        entry.label = row.label.c_str();
        entry.icon = row.icon.c_str();
        entry.parent = row.parent;
        entry.section = row.section;
        entry.selected = row.selected;
        gRows.push_back(entry);
    }
}

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
    if (entry.icon != nullptr && entry.icon[0] != '\0') {
        item.SetIconName(entry.icon);
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

void RoboCatalogAdd(const std::uint8_t* data, std::uint32_t size) {
    if (data == nullptr || size < 12u) {
        return;
    }
    if (data[0] != 'R' || data[1] != 'C' || data[2] != 'A' || data[3] != 'T' || data[4] != '1') {
        return;
    }
    const std::uint32_t count = Read32(data, size, 8u);
    if (count > 20000u) {
        return;
    }
    gPack.assign(data, data + size);
    gStored.clear();
    gStored.reserve(count);
    std::uint32_t cursor = 12u;
    for (std::uint32_t index = 0; index < count; ++index) {
        if (cursor + 2u > size) {
            break;
        }
        StoredRow row;
        const std::uint16_t idLen = Read16(gPack.data(), size, cursor);
        cursor += 2u;
        if (cursor + idLen > size) {
            break;
        }
        if (idLen > 0) {
            row.id.assign(reinterpret_cast<const char*>(gPack.data() + cursor), idLen);
            while (!row.id.empty() && row.id.back() == '\0') {
                row.id.pop_back();
            }
        }
        cursor += idLen;
        if (cursor + 2u > size) {
            break;
        }
        const std::uint16_t labelLen = Read16(gPack.data(), size, cursor);
        cursor += 2u;
        if (cursor + labelLen > size) {
            break;
        }
        if (labelLen > 0) {
            row.label.assign(reinterpret_cast<const char*>(gPack.data() + cursor), labelLen);
            while (!row.label.empty() && row.label.back() == '\0') {
                row.label.pop_back();
            }
        }
        cursor += labelLen;
        if (cursor + 2u > size) {
            break;
        }
        const std::uint16_t iconLen = Read16(gPack.data(), size, cursor);
        cursor += 2u;
        if (cursor + iconLen > size) {
            break;
        }
        if (iconLen > 0) {
            row.icon.assign(reinterpret_cast<const char*>(gPack.data() + cursor), iconLen);
            while (!row.icon.empty() && row.icon.back() == '\0') {
                row.icon.pop_back();
            }
        }
        cursor += iconLen;
        if (cursor + 5u > size) {
            break;
        }
        row.parent = ReadI32(gPack.data(), size, cursor);
        cursor += 4u;
        const std::uint8_t flags = gPack[cursor];
        cursor += 1u;
        row.section = (flags & 1u) != 0;
        row.selected = (flags & 2u) != 0;
        gStored.push_back(std::move(row));
    }
    RebuildViews();
}

void RoboCatalogLoad(const IPlatform& platform) {
    if (gLoaded) {
        return;
    }
    gLoaded = true;
    const char* paths[] = {"ROBOCAT.PAK", "Assets/ROBOCAT.PAK"};
    for (const char* path : paths) {
        const std::vector<std::uint8_t> bytes = platform.LoadAsset(path);
        if (!bytes.empty()) {
            RoboCatalogAdd(bytes.data(), static_cast<std::uint32_t>(bytes.size()));
            return;
        }
    }
}

const RoboCatalogEntry* RoboCatalogRows() {
    return gRows.empty() ? nullptr : gRows.data();
}

int RoboCatalogCount() {
    return static_cast<int>(gRows.size());
}

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

Sidebar* RoboFindSidebar(RoboScene& scene) {
    for (SceneItem& item : scene.Items()) {
        if (item.kind == SceneItemKind::Sidebar && item.sidebar) {
            return item.sidebar.get();
        }
    }
    return nullptr;
}

void RoboEnsureActivitySidebar(RoboScene& scene) {
    if (RoboFindSidebar(scene) != nullptr) {
        return;
    }
    Sidebar sidebar;
    sidebar.SetVariant(SidebarVariant::Activity);
    scene.AddSidebar(std::move(sidebar));
}

int RoboBuildSidebarRailsStep(RoboScene& scene, int railStart, int railBudget) {
    RoboEnsureActivitySidebar(scene);
    Sidebar* sidebar = RoboFindSidebar(scene);
    if (sidebar == nullptr || railBudget <= 0) {
        return RoboCountCatalogRails(RoboCatalogRows(), RoboCatalogCount());
    }
    return RoboBuildCatalogRails(*sidebar, RoboCatalogRows(), RoboCatalogCount(), railStart, railBudget);
}

void RoboBuildSidebarFromCatalog(RoboScene& scene) {
    const int total = RoboCountCatalogRails(RoboCatalogRows(), RoboCatalogCount());
    int next = 0;
    while (next < total) {
        next = RoboBuildSidebarRailsStep(scene, next, total);
    }
    if (Sidebar* sidebar = RoboFindSidebar(scene)) {
        sidebar->Select(RoboDefaultCatalogRail(RoboCatalogRows(), RoboCatalogCount()));
    }
}

void RoboApplyDevKitChrome(RoboScene& scene) {
    scene.SetPagePadding(0.0f);
    Foundation foundationDevKit = Foundation::CopyOf("RoboFoundation");
    foundationDevKit.SetName("DevKit");
    Foundation::Register(foundationDevKit);
    scene.SetFoundation("DevKit");
}

}  // namespace Robo
