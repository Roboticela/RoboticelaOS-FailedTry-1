#include "Robo/RoboIconPack.hpp"

#include "Robo/Color.hpp"

#include <cstring>
#include <string>
#include <vector>

namespace Robo {

namespace {

struct IconRef {
    int pack = 0;
    std::uint32_t offset = 0;
    std::uint32_t nameOffset = 0;
};

struct BakeSlot {
    char name[64];
    float size = 0.0f;
    bool ready = false;
    Svg svg;
};

std::vector<std::vector<std::uint8_t>> gPacks;
std::vector<IconRef> gIcons;
std::vector<int> gHash;
bool gLoaded = false;
BakeSlot gBake[96];

bool SameName(const char* left, const char* right) {
    if (left == nullptr || right == nullptr) {
        return false;
    }
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return *left == '\0' && *right == '\0';
}

std::uint32_t HashName(const char* name) {
    std::uint32_t hash = 2166136261u;
    while (name != nullptr && *name != '\0') {
        hash ^= static_cast<std::uint8_t>(*name);
        hash *= 16777619u;
        ++name;
    }
    return hash;
}

const char* IconNameAt(const IconRef& icon) {
    if (icon.pack < 0 || static_cast<std::size_t>(icon.pack) >= gPacks.size()) {
        return nullptr;
    }
    const std::vector<std::uint8_t>& pack = gPacks[static_cast<std::size_t>(icon.pack)];
    if (icon.nameOffset >= pack.size()) {
        return nullptr;
    }
    return reinterpret_cast<const char*>(pack.data() + icon.nameOffset);
}

void RebuildIconHash() {
    std::size_t slots = 16u;
    while (slots < gIcons.size() * 2u + 8u) {
        slots <<= 1u;
    }
    gHash.assign(slots, -1);
    const std::uint32_t mask = static_cast<std::uint32_t>(slots - 1u);
    for (int index = 0; index < static_cast<int>(gIcons.size()); ++index) {
        const char* name = IconNameAt(gIcons[static_cast<std::size_t>(index)]);
        if (name == nullptr || name[0] == '\0') {
            continue;
        }
        std::uint32_t slot = HashName(name) & mask;
        while (gHash[slot] >= 0) {
            slot = (slot + 1u) & mask;
        }
        gHash[slot] = index;
    }
}

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

float ReadFloat(const std::uint8_t* data, std::uint32_t size, std::uint32_t offset) {
    float value = 0.0f;
    if (offset + 4u > size) {
        return value;
    }
    memcpy(&value, data + offset, 4u);
    return value;
}

std::string ReadText(const std::uint8_t* data, std::uint32_t size, std::uint32_t offset, std::uint16_t length) {
    if (length == 0 || offset + length > size) {
        return {};
    }
    return std::string(reinterpret_cast<const char*>(data + offset), static_cast<std::size_t>(length));
}

const IconRef* FindIcon(const char* name) {
    if (name == nullptr || name[0] == '\0' || gHash.empty()) {
        return nullptr;
    }
    const std::uint32_t mask = static_cast<std::uint32_t>(gHash.size() - 1u);
    std::uint32_t slot = HashName(name) & mask;
    for (std::size_t probe = 0; probe < gHash.size(); ++probe) {
        const int index = gHash[slot];
        if (index < 0) {
            return nullptr;
        }
        const char* stored = IconNameAt(gIcons[static_cast<std::size_t>(index)]);
        if (SameName(stored, name)) {
            return &gIcons[static_cast<std::size_t>(index)];
        }
        slot = (slot + 1u) & mask;
    }
    return nullptr;
}

bool FillFromRef(Icon& icon, const IconRef& ref) {
    if (ref.pack < 0 || static_cast<std::size_t>(ref.pack) >= gPacks.size()) {
        return false;
    }
    const std::vector<std::uint8_t>& pack = gPacks[static_cast<std::size_t>(ref.pack)];
    const std::uint8_t* data = pack.data();
    const std::uint32_t size = static_cast<std::uint32_t>(pack.size());
    std::uint32_t cursor = ref.offset;
    if (cursor + 18u > size) {
        return false;
    }
    const std::uint16_t nameLength = Read16(data, size, cursor);
    cursor += 2u + nameLength;
    if (cursor + 16u + 4u > size) {
        return false;
    }
    const float viewX = ReadFloat(data, size, cursor);
    const float viewY = ReadFloat(data, size, cursor + 4u);
    const float viewWidth = ReadFloat(data, size, cursor + 8u);
    const float viewHeight = ReadFloat(data, size, cursor + 12u);
    cursor += 16u;
    const std::uint32_t shapeCount = Read32(data, size, cursor);
    cursor += 4u;
    icon.SetViewBox(viewX, viewY, viewWidth, viewHeight);
    for (std::uint32_t shapeIndex = 0; shapeIndex < shapeCount; ++shapeIndex) {
        if (cursor + 16u > size) {
            return false;
        }
        const std::uint8_t flags = data[cursor];
        const float strokeWidth = ReadFloat(data, size, cursor + 4u);
        const float opacity = ReadFloat(data, size, cursor + 8u);
        cursor += 12u;
        // Writer emits WriteText ×4 as [len][bytes] interleaved — not a length table.
        auto takeText = [&](std::string& out) -> bool {
            if (cursor + 2u > size) {
                return false;
            }
            const std::uint16_t length = Read16(data, size, cursor);
            cursor += 2u;
            if (cursor + length > size) {
                return false;
            }
            out = ReadText(data, size, cursor, length);
            cursor += length;
            return true;
        };
        std::string fill;
        std::string stroke;
        std::string fillToken;
        std::string strokeToken;
        if (!takeText(fill) || !takeText(stroke) || !takeText(fillToken) || !takeText(strokeToken)) {
            return false;
        }
        if (cursor + 4u > size) {
            return false;
        }
        const std::uint32_t commandCount = Read32(data, size, cursor);
        cursor += 4u;
        const std::uint32_t blobBytes = commandCount * 7u * 4u;
        if (commandCount > 100000u || cursor + blobBytes > size) {
            return false;
        }
        SvgShape shape;
        shape.hasFill = (flags & 1u) != 0;
        shape.hasStroke = (flags & 2u) != 0;
        shape.currentFill = (flags & 4u) != 0;
        shape.currentStroke = (flags & 8u) != 0;
        shape.fillRule = (flags & 16u) != 0 ? SvgFillRule::EvenOdd : SvgFillRule::NonZero;
        shape.strokeWidth = strokeWidth;
        shape.opacity = opacity;
        if (!fillToken.empty()) {
            shape.fillToken = fillToken;
        } else if (!shape.currentFill && !fill.empty()) {
            shape.fill = Color::FromHex(fill);
        }
        if (!strokeToken.empty()) {
            shape.strokeToken = strokeToken;
        } else if (!shape.currentStroke && !stroke.empty()) {
            shape.stroke = Color::FromHex(stroke);
        }
        if (commandCount > 0) {
            std::vector<float> blob(static_cast<std::size_t>(commandCount) * 7u);
            if (blob.size() != static_cast<std::size_t>(commandCount) * 7u || blob.data() == nullptr) {
                return false;
            }
            memcpy(blob.data(), data + cursor, blobBytes);
            AppendSvgCommandsFromBlob(shape, blob.data(), blob.size());
        }
        cursor += blobBytes;
        icon.AddPackedShape(std::move(shape));
    }
    return true;
}

}  // namespace

void RoboIconPackAdd(const std::uint8_t* data, std::uint32_t size) {
    if (data == nullptr || size < 12u) {
        return;
    }
    if (data[0] != 'R' || data[1] != 'P' || data[2] != 'A' || data[3] != 'K' || data[4] != '1') {
        return;
    }
    const std::uint32_t count = Read32(data, size, 8u);
    if (count > 10000u) {
        return;
    }
    const std::size_t beforePacks = gPacks.size();
    gPacks.emplace_back(data, data + size);
    if (gPacks.size() != beforePacks + 1u || gPacks.back().size() != size || gPacks.back().data() == nullptr) {
        return;
    }
    const int packIndex = static_cast<int>(gPacks.size() - 1u);
    const std::uint8_t* bytes = gPacks.back().data();
    std::uint32_t cursor = 12u;
    for (std::uint32_t index = 0; index < count; ++index) {
        if (cursor + 2u > size) {
            break;
        }
        IconRef ref;
        ref.pack = packIndex;
        ref.offset = cursor;
        const std::uint16_t nameLength = Read16(bytes, size, cursor);
        if (cursor + 2u + nameLength + 20u > size) {
            break;
        }
        ref.nameOffset = cursor + 2u;
        cursor += 2u + nameLength + 16u;
        const std::uint32_t shapeCount = Read32(bytes, size, cursor);
        cursor += 4u;
        bool intact = true;
        for (std::uint32_t shapeIndex = 0; shapeIndex < shapeCount; ++shapeIndex) {
            if (cursor + 12u > size) {
                intact = false;
                break;
            }
            cursor += 12u;
            // Match WriteText ×4: each field is u16 length then that many bytes.
            for (int textIndex = 0; textIndex < 4; ++textIndex) {
                if (cursor + 2u > size) {
                    intact = false;
                    break;
                }
                const std::uint16_t textLength = Read16(bytes, size, cursor);
                cursor += 2u + textLength;
                if (cursor > size) {
                    intact = false;
                    break;
                }
            }
            if (!intact || cursor + 4u > size) {
                intact = false;
                break;
            }
            const std::uint32_t commandCount = Read32(bytes, size, cursor);
            cursor += 4u;
            const std::uint32_t blobBytes = commandCount * 28u;
            if (commandCount > 100000u || cursor + blobBytes > size) {
                intact = false;
                break;
            }
            cursor += blobBytes;
        }
        if (!intact) {
            break;
        }
        gIcons.push_back(ref);
    }
    RebuildIconHash();
}

void RoboIconPackLoad(const IPlatform& platform) {
    if (gLoaded && !gIcons.empty()) {
        return;
    }
    const std::size_t before = gIcons.size();
    const char* paths[] = {"ROBOICON.PAK", "Assets/ROBOICON.PAK", "System/ROBOICON.PAK", "ROBOCTRY.PAK",
                           "Assets/ROBOCTRY.PAK", "System/ROBOCTRY.PAK"};
    for (const char* path : paths) {
        const std::vector<std::uint8_t> bytes = platform.LoadAsset(path);
        if (!bytes.empty()) {
            RoboIconPackAdd(bytes.data(), static_cast<std::uint32_t>(bytes.size()));
        }
    }
    // Only lock the pack when at least one icon landed — empty load must retry
    // (Web preload can race the first EnsureApp call).
    gLoaded = !gIcons.empty() || gIcons.size() > before;
}

bool RoboIconPackReady() {
    return !gIcons.empty();
}

bool RoboIconPackContains(const char* name) {
    return FindIcon(name) != nullptr;
}

bool RoboFillIconByName(Icon& icon, const char* name) {
    const IconRef* found = FindIcon(name);
    if (found == nullptr) {
        return false;
    }
    return FillFromRef(icon, *found);
}

Svg RoboBakeNamedIcon(const char* name, float size) {
    if (name == nullptr || name[0] == '\0') {
        return Svg();
    }
    for (BakeSlot& slot : gBake) {
        if (slot.ready && slot.size == size && SameName(slot.name, name)) {
            return slot.svg;
        }
    }
    Icon icon;
    if (!RoboFillIconByName(icon, name)) {
        return Svg();
    }
    if (size > 0.0f) {
        icon.SetWidth(size);
        icon.SetHeight(size);
    }
    Svg baked = icon.Bake();
    for (BakeSlot& slot : gBake) {
        if (!slot.ready) {
            std::size_t length = 0;
            while (name[length] != '\0' && length + 1u < sizeof(slot.name)) {
                slot.name[length] = name[length];
                ++length;
            }
            slot.name[length] = '\0';
            slot.size = size;
            slot.svg = baked;
            slot.ready = true;
            break;
        }
    }
    return baked;
}

}  // namespace Robo
