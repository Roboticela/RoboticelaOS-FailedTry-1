#include "Robo/Paragraph.hpp"

namespace Robo {

TextStyle Paragraph::DefaultStyle() {
    TextStyle style;
    style.fontStyle = FontStyleKind::Normal;
    style.align = TextAlign::Left;
    style.wrap = true;
    style.opacity = 1.0;
    style.width = "100%";
    style.maxWidth = "42rem";
    style.tokens["fontSize"] = "type.body.size";
    style.tokens["color"] = "color.text.secondary";
    style.tokens["fontWeight"] = "type.body.weight";
    style.tokens["lineHeight"] = "type.body.leading";
    style.tokens["letterSpacing"] = "type.body.tracking";
    return style;
}

Paragraph::Paragraph() {
    SetStyle(DefaultStyle());
}

}  // namespace Robo
