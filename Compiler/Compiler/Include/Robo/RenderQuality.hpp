#pragma once

namespace Robo {

// Paint scale against the framebuffer. 1 copies straight across. 2 is the retina paint.
// Values between, such as 1.3, are valid. This is the only number to change.
constexpr float ROBO_RENDER_QUALITY = 1.3f;

inline float RenderQuality() {
    float quality = ROBO_RENDER_QUALITY;
    if (quality < 1.0f) {
        quality = 1.0f;
    }
    if (quality > 2.0f) {
        quality = 2.0f;
    }
    return quality;
}

inline int PaintSize(int pixels, float quality) {
    if (pixels <= 0) {
        return 1;
    }
    if (quality <= 1.001f) {
        return pixels;
    }
    const int size = static_cast<int>(static_cast<float>(pixels) * quality + 0.5f);
    return size > pixels ? size : pixels + 1;
}

}  // namespace Robo
