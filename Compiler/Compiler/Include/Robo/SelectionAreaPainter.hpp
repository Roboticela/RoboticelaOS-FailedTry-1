#pragma once

#include "FontEngine.hpp"
#include "FrameBuffer.hpp"
#include "SelectionArea.hpp"

namespace Robo {

class SelectionAreaPainter {
public:
    static void Paint(FrameBuffer& buffer,
                      const FontEngine& fonts,
                      SelectionArea& area,
                      float x,
                      float y,
                      float width,
                      float height,
                      float pixelScale,
                      bool dark);
};

}  // namespace Robo
