#pragma once

#include "Container.hpp"
#include "FontEngine.hpp"
#include "FrameBuffer.hpp"

namespace Robo {

struct ContainerMeasureResult {
    float width = 0.0f;
    float height = 0.0f;
    float contentExtent = 0.0f;   // Scrollable extent along the scroll axis.
    float viewportExtent = 0.0f;  // Visible extent along the scroll axis.
};

struct ContainerPaintResult {
    InputBounds bounds;
    float advanceY = 0.0f;
};

class ContainerPainter {
public:
    static ContainerMeasureResult Measure(const FontEngine& fonts,
                                          Container& container,
                                          float availableWidth,
                                          float pixelScale = 1.0f,
                                          float forcedWidth = -1.0f,
                                          float forcedHeight = -1.0f);

    static ContainerPaintResult Paint(FrameBuffer& buffer,
                                      const FontEngine& fonts,
                                      Container& container,
                                      float originX,
                                      float originY,
                                      float containerWidth,
                                      float pixelScale = 1.0f,
                                      float forcedWidth = -1.0f,
                                      float forcedHeight = -1.0f);
};

}  // namespace Robo
