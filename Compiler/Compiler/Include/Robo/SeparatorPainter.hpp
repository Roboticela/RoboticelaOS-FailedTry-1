#pragma once

#include "FontEngine.hpp"
#include "FrameBuffer.hpp"
#include "Separator.hpp"

namespace Robo {

struct SeparatorMeasureResult {
    float width = 0.0f;
    float height = 0.0f;
};

struct SeparatorPaintResult {
    float advanceY = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

class SeparatorPainter {
public:
    static SeparatorMeasureResult Measure(const FontEngine& fonts,
                                          const Separator& separator,
                                          float availableWidth,
                                          float availableHeight,
                                          float pixelScale);

    static SeparatorPaintResult Paint(FrameBuffer& buffer,
                                      const FontEngine& fonts,
                                      const Separator& separator,
                                      float originX,
                                      float originY,
                                      float boxWidth,
                                      float boxHeight,
                                      float pixelScale);
};

}  // namespace Robo
