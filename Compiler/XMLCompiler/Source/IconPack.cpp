#include "IconPack.hpp"
#include "XmlParser.hpp"

#include <cstdint>
#include <cstring>
#include <exception>
#include <fstream>
#include <iterator>
#include <sstream>

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

}  // namespace Robo
