#pragma once

#include <string>

namespace Robo {

enum class TextAlign {
    Left,
    Center,
    Right,
    Justify
};

TextAlign ParseTextAlign(const std::string& value);
std::string TextAlignToCss(TextAlign align);

}  // namespace Robo
