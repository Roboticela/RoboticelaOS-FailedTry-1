#include "IconPack.hpp"
#include "XmlParser.hpp"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <iterator>
#include <sstream>
#include <vector>

namespace Robo {

namespace {

std::string ReadFile(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return {};
    }
    return std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
}

void LoadPack(const std::string& path, std::map<std::string, CompiledSvg>& icons) {
    const std::string text = ReadFile(path);
    if (text.empty()) {
        return;
    }
    std::size_t cursor = 0;
    while (cursor < text.size()) {
        const std::size_t lineEnd = text.find('\n', cursor);
        const std::string line = text.substr(cursor, lineEnd == std::string::npos ? std::string::npos : lineEnd - cursor);
        cursor = lineEnd == std::string::npos ? text.size() : lineEnd + 1;
        if (line.compare(0, 5, "icon ") != 0) {
            continue;
        }
        std::string name;
        float viewX = 0.0f;
        float viewY = 0.0f;
        float viewW = 24.0f;
        float viewH = 24.0f;
        std::istringstream header(line.substr(5));
        header >> name >> viewX >> viewY >> viewW >> viewH;
        if (name.empty()) {
            continue;
        }
        const std::size_t svgStart = text.find("<svg", cursor);
        const std::size_t svgEnd = text.find("</svg>", cursor);
        if (svgStart == std::string::npos || svgEnd == std::string::npos || svgStart > svgEnd) {
            continue;
        }
        const std::string fragment = text.substr(svgStart, svgEnd + 6 - svgStart);
        cursor = svgEnd + 6;
        try {
            XmlParser parser;
            const XmlNode node = parser.Parse(fragment);
            CompiledSvg graphic = CompileSvgElement(node, true);
            if (viewW > 0.0f && viewH > 0.0f) {
                graphic.viewX = viewX;
                graphic.viewY = viewY;
                graphic.viewWidth = viewW;
                graphic.viewHeight = viewH;
                graphic.hasViewBox = true;
            }
            icons[name] = std::move(graphic);
        } catch (const std::exception&) {
            continue;
        }
    }
}

}  // namespace

std::map<std::string, CompiledSvg> LoadIconPacks(const std::string& assetDirectory) {
    std::map<std::string, CompiledSvg> icons;
    const std::string separator = assetDirectory.empty() || assetDirectory.back() == '\\' || assetDirectory.back() == '/' ? "" : "/";
    LoadPack(assetDirectory + separator + "RoboIcons.pack", icons);
    LoadPack(assetDirectory + separator + "RoboCountry.pack", icons);
    return icons;
}

namespace {

void Write16(std::ostream& output, std::uint16_t value) {
    const char bytes[2] = {static_cast<char>(value & 0xFFu), static_cast<char>((value >> 8) & 0xFFu)};
    output.write(bytes, 2);
}

void Write32(std::ostream& output, std::uint32_t value) {
    const char bytes[4] = {
        static_cast<char>(value & 0xFFu),
        static_cast<char>((value >> 8) & 0xFFu),
        static_cast<char>((value >> 16) & 0xFFu),
        static_cast<char>((value >> 24) & 0xFFu),
    };
    output.write(bytes, 4);
}

void WriteFloat(std::ostream& output, float value) {
    char bytes[4];
    std::memcpy(bytes, &value, 4);
    output.write(bytes, 4);
}

void WriteText(std::ostream& output, const std::string& text) {
    const std::uint16_t length = static_cast<std::uint16_t>(text.size() > 65535u ? 65535u : text.size());
    Write16(output, length);
    if (length > 0) {
        output.write(text.data(), length);
    }
}

}  // namespace

