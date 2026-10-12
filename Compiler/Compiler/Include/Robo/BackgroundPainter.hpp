#pragma once

#include "Background.hpp"
#include "FrameBuffer.hpp"

namespace Robo {

class BackgroundPainter {
public:
    // Paints the current background into one rectangle of the surface.
    // Pictures, GIFs, and video cover the surface. An SVG is scaled to cover it.
    static void Paint(FrameBuffer& buffer,
                      Background& background,
                      int surfaceWidth,
                      int surfaceHeight,
                      int x,
                      int y,
                      int width,
                      int height);
};

}  // namespace Robo
