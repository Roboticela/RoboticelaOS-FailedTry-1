#pragma once

#include "BrandLogo.hpp"
#include "FontEngine.hpp"
#include "FrameBuffer.hpp"

namespace Robo {

struct BrandLogoMeasureResult {
    float width = 0.0f;
    float height = 0.0f;
    float advanceY = 0.0f;
};

struct BrandLogoPaintResult {
    float advanceY = 0.0f;
};

class BrandLogoPainter {
public:
    static BrandLogoMeasureResult Measure(const FontEngine& fonts, const BrandLogo& logo, float pixelScale);
    static BrandLogoPaintResult Paint(FrameBuffer& buffer,
                                      const FontEngine& fonts,
                                      const BrandLogo& logo,
                                      float originX,
                                      float originY,
                                      float pixelScale);
};

}  // namespace Robo