bool WriteRuntimeIconPack(const std::string& packPath, const std::string& outputPath) {
    std::map<std::string, CompiledSvg> icons;
    LoadPack(packPath, icons);
    if (icons.empty()) {
        return false;
    }
    std::ofstream output(outputPath, std::ios::binary);
    if (!output) {
        return false;
    }
    const char magic[8] = {'R', 'P', 'A', 'K', '1', 0, 0, 0};
    output.write(magic, 8);
    Write32(output, static_cast<std::uint32_t>(icons.size()));
    for (const auto& entry : icons) {
        const CompiledSvg& graphic = entry.second;
        Write16(output, static_cast<std::uint16_t>(entry.first.size() + 1u));
        output.write(entry.first.data(), static_cast<std::streamsize>(entry.first.size()));
        output.put('\0');
        WriteFloat(output, graphic.viewX);
        WriteFloat(output, graphic.viewY);
        WriteFloat(output, graphic.viewWidth);
        WriteFloat(output, graphic.viewHeight);
        Write32(output, static_cast<std::uint32_t>(graphic.shapes.size()));
        for (const CompiledShape& shape : graphic.shapes) {
            std::uint8_t flags = 0;
            if (shape.hasFill) {
                flags = static_cast<std::uint8_t>(flags | 1u);
            }
            if (shape.hasStroke) {
                flags = static_cast<std::uint8_t>(flags | 2u);
            }
            if (shape.currentFill) {
                flags = static_cast<std::uint8_t>(flags | 4u);
            }
            if (shape.currentStroke) {
                flags = static_cast<std::uint8_t>(flags | 8u);
            }
            if (shape.evenOdd) {
                flags = static_cast<std::uint8_t>(flags | 16u);
            }
            output.put(static_cast<char>(flags));
            output.put(0);
            output.put(0);
            output.put(0);
            WriteFloat(output, shape.strokeWidth);
            WriteFloat(output, shape.opacity);
            const std::string fill = shape.fillToken.empty() && !shape.currentFill ? shape.fillHex : "";
            const std::string stroke = shape.strokeToken.empty() && !shape.currentStroke ? shape.strokeHex : "";
            WriteText(output, fill);
            WriteText(output, stroke);
            WriteText(output, shape.fillToken);
            WriteText(output, shape.strokeToken);
            Write32(output, static_cast<std::uint32_t>(shape.commands.size()));
            for (const CompiledCommand& command : shape.commands) {
                WriteFloat(output, static_cast<float>(command.kind));
                WriteFloat(output, command.x);
                WriteFloat(output, command.y);
                WriteFloat(output, command.c1x);
                WriteFloat(output, command.c1y);
                WriteFloat(output, command.c2x);
                WriteFloat(output, command.c2y);
            }
        }
    }
    return static_cast<bool>(output);
}

namespace {

bool IsPaneItemName(const std::string& name) {
    return name == "SidebarItem" || name == "PaneItem" || name == "PaneItems";
}

bool IsWhitespaceOnly(const std::string& text) {
    for (char ch : text) {
        if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r') {
            return false;
        }
    }
    return true;
}

struct CatalogRow {
    std::string id;
    std::string label;
    std::string icon;
    int parent = -1;
    bool section = false;
    bool selected = false;
};

void CollectCatalog(const XmlNode& node, int parent, std::vector<CatalogRow>& rows) {
    for (const XmlNode& child : node.children) {
        if (!IsPaneItemName(child.name)) {
            continue;
        }
        CatalogRow row;
        row.parent = parent;
        if (child.attributes.count("id") != 0) {
            row.id = child.attributes.at("id");
        }
        if (child.attributes.count("label") != 0) {
            row.label = child.attributes.at("label");
        } else {
            for (const XmlNode& textChild : child.children) {
                if (textChild.isText && !IsWhitespaceOnly(textChild.text)) {
                    row.label += textChild.text;
                }
            }
        }
        if (child.attributes.count("icon") != 0) {
            row.icon = child.attributes.at("icon");
        }
        const auto section = child.attributes.find("section");
        const auto variant = child.attributes.find("variant");
        row.section = (section != child.attributes.end() &&
                       (section->second == "true" || section->second == "1" || section->second == "section")) ||
                      (variant != child.attributes.end() && variant->second == "section");
        const auto selected = child.attributes.find("selected");
        row.selected = selected != child.attributes.end() &&
                       (selected->second == "true" || selected->second == "1");
        const int index = static_cast<int>(rows.size());
        rows.push_back(row);
        CollectCatalog(child, index, rows);
    }
}

const XmlNode* FindCatalogRoot(const XmlNode& node) {
    if (node.name == "Sidebar" || node.name == "Pane") {
        return &node;
    }
    for (const XmlNode& child : node.children) {
        if (const XmlNode* found = FindCatalogRoot(child)) {
            return found;
        }
    }
    return nullptr;
}

void WriteCatalogText(std::ostream& output, const std::string& text) {
    const std::uint16_t length = static_cast<std::uint16_t>(text.size() > 65534u ? 65534u : text.size());
    Write16(output, static_cast<std::uint16_t>(length + (length > 0 ? 1u : 0u)));
    if (length > 0) {
        output.write(text.data(), length);
        output.put('\0');
    }
}

}  // namespace

