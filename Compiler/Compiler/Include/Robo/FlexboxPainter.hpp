#pragma once

#include "Flexbox.hpp"
#include "FontEngine.hpp"
#include "FrameBuffer.hpp"

namespace Robo {

struct FlexboxPaintResult {
    float advanceY = 0.0f;
    FlexboxBounds bounds;
};

struct FlexboxMeasureResult {
    float width = 0.0f;
    float height = 0.0f;
};

class FlexboxPainter {
public:
    static FlexboxMeasureResult Measure(const FontEngine& fonts,
                                        Flexbox& flexbox,
                                        float containerWidth,
                                        float pixelScale = 1.0f,
                                        float forcedWidth = -1.0f,
                                        float forcedHeight = -1.0f);

    static FlexboxPaintResult Paint(FrameBuffer& buffer,
                                    const FontEngine& fonts,
                                    Flexbox& flexbox,
                                    float originX,
                                    float originY,
                                    float containerWidth,
                                    float pixelScale = 1.0f,
                                    float forcedWidth = -1.0f,
                                    float forcedHeight = -1.0f);
};

}  // namespace Robo
