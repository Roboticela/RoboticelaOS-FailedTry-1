#pragma once

#include "XmlNode.hpp"

#include <string>
#include <vector>

namespace Robo {

struct CompiledCommand {
    // 0 move, 1 line, 2 cubic, 3 close.
    int kind = 0;
    float x = 0.0f;
    float y = 0.0f;
    float c1x = 0.0f;
    float c1y = 0.0f;
    float c2x = 0.0f;
    float c2y = 0.0f;
};

struct CompiledShape {
    std::vector<CompiledCommand> commands;
    bool hasFill = true;
    bool hasStroke = false;
    bool currentFill = false;
    bool currentStroke = false;
    std::string fillHex = "#000000";
    std::string strokeHex = "#000000";
    std::string fillToken;
    std::string strokeToken;
    float strokeWidth = 1.0f;
    float opacity = 1.0f;
    bool evenOdd = false;
};

struct CompiledSvg {
    float viewX = 0.0f;
    float viewY = 0.0f;
    float viewWidth = 24.0f;
    float viewHeight = 24.0f;
    bool hasViewBox = false;
    std::vector<CompiledShape> shapes;
};

// Turns an <svg> element into path commands. Arcs and basic shapes become cubics
// here so the page runtime does not parse SVG text.
CompiledSvg CompileSvgElement(const XmlNode& svgNode, bool defaultCurrentFill);

std::string EmitCompiledShapes(const std::vector<CompiledShape>& shapes, int indent, const std::string& target, bool packed);

std::vector<CompiledCommand> CompilePathData(const std::string& data);

}  // namespace Robo
