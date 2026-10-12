#pragma once

#include "FontEngine.hpp"
#include "FrameBuffer.hpp"
#include "Input.hpp"

namespace Robo {

struct InputPaintResult {
    float advanceY = 0.0f;
    InputBounds bounds;
};

struct InputMeasureResult {
    float width = 0.0f;
    float height = 0.0f;
    float advanceY = 0.0f;
};

class InputPainter {
public:
    static InputMeasureResult Measure(const FontEngine& fonts,
                                      const Input& input,
                                      float containerWidth,
                                      float pixelScale = 1.0f);

    static InputPaintResult Paint(FrameBuffer& buffer,
                                  const FontEngine& fonts,
                                  Input& input,
                                  float originX,
                                  float originY,
                                  float containerWidth,
                                  float pixelScale = 1.0f,
                                  float forcedWidth = -1.0f,
                                  float forcedHeight = -1.0f);
};

}  // namespace Robo
