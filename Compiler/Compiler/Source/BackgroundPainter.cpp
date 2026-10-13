#include "Robo/BackgroundPainter.hpp"

#include "Robo/SvgPainter.hpp"

#include <algorithm>
#include <cstring>

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
    const bool fitWidth = static_cast<long long>(surfaceWidth) * sourceHeight >= static_cast<long long>(surfaceHeight) * sourceWidth;
    const int drawnWidth = fitWidth ? surfaceWidth : static_cast<int>((static_cast<long long>(sourceWidth) * surfaceHeight) / sourceHeight);
    const int drawnHeight = fitWidth ? static_cast<int>((static_cast<long long>(sourceHeight) * surfaceWidth) / sourceWidth) : surfaceHeight;
    const int originX = (surfaceWidth - drawnWidth) / 2;
    const int originY = (surfaceHeight - drawnHeight) / 2;
    const int x0 = std::max(0, x);
    const int y0 = std::max(0, y);
    const int x1 = std::min(surfaceWidth, x + width);
    const int y1 = std::min(surfaceHeight, y + height);
    if (x0 >= x1 || y0 >= y1 || drawnWidth <= 0 || drawnHeight <= 0) {
        return;
    }
    std::uint8_t* destination = buffer.GetPixels();
    if (sourceWidth == surfaceWidth && sourceHeight == surfaceHeight && drawnWidth == surfaceWidth && drawnHeight == surfaceHeight) {
        const int stride = surfaceWidth * 4;
        for (int py = y0; py < y1; ++py) {
            std::uint8_t* dest = destination + static_cast<std::size_t>(py) * static_cast<std::size_t>(stride) + static_cast<std::size_t>(x0) * 4u;
            const std::uint8_t* source = pixels + static_cast<std::size_t>(py) * static_cast<std::size_t>(stride) + static_cast<std::size_t>(x0) * 4u;
            memcpy(dest, source, static_cast<std::size_t>(x1 - x0) * 4u);
        }
        return;
    }
    const int stride = surfaceWidth * 4;
    for (int py = y0; py < y1; ++py) {
        const int sourceY = py < originY ? 0 : std::min(sourceHeight - 1, ((py - originY) * sourceHeight) / drawnHeight);
        const std::uint8_t* row = pixels + (static_cast<std::size_t>(sourceY) * static_cast<std::size_t>(sourceWidth)) * 4u;
        std::uint8_t* dest = destination + static_cast<std::size_t>(py) * static_cast<std::size_t>(stride) + static_cast<std::size_t>(x0) * 4u;
        int sourceX = 0;
        int accumulator = 0;
        if (x0 > originX) {
            const int delta = x0 - originX;
            sourceX = (delta * sourceWidth) / drawnWidth;
            accumulator = (delta * sourceWidth) % drawnWidth;
        }
        for (int px = x0; px < x1; ++px) {
            const int sampleX = sourceX < 0 ? 0 : std::min(sourceWidth - 1, sourceX);
            const std::uint8_t* source = row + static_cast<std::size_t>(sampleX) * 4u;
            dest[0] = source[0];
            dest[1] = source[1];
            dest[2] = source[2];
            dest[3] = 255;
            dest += 4;
            if (px >= originX) {
                accumulator += sourceWidth;
                while (accumulator >= drawnWidth) {
                    accumulator -= drawnWidth;
                    sourceX += 1;
                }
            }
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
