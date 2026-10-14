#pragma once

#include "ContextMenu.hpp"
#include "FontEngine.hpp"
#include "FrameBuffer.hpp"

namespace Robo {

class ContextMenuPainter {
public:
    static void Paint(FrameBuffer& buffer,
                      const FontEngine& fonts,
                      ContextMenu& menu,
                      float pixelScale,
                      float viewWidth,
                      float viewHeight,
                      float topInset,
                      float bottomInset);
};

}  // namespace Robo
