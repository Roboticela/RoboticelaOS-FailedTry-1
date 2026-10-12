#include "IconPack.hpp"
#include "XmlParser.hpp"

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

}  // namespace Robo
