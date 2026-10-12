#include "Robo/FontStyle.hpp"

namespace Robo {

FontWeight ParseFontWeight(const std::string& value) {
    if (value == "medium") {
        return FontWeight::Medium;
    }
    if (value == "semibold" || value == "semiBold") {
        return FontWeight::SemiBold;
    }
    if (value == "bold") {
        return FontWeight::Bold;
    }
    return FontWeight::Regular;
}

FontStyleKind ParseFontStyle(const std::string& value) {
    if (value == "italic") {
        return FontStyleKind::Italic;
    }
    return FontStyleKind::Normal;
}

TextDecoration ParseTextDecoration(const std::string& value) {
    if (value == "underline") {
        return TextDecoration::Underline;
    }
    if (value == "line-through" || value == "lineThrough") {
        return TextDecoration::LineThrough;
    }
    return TextDecoration::None;
}

std::string FontWeightToCss(FontWeight weight) {
    switch (weight) {
        case FontWeight::Medium:
            return "500";
        case FontWeight::SemiBold:
            return "600";
        case FontWeight::Bold:
            return "700";
        case FontWeight::Regular:
        default:
            return "400";
    }
}

std::string FontStyleToCss(FontStyleKind style) {
    return style == FontStyleKind::Italic ? "italic" : "normal";
}

std::string TextDecorationToCss(TextDecoration decoration) {
    switch (decoration) {
        case TextDecoration::Underline:
            return "underline";
        case TextDecoration::LineThrough:
            return "line-through";
        case TextDecoration::None:
        default:
            return "none";
    }
}

}  // namespace Robo
