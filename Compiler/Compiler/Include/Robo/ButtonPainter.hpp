#pragma once

#include "Button.hpp"
#include "FontEngine.hpp"
#include "FrameBuffer.hpp"

namespace Robo {

struct ButtonPaintResult {
    float advanceY = 0.0f;
    ButtonBounds bounds;
};

struct ButtonMeasureResult {
    float width = 0.0f;
    float height = 0.0f;
    float advanceY = 0.0f;
};

class ButtonPainter {
public:
    static ButtonMeasureResult Measure(const FontEngine& fonts,
                                       const Button& button,
                                       float pixelScale = 1.0f);

    static ButtonPaintResult Paint(FrameBuffer& buffer,
                                   const FontEngine& fonts,
                                   Button& button,
                                   float originX,
                                   float originY,
                                   float containerWidth,
                                   float pixelScale = 1.0f,
                                   float forcedWidth = -1.0f,
                                   float forcedHeight = -1.0f);
};

}  // namespace Robo
