#include "Robo/SvgPainter.hpp"
#include "Robo/Foundation.hpp"
#include "Robo/TextSelection.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

namespace Robo {

namespace {

struct Point {
    float x = 0.0f;
    float y = 0.0f;
};

struct Subpath {
    std::vector<Point> points;
    bool closed = false;
};

float ResolveLength(const std::string& spec, float fallback, float container, float scale) {
    if (spec.empty()) {
        return fallback;
    }
    if (spec.size() > 1 && spec.back() == '%') {
        return container * static_cast<float>(std::atof(spec.c_str())) / 100.0f;
    }
    return static_cast<float>(std::atof(spec.c_str())) * scale;
}

void GraphicSize(const Svg& svg, float containerWidth, float scale, float& outWidth, float& outHeight) {
    SvgStyle style = svg.GetStyle();
    Resolve(style);
    const float viewW = style.viewWidth > 0.0f ? style.viewWidth : 24.0f;
    const float viewH = style.viewHeight > 0.0f ? style.viewHeight : 24.0f;
    const float cssWidth = style.width > 0.0f ? style.width : 24.0f;
    float width = style.widthSpec.empty() ? cssWidth * scale : ResolveLength(style.widthSpec, cssWidth * scale, containerWidth, scale);
    float height = 0.0f;
    if (!style.heightSpec.empty()) {
        height = ResolveLength(style.heightSpec, width * viewH / viewW, containerWidth, scale);
    } else if (style.height > 0.0f) {
        height = style.height * scale;
    } else {
        height = width * viewH / viewW;
    }
    outWidth = std::max(1.0f, width);
    outHeight = std::max(1.0f, height);
}

void AddFlattenedCubic(const Point& p0, const Point& p1, const Point& p2, const Point& p3, std::vector<Point>& out, int depth) {
    const float midX = (p0.x + 3.0f * p1.x + 3.0f * p2.x + p3.x) * 0.125f;
    const float midY = (p0.y + 3.0f * p1.y + 3.0f * p2.y + p3.y) * 0.125f;
    const float chordX = (p0.x + p3.x) * 0.5f;
    const float chordY = (p0.y + p3.y) * 0.5f;
    const float flat = std::fabs(midX - chordX) + std::fabs(midY - chordY);
    if (depth > 8 || flat < 0.35f) {
        out.push_back(p3);
        return;
    }
    const Point a{(p0.x + p1.x) * 0.5f, (p0.y + p1.y) * 0.5f};
    const Point b{(p1.x + p2.x) * 0.5f, (p1.y + p2.y) * 0.5f};
    const Point c{(p2.x + p3.x) * 0.5f, (p2.y + p3.y) * 0.5f};
    const Point d{(a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f};
    const Point e{(b.x + c.x) * 0.5f, (b.y + c.y) * 0.5f};
    const Point f{(d.x + e.x) * 0.5f, (d.y + e.y) * 0.5f};
    AddFlattenedCubic(p0, a, d, f, out, depth + 1);
    AddFlattenedCubic(f, e, c, p3, out, depth + 1);
}

void FlattenShape(const SvgShape& shape, float originX, float originY, float scaleX, float scaleY, std::vector<Subpath>& paths) {
    Subpath current;
    Point pen{originX, originY};
    Point start{originX, originY};
    bool hasPen = false;
    auto map = [&](float x, float y) {
        return Point{originX + x * scaleX, originY + y * scaleY};
    };
    auto ensure = [&]() {
        if (current.points.empty() && hasPen) {
            current.points.push_back(pen);
        }
    };
    for (const SvgCommand& command : shape.commands) {
        if (command.kind == SvgCommandKind::Move) {
            if (current.points.size() > 1) {
                paths.push_back(current);
            }
            current = Subpath();
            pen = map(command.x, command.y);
            start = pen;
            hasPen = true;
            current.points.push_back(pen);
        } else if (command.kind == SvgCommandKind::Line) {
            ensure();
            pen = map(command.x, command.y);
            hasPen = true;
            current.points.push_back(pen);
        } else if (command.kind == SvgCommandKind::Cubic) {
            ensure();
            const Point p1 = map(command.c1x, command.c1y);
            const Point p2 = map(command.c2x, command.c2y);
            const Point p3 = map(command.x, command.y);
            AddFlattenedCubic(pen, p1, p2, p3, current.points, 0);
            pen = p3;
            hasPen = true;
        } else if (command.kind == SvgCommandKind::Close) {
            if (current.points.size() > 1) {
                current.closed = true;
                current.points.push_back(start);
                paths.push_back(current);
            }
            current = Subpath();
            pen = start;
        }
    }
    if (current.points.size() > 1) {
        paths.push_back(current);
    }
}

struct Crossing {
    float x = 0.0f;
    int direction = 0;
};

void FillMask(const std::vector<Subpath>& paths,
              SvgFillRule rule,
              int originX,
              int originY,
              int width,
              int height,
              int sample,
              std::vector<unsigned char>& mask) {
    mask.assign(static_cast<std::size_t>(width * height), 0);
    for (int row = 0; row < height; ++row) {
        const float y = static_cast<float>(originY) + (static_cast<float>(row) + 0.5f) / static_cast<float>(sample);
        std::vector<Crossing> crossings;
        for (const Subpath& path : paths) {
            for (std::size_t i = 1; i < path.points.size(); ++i) {
                const Point& a = path.points[i - 1];
                const Point& b = path.points[i];
                const bool up = a.y <= y && b.y > y;
                const bool down = b.y <= y && a.y > y;
                if (!up && !down) {
                    continue;
                }
                const float t = (y - a.y) / (b.y - a.y);
                Crossing crossing;
                crossing.x = a.x + t * (b.x - a.x);
                crossing.direction = up ? 1 : -1;
                crossings.push_back(crossing);
            }
        }
        if (crossings.size() < 2) {
            continue;
        }
        std::sort(crossings.begin(), crossings.end(), [](const Crossing& left, const Crossing& right) {
            return left.x < right.x;
        });
        if (rule == SvgFillRule::EvenOdd) {
            for (std::size_t i = 0; i + 1 < crossings.size(); i += 2) {
                int x0 = static_cast<int>(std::ceil((crossings[i].x - static_cast<float>(originX)) * static_cast<float>(sample)));
                int x1 = static_cast<int>(std::floor((crossings[i + 1].x - static_cast<float>(originX)) * static_cast<float>(sample)));
                x0 = std::max(0, x0);
                x1 = std::min(width - 1, x1);
                unsigned char* scan = mask.data() + static_cast<std::size_t>(row * width);
                for (int x = x0; x <= x1; ++x) {
                    scan[x] = 1;
                }
            }
        } else {
            int winding = 0;
            for (std::size_t i = 0; i + 1 < crossings.size(); ++i) {
                winding += crossings[i].direction;
                if (winding == 0) {
                    continue;
                }
                int x0 = static_cast<int>(std::ceil((crossings[i].x - static_cast<float>(originX)) * static_cast<float>(sample)));
                int x1 = static_cast<int>(std::floor((crossings[i + 1].x - static_cast<float>(originX)) * static_cast<float>(sample)));
                x0 = std::max(0, x0);
                x1 = std::min(width - 1, x1);
                unsigned char* scan = mask.data() + static_cast<std::size_t>(row * width);
                for (int x = x0; x <= x1; ++x) {
                    scan[x] = 1;
                }
            }
        }
    }
}

void StrokeMask(const std::vector<Subpath>& paths,
                float radius,
                int originX,
                int originY,
                int width,
                int height,
                int sample,
                std::vector<unsigned char>& mask) {
    mask.assign(static_cast<std::size_t>(width * height), 0);
    if (radius <= 0.0f) {
        return;
    }
    const float radius2 = radius * radius;
    for (const Subpath& path : paths) {
        for (std::size_t i = 1; i < path.points.size(); ++i) {
            const Point& a = path.points[i - 1];
            const Point& b = path.points[i];
            const int minX = std::max(0, static_cast<int>(std::floor((std::min(a.x, b.x) - radius - static_cast<float>(originX)) * sample)));
            const int maxX = std::min(width - 1, static_cast<int>(std::ceil((std::max(a.x, b.x) + radius - static_cast<float>(originX)) * sample)));
            const int minY = std::max(0, static_cast<int>(std::floor((std::min(a.y, b.y) - radius - static_cast<float>(originY)) * sample)));
            const int maxY = std::min(height - 1, static_cast<int>(std::ceil((std::max(a.y, b.y) + radius - static_cast<float>(originY)) * sample)));
            const float dx = b.x - a.x;
            const float dy = b.y - a.y;
            const float len2 = dx * dx + dy * dy;
            for (int row = minY; row <= maxY; ++row) {
                const float py = static_cast<float>(originY) + (static_cast<float>(row) + 0.5f) / static_cast<float>(sample);
                unsigned char* scan = mask.data() + static_cast<std::size_t>(row * width);
                for (int col = minX; col <= maxX; ++col) {
                    const float px = static_cast<float>(originX) + (static_cast<float>(col) + 0.5f) / static_cast<float>(sample);
                    float t = 0.0f;
                    if (len2 > 0.00001f) {
                        t = ((px - a.x) * dx + (py - a.y) * dy) / len2;
                        t = std::max(0.0f, std::min(1.0f, t));
                    }
                    const float qx = a.x + t * dx;
                    const float qy = a.y + t * dy;
                    const float distX = px - qx;
                    const float distY = py - qy;
                    if (distX * distX + distY * distY <= radius2) {
                        scan[col] = 1;
                    }
                }
            }
        }
    }
}

void BlitMask(FrameBuffer& buffer,
              const std::vector<unsigned char>& mask,
              int originX,
              int originY,
              int width,
              int height,
              int sample,
              const Color& color,
              float opacity) {
    const int destW = (width + sample - 1) / sample;
    const int destH = (height + sample - 1) / sample;
    const float sampleCount = static_cast<float>(sample * sample);
    for (int row = 0; row < destH; ++row) {
        for (int col = 0; col < destW; ++col) {
            int covered = 0;
            const int y0 = row * sample;
            const int x0 = col * sample;
            const int y1 = std::min(height, y0 + sample);
            const int x1 = std::min(width, x0 + sample);
            for (int y = y0; y < y1; ++y) {
                const unsigned char* scan = mask.data() + static_cast<std::size_t>(y * width);
                for (int x = x0; x < x1; ++x) {
                    covered += scan[x];
                }
            }
            if (covered == 0) {
                continue;
            }
            buffer.BlendPixel(originX + col, originY + row, color, (static_cast<float>(covered) / sampleCount) * opacity);
        }
    }
}

void PaintShape(FrameBuffer& buffer, const SvgShape& shape, const SvgStyle& style, float drawX, float drawY, float fitScale) {
    std::vector<Subpath> paths;
    FlattenShape(shape, drawX, drawY, fitScale, fitScale, paths);
    if (paths.empty()) {
        return;
    }
    float minX = paths[0].points[0].x;
    float minY = paths[0].points[0].y;
    float maxX = minX;
    float maxY = minY;
    for (const Subpath& path : paths) {
        for (const Point& point : path.points) {
            minX = std::min(minX, point.x);
            minY = std::min(minY, point.y);
            maxX = std::max(maxX, point.x);
            maxY = std::max(maxY, point.y);
        }
    }
    const float radius = shape.hasStroke ? shape.strokeWidth * fitScale * 0.5f : 0.0f;
    minX -= radius + 1.0f;
    minY -= radius + 1.0f;
    maxX += radius + 1.0f;
    maxY += radius + 1.0f;
    const int sample = (maxX - minX) * (maxY - minY) > 180000.0f ? 2 : 4;
    const int originX = static_cast<int>(std::floor(minX));
    const int originY = static_cast<int>(std::floor(minY));
    const int maskW = std::max(1, static_cast<int>(std::ceil((maxX - static_cast<float>(originX)) * sample)));
    const int maskH = std::max(1, static_cast<int>(std::ceil((maxY - static_cast<float>(originY)) * sample)));
    if (maskW > 4096 || maskH > 4096 || static_cast<long long>(maskW) * maskH > 4000000) {
        return;
    }
    std::vector<unsigned char> mask;
    const float opacity = style.opacity * shape.opacity;
    if (shape.hasFill) {
        FillMask(paths, shape.fillRule, originX, originY, maskW, maskH, sample, mask);
        Color fill = shape.fill;
        if (!shape.fillToken.empty()) {
            fill = ResolveColor(shape.fill, shape.fillToken);
        } else if (shape.currentFill) {
            fill = style.color;
        }
        BlitMask(buffer, mask, originX, originY, maskW, maskH, sample, fill, opacity);
    }
    if (shape.hasStroke && radius > 0.0f) {
        StrokeMask(paths, radius, originX, originY, maskW, maskH, sample, mask);
        Color stroke = shape.stroke;
        if (!shape.strokeToken.empty()) {
            stroke = ResolveColor(shape.stroke, shape.strokeToken);
        } else if (shape.currentStroke) {
            stroke = style.color;
        }
        BlitMask(buffer, mask, originX, originY, maskW, maskH, sample, stroke, opacity);
    }
}

}  // namespace

SvgMeasureResult SvgPainter::Measure(const Svg& svg, float pixelScale) {
    float width = 0.0f;
    float height = 0.0f;
    GraphicSize(svg, 100000.0f, pixelScale, width, height);
    SvgMeasureResult result;
    result.width = width;
    result.height = height;
    return result;
}

SvgPaintResult SvgPainter::Paint(FrameBuffer& buffer,
                                 Svg& svg,
                                 float originX,
                                 float originY,
                                 float containerWidth,
                                 float pixelScale) {
    const SelectableScope selection(&svg, svg.GetStyle().selectable);
    SvgStyle style = svg.GetStyle();
    Resolve(style);
    float width = 0.0f;
    float height = 0.0f;
    GraphicSize(svg, containerWidth, pixelScale, width, height);
    float drawX = originX;
    if (style.align == TextAlign::Center) {
        drawX = originX + std::max(0.0f, (containerWidth - width) * 0.5f);
    } else if (style.align == TextAlign::Right) {
        drawX = originX + std::max(0.0f, containerWidth - width);
    }
    const float viewW = style.viewWidth > 0.0f ? style.viewWidth : 24.0f;
    const float viewH = style.viewHeight > 0.0f ? style.viewHeight : 24.0f;
    const float fit = std::min(width / viewW, height / viewH);
    const float fittedW = viewW * fit;
    const float fittedH = viewH * fit;
    const float contentX = drawX + (width - fittedW) * 0.5f - style.viewX * fit;
    const float contentY = originY + (height - fittedH) * 0.5f - style.viewY * fit;
    for (const SvgShape& shape : svg.GetShapes()) {
        PaintShape(buffer, shape, style, contentX, contentY, fit);
    }
    SvgBounds bounds;
    bounds.x = drawX;
    bounds.y = originY;
    bounds.width = width;
    bounds.height = height;
    svg.SetBounds(bounds);
    SvgPaintResult result;
    result.advanceY = height + 12.0f * pixelScale;
    return result;
}

}  // namespace Robo
