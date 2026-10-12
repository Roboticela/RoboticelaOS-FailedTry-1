#pragma once

#include "FontEngine.hpp"
#include "FrameBuffer.hpp"
#include "Grid.hpp"

namespace Robo {

struct GridMeasureResult {
    float width = 0.0f;
    float height = 0.0f;
};

struct GridPaintResult {
    GridBounds bounds;
    float advanceY = 0.0f;
};

class GridPainter {
public:
    static GridMeasureResult Measure(const FontEngine& fonts,
                                     Grid& grid,
                                     float containerWidth,
                                     float pixelScale = 1.0f,
                                     float forcedWidth = -1.0f,
                                     float forcedHeight = -1.0f);

    static GridPaintResult Paint(FrameBuffer& buffer,
                                 const FontEngine& fonts,
                                 Grid& grid,
                                 float originX,
                                 float originY,
                                 float containerWidth,
                                 float pixelScale = 1.0f,
                                 float forcedWidth = -1.0f,
                                 float forcedHeight = -1.0f);
};

}  // namespace Robo
