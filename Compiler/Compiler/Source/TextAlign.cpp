#include "Robo/TextAlign.hpp"

namespace Robo {

TextAlign ParseTextAlign(const std::string& value) {
    if (value == "center") {
        return TextAlign::Center;
    }
    if (value == "right") {
        return TextAlign::Right;
    }
    if (value == "justify") {
        return TextAlign::Justify;
    }
    return TextAlign::Left;
}

std::string TextAlignToCss(TextAlign align) {
    switch (align) {
        case TextAlign::Center:
            return "center";
        case TextAlign::Right:
            return "right";
        case TextAlign::Justify:
            return "justify";
        case TextAlign::Left:
        default:
            return "left";
    }
}

}  // namespace Robo
