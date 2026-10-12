#include "Robo/ContainerPainter.hpp"
#include "Robo/Foundation.hpp"
#include "Robo/TextSelection.hpp"
#include "Robo/BrandLogoPainter.hpp"
#include "Robo/TitleBarPainter.hpp"
#include "Robo/SidebarPainter.hpp"
#include "Robo/SeparatorPainter.hpp"
#include "Robo/ThemeIconTogglePainter.hpp"
#include "Robo/ThemeTogglePainter.hpp"
#include "Robo/ButtonPainter.hpp"
#include "Robo/FlexboxPainter.hpp"
#include "Robo/GridPainter.hpp"
#include "Robo/InputPainter.hpp"
#include "Robo/PositionPainter.hpp"
#include "Robo/ScrollbarPainter.hpp"
#include "Robo/SvgPainter.hpp"
#include "Robo/TextPainter.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

namespace Robo {

namespace {

float ParseDimension(const std::string& str, float containerRef, float fallback, float scale = 1.0f) {
    if (str.empty() || str == "auto") {
        return fallback;
    }
    const std::size_t pctPos = str.find('%');
    if (pctPos != std::string::npos) {
        const float pct = static_cast<float>(std::atof(str.substr(0, pctPos).c_str()));
        return containerRef * (pct / 100.0f);
    }
    const std::size_t pxPos = str.find("px");
    if (pxPos != std::string::npos) {
        return static_cast<float>(std::atof(str.substr(0, pxPos).c_str())) * scale;
    }
    return static_cast<float>(std::atof(str.c_str())) * scale;
}

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
    if (width <= 0.0f || height <= 0.0f || opacity <= 0.0f || color.alpha == 0) {
        return;
    }

    const int x0 = std::max(0, static_cast<int>(std::floor(x)));
    const int y0 = std::max(0, static_cast<int>(std::floor(y)));
    const int x1 = std::min(buffer.GetWidth(), static_cast<int>(std::ceil(x + width)));
    const int y1 = std::min(buffer.GetHeight(), static_cast<int>(std::ceil(y + height)));

    const float cappedRadius = std::max(0.0f, std::min(radius, std::min(width, height) * 0.5f));
    const float left = x + cappedRadius;
    const float right = x + width - cappedRadius;
    const float top = y + cappedRadius;
    const float bottom = y + height - cappedRadius;

    auto DistanceToRoundedRect = [&](float fx, float fy) -> float {
        float cx = fx;
        float cy = fy;
        if (fx < left) {
            cx = left;
        } else if (fx > right) {
            cx = right;
        }
        if (fy < top) {
            cy = top;
        } else if (fy > bottom) {
            cy = bottom;
        }
        const float dx = fx - cx;
        const float dy = fy - cy;
        return std::sqrt(dx * dx + dy * dy) - cappedRadius;
    };

    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            const float fx = static_cast<float>(px) + 0.5f;
            const float fy = static_cast<float>(py) + 0.5f;
            if (fx >= left && fx <= right && fy >= y && fy <= y + height) {
                buffer.BlendPixel(px, py, color, opacity);
            } else if (fy >= top && fy <= bottom && fx >= x && fx <= x + width) {
                buffer.BlendPixel(px, py, color, opacity);
            } else {
                const float dist = DistanceToRoundedRect(fx, fy);
                if (dist <= 0.0f) {
                    buffer.BlendPixel(px, py, color, opacity);
                } else if (dist < 1.0f) {
                    buffer.BlendPixel(px, py, color, opacity * (1.0f - dist));
                }
            }
        }
    }
}

