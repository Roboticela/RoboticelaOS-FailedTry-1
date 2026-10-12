#pragma once

#include "Color.hpp"
#include "FrameBuffer.hpp"

namespace Robo {

enum class AppearanceMark {
    Sun,
    Moon
};

void PaintAppearanceMark(FrameBuffer& buffer,
                         AppearanceMark mark,
                         float x,
                         float y,
                         float cssSize,
                         float strokeCss,
                         const Color& color,
                         float opacity,
                         float pixelScale);

}  // namespace Robo
