#pragma once

#include "Desktop.hpp"
#include "FontEngine.hpp"
#include "FrameBuffer.hpp"

namespace Robo {

struct DesktopChrome {
    float contentY = 0.0f;
    float contentHeight = 0.0f;
    float menuHeight = 0.0f;
    float shellHeight = 0.0f;
    float dockY = 0.0f;
    float dockHeight = 0.0f;
};

class DesktopPainter {
public:
    static DesktopChrome Layout(float surfaceWidth,
                                float surfaceHeight,
                                float pixelScale,
                                bool hasMenu,
                                bool hasShell,
                                bool hasDock);
    static void Paint(FrameBuffer& buffer,
                      const FontEngine& fonts,
                      Desktop& desktop,
                      const DesktopChrome& chrome,
                      float pixelScale,
                      int hoveredIndex);
};

}  // namespace Robo
