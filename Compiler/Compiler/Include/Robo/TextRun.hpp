#pragma once

#include "Color.hpp"
#include "FontStyle.hpp"

#include <map>
#include <string>

namespace Robo {

struct TextRun {
    std::string content;
    double fontSize = 16.0;
    Color color = Color::FromHex("#1A1A1A");
    FontWeight fontWeight = FontWeight::Regular;
    FontStyleKind fontStyle = FontStyleKind::Normal;
    TextDecoration textDecoration = TextDecoration::None;
    double letterSpacing = 0.0;
    double opacity = 1.0;
    bool hasFontSize = false;
    bool hasColor = false;
    bool hasFontWeight = false;
    bool hasFontStyle = false;
    bool hasTextDecoration = false;
    bool hasLetterSpacing = false;
    bool hasOpacity = false;
    std::map<std::string, std::string> tokens;
};

}  // namespace Robo
