#include "Robo/FlexboxPainter.hpp"
#include "Robo/Foundation.hpp"
#include "Robo/SvgPainter.hpp"
#include "Robo/TextSelection.hpp"
#include "Robo/ContainerPainter.hpp"
#include "Robo/BrandLogoPainter.hpp"
#include "Robo/TitleBarPainter.hpp"
#include "Robo/SidebarPainter.hpp"
#include "Robo/SeparatorPainter.hpp"
#include "Robo/ThemeIconTogglePainter.hpp"
#include "Robo/ThemeTogglePainter.hpp"
#include "Robo/ButtonPainter.hpp"
#include "Robo/InputPainter.hpp"
#include "Robo/ScrollbarPainter.hpp"
#include "Robo/TextPainter.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>
#include <vector>

namespace Robo {

namespace {

float ParseDimension(const std::string& value, float parentSize, float fallback, float scale = 1.0f) {
    if (value.empty() || value == "auto") {
        return fallback;
    }
    if (value.size() > 1 && value.back() == '%') {
        const float percent = static_cast<float>(std::atof(value.c_str()));
        return parentSize * (percent / 100.0f);
    }
    if (value.size() > 3 && value.compare(value.size() - 3, 3, "rem") == 0) {
        return static_cast<float>(std::atof(value.c_str()) * 16.0f) * scale;
    }
    if (value.size() > 2 && value.compare(value.size() - 2, 2, "px") == 0) {
        return static_cast<float>(std::atof(value.c_str())) * scale;
    }
    return static_cast<float>(std::atof(value.c_str())) * scale;
}

float ResolvePropLength(bool hasLength, const Length& length, const std::string& legacy, float parentSize,
                        float fallback, float scale) {
    if (hasLength && !length.IsAuto()) {
        return length.Resolve(parentSize / scale, 1.0f) * scale;
    }
    if (!legacy.empty() && legacy != "auto") {
        return ParseDimension(legacy, parentSize, fallback, scale);
    }
    return fallback;
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

void StrokeRoundedRect(FrameBuffer& buffer,
                       float x,
                       float y,
                       float width,
                       float height,
                       float radius,
                       float strokeWidth,
                       const Color& color,
                       float opacity) {
    if (width <= 0.0f || height <= 0.0f || strokeWidth <= 0.0f || color.alpha == 0) {
        return;
    }

    const float cappedRadius = std::max(0.0f, std::min({radius, width * 0.5f, height * 0.5f}));
    const float half = strokeWidth * 0.5f;
    const int x0 = static_cast<int>(std::floor(x - half));
    const int y0 = static_cast<int>(std::floor(y - half));
    const int x1 = static_cast<int>(std::ceil(x + width + half));
    const int y1 = static_cast<int>(std::ceil(y + height + half));
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

            float dist = 0.0f;
            if (fx >= left && fx <= right && fy >= y && fy <= y + height) {
                const float dTop = fy - y;
                const float dBottom = (y + height) - fy;
                dist = -std::min(dTop, dBottom);
            } else if (fy >= top && fy <= bottom && fx >= x && fx <= x + width) {
                const float dLeft = fx - x;
                const float dRight = (x + width) - fx;
                dist = -std::min(dLeft, dRight);
            } else {
                dist = DistanceToRoundedRect(fx, fy);
            }

            const float edgeDist = std::abs(dist + half) - half;
            if (edgeDist <= 0.0f) {
                buffer.BlendPixel(px, py, color, opacity);
            } else if (edgeDist < 1.0f) {
                buffer.BlendPixel(px, py, color, opacity * (1.0f - edgeDist));
            }
        }
    }
}

bool KeepsIntrinsicCrossSize(const FlexboxItem& item) {
    if (item.kind == FlexItemKind::Button) {
        return item.props.width.empty();
    }
    if (item.kind == FlexItemKind::Scrollbar) {
        return item.props.width.empty() && item.props.height.empty();
    }
    return false;
}

ComputedFlexLayout ComputeLayout(const FontEngine& fonts,
                                Flexbox& flexbox,
                                float containerWidth,
                                float pixelScale,
                                float forcedWidth,
                                float forcedHeight) {
    const float scale = std::max(0.5f, pixelScale);
    FlexboxStyle style = flexbox.GetStyle();
    Resolve(style);

    float resolvedW = containerWidth;
    if (forcedWidth > 0.0f) {
        resolvedW = forcedWidth;
    } else if (style.hasWidthLength && !style.widthLength.IsAuto()) {
        resolvedW = style.widthLength.Resolve(containerWidth / scale, 1.0f) * scale;
    } else if (!style.width.empty()) {
        resolvedW = ParseDimension(style.width, containerWidth, containerWidth, scale);
    }
    resolvedW = std::max(style.minWidth * scale, std::min(resolvedW, style.maxWidth * scale));

    const float padL = style.paddingLeft * scale;
    const float padR = style.paddingRight * scale;
    const float padT = style.paddingTop * scale;
    const float padB = style.paddingBottom * scale;

    const float innerW = std::max(0.0f, resolvedW - padL - padR);

    const bool isRow = (style.direction == FlexDirection::Row || style.direction == FlexDirection::RowReverse);
    const float mainGap = isRow ? flexbox.ResolveGapX(scale) : flexbox.ResolveGapY(scale);
    const float crossGap = isRow ? flexbox.ResolveGapY(scale) : flexbox.ResolveGapX(scale);

    std::vector<FlexboxItem>& items = flexbox.GetItems();
    std::vector<MeasuredFlexChild> measuredChildren;
    measuredChildren.reserve(items.size());

    for (std::size_t i = 0; i < items.size(); ++i) {
        FlexboxItem& item = items[i];
        MeasuredFlexChild child;
        child.itemIndex = i;

        const bool hasExplicitWidth = !item.props.width.empty();
        const bool hasExplicitHeight = !item.props.height.empty();

        switch (item.kind) {
            case FlexItemKind::Text: {
                const float availW = hasExplicitWidth
                    ? ResolvePropLength(item.props.hasWidthLength, item.props.widthLength, item.props.width, innerW,
                                        innerW, scale)
                    : (isRow ? 100000.0f : innerW);
                TextMeasureResult measured = TextPainter::Measure(fonts, item.text, availW, scale);
                child.baseWidth = measured.width;
                child.baseHeight = measured.height;
                break;
            }
            case FlexItemKind::Svg: {
                SvgMeasureResult measured = SvgPainter::Measure(item.svg, scale);
                child.baseWidth = hasExplicitWidth ? ParseDimension(item.props.width, innerW, measured.width, scale) : measured.width;
                child.baseHeight = hasExplicitHeight ? ParseDimension(item.props.height, innerW, measured.height, scale) : measured.height;
                break;
            }
            case FlexItemKind::BrandLogo: {
                BrandLogoMeasureResult measured = BrandLogoPainter::Measure(fonts, *item.brandLogo, scale);
                child.baseWidth = measured.width;
                child.baseHeight = measured.height;
                break;
            }
            case FlexItemKind::TitleBar: {
                TitleBarMeasureResult measured = TitleBarPainter::Measure(fonts, *item.titleBar, innerW, scale);
                child.baseWidth = measured.width;
                child.baseHeight = measured.height;
                break;
            }
            case FlexItemKind::Sidebar: {
                const float offered = forcedHeight > 0.0f ? std::max(0.0f, forcedHeight - padT - padB) : 0.0f;
                SidebarMeasureResult measured = SidebarPainter::Measure(*item.sidebar, offered, scale);
                child.baseWidth = measured.width;
                child.baseHeight = measured.height;
                break;
            }
            case FlexItemKind::SidebarItem: {
                SidebarItemMeasureResult measured = SidebarPainter::MeasureItem(fonts, *item.sidebarItem, scale);
                child.baseWidth = measured.width;
                child.baseHeight = measured.height;
                break;
            }
            case FlexItemKind::ThemeToggle: {
                ThemeToggleMeasureResult measured = ThemeTogglePainter::Measure(fonts, *item.themeToggle, innerW, scale);
                child.baseWidth = measured.width;
                child.baseHeight = measured.height;
                break;
            }
            case FlexItemKind::ThemeIconToggle: {
                ThemeIconToggleMeasureResult measured = ThemeIconTogglePainter::Measure(*item.themeIconToggle, scale);
                child.baseWidth = measured.width;
                child.baseHeight = measured.height;
                break;
            }
            case FlexItemKind::Separator: {
                const float offeredH = forcedHeight > 0.0f ? std::max(0.0f, forcedHeight - padT - padB) : 0.0f;
                SeparatorMeasureResult measured = SeparatorPainter::Measure(fonts, *item.separator, innerW, offeredH, scale);
                child.baseWidth = measured.width;
                child.baseHeight = measured.height;
                break;
            }
            case FlexItemKind::MotionShape: {
                child.baseWidth = item.motionShape->Width() * scale;
                child.baseHeight = item.motionShape->Height() * scale;
                break;
            }
            case FlexItemKind::Button: {
                ButtonMeasureResult measured = ButtonPainter::Measure(fonts, item.button, scale);
                child.baseWidth = measured.width;
                child.baseHeight = measured.height;
                break;
            }
            case FlexItemKind::Input: {
                const InputStyle& inputStyle = item.input.GetStyle();
                float intrinsicWidth = static_cast<float>(inputStyle.minWidth) * scale;
                if (hasExplicitWidth) {
                    intrinsicWidth = ParseDimension(item.props.width, innerW, intrinsicWidth, scale);
                } else if (item.props.flexGrow > 0.0f) {
                    intrinsicWidth = std::min(intrinsicWidth, 120.0f * scale);
                }
                InputMeasureResult measured = InputPainter::Measure(fonts, item.input, intrinsicWidth, scale);
                child.baseWidth = intrinsicWidth;
                child.baseHeight = measured.height;
                break;
            }
            case FlexItemKind::Scrollbar: {
                const float availW = hasExplicitWidth
                    ? ParseDimension(item.props.width, innerW, innerW, scale)
                    : innerW;
                ScrollbarMeasureResult measured = ScrollbarPainter::Measure(item.scrollbar, availW, scale);
                child.baseWidth = measured.width;
                child.baseHeight = measured.height;
                break;
            }
            case FlexItemKind::Container: {
                if (item.container) {
                    const float availW = hasExplicitWidth
                        ? ParseDimension(item.props.width, innerW, innerW, scale)
                        : innerW;
                    const float forcedH = hasExplicitHeight
                        ? ParseDimension(item.props.height, 0.0f, -1.0f, scale)
                        : -1.0f;
                    ContainerMeasureResult measured =
                        ContainerPainter::Measure(fonts, *item.container, availW, scale, -1.0f, forcedH);
                    child.baseWidth = measured.width;
                    child.baseHeight = measured.height;
                }
                break;
            }
            case FlexItemKind::Flexbox: {
                if (item.flexbox) {
                    const float availW = hasExplicitWidth
                        ? ParseDimension(item.props.width, innerW, innerW, scale)
                        : innerW;
                    FlexboxMeasureResult measured =
                        FlexboxPainter::Measure(fonts, *item.flexbox, availW, scale);
                    child.baseWidth = measured.width;
                    child.baseHeight = measured.height;
                }
                break;
            }
        }

        if (hasExplicitWidth) {
            child.baseWidth = ParseDimension(item.props.width, innerW, child.baseWidth, scale);
        }
        if (hasExplicitHeight) {
            const float specifiedH = ParseDimension(item.props.height, 0.0f, child.baseHeight, scale);
            child.baseHeight = std::max(specifiedH, child.baseHeight);
        }

        if (!item.props.minWidth.empty()) {
            child.baseWidth = std::max(child.baseWidth, ParseDimension(item.props.minWidth, innerW, 0.0f, scale));
        }
        if (!item.props.maxWidth.empty()) {
            child.baseWidth = std::min(child.baseWidth, ParseDimension(item.props.maxWidth, innerW, 100000.0f, scale));
        }
        if (!item.props.minHeight.empty()) {
            child.baseHeight = std::max(child.baseHeight, ParseDimension(item.props.minHeight, 0.0f, 0.0f, scale));
        }
        if (!item.props.maxHeight.empty()) {
            child.baseHeight = std::min(child.baseHeight, ParseDimension(item.props.maxHeight, 0.0f, 100000.0f, scale));
        }

        if (isRow) {
            child.baseMain = child.baseWidth;
            child.baseCross = child.baseHeight;
            if (item.props.flexBasis >= 0.0f) {
                child.baseMain = item.props.flexBasis * scale;
            } else if (!item.props.flexBasisString.empty()) {
                child.baseMain = ParseDimension(item.props.flexBasisString, innerW, child.baseMain, scale);
            }
        } else {
            child.baseMain = child.baseHeight;
            child.baseCross = child.baseWidth;
            if (item.props.flexBasis >= 0.0f) {
                child.baseMain = item.props.flexBasis * scale;
            } else if (!item.props.flexBasisString.empty()) {
                child.baseMain = ParseDimension(item.props.flexBasisString, 0.0f, child.baseMain, scale);
            }
        }

        child.allocatedMain = child.baseMain;
        child.allocatedCross = child.baseCross;
        measuredChildren.push_back(child);
    }

    std::vector<FlexLine> lines;
    if (measuredChildren.empty()) {
        ComputedFlexLayout emptyLayout;
        emptyLayout.containerWidth = resolvedW;
        emptyLayout.containerHeight = padT + padB;
        return emptyLayout;
    }

    const float mainCapacity = isRow ? innerW : 100000.0f;
    const bool allowWrap = (style.wrap == FlexWrap::Wrap || style.wrap == FlexWrap::WrapReverse);

    FlexLine currentLine;
    float currentLineMain = 0.0f;

    for (std::size_t i = 0; i < measuredChildren.size(); ++i) {
        const MeasuredFlexChild& child = measuredChildren[i];
        const float gapNeeded = currentLine.children.empty() ? 0.0f : mainGap;

        if (allowWrap && !currentLine.children.empty() &&
            (currentLineMain + gapNeeded + child.baseMain > mainCapacity)) {
            lines.push_back(currentLine);
            currentLine = FlexLine();
            currentLineMain = 0.0f;
        }

        currentLineMain += (currentLine.children.empty() ? 0.0f : mainGap) + child.baseMain;
        currentLine.children.push_back(child);
    }
    if (!currentLine.children.empty()) {
        lines.push_back(currentLine);
    }

    for (FlexLine& line : lines) {
        float sumBaseMain = 0.0f;
        float totalGrow = 0.0f;
        float totalShrink = 0.0f;
        for (const MeasuredFlexChild& child : line.children) {
            sumBaseMain += child.baseMain;
            const FlexItemProps& props = items[child.itemIndex].props;
            totalGrow += props.flexGrow;
            totalShrink += props.flexShrink * child.baseMain;
        }

        const std::size_t childCount = line.children.size();
        const float totalGaps = childCount > 1 ? static_cast<float>(childCount - 1) * mainGap : 0.0f;
        float freeMain = isRow ? (innerW - sumBaseMain - totalGaps) : 0.0f;

        if (freeMain > 0.0f && totalGrow > 0.0f) {
            for (MeasuredFlexChild& child : line.children) {
                const FlexItemProps& props = items[child.itemIndex].props;
                if (props.flexGrow > 0.0f) {
                    const float growAmount = freeMain * (props.flexGrow / totalGrow);
                    child.allocatedMain = child.baseMain + growAmount;
                    if (!props.maxWidth.empty()) {
                        const float maxW = ParseDimension(props.maxWidth, innerW, 100000.0f, scale);
                        child.allocatedMain = std::min(child.allocatedMain, maxW);
                    }
                }
            }
        } else if (freeMain < 0.0f && totalShrink > 0.0f) {
            for (MeasuredFlexChild& child : line.children) {
                const FlexItemProps& props = items[child.itemIndex].props;
                if (props.flexShrink > 0.0f) {
                    const float shrinkAmount =
                        (-freeMain) * (props.flexShrink * child.baseMain / totalShrink);
                    float minLimit = 20.0f * scale;
                    if (!props.minWidth.empty()) {
                        minLimit = ParseDimension(props.minWidth, innerW, minLimit, scale);
                    }
                    child.allocatedMain = std::max(minLimit, child.baseMain - shrinkAmount);
                }
            }
        }

        float maxCross = 0.0f;
        for (MeasuredFlexChild& child : line.children) {
            maxCross = std::max(maxCross, child.baseCross);
        }
        if (!isRow && style.wrap == FlexWrap::NoWrap) {
            line.lineCrossSize = innerW;
        } else {
            line.lineCrossSize = maxCross;
        }

        for (MeasuredFlexChild& child : line.children) {
            const FlexItemProps& props = items[child.itemIndex].props;
            FlexAlign align = props.alignSelf != FlexAlign::Auto ? props.alignSelf : style.align;
            if (align == FlexAlign::Stretch) {
                if (KeepsIntrinsicCrossSize(items[child.itemIndex]) &&
                    props.alignSelf != FlexAlign::Stretch) {
                    child.allocatedCross = child.baseCross;
                } else {
                    child.allocatedCross = line.lineCrossSize;
                }
            }
            if (!isRow) {
                child.allocatedCross = std::min(child.allocatedCross, innerW);
            }
        }
    }

    if (style.wrap == FlexWrap::WrapReverse) {
        std::reverse(lines.begin(), lines.end());
    }

    float totalLinesCross = 0.0f;
    for (std::size_t lineIdx = 0; lineIdx < lines.size(); ++lineIdx) {
        if (lineIdx > 0) {
            totalLinesCross += crossGap;
        }
        totalLinesCross += lines[lineIdx].lineCrossSize;
    }

    float maxLineW = 0.0f;
    for (const FlexLine& line : lines) {
        float lineW = 0.0f;
        for (std::size_t c = 0; c < line.children.size(); ++c) {
            if (c > 0) lineW += mainGap;
            lineW += line.children[c].allocatedMain;
        }
        maxLineW = std::max(maxLineW, lineW);
    }
    const float naturalContentWidth = isRow ? (padL + padR + maxLineW) : (padL + padR + totalLinesCross);

    if (forcedWidth > 0.0f) {
        resolvedW = forcedWidth;
    } else if (!style.width.empty() && style.width != "auto") {
        resolvedW = ParseDimension(style.width, containerWidth, naturalContentWidth, scale);
    } else {
        resolvedW = std::min(containerWidth, naturalContentWidth);
    }
    resolvedW = std::max(style.minWidth * scale, std::min(resolvedW, style.maxWidth * scale));

    const float finalInnerW = std::max(0.0f, resolvedW - padL - padR);

    if (!isRow && style.wrap == FlexWrap::NoWrap) {
        for (FlexLine& line : lines) {
            line.lineCrossSize = finalInnerW;
            for (MeasuredFlexChild& child : line.children) {
                const FlexItemProps& props = items[child.itemIndex].props;
                FlexAlign align = props.alignSelf != FlexAlign::Auto ? props.alignSelf : style.align;
                if (align == FlexAlign::Stretch) {
                    if (KeepsIntrinsicCrossSize(items[child.itemIndex]) &&
                        props.alignSelf != FlexAlign::Stretch) {
                        child.allocatedCross = child.baseCross;
                    } else {
                        child.allocatedCross = finalInnerW;
                    }
                }
                child.allocatedCross = std::min(child.allocatedCross, finalInnerW);
            }
        }
    }

    float resolvedH = padT + padB + (isRow ? totalLinesCross : 0.0f);
    if (!isRow) {
        float maxColH = 0.0f;
        for (const FlexLine& line : lines) {
            float colH = 0.0f;
            for (std::size_t c = 0; c < line.children.size(); ++c) {
                if (c > 0) {
                    colH += mainGap;
                }
                colH += line.children[c].allocatedMain;
            }
            maxColH = std::max(maxColH, colH);
        }
        resolvedH = padT + padB + maxColH;
    }

    const float naturalContentHeight = resolvedH;
    if (forcedHeight > 0.0f) {
        resolvedH = std::max(forcedHeight, naturalContentHeight);
    } else if (!style.height.empty() && style.height != "auto") {
        resolvedH = std::max(ParseDimension(style.height, 0.0f, naturalContentHeight, scale), naturalContentHeight);
    }
    resolvedH = std::max(style.minHeight * scale, std::min(resolvedH, style.maxHeight * scale));

    float currentCrossOffset = isRow ? padT : padL;

    for (FlexLine& line : lines) {
        float sumAllocatedMain = 0.0f;
        for (const MeasuredFlexChild& child : line.children) {
            sumAllocatedMain += child.allocatedMain;
        }

        const std::size_t childCount = line.children.size();
        const float totalGaps = childCount > 1 ? static_cast<float>(childCount - 1) * mainGap : 0.0f;
        const float mainCapacity = isRow ? innerW : std::max(0.0f, resolvedH - padT - padB);
        const float remainingMain = std::max(0.0f, mainCapacity - sumAllocatedMain - totalGaps);

        float mainCursor = isRow ? padL : padT;
        float actualGap = mainGap;

        if (remainingMain > 0.0f) {
            switch (style.justify) {
                case FlexJustify::Center:
                    mainCursor += remainingMain * 0.5f;
                    break;
                case FlexJustify::FlexEnd:
                    mainCursor += remainingMain;
                    break;
                case FlexJustify::SpaceBetween:
                    if (childCount > 1) {
                        actualGap = mainGap + remainingMain / static_cast<float>(childCount - 1);
                    }
                    break;
                case FlexJustify::SpaceAround:
                    if (childCount > 0) {
                        const float margin = remainingMain / static_cast<float>(childCount * 2);
                        mainCursor += margin;
                        actualGap = mainGap + margin * 2.0f;
                    }
                    break;
                case FlexJustify::SpaceEvenly:
                    if (childCount > 0) {
                        const float space = remainingMain / static_cast<float>(childCount + 1);
                        mainCursor += space;
                        actualGap = mainGap + space;
                    }
                    break;
                case FlexJustify::FlexStart:
                default:
                    break;
            }
        }

        if (style.direction == FlexDirection::RowReverse ||
            style.direction == FlexDirection::ColumnReverse) {
            std::reverse(line.children.begin(), line.children.end());
        }

        for (MeasuredFlexChild& child : line.children) {
            const FlexItemProps& props = items[child.itemIndex].props;
            FlexAlign align = props.alignSelf != FlexAlign::Auto ? props.alignSelf : style.align;

            float crossOffset = currentCrossOffset;
            if (align == FlexAlign::Center) {
                crossOffset += (line.lineCrossSize - child.allocatedCross) * 0.5f;
            } else if (align == FlexAlign::FlexEnd) {
                crossOffset += (line.lineCrossSize - child.allocatedCross);
            }

            if (isRow) {
                child.finalX = mainCursor;
                child.finalY = crossOffset;
                child.finalWidth = child.allocatedMain;
                child.finalHeight = child.allocatedCross;
                mainCursor += child.allocatedMain + actualGap;
            } else {
                child.finalX = crossOffset;
                child.finalY = mainCursor;
                child.finalWidth = child.allocatedCross;
                child.finalHeight = child.allocatedMain;
                mainCursor += child.allocatedMain + actualGap;
            }
        }

        currentCrossOffset += line.lineCrossSize + crossGap;
    }

    ComputedFlexLayout layout;
    layout.containerWidth = resolvedW;
    layout.containerHeight = resolvedH;
    layout.lines = std::move(lines);
    return layout;

}

}  // namespace