bool WriteRuntimeCatalog(const std::string& xmlPath, const std::string& outputPath) {
    const std::string xml = ReadFile(xmlPath);
    if (xml.empty()) {
        return false;
    }
    XmlParser parser;
    XmlNode root;
    try {
        root = parser.Parse(xml);
    } catch (const std::exception&) {
        return false;
    }
    const XmlNode* catalogRoot = FindCatalogRoot(root);
    if (catalogRoot == nullptr) {
        return false;
    }
    std::vector<CatalogRow> rows;
    CollectCatalog(*catalogRoot, -1, rows);
    if (rows.empty()) {
        return false;
    }
    std::ofstream output(outputPath, std::ios::binary);
    if (!output) {
        return false;
    }
    const char magic[8] = {'R', 'C', 'A', 'T', '1', 0, 0, 0};
    output.write(magic, 8);
    Write32(output, static_cast<std::uint32_t>(rows.size()));
    for (const CatalogRow& row : rows) {
        WriteCatalogText(output, row.id);
        WriteCatalogText(output, row.label);
        WriteCatalogText(output, row.icon);
        Write32(output, static_cast<std::uint32_t>(row.parent));
        std::uint8_t flags = 0;
        if (row.section) {
            flags = static_cast<std::uint8_t>(flags | 1u);
        }
        if (row.selected) {
            flags = static_cast<std::uint8_t>(flags | 2u);
        }
        output.put(static_cast<char>(flags));
    }
    return static_cast<bool>(output);
}

namespace {

enum PaneKind : std::uint16_t {
    PaneKindContainer = 1,
    PaneKindFlexbox = 2,
    PaneKindHeading = 3,
    PaneKindParagraph = 4,
    PaneKindText = 5,
    PaneKindButton = 6,
    PaneKindInput = 7,
    PaneKindIcon = 8,
    PaneKindIconButton = 9,
    PaneKindGrid = 10
};

struct PaneAttr {
    std::uint32_t key = 0;
    std::uint32_t value = 0;
};

struct PaneNode {
    std::uint16_t kind = 0;
    std::uint32_t text = 0xFFFFFFFFu;
    std::vector<PaneAttr> attrs;
    std::vector<std::uint32_t> children;
};

struct PaneBlob {
    std::uint32_t id = 0;
    std::uint32_t root = 0;
    std::vector<PaneNode> nodes;
};

struct PaneWriter {
    std::vector<std::string> pool;
    std::map<std::string, std::uint32_t> index;
    std::map<std::string, CompiledSvg> icons;

    std::uint32_t Intern(const std::string& text) {
        const auto found = index.find(text);
        if (found != index.end()) {
            return found->second;
        }
        const std::uint32_t id = static_cast<std::uint32_t>(pool.size());
        pool.push_back(text);
        index[text] = id;
        return id;
    }

    bool IsIconName(const std::string& name) const {
        return icons.find(name) != icons.end();
    }

    std::uint16_t KindOf(const XmlNode& node) const {
        if (node.name == "Container" || node.name == "Box") {
            return PaneKindContainer;
        }
        if (node.name == "Flexbox" || node.name == "Flex") {
            return PaneKindFlexbox;
        }
        if (node.name == "Paragraph" || node.name == "P" || node.name == "p") {
            return PaneKindParagraph;
        }
        if (node.name == "Text" || node.name == "text") {
            return PaneKindText;
        }
        if (node.name == "Button") {
            return PaneKindButton;
        }
        if (node.name == "IconButton") {
            return PaneKindIconButton;
        }
        if (node.name == "Input") {
            return PaneKindInput;
        }
        if (node.name == "Grid") {
            return PaneKindGrid;
        }
        if (node.name == "Heading" || node.name == "H1" || node.name == "H2" || node.name == "H3" ||
            node.name == "H4" || node.name == "H5" || node.name == "H6" || node.name == "h1" ||
            node.name == "h2" || node.name == "h3" || node.name == "h4" || node.name == "h5" ||
            node.name == "h6") {
            return PaneKindHeading;
        }
        if (IsIconName(node.name)) {
            return PaneKindIcon;
        }
        return 0;
    }

    std::string PlainText(const XmlNode& node) const {
        std::string text;
        for (const XmlNode& child : node.children) {
            if (child.isText) {
                text += child.text;
            }
        }
        return text;
    }

