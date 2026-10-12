#pragma once

#include <string>

namespace Robo {

enum class FontWeight {
    Regular,
    Medium,
    SemiBold,
    Bold
};

enum class FontStyleKind {
    Normal,
    Italic
};

enum class TextDecoration {
    None,
    Underline,
    LineThrough
};

FontWeight ParseFontWeight(const std::string& value);
FontStyleKind ParseFontStyle(const std::string& value);
TextDecoration ParseTextDecoration(const std::string& value);

std::string FontWeightToCss(FontWeight weight);
std::string FontStyleToCss(FontStyleKind style);
std::string TextDecorationToCss(TextDecoration decoration);

}  // namespace Robo