void StrokeRoundedRect(FrameBuffer& buffer,
                       float x,
                       float y,
                       float width,
                       float height,
                       float radius,
                       float strokeWidth,
                       const Color& color,
                       float opacity) {
    if (strokeWidth <= 0.0f || width <= 0.0f || height <= 0.0f || opacity <= 0.0f || color.alpha == 0) {
        return;
    }

    const float half = strokeWidth * 0.5f;
    const int x0 = std::max(0, static_cast<int>(std::floor(x - half)));
    const int y0 = std::max(0, static_cast<int>(std::floor(y - half)));
    const int x1 = std::min(buffer.GetWidth(), static_cast<int>(std::ceil(x + width + half)));
    const int y1 = std::min(buffer.GetHeight(), static_cast<int>(std::ceil(y + height + half)));

    const float cappedRadius = std::max(0.0f, std::min(radius, std::min(width, height) * 0.5f));
    const float left = x + cappedRadius;
    const float right = x + width - cappedRadius;
    const float top = y + cappedRadius;
    const float bottom = y + height - cappedRadius;

    auto DistanceToRoundedRect = [&](float fx, float fy) -> float {
        float cx = fx;
        float cy = fy;
        if (fx < left) {
            cx = left;
        } else if (fx > right) {
            cx = right;
        }
        if (fy < top) {
            cy = top;
        } else if (fy > bottom) {
            cy = bottom;
        }
        const float dx = fx - cx;
        const float dy = fy - cy;
        return std::sqrt(dx * dx + dy * dy) - cappedRadius;
    };

    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            const float fx = static_cast<float>(px) + 0.5f;
            const float fy = static_cast<float>(py) + 0.5f;
            const float dist = std::abs(DistanceToRoundedRect(fx, fy));
            if (dist <= half) {
                const float coverage = std::min(1.0f, half - dist + 0.5f);
                buffer.BlendPixel(px, py, color, opacity * coverage);
            }
        }
    }
}

bool IsVertical(const ContainerStyle& style) {
    return style.orientation == ContainerOrientation::Vertical;
}

// Measures one content child exactly like PositionPainter does.
void MeasureChild(const FontEngine& fonts,
                  PositionItem& item,
                  float innerAvailW,
                  float scale,
                  float& outWidth,
                  float& outHeight) {
    outWidth = 0.0f;
    outHeight = 0.0f;

    switch (item.kind) {
        case PositionItemKind::Text: {
            const TextMeasureResult res = TextPainter::Measure(fonts, item.text, innerAvailW, scale);
            outWidth = res.width;
            outHeight = res.height;
            break;
        }
        case PositionItemKind::Svg: {
            const SvgMeasureResult res = SvgPainter::Measure(item.svg, scale);
            outWidth = res.width;
            outHeight = res.height;
            break;
        }
        case PositionItemKind::BrandLogo: {
            const BrandLogoMeasureResult res = BrandLogoPainter::Measure(fonts, item.brandLogo, scale);
            outWidth = res.width;
            outHeight = res.height;
            break;
        }
        case PositionItemKind::TitleBar: {
            const TitleBarMeasureResult res = TitleBarPainter::Measure(fonts, item.titleBar, innerAvailW, scale);
            outWidth = res.width;
            outHeight = res.height;
            break;
        }
        case PositionItemKind::Sidebar: {
            const SidebarMeasureResult res = SidebarPainter::Measure(item.sidebar, 0.0f, scale);
            outWidth = res.width;
            outHeight = res.height;
            break;
        }
        case PositionItemKind::ThemeToggle: {
            const ThemeToggleMeasureResult res = ThemeTogglePainter::Measure(fonts, item.themeToggle, innerAvailW, scale);
            outWidth = res.width;
            outHeight = res.height;
            break;
        }
        case PositionItemKind::ThemeIconToggle: {
            const ThemeIconToggleMeasureResult res = ThemeIconTogglePainter::Measure(item.themeIconToggle, scale);
            outWidth = res.width;
            outHeight = res.height;
            break;
        }
        case PositionItemKind::SidebarItem: {
            const SidebarItemMeasureResult res = SidebarPainter::MeasureItem(fonts, item.sidebarItem, scale);
            outWidth = res.width;
            outHeight = res.height;
            break;
        }
        case PositionItemKind::Separator: {
            const SeparatorMeasureResult res = SeparatorPainter::Measure(fonts, item.separator, innerAvailW, 0.0f, scale);
            outWidth = res.width;
            outHeight = res.height;
            break;
        }
        case PositionItemKind::MotionShape: {
            outWidth = item.motionShape.Width() * scale;
            outHeight = item.motionShape.Height() * scale;
            break;
        }
        case PositionItemKind::Button: {
            const ButtonMeasureResult res = ButtonPainter::Measure(fonts, item.button, scale);
            outWidth = res.width;
            outHeight = res.height;
            break;
        }
        case PositionItemKind::Input: {
            const InputStyle& inputStyle = item.input.GetStyle();
            float intrinsicWidth = static_cast<float>(inputStyle.minWidth) * scale;
            if (!item.props.width.empty() && item.props.width != "auto") {
                intrinsicWidth = ParseDimension(item.props.width, innerAvailW, intrinsicWidth, scale);
            }
            const InputMeasureResult res = InputPainter::Measure(fonts, item.input, intrinsicWidth, scale);
            outWidth = intrinsicWidth;
            outHeight = res.height;
            break;
        }
        case PositionItemKind::Scrollbar: {
            const ScrollbarMeasureResult res = ScrollbarPainter::Measure(item.scrollbar, innerAvailW, scale);
            outWidth = res.width;
            outHeight = res.height;
            break;
        }
        case PositionItemKind::Container: {
            if (item.container) {
                const ContainerMeasureResult res =
                    ContainerPainter::Measure(fonts, *item.container, innerAvailW, scale);
                outWidth = res.width;
                outHeight = res.height;
            }
            break;
        }
        case PositionItemKind::Flexbox: {
            if (item.flexbox) {
                const FlexboxMeasureResult res = FlexboxPainter::Measure(fonts, *item.flexbox, innerAvailW, scale);
                outWidth = res.width;
                outHeight = res.height;
            }
            break;
        }
        case PositionItemKind::Grid: {
            if (item.grid) {
                const GridMeasureResult res = GridPainter::Measure(fonts, *item.grid, innerAvailW, scale);
                outWidth = res.width;
                outHeight = res.height;
            }
            break;
        }
        case PositionItemKind::Position: {
            if (item.position) {
                const PositionMeasureResult res = PositionPainter::Measure(fonts, *item.position, innerAvailW, scale);
                outWidth = res.width;
                outHeight = res.height;
            }
            break;
        }
    }

    if (!item.props.width.empty() && item.props.width != "auto") {
        outWidth = ParseDimension(item.props.width, innerAvailW, outWidth, scale);
    }
    if (!item.props.height.empty() && item.props.height != "auto") {
        outHeight = std::max(ParseDimension(item.props.height, 0.0f, outHeight, scale), outHeight);
    }
}