FlexboxMeasureResult FlexboxPainter::Measure(const FontEngine& fonts,
                                            Flexbox& flexbox,
                                            float containerWidth,
                                            float pixelScale,
                                            float forcedWidth,
                                            float forcedHeight) {
    const float scale = std::max(0.5f, pixelScale);
    if (flexbox.HasLayoutCache(containerWidth, scale, forcedWidth, forcedHeight)) {
        FlexboxMeasureResult cached;
        cached.width = flexbox.LayoutCache().containerWidth;
        cached.height = flexbox.LayoutCache().containerHeight;
        return cached;
    }
    if (flexbox.HasMeasureCache(containerWidth, scale, forcedWidth, forcedHeight) &&
        !flexbox.NeedsLayout()) {
        FlexboxMeasureResult cached;
        flexbox.ReadMeasureCache(cached.width, cached.height);
        return cached;
    }
    ComputedFlexLayout layout =
        ComputeLayout(fonts, flexbox, containerWidth, pixelScale, forcedWidth, forcedHeight);
    FlexboxMeasureResult result;
    result.width = layout.containerWidth;
    result.height = layout.containerHeight;
    flexbox.StoreLayoutCache(containerWidth, scale, forcedWidth, forcedHeight, std::move(layout));
    return result;
}

FlexboxPaintResult FlexboxPainter::Paint(FrameBuffer& buffer,
                                        const FontEngine& fonts,
                                        Flexbox& flexbox,
                                        float originX,
                                        float originY,
                                        float containerWidth,
                                        float pixelScale,
                                        float forcedWidth,
                                        float forcedHeight) {
    const SelectableScope selection(&flexbox, flexbox.GetStyle().selectable);
    const float scale = std::max(0.5f, pixelScale);
    FlexboxStyle style = flexbox.GetStyle();
    Resolve(style);

    const ComputedFlexLayout* layoutPtr = nullptr;
    ComputedFlexLayout layoutOwned;
    if (flexbox.HasLayoutCache(containerWidth, scale, forcedWidth, forcedHeight)) {
        layoutPtr = &flexbox.LayoutCache();
    } else {
        layoutOwned = ComputeLayout(fonts, flexbox, containerWidth, scale, forcedWidth, forcedHeight);
        flexbox.StoreLayoutCache(containerWidth, scale, forcedWidth, forcedHeight, std::move(layoutOwned));
        layoutPtr = &flexbox.LayoutCache();
    }
    const ComputedFlexLayout& layout = *layoutPtr;
    flexbox.SetNeedsPaint(false);

    const float containerX = originX + style.marginLeft * scale;
    const float containerY = originY + style.marginTop * scale;
    const float containerW = layout.containerWidth;
    const float containerH = layout.containerHeight;

    if (style.backgroundColor.alpha > 0) {
        FillRoundedRect(buffer,
                        containerX,
                        containerY,
                        containerW,
                        containerH,
                        style.cornerRadius * scale,
                        style.backgroundColor,
                        style.opacity);
    }

    if (style.borderWidth > 0.0f && style.borderColor.alpha > 0) {
        StrokeRoundedRect(buffer,
                          containerX,
                          containerY,
                          containerW,
                          containerH,
                          style.cornerRadius * scale,
                          style.borderWidth * scale,
                          style.borderColor,
                          style.opacity);
    }

    std::vector<FlexboxItem>& items = flexbox.GetItems();

    for (const FlexLine& line : layout.lines) {
        for (const MeasuredFlexChild& child : line.children) {
            FlexboxItem& item = items[child.itemIndex];
            const float childX = containerX + child.finalX;
            const float childY = containerY + child.finalY;
            const float childW = child.finalWidth;
            const float childH = child.finalHeight;

            switch (item.kind) {
                case FlexItemKind::Text: {
                    TextPainter::Paint(buffer, fonts, item.text, childX, childY, childW, scale);
                    break;
                }
                case FlexItemKind::Svg: {
                    SvgPainter::Paint(buffer, item.svg, childX, childY, childW, scale);
                    break;
                }
                case FlexItemKind::BrandLogo: {
                    BrandLogoPainter::Paint(buffer, fonts, *item.brandLogo, childX, childY, scale);
                    break;
                }
                case FlexItemKind::TitleBar: {
                    TitleBarPainter::Paint(buffer, fonts, *item.titleBar, childX, childY, childW, scale);
                    break;
                }
                case FlexItemKind::Sidebar: {
                    SidebarPainter::Paint(buffer, fonts, *item.sidebar, childX, childY, childH, scale, 1.0f);
                    break;
                }
                case FlexItemKind::SidebarItem: {
                    SidebarPainter::PaintItem(buffer, fonts, *item.sidebarItem, childX, childY, childW, childH, scale, false);
                    break;
                }
                case FlexItemKind::ThemeToggle: {
                    ThemeTogglePainter::Paint(buffer, fonts, *item.themeToggle, childX, childY, childW, scale);
                    break;
                }
                case FlexItemKind::ThemeIconToggle: {
                    ThemeIconTogglePainter::Paint(buffer, *item.themeIconToggle, childX, childY, scale);
                    break;
                }
                case FlexItemKind::Separator: {
                    SeparatorPainter::Paint(buffer, fonts, *item.separator, childX, childY, childW, childH, scale);
                    break;
                }
                case FlexItemKind::MotionShape: {
                    item.motionShape->Paint(buffer, childX, childY, childW, childH, scale);
                    break;
                }
                case FlexItemKind::Button: {
                    ButtonPainter::Paint(buffer,
                                         fonts,
                                         item.button,
                                         childX,
                                         childY,
                                         childW,
                                         scale,
                                         childW,
                                         childH);
                    break;
                }
                case FlexItemKind::Input: {
                    InputPainter::Paint(buffer,
                                        fonts,
                                        item.input,
                                        childX,
                                        childY,
                                        childW,
                                        scale,
                                        childW,
                                        childH);
                    break;
                }
                case FlexItemKind::Scrollbar: {
                    const bool vertical =
                        item.scrollbar.GetOrientation() == ScrollbarOrientation::Vertical;
                    ScrollbarPainter::Paint(buffer,
                                            item.scrollbar,
                                            childX,
                                            childY,
                                            childW,
                                            scale,
                                            vertical ? -1.0f : childW,
                                            vertical ? childH : -1.0f);
                    break;
                }
                case FlexItemKind::Container: {
                    if (item.container) {
                        const float cw = (!item.props.width.empty() && item.props.width != "auto") ? childW : -1.0f;
                        const float ch = (!item.props.height.empty() && item.props.height != "auto") ? childH : -1.0f;
                        ContainerPainter::Paint(buffer,
                                                fonts,
                                                *item.container,
                                                childX,
                                                childY,
                                                childW,
                                                scale,
                                                cw,
                                                ch);
                    }
                    break;
                }
                case FlexItemKind::Flexbox: {
                    if (item.flexbox) {
                        const float fw = (!item.props.width.empty() && item.props.width != "auto") ? childW : -1.0f;
                        const float fh = (!item.props.height.empty() && item.props.height != "auto") ? childH : -1.0f;
                        FlexboxPainter::Paint(buffer,
                                              fonts,
                                              *item.flexbox,
                                              childX,
                                              childY,
                                              childW,
                                              scale,
                                              fw,
                                              fh);
                    }
                    break;
                }
            }
        }
    }

    FlexboxBounds bounds;
    bounds.x = containerX;
    bounds.y = containerY;
    bounds.width = containerW;
    bounds.height = containerH;
    flexbox.SetBounds(bounds);

    FlexboxPaintResult result;
    result.bounds = bounds;
    result.advanceY = containerH + (style.marginBottom + 16.0f) * scale;
    return result;
}

}  // namespace Robo
