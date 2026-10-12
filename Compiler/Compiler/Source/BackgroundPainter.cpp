#include "Robo/BackgroundPainter.hpp"

#include "Robo/SvgPainter.hpp"

#include <algorithm>

namespace Robo {

namespace {

void PaintPixels(FrameBuffer& buffer,
                 const std::uint8_t* pixels,
                 int sourceWidth,
                 int sourceHeight,
                 int surfaceWidth,
                 int surfaceHeight,
                 int x,
                 int y,
                 int width,
                 int height) {
    if (pixels == nullptr || sourceWidth <= 0 || sourceHeight <= 0 || surfaceWidth <= 0 || surfaceHeight <= 0) {
        return;
    }
    const float scale = std::max(static_cast<float>(surfaceWidth) / static_cast<float>(sourceWidth),
                                  static_cast<float>(surfaceHeight) / static_cast<float>(sourceHeight));
    const float drawnWidth = static_cast<float>(sourceWidth) * scale;
    const float drawnHeight = static_cast<float>(sourceHeight) * scale;
    const float originX = (static_cast<float>(surfaceWidth) - drawnWidth) * 0.5f;
    const float originY = (static_cast<float>(surfaceHeight) - drawnHeight) * 0.5f;
    const int x1 = std::min(surfaceWidth, x + width);
    const int y1 = std::min(surfaceHeight, y + height);
    for (int py = std::max(0, y); py < y1; ++py) {
        const int sourceY = std::max(0, std::min(sourceHeight - 1, static_cast<int>((static_cast<float>(py) - originY) / scale)));
        for (int px = std::max(0, x); px < x1; ++px) {
            const int sourceX = std::max(0, std::min(sourceWidth - 1, static_cast<int>((static_cast<float>(px) - originX) / scale)));
            const std::size_t index = (static_cast<std::size_t>(sourceY) * static_cast<std::size_t>(sourceWidth) + static_cast<std::size_t>(sourceX)) * 4u;
            const Color color = Color::FromRgba(pixels[index], pixels[index + 1], pixels[index + 2], pixels[index + 3]);
            buffer.BlendPixel(px, py, color, 1.0f);
        }
    }
}

}  // namespace

void BackgroundPainter::Paint(FrameBuffer& buffer,
                              Background& background,
                              int surfaceWidth,
                              int surfaceHeight,
                              int x,
                              int y,
                              int width,
                              int height) {
    if (width <= 0 || height <= 0) {
        return;
    }
    if (background.GetKind() == BackgroundKind::Svg) {
        Svg* graphic = background.Graphic();
        if (graphic == nullptr) {
            return;
        }
        SvgStyle style = graphic->GetStyle();
        const float viewWidth = style.viewWidth > 0.0f ? style.viewWidth : static_cast<float>(surfaceWidth);
        const float viewHeight = style.viewHeight > 0.0f ? style.viewHeight : static_cast<float>(surfaceHeight);
        const float scale = std::max(static_cast<float>(surfaceWidth) / viewWidth, static_cast<float>(surfaceHeight) / viewHeight);
        const float drawnWidth = viewWidth * scale;
        const float drawnHeight = viewHeight * scale;
        style.width = drawnWidth;
        style.height = drawnHeight;
        style.widthSpec.clear();
        style.heightSpec.clear();
        graphic->SetStyle(style);
        SvgPainter::Paint(buffer, *graphic, (static_cast<float>(surfaceWidth) - drawnWidth) * 0.5f,
                          (static_cast<float>(surfaceHeight) - drawnHeight) * 0.5f, drawnWidth, 1.0f);
        return;
    }
    PaintPixels(buffer, background.Pixels(), background.PixelWidth(), background.PixelHeight(), surfaceWidth, surfaceHeight, x, y, width, height);
}

}  // namespace Robo