// Children without an explicit offset are stacked along the scroll axis, so a
// container behaves like a block flow while still honouring explicit left/top.
// `crossExtent` is the cross-axis space to stretch block-like children into, or
// a negative value when the cross size is still unknown.
float StackChildren(const FontEngine& fonts,
                    Container& container,
                    float innerAvailW,
                    float crossExtent,
                    float scale) {
    ContainerStyle style = container.GetStyle();
    Resolve(style);
    const bool vertical = IsVertical(style);
    const float gap = std::max(0.0f, style.gap * scale);

    float cursor = 0.0f;
    float extent = 0.0f;
    bool first = true;

    for (PositionItem& item : container.GetContent().GetItems()) {
        float childW = 0.0f;
        float childH = 0.0f;
        MeasureChild(fonts, item, innerAvailW, scale, childW, childH);

        // Cross-axis fill: block-like children stretch across the scroll axis.
        const bool stretchy = item.kind == PositionItemKind::Text || item.kind == PositionItemKind::Input ||
                              item.kind == PositionItemKind::Container || item.kind == PositionItemKind::Flexbox ||
                              item.kind == PositionItemKind::Grid || item.kind == PositionItemKind::Position;
        if (stretchy) {
            if (vertical) {
                if (item.props.width.empty() || item.props.width == "auto") {
                    item.props.width = std::to_string(innerAvailW / scale) + "px";
                }
            } else if (crossExtent > 0.0f && (item.props.height.empty() || item.props.height == "auto")) {
                item.props.height = std::to_string(crossExtent / scale) + "px";
            }
        }

        float lead = 0.0f;
        const bool hasLead = vertical ? item.props.hasTop : item.props.hasLeft;
        if (hasLead) {
            lead = (vertical ? item.props.top : item.props.left) * scale;
        } else {
            lead = first ? 0.0f : cursor + gap;
            if (vertical) {
                item.props.hasTop = true;
                item.props.top = lead / scale;
            } else {
                item.props.hasLeft = true;
                item.props.left = lead / scale;
            }
        }

        const float childExtent = vertical ? childH : childW;
        cursor = std::max(cursor, lead + childExtent);
        extent = std::max(extent, lead + childExtent);
        first = false;
    }

    return extent;
}

struct BodyLayout {
    float width = 0.0f;
    float height = 0.0f;
    float innerWidth = 0.0f;
    float innerHeight = 0.0f;
    float contentExtent = 0.0f;
    float viewportExtent = 0.0f;
    float scale = 1.0f;
    bool vertical = true;
};