    std::uint32_t Emit(const XmlNode& node, std::vector<PaneNode>& nodes) {
        const std::uint16_t kind = KindOf(node);
        if (kind == 0) {
            return 0xFFFFFFFFu;
        }
        PaneNode out;
        out.kind = kind;
        const std::string text = PlainText(node);
        if (!text.empty()) {
            out.text = Intern(text);
        }
        for (const auto& attribute : node.attributes) {
            PaneAttr attr;
            attr.key = Intern(attribute.first);
            attr.value = Intern(attribute.second);
            out.attrs.push_back(attr);
        }
        if (kind == PaneKindHeading) {
            int level = 1;
            if (node.name.size() == 2 && (node.name[0] == 'H' || node.name[0] == 'h') && node.name[1] >= '1' &&
                node.name[1] <= '6') {
                level = node.name[1] - '0';
            } else if (node.attributes.count("level") != 0) {
                level = std::atoi(node.attributes.at("level").c_str());
            }
            PaneAttr attr;
            attr.key = Intern("level");
            attr.value = Intern(std::to_string(level));
            bool hasLevel = false;
            for (const PaneAttr& existing : out.attrs) {
                if (pool[existing.key] == "level") {
                    hasLevel = true;
                    break;
                }
            }
            if (!hasLevel) {
                out.attrs.push_back(attr);
            }
        }
        if (kind == PaneKindIcon) {
            PaneAttr attr;
            attr.key = Intern("name");
            attr.value = Intern(node.name);
            out.attrs.push_back(attr);
        }
        const std::uint32_t self = static_cast<std::uint32_t>(nodes.size());
        nodes.push_back(out);
        for (const XmlNode& child : node.children) {
            if (child.isText) {
                continue;
            }
            const std::uint32_t childIndex = Emit(child, nodes);
            if (childIndex != 0xFFFFFFFFu) {
                nodes[self].children.push_back(childIndex);
            }
        }
        return self;
    }
};

void CollectPaneRoots(const XmlNode& node, std::vector<const XmlNode*>& panes) {
    if (!node.isText && node.name == "Container" && node.attributes.count("when") != 0) {
        panes.push_back(&node);
    }
    for (const XmlNode& child : node.children) {
        CollectPaneRoots(child, panes);
    }
}

}  // namespace

bool WriteRuntimePanes(const std::string& xmlPath, const std::string& outputPath, const std::string& assetDirectory) {
    const std::string xml = ReadFile(xmlPath);
    if (xml.empty()) {
        return false;
    }
    XmlParser parser;
    XmlNode root;
    try {
        root = parser.Parse(xml);
    } catch (const std::exception&) {
        return false;
    }
    std::vector<const XmlNode*> paneRoots;
    CollectPaneRoots(root, paneRoots);
    if (paneRoots.empty()) {
        return false;
    }
    PaneWriter writer;
    writer.icons = LoadIconPacks(assetDirectory.empty() ? "Assets" : assetDirectory);
    std::vector<PaneBlob> panes;
    for (const XmlNode* pane : paneRoots) {
        PaneBlob blob;
        blob.id = writer.Intern(pane->attributes.at("when"));
        blob.root = writer.Emit(*pane, blob.nodes);
        if (blob.root == 0xFFFFFFFFu || blob.nodes.empty()) {
            continue;
        }
        panes.push_back(std::move(blob));
    }
    if (panes.empty()) {
        return false;
    }
    std::ofstream output(outputPath, std::ios::binary);
    if (!output) {
        return false;
    }
    const char magic[8] = {'R', 'P', 'A', 'N', '1', 0, 0, 0};
    output.write(magic, 8);
    Write32(output, static_cast<std::uint32_t>(writer.pool.size()));
    for (const std::string& entry : writer.pool) {
        Write16(output, static_cast<std::uint16_t>(entry.size() + 1u));
        output.write(entry.data(), static_cast<std::streamsize>(entry.size()));
        output.put('\0');
    }
    Write32(output, static_cast<std::uint32_t>(panes.size()));
    for (const PaneBlob& pane : panes) {
        Write32(output, pane.id);
        Write32(output, pane.root);
        Write32(output, static_cast<std::uint32_t>(pane.nodes.size()));
        for (const PaneNode& node : pane.nodes) {
            Write16(output, node.kind);
            Write16(output, static_cast<std::uint16_t>(node.attrs.size()));
            Write16(output, static_cast<std::uint16_t>(node.children.size()));
            Write16(output, 0);
            Write32(output, node.text);
            for (const PaneAttr& attr : node.attrs) {
                Write32(output, attr.key);
                Write32(output, attr.value);
            }
            for (std::uint32_t child : node.children) {
                Write32(output, child);
            }
        }
    }
    return static_cast<bool>(output);
}

}  // namespace Robo
