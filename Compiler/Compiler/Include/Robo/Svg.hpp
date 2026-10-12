#pragma once

#include "Color.hpp"
#include "Selectable.hpp"
#include "TextAlign.hpp"

#include <map>
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

}  // namespace Robo
