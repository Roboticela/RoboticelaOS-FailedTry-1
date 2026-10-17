#include "Robo/PositionPainter.hpp"
#include "Robo/PropLength.hpp"
#include "Robo/Foundation.hpp"
#include "Robo/TextSelection.hpp"
#include "Robo/BrandLogoPainter.hpp"
#include "Robo/TitleBarPainter.hpp"
#include "Robo/SidebarPainter.hpp"
#include "Robo/SeparatorPainter.hpp"
#include "Robo/ThemeIconTogglePainter.hpp"
#include "Robo/ThemeTogglePainter.hpp"
#include "Robo/ButtonPainter.hpp"
#include "Robo/ContainerPainter.hpp"
#include "Robo/FlexboxPainter.hpp"
#include "Robo/GridPainter.hpp"
#include "Robo/InputPainter.hpp"
#include "Robo/ScrollbarPainter.hpp"
#include "Robo/SvgPainter.hpp"
#include "Robo/TextPainter.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>
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
        const float distToCenter = std::sqrt(dx * dx + dy * dy);
        return distToCenter - cappedRadius;
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
        const float distToCenter = std::sqrt(dx * dx + dy * dy);
        return distToCenter - cappedRadius;
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

}  // namespace

PositionMeasureResult PositionPainter::Measure(const FontEngine& fonts,
                                               Position& position,
                                               float availableWidth,
                                               float pixelScale,
                                               float forcedWidth,
                                               float forcedHeight) {
    const float scale = std::max(0.5f, pixelScale);
    if (position.HasLayoutCache(availableWidth, scale, forcedWidth, forcedHeight)) {
        PositionMeasureResult cached;
        cached.width = position.LayoutCache().containerWidth;
        cached.height = position.LayoutCache().containerHeight;
        return cached;
    }
    PositionStyle style = position.GetStyle();
    Resolve(style);

    const float padL = style.paddingLeft * scale;
    const float padR = style.paddingRight * scale;
    const float padT = style.paddingTop * scale;
    const float padB = style.paddingBottom * scale;
    const float innerAvailW = std::max(0.0f, availableWidth - padL - padR);

    float maxExtentX = 0.0f;
    float maxExtentY = 0.0f;

    std::vector<PositionItem>& items = position.GetItems();
    for (PositionItem& item : items) {
        float childW = 0.0f;
        float childH = 0.0f;

        switch (item.kind) {
            case PositionItemKind::Text: {
                TextMeasureResult res = TextPainter::Measure(fonts, *item.text, innerAvailW, scale);
                childW = res.width;
                childH = res.height;
                break;
            }
            case PositionItemKind::Svg: {
                SvgMeasureResult res = SvgPainter::Measure(*item.svg, scale);
                childW = res.width;
                childH = res.height;
                break;
            }
            case PositionItemKind::BrandLogo: {
                BrandLogoMeasureResult res = BrandLogoPainter::Measure(fonts, *item.brandLogo, scale);
                childW = res.width;
                childH = res.height;
                break;
            }
            case PositionItemKind::TitleBar: {
                TitleBarMeasureResult res = TitleBarPainter::Measure(fonts, *item.titleBar, innerAvailW, scale);
                childW = res.width;
                childH = res.height;
                break;
            }
            case PositionItemKind::Sidebar: {
                SidebarMeasureResult res = SidebarPainter::Measure(*item.sidebar, 0.0f, scale);
                childW = res.width;
                childH = res.height;
                break;
            }
            case PositionItemKind::ThemeToggle: {
                ThemeToggleMeasureResult res = ThemeTogglePainter::Measure(fonts, *item.themeToggle, innerAvailW, scale);
                childW = res.width;
                childH = res.height;
                break;
            }
            case PositionItemKind::ThemeIconToggle: {
                ThemeIconToggleMeasureResult res = ThemeIconTogglePainter::Measure(*item.themeIconToggle, scale);
                childW = res.width;
                childH = res.height;
                break;
            }
            case PositionItemKind::SidebarItem: {
                SidebarItemMeasureResult res = SidebarPainter::MeasureItem(fonts, *item.sidebarItem, scale);
                childW = res.width;
                childH = res.height;
                break;
            }
            case PositionItemKind::Separator: {
                SeparatorMeasureResult res = SeparatorPainter::Measure(fonts, *item.separator, innerAvailW, 0.0f, scale);
                childW = res.width;
                childH = res.height;
                break;
            }
            case PositionItemKind::MotionShape: {
                childW = item.motionShape->Width() * scale;
                childH = item.motionShape->Height() * scale;
                break;
            }
            case PositionItemKind::Button: {
                ButtonMeasureResult res = ButtonPainter::Measure(fonts, *item.button, scale);
                childW = res.width;
                childH = res.height;
                break;
            }
            case PositionItemKind::Input: {
                const InputStyle& inputStyle = item.input->GetStyle();
                float intrinsicWidth = static_cast<float>(inputStyle.minWidth) * scale;
                if (PropHasWidth(item.props.hasWidthLength, item.props.widthLength, item.props.width)) {
                    intrinsicWidth = ResolvePropLength(item.props.hasWidthLength, item.props.widthLength, item.props.width, innerAvailW, intrinsicWidth, scale);
                }
                InputMeasureResult res = InputPainter::Measure(fonts, *item.input, intrinsicWidth, scale);
                childW = intrinsicWidth;
                childH = res.height;
                break;
            }
            case PositionItemKind::Scrollbar: {
                ScrollbarMeasureResult res = ScrollbarPainter::Measure(*item.scrollbar, innerAvailW, scale);
                childW = res.width;
                childH = res.height;
                break;
            }
            case PositionItemKind::Container: {
                if (item.container) {
                    float cw = -1.0f;
                    float ch = -1.0f;
                    if (PropHasWidth(item.props.hasWidthLength, item.props.widthLength, item.props.width)) {
                        cw = ResolvePropLength(item.props.hasWidthLength, item.props.widthLength, item.props.width, innerAvailW, innerAvailW, scale);
                    }
                    if (PropHasHeight(item.props.hasHeightLength, item.props.heightLength, item.props.height)) {
                        ch = ResolvePropLength(item.props.hasHeightLength, item.props.heightLength, item.props.height, 0.0f, 0.0f, scale);
                    }
                    ContainerMeasureResult res =
                        ContainerPainter::Measure(fonts, *item.container, innerAvailW, scale, cw, ch);
                    childW = res.width;
                    childH = res.height;
                }
                break;
            }
            case PositionItemKind::Flexbox: {
                if (item.flexbox) {
                    float fw = -1.0f;
                    float fh = -1.0f;
                    if (PropHasWidth(item.props.hasWidthLength, item.props.widthLength, item.props.width)) {
                        fw = ResolvePropLength(item.props.hasWidthLength, item.props.widthLength, item.props.width, innerAvailW, innerAvailW, scale);
                    }
                    if (PropHasHeight(item.props.hasHeightLength, item.props.heightLength, item.props.height)) {
                        fh = ResolvePropLength(item.props.hasHeightLength, item.props.heightLength, item.props.height, 0.0f, 0.0f, scale);
                    }
                    FlexboxMeasureResult res = FlexboxPainter::Measure(fonts, *item.flexbox, innerAvailW, scale, fw, fh);
                    childW = res.width;
                    childH = res.height;
                }
                break;
            }
            case PositionItemKind::Grid: {
                if (item.grid) {
                    float gw = -1.0f;
                    float gh = -1.0f;
                    if (PropHasWidth(item.props.hasWidthLength, item.props.widthLength, item.props.width)) {
                        gw = ResolvePropLength(item.props.hasWidthLength, item.props.widthLength, item.props.width, innerAvailW, innerAvailW, scale);
                    }
                    if (PropHasHeight(item.props.hasHeightLength, item.props.heightLength, item.props.height)) {
                        gh = ResolvePropLength(item.props.hasHeightLength, item.props.heightLength, item.props.height, 0.0f, 0.0f, scale);
                    }
                    GridMeasureResult res = GridPainter::Measure(fonts, *item.grid, innerAvailW, scale, gw, gh);
                    childW = res.width;
                    childH = res.height;
                }
                break;
            }
            case PositionItemKind::Position: {
                if (item.position) {
                    float pw = -1.0f;
                    float ph = -1.0f;
                    if (PropHasWidth(item.props.hasWidthLength, item.props.widthLength, item.props.width)) {
                        pw = ResolvePropLength(item.props.hasWidthLength, item.props.widthLength, item.props.width, innerAvailW, innerAvailW, scale);
                    }
                    if (PropHasHeight(item.props.hasHeightLength, item.props.heightLength, item.props.height)) {
                        ph = ResolvePropLength(item.props.hasHeightLength, item.props.heightLength, item.props.height, 0.0f, 0.0f, scale);
                    }
                    PositionMeasureResult res = PositionPainter::Measure(fonts, *item.position, innerAvailW, scale, pw, ph);
                    childW = res.width;
                    childH = res.height;
                }
                break;
            }
        }

        if (PropHasWidth(item.props.hasWidthLength, item.props.widthLength, item.props.width)) {
            childW = ResolvePropLength(item.props.hasWidthLength, item.props.widthLength, item.props.width, innerAvailW, childW, scale);
        }
        if (PropHasHeight(item.props.hasHeightLength, item.props.heightLength, item.props.height)) {
            const float specifiedH = ResolvePropLength(item.props.hasHeightLength, item.props.heightLength, item.props.height, 0.0f, childH, scale);
            childH = std::max(specifiedH, childH);
        }
        if (PropHasWidth(item.props.hasMinWidthLength, item.props.minWidthLength, item.props.minWidth)) {
            childW = std::max(childW, ResolvePropLength(item.props.hasMinWidthLength, item.props.minWidthLength, item.props.minWidth, innerAvailW, 0.0f, scale));
        }
        if (PropHasWidth(item.props.hasMaxWidthLength, item.props.maxWidthLength, item.props.maxWidth)) {
            childW = std::min(childW, ResolvePropLength(item.props.hasMaxWidthLength, item.props.maxWidthLength, item.props.maxWidth, innerAvailW, 100000.0f, scale));
        }
        if (PropHasHeight(item.props.hasMinHeightLength, item.props.minHeightLength, item.props.minHeight)) {
            childH = std::max(childH, ResolvePropLength(item.props.hasMinHeightLength, item.props.minHeightLength, item.props.minHeight, 0.0f, 0.0f, scale));
        }
        if (PropHasHeight(item.props.hasMaxHeightLength, item.props.maxHeightLength, item.props.maxHeight)) {
            childH = std::min(childH, ResolvePropLength(item.props.hasMaxHeightLength, item.props.maxHeightLength, item.props.maxHeight, 0.0f, 100000.0f, scale));
        }

        float itemRight = childW;
        if (item.props.hasLeft) {
            itemRight = (item.props.left * scale) + childW;
        } else if (item.props.hasRight) {
            itemRight = childW + (item.props.right * scale);
        }

        float itemBottom = childH;
        if (item.props.hasTop) {
            itemBottom = (item.props.top * scale) + childH;
        } else if (item.props.hasBottom) {
            itemBottom = childH + (item.props.bottom * scale);
        }

        maxExtentX = std::max(maxExtentX, itemRight);
        maxExtentY = std::max(maxExtentY, itemBottom);
    }

    const float naturalW = maxExtentX + padL + padR;
    float resolvedW = availableWidth;
    if (forcedWidth > 0.0f) {
        resolvedW = forcedWidth;
    } else if (!style.width.empty() && style.width != "auto") {
        resolvedW = ParseDimension(style.width, availableWidth, naturalW, scale);
    } else if (naturalW > padL + padR) {
        resolvedW = std::min(availableWidth, naturalW);
    }
    if (style.minWidth > 0.0f) {
        resolvedW = std::max(resolvedW, style.minWidth * scale);
    }
    if (style.maxWidth > 0.0f) {
        resolvedW = std::min(resolvedW, style.maxWidth * scale);
    }

    const float naturalH = maxExtentY + padT + padB;
    float resolvedH = naturalH;
    if (forcedHeight > 0.0f) {
        resolvedH = std::max(forcedHeight, naturalH);
    } else if (!style.height.empty() && style.height != "auto") {
        resolvedH = std::max(ParseDimension(style.height, availableWidth, naturalH, scale), naturalH);
    }
    if (style.minHeight > 0.0f) {
        resolvedH = std::max(resolvedH, style.minHeight * scale);
    }
    if (style.maxHeight > 0.0f) {
        resolvedH = std::min(resolvedH, style.maxHeight * scale);
    }

    return { resolvedW, resolvedH };
}

