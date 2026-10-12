#pragma once

#include "FontEngine.hpp"
#include "FrameBuffer.hpp"
#include "Sidebar.hpp"

namespace Robo {

struct SidebarMeasureResult {
    float width = 0.0f;
    float height = 0.0f;
};

struct SidebarItemMeasureResult {
    float width = 0.0f;
    float height = 0.0f;
};

class SidebarPainter {
public:
    static SidebarMeasureResult Measure(const Sidebar& sidebar, float availableHeight, float pixelScale, float openAmount = 1.0f);
    static SidebarItemMeasureResult MeasureItem(const FontEngine& fonts, const SidebarItem& item, float pixelScale);
    static void Paint(FrameBuffer& buffer,
                      const FontEngine& fonts,
                      Sidebar& sidebar,
                      float originX,
                      float originY,
                      float height,
                      float pixelScale,
                      float openAmount);
    static void PaintItem(FrameBuffer& buffer,
                          const FontEngine& fonts,
                          SidebarItem& item,
                          float originX,
                          float originY,
                          float width,
                          float height,
                          float pixelScale,
                          bool selected);
};

}  // namespace Robo
