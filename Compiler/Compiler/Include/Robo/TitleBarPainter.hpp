#pragma once

#include "FontEngine.hpp"
#include "FrameBuffer.hpp"
#include "TitleBar.hpp"

namespace Robo {

struct TitleBarMeasureResult {
    float width = 0.0f;
    float height = 0.0f;
    float advanceY = 0.0f;
};

struct TitleBarPaintResult {
    float advanceY = 0.0f;
    float height = 0.0f;
};

class TitleBarPainter {
public:
    static TitleBarMeasureResult Measure(const FontEngine& fonts, const TitleBar& bar, float containerWidth, float pixelScale);
    static TitleBarPaintResult Paint(FrameBuffer& buffer,
                                     const FontEngine& fonts,
                                     TitleBar& bar,
                                     float originX,
                                     float originY,
                                     float containerWidth,
                                     float pixelScale);
};

}  // namespace Robo
