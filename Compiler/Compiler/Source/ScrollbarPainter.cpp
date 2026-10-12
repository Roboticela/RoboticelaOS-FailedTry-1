#include "Robo/ScrollbarPainter.hpp"
#include "Robo/Foundation.hpp"

#include <algorithm>
#include <cmath>

namespace Robo {

namespace {

constexpr float MIN_VISIBLE_OPACITY = 0.004f;

void FillRoundedRect(FrameBuffer& buffer,
                     float x,
                     float y,
                     float width,
                     float height,
                     float radius,
                     const Color& color,
                     float opacity) {
    buffer.FillRoundedRect(x, y, width, height, radius, color, opacity);
    return;
    if (width <= 0.0f || height <= 0.0f || color.alpha == 0) {
        return;
    }

    const float cappedRadius = std::max(0.0f, std::min({radius, width * 0.5f, height * 0.5f}));
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const int x1 = static_cast<int>(std::ceil(x + width));
    const int y1 = static_cast<int>(std::ceil(y + height));
    const float left = x + cappedRadius;
    const float right = x + width - cappedRadius;
    const float top = y + cappedRadius;
    const float bottom = y + height - cappedRadius;
    const float radiusSq = cappedRadius * cappedRadius;

    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            const float fx = static_cast<float>(px) + 0.5f;
            const float fy = static_cast<float>(py) + 0.5f;
            if (fx < x || fx > x + width || fy < y || fy > y + height) {
                continue;
            }

            bool inside = true;
            if (cappedRadius > 0.0f) {
                float cx = fx;
                float cy = fy;
                if (fx < left && fy < top) {
                    cx = left;
                    cy = top;
                } else if (fx > right && fy < top) {
                    cx = right;
                    cy = top;
                } else if (fx < left && fy > bottom) {
                    cx = left;
                    cy = bottom;
                } else if (fx > right && fy > bottom) {
                    cx = right;
                    cy = bottom;
                }

                if (cx != fx || cy != fy) {
                    const float dx = fx - cx;
                    const float dy = fy - cy;
                    inside = (dx * dx + dy * dy) <= radiusSq;
                }
            }

            if (inside) {
                buffer.BlendPixel(px, py, color, opacity);
            }
        }
    }
}

float SignedDistanceToLine(float px, float py, float ax, float ay, float bx, float by) {
    const float vx = bx - ax;
    const float vy = by - ay;
    const float length = std::sqrt(vx * vx + vy * vy);
    if (length <= 0.001f) {
        return 0.0f;
    }
    return ((px - ax) * vy - (py - ay) * vx) / length;
}

// Filled triangle. `forward` points down or right.
void PaintFilledArrow(FrameBuffer& buffer,
                      const ScrollbarBounds& cap,
                      bool vertical,
                      bool forward,
                      const Color& color,
                      float opacity) {
    if (cap.width < 4.0f || cap.height < 4.0f || opacity <= 0.0f || color.alpha == 0) {
        return;
    }
    const float along = vertical ? cap.height : cap.width;
    const float cross = vertical ? cap.width : cap.height;
    const float alongCenter = (vertical ? cap.y : cap.x) + along * 0.5f;
    const float crossCenter = (vertical ? cap.x : cap.y) + cross * 0.5f;
    const float halfBase = cross * 0.34f;
    const float halfHeight = std::min(along, cross) * 0.28f;
    const float tipAlong = forward ? alongCenter + halfHeight : alongCenter - halfHeight;
    const float baseAlong = forward ? alongCenter - halfHeight : alongCenter + halfHeight;
    const auto toX = [vertical](float crossCoord, float alongCoord) {
        return vertical ? crossCoord : alongCoord;
    };
    const auto toY = [vertical](float crossCoord, float alongCoord) {
        return vertical ? alongCoord : crossCoord;
    };
    float ax = toX(crossCenter, tipAlong);
    float ay = toY(crossCenter, tipAlong);
    float bx = toX(crossCenter + halfBase, baseAlong);
    float by = toY(crossCenter + halfBase, baseAlong);
    float cx = toX(crossCenter - halfBase, baseAlong);
    float cy = toY(crossCenter - halfBase, baseAlong);
    const float winding = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
    if (winding > 0.0f) {
        const float swapX = bx;
        const float swapY = by;
        bx = cx;
        by = cy;
        cx = swapX;
        cy = swapY;
    }
    const int x0 = static_cast<int>(std::floor(std::min({ax, bx, cx}) - 1.0f));
    const int y0 = static_cast<int>(std::floor(std::min({ay, by, cy}) - 1.0f));
    const int x1 = static_cast<int>(std::ceil(std::max({ax, bx, cx}) + 1.0f));
    const int y1 = static_cast<int>(std::ceil(std::max({ay, by, cy}) + 1.0f));
    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            const float sampleX = static_cast<float>(px) + 0.5f;
            const float sampleY = static_cast<float>(py) + 0.5f;
            const float inside = std::min({SignedDistanceToLine(sampleX, sampleY, ax, ay, bx, by),
                                           SignedDistanceToLine(sampleX, sampleY, bx, by, cx, cy),
                                           SignedDistanceToLine(sampleX, sampleY, cx, cy, ax, ay)});
            const float coverage = std::max(0.0f, std::min(1.0f, inside + 0.5f));
            if (coverage > 0.0f) {
                buffer.BlendPixel(px, py, color, opacity * coverage);
            }
        }
    }
}

}  // namespace