BodyLayout ResolveBody(const FontEngine& fonts,
                       Container& container,
                       float availableWidth,
                       float scale,
                       float forcedWidth,
                       float forcedHeight) {
    ContainerStyle style = container.GetStyle();
    Resolve(style);
    BodyLayout out;
    out.scale = scale;
    out.vertical = IsVertical(style);

    const float padL = style.paddingLeft * scale;
    const float padR = style.paddingRight * scale;
    const float padT = style.paddingTop * scale;
    const float padB = style.paddingBottom * scale;
    const float bar = style.showScrollbar ? static_cast<float>(style.scrollbarStyle.thickness) * scale : 0.0f;
    const float gap = static_cast<float>(style.scrollbarGap) * scale;
    const float crossReserve = (out.vertical ? bar + gap : 0.0f);

    float resolvedW = availableWidth;
    if (forcedWidth > 0.0f) {
        resolvedW = forcedWidth;
    } else if (!style.width.empty() && style.width != "auto") {
        resolvedW = ParseDimension(style.width, availableWidth, availableWidth, scale);
    }
    if (style.minWidth > 0.0f) {
        resolvedW = std::max(resolvedW, style.minWidth * scale);
    }
    if (style.maxWidth > 0.0f) {
        resolvedW = std::min(resolvedW, style.maxWidth * scale);
    }
    resolvedW = std::max(resolvedW, 0.0f);

    const float innerAvailW = std::max(0.0f, resolvedW - padL - padR - crossReserve);

    // A horizontal container needs its cross size before stacking so children can
    // stretch into it; an auto height leaves them at their natural height.
    float crossExtent = -1.0f;
    if (!out.vertical) {
        if (forcedHeight > 0.0f) {
            crossExtent = forcedHeight - padT - padB - bar - gap;
        } else if (!style.height.empty() && style.height != "auto") {
            crossExtent = ParseDimension(style.height, availableWidth, -1.0f, scale) - padT - padB - bar - gap;
        }
    }

    float contentW = 0.0f;
    float contentH = 0.0f;
    const ContainerLayout layout = style.layout;
    if (layout == ContainerLayout::Flex) {
        const float forceW = out.vertical ? innerAvailW : -1.0f;
        const float forceH = out.vertical ? -1.0f : crossExtent;
        const FlexboxMeasureResult measured =
            FlexboxPainter::Measure(fonts, container.GetFlexbox(), innerAvailW, scale, forceW, forceH);
        contentW = measured.width;
        contentH = measured.height;
    } else if (layout == ContainerLayout::Grid) {
        const GridMeasureResult measured =
            GridPainter::Measure(fonts, container.GetGrid(), innerAvailW, scale, innerAvailW, -1.0f);
        contentW = measured.width;
        contentH = measured.height;
    } else if (layout == ContainerLayout::Position) {
        const PositionMeasureResult measured =
            PositionPainter::Measure(fonts, container.GetContent(), innerAvailW, scale);
        contentW = measured.width;
        contentH = measured.height;
    } else {
        const float stacked = StackChildren(fonts, container, innerAvailW, crossExtent, scale);
        const PositionMeasureResult contentMeasure =
            PositionPainter::Measure(fonts, container.GetContent(), innerAvailW, scale);
        contentW = std::max(contentMeasure.width, out.vertical ? 0.0f : stacked);
        contentH = std::max(contentMeasure.height, out.vertical ? stacked : 0.0f);
    }

    const float flowReserve = out.vertical ? 0.0f : bar + gap;
    float resolvedH = contentH + padT + padB + flowReserve;
    if (forcedHeight > 0.0f) {
        resolvedH = forcedHeight;
    } else if (!style.height.empty() && style.height != "auto") {
        resolvedH = ParseDimension(style.height, availableWidth, resolvedH, scale);
    }
    if (style.minHeight > 0.0f) {
        resolvedH = std::max(resolvedH, style.minHeight * scale);
    }
    if (style.maxHeight > 0.0f) {
        resolvedH = std::min(resolvedH, style.maxHeight * scale);
    }
    resolvedH = std::max(resolvedH, 0.0f);

    out.width = resolvedW;
    out.height = resolvedH;
    out.innerWidth = std::max(0.0f, resolvedW - padL - padR - crossReserve);
    out.innerHeight = std::max(0.0f, resolvedH - padT - padB - flowReserve);

    out.contentExtent = out.vertical ? contentH : contentW;
    out.viewportExtent = out.vertical ? out.innerHeight : out.innerWidth;

    if (style.contentExtent > 0.0) {
        out.contentExtent = static_cast<float>(style.contentExtent * scale);
    }
    if (style.viewportExtent > 0.0) {
        out.viewportExtent = static_cast<float>(style.viewportExtent * scale);
    }
    return out;
}

}  // namespace

