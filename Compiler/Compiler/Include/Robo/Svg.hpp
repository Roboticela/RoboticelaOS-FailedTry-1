#pragma once

#include "Color.hpp"
#include "Selectable.hpp"
#include "TextAlign.hpp"

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Robo {

enum class SvgCommandKind {
    Move,
    Line,
    Cubic,
    Close
};

enum class SvgFillRule {
    NonZero,
    EvenOdd
};

// One path command in user space. The compiler emits these from XML path data.
// A later pen tool edits the XML; it does not need a second SVG language.
struct SvgCommand {
    SvgCommandKind kind = SvgCommandKind::Move;
    float x = 0.0f;
    float y = 0.0f;
    float c1x = 0.0f;
    float c1y = 0.0f;
    float c2x = 0.0f;
    float c2y = 0.0f;

    static SvgCommand Move(float x, float y);
    static SvgCommand Line(float x, float y);
    static SvgCommand Cubic(float c1x, float c1y, float c2x, float c2y, float x, float y);
    static SvgCommand Close();
};

struct SvgShape {
    std::vector<SvgCommand> commands;
    std::shared_ptr<std::vector<SvgCommand>> sharedCommands;

    void ShareCommands() {
        if (sharedCommands || commands.empty()) {
            return;
        }
        sharedCommands = std::make_shared<std::vector<SvgCommand>>(std::move(commands));
        commands.clear();
    }

    Color fill = Color::FromHex("#000000");
    Color stroke = Color::FromHex("#000000");
    bool hasFill = true;
    bool hasStroke = false;
    bool currentFill = false;
    bool currentStroke = false;
    float strokeWidth = 1.0f;
    float opacity = 1.0f;
    SvgFillRule fillRule = SvgFillRule::NonZero;
    std::string fillToken;
    std::string strokeToken;
};

inline const std::vector<SvgCommand>& SvgCommands(const SvgShape& shape) {
    if (shape.sharedCommands && !shape.sharedCommands->empty()) {
        return *shape.sharedCommands;
    }
    return shape.commands;
}

struct SvgStyle {
    float width = 0.0f;
    float height = 0.0f;
    std::string widthSpec;
    std::string heightSpec;
    float viewX = 0.0f;
    float viewY = 0.0f;
    float viewWidth = 0.0f;
    float viewHeight = 0.0f;
    Color color = Color::FromHex("#1C1C1E");
    float opacity = 1.0f;
    TextAlign align = TextAlign::Left;
    SelectableMode selectable = SelectableMode::Inherit;
    std::map<std::string, std::string> tokens;
};

struct SvgBounds {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    bool Contains(float pointX, float pointY) const;
};

// A graphic written directly in XML. Paths, shapes and groups compile into shapes.
class Svg {
public:
    void SetStyle(const SvgStyle& style);
    const SvgStyle& GetStyle() const;
    SvgStyle& GetStyle();

    void AddShape(SvgShape shape);
    const std::vector<SvgShape>& GetShapes() const;

    void SetBounds(const SvgBounds& bounds);
    const SvgBounds& GetBounds() const;

private:
    SvgStyle style_;
    std::vector<SvgShape> shapes_;
    SvgBounds bounds_;
};

// Reads an SVG document into filled shapes. A gradient fill uses the average of its stops.
bool LoadSvgDocument(const std::uint8_t* bytes, std::size_t size, Svg& graphic);

// Packed command blob: kind,x,y,c1x,c1y,c2x,c2y per command (kind as float).
// Close uses kind=3. Avoids per-command push_back floods in generated bake helpers.
void AppendSvgCommandsFromBlob(SvgShape& shape, const float* data, std::size_t count);

}  // namespace Robo
