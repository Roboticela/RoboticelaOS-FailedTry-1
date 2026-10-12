#pragma once

#include "FrameBuffer.hpp"
#include "Svg.hpp"

namespace Robo {

class FontEngine;

struct SvgMeasureResult {
    float width = 0.0f;
    float height = 0.0f;
};

struct SvgPaintResult {
    float advanceY = 0.0f;
};

class SvgPainter {
public:
    static SvgMeasureResult Measure(const Svg& svg, float pixelScale);
    static SvgPaintResult Paint(FrameBuffer& buffer,
                                Svg& svg,
                                float originX,
                                float originY,
                                float containerWidth,
                                float pixelScale);
};

}  // namespace Robo