ContainerMeasureResult ContainerPainter::Measure(const FontEngine& fonts,
                                                 Container& container,
                                                 float availableWidth,
                                                 float pixelScale,
                                                 float forcedWidth,
                                                 float forcedHeight) {
    const float scale = std::max(0.5f, pixelScale);
    const BodyLayout body = ResolveBody(fonts, container, availableWidth, scale, forcedWidth, forcedHeight);

    ContainerMeasureResult result;
    result.width = body.width;
    result.height = body.height;
    result.contentExtent = body.contentExtent;
    result.viewportExtent = body.viewportExtent;
    return result;
}

ContainerPaintResult ContainerPainter::Paint(FrameBuffer& buffer,
                                             const FontEngine& fonts,
                                             Container& container,
                                             float originX,
                                             float originY,
                                             float containerWidth,
                                             float pixelScale,
                                             float forcedWidth,
                                             float forcedHeight) {
    const SelectableScope selection(&container, container.GetStyle().selectable);
    const float scale = std::max(0.5f, pixelScale);
    ContainerStyle style = container.GetStyle();
    Resolve(style);
    const BodyLayout body = ResolveBody(fonts, container, containerWidth, scale, forcedWidth, forcedHeight);

    const float padL = style.paddingLeft * scale;
    const float padT = style.paddingTop * scale;
    const float bar = style.showScrollbar ? static_cast<float>(style.scrollbarStyle.thickness) * scale : 0.0f;

    const float innerX = originX + padL;
    const float innerY = originY + padT;

    // The bar sits on the container edge. Content padding does not push it inward.
    const float edge = 3.0f * scale;
    ScrollbarBounds track;
    if (body.vertical) {
        track.x = originX + body.width - bar;
        track.y = originY + edge;
        track.width = bar;
        track.height = std::max(0.0f, body.height - edge * 2.0f);
    } else {
        track.x = originX + edge;
        track.y = originY + body.height - bar;
        track.width = std::max(0.0f, body.width - edge * 2.0f);
        track.height = bar;
    }

    container.SetPixelScale(scale);
    container.SyncScrollbar(body.contentExtent, body.viewportExtent, track);
    container.SetBounds({ originX, originY, body.width, body.height });

    if (style.backgroundColor.alpha > 0) {
        FillRoundedRect(buffer, originX, originY, body.width, body.height, style.cornerRadius * scale,
                        style.backgroundColor, style.opacity);
    }
    if (style.borderWidth > 0.0f && style.borderColor.alpha > 0) {
        StrokeRoundedRect(buffer, originX, originY, body.width, body.height, style.cornerRadius * scale,
                          style.borderWidth * scale, style.borderColor, style.opacity);
    }

    // The content is painted at the scrollbar's value and clipped to the body.
    const float offset = static_cast<float>(container.GetScrollOffset());
    const float contentX = body.vertical ? innerX : innerX - offset;
    const float contentY = body.vertical ? innerY - offset : innerY;
    const float forceW = body.vertical ? body.innerWidth : -1.0f;
    const float forceH = body.vertical ? -1.0f : body.innerHeight;

    if (style.clipToBounds) {
        buffer.PushClipRect(originX, originY, body.width, body.height);
    }
    if (style.layout == ContainerLayout::Flex) {
        FlexboxPainter::Paint(buffer, fonts, container.GetFlexbox(), contentX, contentY, body.innerWidth, scale, forceW,
                              forceH);
    } else if (style.layout == ContainerLayout::Grid) {
        GridPainter::Paint(buffer, fonts, container.GetGrid(), contentX, contentY, body.innerWidth, scale,
                           body.innerWidth, -1.0f);
    } else {
        PositionPainter::Paint(buffer, fonts, container.GetContent(), contentX, contentY, body.innerWidth, scale,
                               body.vertical ? -1.0f : body.contentExtent, body.vertical ? body.contentExtent : -1.0f);
    }
    if (style.clipToBounds) {
        buffer.PopClip();
    }

    if (style.showScrollbar) {
        ScrollbarPainter::Paint(buffer,
                                container.GetScrollbar(),
                                track.x,
                                track.y,
                                body.vertical ? track.width : track.height,
                                scale,
                                body.vertical ? track.width : track.width,
                                body.vertical ? track.height : track.height);
    }

    ContainerPaintResult result;
    result.bounds = { originX, originY, body.width, body.height };
    result.advanceY = body.height + 16.0f * scale;
    return result;
}

}  // namespace Robo