ScrollbarMeasureResult ScrollbarPainter::Measure(const Scrollbar& scrollbar,
                                                 float containerWidth,
                                                 float pixelScale) {
    const float scale = std::max(0.5f, pixelScale);
    ScrollbarStyle style = scrollbar.GetStyle();
    Resolve(style);

    ScrollbarMeasureResult result;
    // A scrollbar that is invisible because nothing overflows must not reserve
    // any layout space at all.
    if (!scrollbar.IsVisible()) {
        return result;
    }

    const float thickness = static_cast<float>(style.thickness) * scale;
    float length = static_cast<float>(style.length) * scale;
    if (style.orientation == ScrollbarOrientation::Horizontal && containerWidth > 0.0f) {
        length = std::min(length, containerWidth);
    }
    length = std::max(0.0f, length);

    if (style.orientation == ScrollbarOrientation::Vertical) {
        result.width = thickness;
        result.height = length;
    } else {
        result.width = length;
        result.height = thickness;
    }
    result.advanceY = result.height + 16.0f * scale;
    return result;
}

ScrollbarPaintResult ScrollbarPainter::Paint(FrameBuffer& buffer,
                                             Scrollbar& scrollbar,
                                             float originX,
                                             float originY,
                                             float containerWidth,
                                             float pixelScale,
                                             float forcedWidth,
                                             float forcedHeight) {
    const float scale = std::max(0.5f, pixelScale);
    ScrollbarStyle style = scrollbar.GetStyle();
    Resolve(style);

    const ScrollbarMeasureResult natural = Measure(scrollbar, containerWidth, scale);

    const float trackWidth = forcedWidth > 0.0f ? forcedWidth : natural.width;
    const float trackHeight = forcedHeight > 0.0f ? forcedHeight : natural.height;

    const float trackX = originX;
    const float trackY = originY;

    ScrollbarBounds bounds;
    bounds.x = trackX;
    bounds.y = trackY;
    bounds.width = std::max(0.0f, trackWidth);
    bounds.height = std::max(0.0f, trackHeight);

    scrollbar.SetPixelScale(scale);
    scrollbar.SetBounds(bounds);

    ScrollbarPaintResult result;
    result.bounds = bounds;
    result.advanceY = bounds.height + 16.0f * scale;

    // Overflow-less bars collapse to zero bounds so hit testing ignores them.
    if (!scrollbar.IsVisible()) {
        ScrollbarBounds empty;
        empty.x = trackX;
        empty.y = trackY;
        scrollbar.SetBounds(empty);
        result.bounds = empty;
        result.advanceY = 0.0f;
        return result;
    }

    const float opacity = scrollbar.ResolveEffectiveOpacity();
    if (opacity <= MIN_VISIBLE_OPACITY) {
        // Auto-hidden: keep the geometry (so a hover can reveal it) but draw nothing.
        return result;
    }

    const bool vertical = style.orientation == ScrollbarOrientation::Vertical;
    if (bounds.width > 0.0f && bounds.height > 0.0f) {
        const Color trackColor = scrollbar.ResolveTrackColor();
        const Color laneColor = style.hasCustomTrackColor
                                    ? trackColor
                                    : ResolveColor(Color::FromHex("#F5F5F7"), "color.bg.secondary");
        if (laneColor.alpha > 0) {
            buffer.FillRect(static_cast<int>(std::floor(bounds.x)),
                            static_cast<int>(std::floor(bounds.y)),
                            std::max(1, static_cast<int>(std::ceil(bounds.width))),
                            std::max(1, static_cast<int>(std::ceil(bounds.height))),
                            laneColor,
                            opacity);
        }
        const Color hairline = ResolveColor(Color::FromHex("#E5E5E7"), "color.border.subtle");
        const int line = std::max(1, static_cast<int>(std::round(scale)));
        if (vertical) {
            buffer.FillRect(static_cast<int>(std::floor(bounds.x)),
                            static_cast<int>(std::floor(bounds.y)),
                            line,
                            std::max(1, static_cast<int>(std::ceil(bounds.height))),
                            hairline,
                            opacity);
        } else {
            buffer.FillRect(static_cast<int>(std::floor(bounds.x)),
                            static_cast<int>(std::floor(bounds.y)),
                            std::max(1, static_cast<int>(std::ceil(bounds.width))),
                            line,
                            hairline,
                            opacity);
        }
    }

    const double arrowSize = scrollbar.ResolveArrowSize();
    if (arrowSize > 0.0) {
        const ScrollbarArrow arrowEnds[2] = {ScrollbarArrow::Start, ScrollbarArrow::End};
        for (ScrollbarArrow arrow : arrowEnds) {
            const ScrollbarBounds cap = scrollbar.ResolveArrowBounds(arrow);
            PaintFilledArrow(buffer,
                             cap,
                             vertical,
                             arrow == ScrollbarArrow::End,
                             scrollbar.ResolveArrowColor(arrow),
                             opacity);
        }
    }

    const float inset = 1.0f * scale;
    ScrollbarBounds thumb = scrollbar.ResolveThumbBounds();
    if (vertical) {
        thumb.x = bounds.x + inset;
        thumb.width = std::max(1.0f, bounds.width - inset * 2.0f);
    } else {
        thumb.y = bounds.y + inset;
        thumb.height = std::max(1.0f, bounds.height - inset * 2.0f);
    }
    const Color thumbColor = scrollbar.ResolveThumbColor();
    if (thumbColor.alpha > 0 && thumb.width > 0.0f && thumb.height > 0.0f) {
        const float thumbRadius = std::min(thumb.width, thumb.height) * 0.5f;
        FillRoundedRect(buffer, thumb.x, thumb.y, thumb.width, thumb.height, thumbRadius, thumbColor, opacity);
    }

    return result;
}

}  // namespace Robo
