#pragma once

#include "FontEngine.hpp"
#include "FrameBuffer.hpp"
#include "Length.hpp"
#include "Text.hpp"

namespace Robo {

struct TextMeasureResult {
    float width = 0.0f;
    float height = 0.0f;
    float advanceY = 0.0f;
};

class TextPainter {
public:
    static float ResolveWidthValue(const std::string& value, float referenceWidth);
    static float ResolveWidth(const TextStyle& style, float referenceWidth, float pixelScale);
    // Builds or reuses retained line/span layout on Text. Measure and Paint share this.
    static const TextLayoutCache& EnsureLayout(const FontEngine& fonts,
                                              const Text& text,
                                              float containerWidth,
                                              float pixelScale);
    static TextMeasureResult Measure(const FontEngine& fonts,
                                     const Text& text,
                                     float containerWidth,
                                     float pixelScale = 1.0f);
    static float Paint(FrameBuffer& buffer,
                       const FontEngine& fonts,
                       const Text& text,
                       float originX,
                       float originY,
                       float containerWidth,
                       float pixelScale = 1.0f);
};

}  // namespace Robo
