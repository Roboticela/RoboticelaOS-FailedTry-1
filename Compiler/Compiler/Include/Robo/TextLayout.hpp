#pragma once

#include "Color.hpp"
#include "FontEngine.hpp"
#include "FontStyle.hpp"
#include "TextAlign.hpp"

#include <string>
#include <vector>

namespace Robo {

// Retained text layout: Measure fills this; Paint stamps glyphs without re-wrapping.
struct TextLayoutSpan {
    std::string text;
    float width = 0.0f;
    bool isSpace = false;
    float fontSize = 16.0f;
    Color color;
    FontFaceId faceId = FontFaceId::Regular;
    TextDecoration decoration = TextDecoration::None;
    float letterSpacing = 0.0f;
    float opacity = 1.0f;
};

struct TextLayoutLine {
    std::vector<TextLayoutSpan> spans;
    float width = 0.0f;
    float y = 0.0f;
    float height = 0.0f;
};

struct TextLayoutCache {
    std::vector<TextLayoutLine> lines;
    float blockWidth = 0.0f;
    float maxAscent = 0.0f;
    float baseLineHeight = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float advanceY = 0.0f;
    TextAlign align = TextAlign::Left;
    float styleOpacity = 1.0f;
    bool valid = false;
    float containerWidth = -1.0f;
    float pixelScale = -1.0f;
    float originX = 0.0f;
    float originY = 0.0f;
};

}  // namespace Robo
