#pragma once

#include "FrameBuffer.hpp"
#include "Scrollbar.hpp"

namespace Robo {

struct ScrollbarPaintResult {
    float advanceY = 0.0f;
    ScrollbarBounds bounds;
};

struct ScrollbarMeasureResult {
    float width = 0.0f;
    float height = 0.0f;
    float advanceY = 0.0f;
};

class ScrollbarPainter {
public:
    static ScrollbarMeasureResult Measure(const Scrollbar& scrollbar,
                                          float containerWidth,
                                          float pixelScale = 1.0f);

    static ScrollbarPaintResult Paint(FrameBuffer& buffer,
                                      Scrollbar& scrollbar,
                                      float originX,
                                      float originY,
                                      float containerWidth,
                                      float pixelScale = 1.0f,
                                      float forcedWidth = -1.0f,
                                      float forcedHeight = -1.0f);
};

}  // namespace Robo
