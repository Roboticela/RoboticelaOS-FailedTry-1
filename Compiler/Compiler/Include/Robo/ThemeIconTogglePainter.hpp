#pragma once

#include "FrameBuffer.hpp"
#include "ThemeIconToggle.hpp"

namespace Robo {

struct ThemeIconToggleMeasureResult {
    float width = 0.0f;
    float height = 0.0f;
    float advanceY = 0.0f;
};

struct ThemeIconTogglePaintResult {
    float advanceY = 0.0f;
};

class ThemeIconTogglePainter {
public:
    static ThemeIconToggleMeasureResult Measure(const ThemeIconToggle& toggle, float pixelScale);
    static ThemeIconTogglePaintResult Paint(FrameBuffer& buffer,
                                            ThemeIconToggle& toggle,
                                            float originX,
                                            float originY,
                                            float pixelScale);
};

}  // namespace Robo
