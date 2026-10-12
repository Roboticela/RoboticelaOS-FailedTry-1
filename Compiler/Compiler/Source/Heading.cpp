#include "Robo/Heading.hpp"

#include <algorithm>
#include <string>

namespace Robo {

int Heading::ClampLevel(int level) {
    return std::max(1, std::min(6, level));
}

TextStyle Heading::DefaultStyle(int level) {
    TextStyle style;
    style.wrap = true;
    style.width = "100%";
    style.align = TextAlign::Left;
    style.fontStyle = FontStyleKind::Normal;
    style.tokens["color"] = "color.text.primary";

    const char* role = "type.h1";
    switch (ClampLevel(level)) {
        case 1:
            role = "type.h1";
            break;
        case 2:
            role = "type.h2";
            break;
        case 3:
            role = "type.h3";
            break;
        case 4:
            role = "type.subhead";
            break;
        case 5:
            role = "type.bodyLg";
            break;
        case 6:
        default:
            role = "type.body";
            break;
    }
    style.tokens["fontSize"] = std::string(role) + ".size";
    style.tokens["lineHeight"] = std::string(role) + ".leading";
    style.tokens["fontWeight"] = std::string(role) + ".weight";
    style.tokens["letterSpacing"] = std::string(role) + ".tracking";
    return style;
}

Heading::Heading(int level) : level_(ClampLevel(level)) {
    SetStyle(DefaultStyle(level_));
}

int Heading::GetLevel() const {
    return level_;
}

}  // namespace Robo