PositionPaintResult PositionPainter::Paint(FrameBuffer& buffer,
                                           const FontEngine& fonts,
                                           Position& position,
                                           float originX,
                                           float originY,
                                           float containerWidth,
                                           float pixelScale,
                                           float forcedWidth,
                                           float forcedHeight) {
    const SelectableScope selection(&position, position.GetStyle().selectable);
    const float scale = std::max(0.5f, pixelScale);
    PositionStyle style = position.GetStyle();
    Resolve(style);

    const float padL = style.paddingLeft * scale;
    const float padR = style.paddingRight * scale;
    const float padT = style.paddingTop * scale;
    const float padB = style.paddingBottom * scale;
    const float innerAvailW = std::max(0.0f, containerWidth - padL - padR);

    std::vector<PositionItem>& items = position.GetItems();

    struct LayoutChild {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
    };
    std::vector<LayoutChild> layout(items.size());
    float resolvedW = 0.0f;
    float resolvedH = 0.0f;
    bool usedLayoutCache = false;

    if (position.HasLayoutCache(containerWidth, scale, forcedWidth, forcedHeight)) {
        const ComputedPositionLayout& cached = position.LayoutCache();
        layout.resize(cached.children.size());
        for (std::size_t i = 0; i < cached.children.size(); ++i) {
            const PlacedPositionChild& child = cached.children[i];
            layout[i].x = originX + child.x;
            layout[i].y = originY + child.y;
            layout[i].width = child.width;
            layout[i].height = child.height;
            if (i < items.size()) {
                PositionItem& item = items[i];
                item.computedBounds = { layout[i].x, layout[i].y, layout[i].width, layout[i].height };
                if (item.kind == PositionItemKind::Button) {
                    item.button->SetBounds({ layout[i].x, layout[i].y, layout[i].width, layout[i].height });
                } else if (item.kind == PositionItemKind::Input) {
                    item.input->SetBounds(item.computedBounds);
                } else if (item.kind == PositionItemKind::Position && item.position) {
                    item.position->SetBounds(item.computedBounds);
                }
            }
        }
        resolvedW = cached.containerWidth;
        resolvedH = cached.containerHeight;
        usedLayoutCache = true;
        position.SetNeedsPaint(false);
    }

    float maxExtentX = 0.0f;
    float maxExtentY = 0.0f;

    // First pass: measure intrinsic dimensions and preliminary extents
    for (std::size_t i = 0; !usedLayoutCache && i < items.size(); ++i) {
        PositionItem& item = items[i];
        LayoutChild& lc = layout[i];

        switch (item.kind) {
            case PositionItemKind::Text: {
                TextMeasureResult res = TextPainter::Measure(fonts, *item.text, innerAvailW, scale);
                lc.width = res.width;
                lc.height = res.height;
                break;
            }
            case PositionItemKind::Svg: {
                SvgMeasureResult res = SvgPainter::Measure(*item.svg, scale);
                lc.width = res.width;
                lc.height = res.height;
                break;
            }
            case PositionItemKind::BrandLogo: {
                BrandLogoMeasureResult res = BrandLogoPainter::Measure(fonts, *item.brandLogo, scale);
                lc.width = res.width;
                lc.height = res.height;
                break;
            }
            case PositionItemKind::TitleBar: {
                TitleBarMeasureResult res = TitleBarPainter::Measure(fonts, *item.titleBar, innerAvailW, scale);
                lc.width = res.width;
                lc.height = res.height;
                break;
            }
            case PositionItemKind::Sidebar: {
                SidebarMeasureResult res = SidebarPainter::Measure(*item.sidebar, 0.0f, scale);
                lc.width = res.width;
                lc.height = res.height;
                break;
            }
            case PositionItemKind::ThemeToggle: {
                ThemeToggleMeasureResult res = ThemeTogglePainter::Measure(fonts, *item.themeToggle, innerAvailW, scale);
                lc.width = res.width;
                lc.height = res.height;
                break;
            }
            case PositionItemKind::ThemeIconToggle: {
                ThemeIconToggleMeasureResult res = ThemeIconTogglePainter::Measure(*item.themeIconToggle, scale);
                lc.width = res.width;
                lc.height = res.height;
                break;
            }
            case PositionItemKind::SidebarItem: {
                SidebarItemMeasureResult res = SidebarPainter::MeasureItem(fonts, *item.sidebarItem, scale);
                lc.width = res.width;
                lc.height = res.height;
                break;
            }
            case PositionItemKind::Separator: {
                SeparatorMeasureResult res = SeparatorPainter::Measure(fonts, *item.separator, innerAvailW, 0.0f, scale);
                lc.width = res.width;
                lc.height = res.height;
                break;
            }
            case PositionItemKind::MotionShape: {
                lc.width = item.motionShape->Width() * scale;
                lc.height = item.motionShape->Height() * scale;
                break;
            }
            case PositionItemKind::Button: {
                ButtonMeasureResult res = ButtonPainter::Measure(fonts, *item.button, scale);
                lc.width = res.width;
                lc.height = res.height;
                break;
            }
            case PositionItemKind::Input: {
                const InputStyle& inputStyle = item.input->GetStyle();
                float intrinsicWidth = static_cast<float>(inputStyle.minWidth) * scale;
                if (PropHasWidth(item.props.hasWidthLength, item.props.widthLength, item.props.width)) {
                    intrinsicWidth = ResolvePropLength(item.props.hasWidthLength, item.props.widthLength, item.props.width, innerAvailW, intrinsicWidth, scale);
                }
                InputMeasureResult res = InputPainter::Measure(fonts, *item.input, intrinsicWidth, scale);
                lc.width = intrinsicWidth;
                lc.height = res.height;
                break;
            }
            case PositionItemKind::Scrollbar: {
                ScrollbarMeasureResult res = ScrollbarPainter::Measure(*item.scrollbar, innerAvailW, scale);
                lc.width = res.width;
                lc.height = res.height;
                break;
            }
            case PositionItemKind::Container: {
                if (item.container) {
                    float cw = -1.0f;
                    float ch = -1.0f;
                    if (PropHasWidth(item.props.hasWidthLength, item.props.widthLength, item.props.width)) {
                        cw = ResolvePropLength(item.props.hasWidthLength, item.props.widthLength, item.props.width, innerAvailW, innerAvailW, scale);
                    }
                    if (PropHasHeight(item.props.hasHeightLength, item.props.heightLength, item.props.height)) {
                        ch = ResolvePropLength(item.props.hasHeightLength, item.props.heightLength, item.props.height, 0.0f, 0.0f, scale);
                    }
                    ContainerMeasureResult res =
                        ContainerPainter::Measure(fonts, *item.container, innerAvailW, scale, cw, ch);
                    lc.width = res.width;
                    lc.height = res.height;
                }
                break;
            }
            case PositionItemKind::Flexbox: {
                if (item.flexbox) {
                    float fw = -1.0f;
                    float fh = -1.0f;
                    if (PropHasWidth(item.props.hasWidthLength, item.props.widthLength, item.props.width)) {
                        fw = ResolvePropLength(item.props.hasWidthLength, item.props.widthLength, item.props.width, innerAvailW, innerAvailW, scale);
                    }
                    if (PropHasHeight(item.props.hasHeightLength, item.props.heightLength, item.props.height)) {
                        fh = ResolvePropLength(item.props.hasHeightLength, item.props.heightLength, item.props.height, 0.0f, 0.0f, scale);
                    }
                    FlexboxMeasureResult res = FlexboxPainter::Measure(fonts, *item.flexbox, innerAvailW, scale, fw, fh);
                    lc.width = res.width;
                    lc.height = res.height;
                }
                break;
            }
            case PositionItemKind::Grid: {
                if (item.grid) {
                    float gw = -1.0f;
                    float gh = -1.0f;
                    if (PropHasWidth(item.props.hasWidthLength, item.props.widthLength, item.props.width)) {
                        gw = ResolvePropLength(item.props.hasWidthLength, item.props.widthLength, item.props.width, innerAvailW, innerAvailW, scale);
                    }
                    if (PropHasHeight(item.props.hasHeightLength, item.props.heightLength, item.props.height)) {
                        gh = ResolvePropLength(item.props.hasHeightLength, item.props.heightLength, item.props.height, 0.0f, 0.0f, scale);
                    }
                    GridMeasureResult res = GridPainter::Measure(fonts, *item.grid, innerAvailW, scale, gw, gh);
                    lc.width = res.width;
                    lc.height = res.height;
                }
                break;
            }
            case PositionItemKind::Position: {
                if (item.position) {
                    float pw = -1.0f;
                    float ph = -1.0f;
                    if (PropHasWidth(item.props.hasWidthLength, item.props.widthLength, item.props.width)) {
                        pw = ResolvePropLength(item.props.hasWidthLength, item.props.widthLength, item.props.width, innerAvailW, innerAvailW, scale);
                    }
                    if (PropHasHeight(item.props.hasHeightLength, item.props.heightLength, item.props.height)) {
                        ph = ResolvePropLength(item.props.hasHeightLength, item.props.heightLength, item.props.height, 0.0f, 0.0f, scale);
                    }
                    PositionMeasureResult res = PositionPainter::Measure(fonts, *item.position, innerAvailW, scale, pw, ph);
                    lc.width = res.width;
                    lc.height = res.height;
                }
                break;
            }
        }

        if (PropHasWidth(item.props.hasWidthLength, item.props.widthLength, item.props.width)) {
            lc.width = ResolvePropLength(item.props.hasWidthLength, item.props.widthLength, item.props.width, innerAvailW, lc.width, scale);
        }
        if (PropHasHeight(item.props.hasHeightLength, item.props.heightLength, item.props.height)) {
            const float specifiedH = ResolvePropLength(item.props.hasHeightLength, item.props.heightLength, item.props.height, 0.0f, lc.height, scale);
            lc.height = std::max(specifiedH, lc.height);
        }
        if (PropHasWidth(item.props.hasMinWidthLength, item.props.minWidthLength, item.props.minWidth)) {
            lc.width = std::max(lc.width, ResolvePropLength(item.props.hasMinWidthLength, item.props.minWidthLength, item.props.minWidth, innerAvailW, 0.0f, scale));
        }
        if (PropHasWidth(item.props.hasMaxWidthLength, item.props.maxWidthLength, item.props.maxWidth)) {
            lc.width = std::min(lc.width, ResolvePropLength(item.props.hasMaxWidthLength, item.props.maxWidthLength, item.props.maxWidth, innerAvailW, 100000.0f, scale));
        }
        if (PropHasHeight(item.props.hasMinHeightLength, item.props.minHeightLength, item.props.minHeight)) {
            lc.height = std::max(lc.height, ResolvePropLength(item.props.hasMinHeightLength, item.props.minHeightLength, item.props.minHeight, 0.0f, 0.0f, scale));
        }
        if (PropHasHeight(item.props.hasMaxHeightLength, item.props.maxHeightLength, item.props.maxHeight)) {
            lc.height = std::min(lc.height, ResolvePropLength(item.props.hasMaxHeightLength, item.props.maxHeightLength, item.props.maxHeight, 0.0f, 100000.0f, scale));
        }

        // If both left and right are specified: width fills span
        if (item.props.hasLeft && item.props.hasRight) {
            lc.width = std::max(0.0f, innerAvailW - (item.props.left * scale) - (item.props.right * scale));
        }

        float itemRight = lc.width;
        if (item.props.hasLeft) {
            itemRight = (item.props.left * scale) + lc.width;
        } else if (item.props.hasRight) {
            itemRight = lc.width + (item.props.right * scale);
        }

        float itemBottom = lc.height;
        if (item.props.hasTop) {
            itemBottom = (item.props.top * scale) + lc.height;
        } else if (item.props.hasBottom) {
            itemBottom = lc.height + (item.props.bottom * scale);
        }

        maxExtentX = std::max(maxExtentX, itemRight);
        maxExtentY = std::max(maxExtentY, itemBottom);
    }

    if (!usedLayoutCache) {
    const float naturalW = maxExtentX + padL + padR;
    resolvedW = containerWidth;
    if (forcedWidth > 0.0f) {
        resolvedW = forcedWidth;
    } else if (style.hasWidthLength && !style.widthLength.IsAuto()) {
        resolvedW = style.widthLength.Resolve(containerWidth / scale, 1.0f) * scale;
    } else if (!style.width.empty() && style.width != "auto") {
        resolvedW = ParseDimension(style.width, containerWidth, naturalW, scale);
    } else if (naturalW > padL + padR) {
        resolvedW = std::min(containerWidth, naturalW);
    }
    if (style.minWidth > 0.0f) {
        resolvedW = std::max(resolvedW, style.minWidth * scale);
    }
    if (style.maxWidth > 0.0f) {
        resolvedW = std::min(resolvedW, style.maxWidth * scale);
    }

    const float naturalH = maxExtentY + padT + padB;
    resolvedH = naturalH;
    if (forcedHeight > 0.0f) {
        resolvedH = std::max(forcedHeight, naturalH);
    } else if (style.hasHeightLength && !style.heightLength.IsAuto()) {
        resolvedH = style.heightLength.Resolve(containerWidth / scale, 1.0f) * scale;
    } else if (!style.height.empty() && style.height != "auto") {
        resolvedH = std::max(ParseDimension(style.height, containerWidth, naturalH, scale), naturalH);
    }
    if (style.minHeight > 0.0f) {
        resolvedH = std::max(resolvedH, style.minHeight * scale);
    }
    if (style.maxHeight > 0.0f) {
        resolvedH = std::min(resolvedH, style.maxHeight * scale);
    }

    const float innerW = std::max(0.0f, resolvedW - padL - padR);
    const float innerH = std::max(0.0f, resolvedH - padT - padB);
    const float innerX = originX + padL;
    const float innerY = originY + padT;


    // Second pass: compute final X and Y
    for (std::size_t i = 0; i < items.size(); ++i) {
        PositionItem& item = items[i];
        LayoutChild& lc = layout[i];

        if (item.props.hasTop && item.props.hasBottom) {
            lc.height = std::max(0.0f, innerH - (item.props.top * scale) - (item.props.bottom * scale));
        }

        // Compute X
        if (item.props.hasLeft) {
            lc.x = innerX + item.props.left * scale;
        } else if (item.props.hasRight) {
            lc.x = innerX + innerW - (item.props.right * scale) - lc.width;
        } else {
            switch (item.props.anchor) {
                case PositionAnchor::TopRight:
                case PositionAnchor::BottomRight:
                    lc.x = innerX + innerW - lc.width;
                    break;
                case PositionAnchor::Center:
                case PositionAnchor::TopCenter:
                case PositionAnchor::BottomCenter:
                    lc.x = innerX + (innerW - lc.width) * 0.5f;
                    break;
                default:
                    lc.x = innerX;
                    break;
            }
        }

        // Compute Y
        if (item.props.hasTop) {
            lc.y = innerY + item.props.top * scale;
        } else if (item.props.hasBottom) {
            lc.y = innerY + innerH - (item.props.bottom * scale) - lc.height;
        } else {
            switch (item.props.anchor) {
                case PositionAnchor::BottomLeft:
                case PositionAnchor::BottomRight:
                case PositionAnchor::BottomCenter:
                    lc.y = innerY + innerH - lc.height;
                    break;
                case PositionAnchor::Center:
                    lc.y = innerY + (innerH - lc.height) * 0.5f;
                    break;
                default:
                    lc.y = innerY;
                    break;
            }
        }

        // Apply relative mode offsets
        if (item.props.mode == PositionMode::Relative) {
            if (item.props.hasLeft && !item.props.hasRight) {
                lc.x = innerX + item.props.left * scale;
            }
            if (item.props.hasTop && !item.props.hasBottom) {
                lc.y = innerY + item.props.top * scale;
            }
        }

        item.computedBounds = { lc.x, lc.y, lc.width, lc.height };

        if (item.kind == PositionItemKind::Button) {
            item.button->SetBounds({ lc.x, lc.y, lc.width, lc.height });
        } else if (item.kind == PositionItemKind::Input) {
            item.input->SetBounds({ lc.x, lc.y, lc.width, lc.height });
        } else if (item.kind == PositionItemKind::Position && item.position) {
            item.position->SetBounds({ lc.x, lc.y, lc.width, lc.height });
        }
    }

    ComputedPositionLayout stored;
    stored.containerWidth = resolvedW;
    stored.containerHeight = resolvedH;
    stored.children.resize(layout.size());
    for (std::size_t i = 0; i < layout.size(); ++i) {
        stored.children[i].x = layout[i].x - originX;
        stored.children[i].y = layout[i].y - originY;
        stored.children[i].width = layout[i].width;
        stored.children[i].height = layout[i].height;
    }
    position.StoreLayoutCache(containerWidth, scale, forcedWidth, forcedHeight, std::move(stored));
    position.SetNeedsPaint(false);
    }  // !usedLayoutCache

    position.SetBounds({ originX, originY, resolvedW, resolvedH });

    // Paint container background
    if (style.backgroundColor.alpha > 0) {
        FillRoundedRect(buffer, originX, originY, resolvedW, resolvedH, style.cornerRadius * scale, style.backgroundColor, style.opacity);
    }

    // Paint container border
    if (style.borderWidth > 0.0f && style.borderColor.alpha > 0) {
        StrokeRoundedRect(buffer, originX, originY, resolvedW, resolvedH, style.cornerRadius * scale, style.borderWidth * scale, style.borderColor, style.opacity);
    }

    // Sort items by zIndex ascending for paint order
    std::vector<std::size_t> renderIndices(items.size());
    for (std::size_t i = 0; i < items.size(); ++i) {
        renderIndices[i] = i;
    }
    std::stable_sort(renderIndices.begin(), renderIndices.end(), [&](std::size_t a, std::size_t b) {
        return items[a].props.zIndex < items[b].props.zIndex;
    });

    // Paint items in zIndex order
    for (std::size_t idx : renderIndices) {
        PositionItem& item = items[idx];
        const LayoutChild& lc = layout[idx];

        switch (item.kind) {
            case PositionItemKind::Text:
                TextPainter::Paint(buffer, fonts, *item.text, lc.x, lc.y, lc.width, scale);
                break;
            case PositionItemKind::Svg:
                SvgPainter::Paint(buffer, *item.svg, lc.x, lc.y, lc.width, scale);
                break;
            case PositionItemKind::BrandLogo:
                BrandLogoPainter::Paint(buffer, fonts, *item.brandLogo, lc.x, lc.y, scale);
                break;
            case PositionItemKind::TitleBar:
                TitleBarPainter::Paint(buffer, fonts, *item.titleBar, lc.x, lc.y, lc.width, scale);
                break;
            case PositionItemKind::Sidebar:
                SidebarPainter::Paint(buffer, fonts, *item.sidebar, lc.x, lc.y, lc.height, scale, 1.0f);
                break;
            case PositionItemKind::SidebarItem:
                SidebarPainter::PaintItem(buffer, fonts, *item.sidebarItem, lc.x, lc.y, lc.width, lc.height, scale, false);
                break;
            case PositionItemKind::ThemeToggle:
                ThemeTogglePainter::Paint(buffer, fonts, *item.themeToggle, lc.x, lc.y, lc.width, scale);
                break;
            case PositionItemKind::ThemeIconToggle:
                ThemeIconTogglePainter::Paint(buffer, *item.themeIconToggle, lc.x, lc.y, scale);
                break;
            case PositionItemKind::Separator:
                SeparatorPainter::Paint(buffer, fonts, *item.separator, lc.x, lc.y, lc.width, lc.height, scale);
                break;
            case PositionItemKind::MotionShape:
                item.motionShape->Paint(buffer, lc.x, lc.y, lc.width, lc.height, scale);
                break;
            case PositionItemKind::Button:
                ButtonPainter::Paint(buffer, fonts, *item.button, lc.x, lc.y, lc.width, scale, lc.width, lc.height);
                break;
            case PositionItemKind::Input:
                InputPainter::Paint(buffer, fonts, *item.input, lc.x, lc.y, lc.width, scale, lc.width, lc.height);
                break;
            case PositionItemKind::Scrollbar: {
                const bool vertical =
                    item.scrollbar->GetOrientation() == ScrollbarOrientation::Vertical;
                ScrollbarPainter::Paint(buffer,
                                        *item.scrollbar,
                                        lc.x,
                                        lc.y,
                                        lc.width,
                                        scale,
                                        vertical ? -1.0f : lc.width,
                                        vertical ? lc.height : -1.0f);
                break;
            }
            case PositionItemKind::Flexbox:
                if (item.flexbox) {
                    const float fw = (PropHasWidth(item.props.hasWidthLength, item.props.widthLength, item.props.width)) ? lc.width : -1.0f;
                    const float fh = (PropHasHeight(item.props.hasHeightLength, item.props.heightLength, item.props.height)) ? lc.height : -1.0f;
                    FlexboxPainter::Paint(buffer, fonts, *item.flexbox, lc.x, lc.y, lc.width, scale, fw, fh);
                }
                break;
            case PositionItemKind::Grid:
                if (item.grid) {
                    const float gw = (PropHasWidth(item.props.hasWidthLength, item.props.widthLength, item.props.width)) ? lc.width : -1.0f;
                    const float gh = (PropHasHeight(item.props.hasHeightLength, item.props.heightLength, item.props.height)) ? lc.height : -1.0f;
                    GridPainter::Paint(buffer, fonts, *item.grid, lc.x, lc.y, lc.width, scale, gw, gh);
                }
                break;
            case PositionItemKind::Container:
                if (item.container) {
                    const float cw = (PropHasWidth(item.props.hasWidthLength, item.props.widthLength, item.props.width)) ? lc.width : -1.0f;
                    const float ch = (PropHasHeight(item.props.hasHeightLength, item.props.heightLength, item.props.height)) ? lc.height : -1.0f;
                    ContainerPainter::Paint(buffer, fonts, *item.container, lc.x, lc.y, lc.width, scale, cw, ch);
                }
                break;
            case PositionItemKind::Position:
                if (item.position) {
                    const float pw = (PropHasWidth(item.props.hasWidthLength, item.props.widthLength, item.props.width)) ? lc.width : -1.0f;
                    const float ph = (PropHasHeight(item.props.hasHeightLength, item.props.heightLength, item.props.height)) ? lc.height : -1.0f;
                    PositionPainter::Paint(buffer, fonts, *item.position, lc.x, lc.y, lc.width, scale, pw, ph);
                }
                break;
        }
    }

    PositionPaintResult result;
    result.bounds = { originX, originY, resolvedW, resolvedH };
    result.advanceY = resolvedH + 16.0f * scale;
    return result;
}

}  // namespace Robo
