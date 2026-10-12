#pragma once

#include "FontEngine.hpp"
#include "FrameBuffer.hpp"
#include "Position.hpp"

namespace Robo {

struct PositionMeasureResult {
    float width = 0.0f;
    float height = 0.0f;
};

struct PositionPaintResult {
    InputBounds bounds;
    float advanceY = 0.0f;
};

class PositionPainter {
public:
    static PositionMeasureResult Measure(const FontEngine& fonts,
                                         Position& position,
                                         float availableWidth,
                                         float pixelScale = 1.0f,
                                         float forcedWidth = -1.0f,
                                         float forcedHeight = -1.0f);

    static PositionPaintResult Paint(FrameBuffer& buffer,
                                     const FontEngine& fonts,
                                     Position& position,
                                     float originX,
                                     float originY,
                                     float containerWidth,
                                     float pixelScale = 1.0f,
                                     float forcedWidth = -1.0f,
                                     float forcedHeight = -1.0f);
};

}  // namespace Robo
