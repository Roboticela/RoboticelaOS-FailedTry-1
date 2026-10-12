#pragma once

#include "FontEngine.hpp"
#include "FrameBuffer.hpp"
#include "ThemeToggle.hpp"

namespace Robo {

struct ThemeToggleMeasureResult {
    float width = 0.0f;
    float height = 0.0f;
    float advanceY = 0.0f;
};

struct ThemeTogglePaintResult {
    float advanceY = 0.0f;
};

class ThemeTogglePainter {
public:
    static ThemeToggleMeasureResult Measure(const FontEngine& fonts,
                                            const ThemeToggle& toggle,
                                            float availableWidth,
                                            float pixelScale);
    static ThemeTogglePaintResult Paint(FrameBuffer& buffer,
                                        const FontEngine& fonts,
                                        ThemeToggle& toggle,
                                        float originX,
                                        float originY,
                                        float availableWidth,
                                        float pixelScale);
};

}  // namespace Robo
