#include "Robo/GridPainter.hpp"
#include "Robo/PropLength.hpp"
#include "Robo/Foundation.hpp"
#include "Robo/TextSelection.hpp"
#include "Robo/ContainerPainter.hpp"
#include "Robo/BrandLogoPainter.hpp"
#include "Robo/TitleBarPainter.hpp"
#include "Robo/SidebarPainter.hpp"
#include "Robo/SeparatorPainter.hpp"
#include "Robo/ThemeIconTogglePainter.hpp"
#include "Robo/ThemeTogglePainter.hpp"
#include "Robo/ButtonPainter.hpp"
#include "Robo/FlexboxPainter.hpp"
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
    if (str.size() > 3 && str.compare(str.size() - 3, 3, "rem") == 0) {
        return static_cast<float>(std::atof(str.c_str()) * 16.0f) * scale;
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

ComputedGridLayout ComputeLayout(const FontEngine& fonts,
                                Grid& grid,
                                float containerWidth,
                                float pixelScale,
                                float forcedWidth,
                                float forcedHeight) {
    const float scale = std::max(0.5f, pixelScale);
    GridStyle style = grid.GetStyle();
    Resolve(style);

    float resolvedW = containerWidth;
    if (forcedWidth > 0.0f) {
        resolvedW = forcedWidth;
    } else if (style.hasWidthLength && !style.widthLength.IsAuto()) {
        resolvedW = style.widthLength.Resolve(containerWidth / scale, 1.0f) * scale;
    } else if (!style.width.empty() && style.width != "auto") {
        resolvedW = ParseDimension(style.width, containerWidth, containerWidth, scale);
    }
    resolvedW = std::max(style.minWidth * scale, std::min(resolvedW, style.maxWidth * scale));

    const float padL = style.paddingLeft * scale;
    const float padR = style.paddingRight * scale;
    const float padT = style.paddingTop * scale;
    const float padB = style.paddingBottom * scale;
    const float innerW = std::max(0.0f, resolvedW - padL - padR);

    std::vector<GridTrack> colTracks = style.templateColumns;
    if (colTracks.empty() && !style.columnsString.empty()) {
        colTracks = ParseGridTracks(style.columnsString);
    }
    if (colTracks.empty() && style.columnCount > 0) {
        colTracks.assign(static_cast<std::size_t>(style.columnCount), {GridTrackType::Fraction, 1.0f});
    }
    if (colTracks.empty()) {
        std::size_t count = grid.GetItems().size();
        if (count == 0) {
            count = 1;
        } else if (count > 4) {
            count = 4;
        }
        colTracks.assign(count, {GridTrackType::Fraction, 1.0f});
    }

    const int numCols = std::max(1, static_cast<int>(colTracks.size()));
    const float colGap = grid.ResolveGapX(scale);
    const float rowGap = grid.ResolveGapY(scale);

    std::vector<GridItem>& items = grid.GetItems();
    std::vector<PlacedGridChild> placedChildren;
    placedChildren.reserve(items.size());

    // 2D occupancy matrix for auto-placement
    std::vector<std::vector<bool>> occupied;

    auto EnsureRows = [&](int neededRows) {
        while (static_cast<int>(occupied.size()) < neededRows) {
            occupied.push_back(std::vector<bool>(static_cast<std::size_t>(numCols), false));
        }
    };

    auto IsCellAvailable = [&](int r, int c, int rSpan, int cSpan) -> bool {
        if (c + cSpan > numCols) {
            return false;
        }
        for (int row = r; row < r + rSpan; ++row) {
            if (row < static_cast<int>(occupied.size())) {
                for (int col = c; col < c + cSpan; ++col) {
                    if (occupied[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)]) {
                        return false;
                    }
                }
            }
        }
        return true;
    };

    auto MarkOccupied = [&](int r, int c, int rSpan, int cSpan) {
        EnsureRows(r + rSpan);
        for (int row = r; row < r + rSpan; ++row) {
            for (int col = c; col < c + cSpan; ++col) {
                occupied[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = true;
            }
        }
    };

    int autoRow = 0;
    int autoCol = 0;

    for (std::size_t i = 0; i < items.size(); ++i) {
        GridItem& item = items[i];
        PlacedGridChild child;
        child.itemIndex = i;

        const int colSpan = std::max(1, std::min(item.props.columnSpan, numCols));
        const int rowSpan = std::max(1, item.props.rowSpan);
        child.colSpan = colSpan;
        child.rowSpan = rowSpan;

        if (item.props.column > 0 && item.props.row > 0) {
            child.colStart = std::max(0, std::min(item.props.column - 1, numCols - colSpan));
            child.rowStart = std::max(0, item.props.row - 1);
            MarkOccupied(child.rowStart, child.colStart, rowSpan, colSpan);
        } else {
            // Find next free cell
            bool placed = false;
            while (!placed) {
                if (autoCol + colSpan > numCols) {
                    autoCol = 0;
                    ++autoRow;
                }
                if (IsCellAvailable(autoRow, autoCol, rowSpan, colSpan)) {
                    child.rowStart = autoRow;
                    child.colStart = autoCol;
                    MarkOccupied(autoRow, autoCol, rowSpan, colSpan);
                    autoCol += colSpan;
                    placed = true;
                } else {
                    ++autoCol;
                }
            }
        }

        // Measure item intrinsic sizes
        const bool hasExplicitWidth = PropHasWidth(item.props.hasWidthLength, item.props.widthLength, item.props.width);
        const bool hasExplicitHeight = PropHasHeight(item.props.hasHeightLength, item.props.heightLength, item.props.height);

        switch (item.kind) {
            case GridItemKind::Text: {
                const float availW = hasExplicitWidth ? ResolvePropLength(item.props.hasWidthLength, item.props.widthLength, item.props.width, innerW, innerW, scale) : innerW;
                TextMeasureResult measured = TextPainter::Measure(fonts, *item.text, availW, scale);
                child.measuredWidth = measured.width;
                child.measuredHeight = measured.height;
                break;
            }
            case GridItemKind::Svg: {
                SvgMeasureResult measured = SvgPainter::Measure(*item.svg, scale);
                child.measuredWidth = hasExplicitWidth ? ResolvePropLength(item.props.hasWidthLength, item.props.widthLength, item.props.width, innerW, measured.width, scale) : measured.width;
                child.measuredHeight = hasExplicitHeight ? ResolvePropLength(item.props.hasHeightLength, item.props.heightLength, item.props.height, innerW, measured.height, scale) : measured.height;
                break;
            }
            case GridItemKind::TitleBar: {
                TitleBarMeasureResult measured = TitleBarPainter::Measure(fonts, *item.titleBar, innerW, scale);
                child.measuredWidth = measured.width;
                child.measuredHeight = measured.height;
                break;
            }
            case GridItemKind::Sidebar: {
                SidebarMeasureResult measured = SidebarPainter::Measure(*item.sidebar, 0.0f, scale);
                child.measuredWidth = measured.width;
                child.measuredHeight = measured.height;
                break;
            }
            case GridItemKind::BrandLogo: {
                BrandLogoMeasureResult measured = BrandLogoPainter::Measure(fonts, *item.brandLogo, scale);
                child.measuredWidth = measured.width;
                child.measuredHeight = measured.height;
                break;
            }
            case GridItemKind::ThemeToggle: {
                ThemeToggleMeasureResult measured = ThemeTogglePainter::Measure(fonts, *item.themeToggle, innerW, scale);
                child.measuredWidth = measured.width;
                child.measuredHeight = measured.height;
                break;
            }
            case GridItemKind::ThemeIconToggle: {
                ThemeIconToggleMeasureResult measured = ThemeIconTogglePainter::Measure(*item.themeIconToggle, scale);
                child.measuredWidth = measured.width;
                child.measuredHeight = measured.height;
                break;
            }
            case GridItemKind::SidebarItem: {
                SidebarItemMeasureResult measured = SidebarPainter::MeasureItem(fonts, *item.sidebarItem, scale);
                child.measuredWidth = measured.width;
                child.measuredHeight = measured.height;
                break;
            }
            case GridItemKind::Separator: {
                SeparatorMeasureResult measured = SeparatorPainter::Measure(fonts, *item.separator, innerW, 0.0f, scale);
                child.measuredWidth = measured.width;
                child.measuredHeight = measured.height;
                break;
            }
            case GridItemKind::MotionShape: {
                child.measuredWidth = item.motionShape->Width() * scale;
                child.measuredHeight = item.motionShape->Height() * scale;
                break;
            }
            case GridItemKind::Button: {
                ButtonMeasureResult measured = ButtonPainter::Measure(fonts, *item.button, scale);
                child.measuredWidth = measured.width;
                child.measuredHeight = measured.height;
                break;
            }
            case GridItemKind::Input: {
                const InputStyle& inputStyle = item.input->GetStyle();
                float intrinsicWidth = static_cast<float>(inputStyle.minWidth) * scale;
                if (PropHasWidth(item.props.hasMinWidthLength, item.props.minWidthLength, item.props.minWidth)) {
                    intrinsicWidth = ResolvePropLength(item.props.hasMinWidthLength, item.props.minWidthLength, item.props.minWidth, innerW, intrinsicWidth, scale);
                }
                if (hasExplicitWidth) {
                    intrinsicWidth = ResolvePropLength(item.props.hasWidthLength, item.props.widthLength, item.props.width, innerW, intrinsicWidth, scale);
                }
                InputMeasureResult measured = InputPainter::Measure(fonts, *item.input, intrinsicWidth, scale);
                child.measuredWidth = intrinsicWidth;
                child.measuredHeight = measured.height;
                break;
            }
            case GridItemKind::Scrollbar: {
                const float availW =
                    hasExplicitWidth ? ResolvePropLength(item.props.hasWidthLength, item.props.widthLength, item.props.width, innerW, innerW, scale) : innerW;
                ScrollbarMeasureResult measured = ScrollbarPainter::Measure(*item.scrollbar, availW, scale);
                child.measuredWidth = measured.width;
                child.measuredHeight = measured.height;
                break;
            }
            case GridItemKind::Container: {
                if (item.container) {
                    const float availW = hasExplicitWidth ? ResolvePropLength(item.props.hasWidthLength, item.props.widthLength, item.props.width, innerW, innerW, scale) : innerW;
                    const float forcedH = (PropHasHeight(item.props.hasHeightLength, item.props.heightLength, item.props.height) && item.props.height != "auto")
                        ? ResolvePropLength(item.props.hasHeightLength, item.props.heightLength, item.props.height, 0.0f, -1.0f, scale) : -1.0f;
                    ContainerMeasureResult measured =
                        ContainerPainter::Measure(fonts, *item.container, availW, scale, -1.0f, forcedH);
                    child.measuredWidth = measured.width;
                    child.measuredHeight = measured.height;
                }
                break;
            }
            case GridItemKind::Flexbox: {
                if (item.flexbox) {
                    const float availW = hasExplicitWidth ? ResolvePropLength(item.props.hasWidthLength, item.props.widthLength, item.props.width, innerW, innerW, scale) : innerW;
                    const float forcedH = (PropHasHeight(item.props.hasHeightLength, item.props.heightLength, item.props.height) && item.props.height != "auto")
                        ? ResolvePropLength(item.props.hasHeightLength, item.props.heightLength, item.props.height, 0.0f, -1.0f, scale) : -1.0f;
                    FlexboxMeasureResult measured =
                        FlexboxPainter::Measure(fonts, *item.flexbox, availW, scale, -1.0f, forcedH);
                    child.measuredWidth = measured.width;
                    child.measuredHeight = measured.height;
                }
                break;
            }
            case GridItemKind::Grid: {
                if (item.grid) {
                    const float availW = hasExplicitWidth ? ResolvePropLength(item.props.hasWidthLength, item.props.widthLength, item.props.width, innerW, innerW, scale) : innerW;
                    const float forcedH = (PropHasHeight(item.props.hasHeightLength, item.props.heightLength, item.props.height) && item.props.height != "auto")
                        ? ResolvePropLength(item.props.hasHeightLength, item.props.heightLength, item.props.height, 0.0f, -1.0f, scale) : -1.0f;
                    GridMeasureResult measured =
                        GridPainter::Measure(fonts, *item.grid, availW, scale, -1.0f, forcedH);
                    child.measuredWidth = measured.width;
                    child.measuredHeight = measured.height;
                }
                break;
            }
        }

        if (hasExplicitWidth) {
            child.measuredWidth = ResolvePropLength(item.props.hasWidthLength, item.props.widthLength, item.props.width, innerW, child.measuredWidth, scale);
        }
        if (hasExplicitHeight) {
            child.measuredHeight = ResolvePropLength(item.props.hasHeightLength, item.props.heightLength, item.props.height, 0.0f, child.measuredHeight, scale);
        }
        if (PropHasWidth(item.props.hasMinWidthLength, item.props.minWidthLength, item.props.minWidth)) {
            child.measuredWidth = std::max(child.measuredWidth, ResolvePropLength(item.props.hasMinWidthLength, item.props.minWidthLength, item.props.minWidth, innerW, 0.0f, scale));
        }
        if (PropHasWidth(item.props.hasMaxWidthLength, item.props.maxWidthLength, item.props.maxWidth)) {
            child.measuredWidth = std::min(child.measuredWidth, ResolvePropLength(item.props.hasMaxWidthLength, item.props.maxWidthLength, item.props.maxWidth, innerW, 100000.0f, scale));
        }
        if (PropHasHeight(item.props.hasMinHeightLength, item.props.minHeightLength, item.props.minHeight)) {
            child.measuredHeight = std::max(child.measuredHeight, ResolvePropLength(item.props.hasMinHeightLength, item.props.minHeightLength, item.props.minHeight, 0.0f, 0.0f, scale));
        }
        if (PropHasHeight(item.props.hasMaxHeightLength, item.props.maxHeightLength, item.props.maxHeight)) {
            child.measuredHeight = std::min(child.measuredHeight, ResolvePropLength(item.props.hasMaxHeightLength, item.props.maxHeightLength, item.props.maxHeight, 0.0f, 100000.0f, scale));
        }

        placedChildren.push_back(child);
    }

    int numRows = 0;
    for (const PlacedGridChild& c : placedChildren) {
        numRows = std::max(numRows, c.rowStart + c.rowSpan);
    }
    if (numRows == 0) {
        numRows = 1;
    }

    // Resolve column widths
    const float totalColGaps = (numCols > 1) ? static_cast<float>(numCols - 1) * colGap : 0.0f;
    std::vector<float> colWidths(static_cast<std::size_t>(numCols), 0.0f);
    float nonFrTotal = 0.0f;
    float sumFr = 0.0f;

    for (int c = 0; c < numCols; ++c) {
        const GridTrack& track = colTracks[static_cast<std::size_t>(c)];
        if (track.type == GridTrackType::Pixel) {
            colWidths[static_cast<std::size_t>(c)] = track.value * scale;
            nonFrTotal += colWidths[static_cast<std::size_t>(c)];
        } else if (track.type == GridTrackType::Percent) {
            colWidths[static_cast<std::size_t>(c)] = innerW * (track.value / 100.0f);
            nonFrTotal += colWidths[static_cast<std::size_t>(c)];
        } else if (track.type == GridTrackType::Auto) {
            float maxAuto = 40.0f * scale;
            for (const PlacedGridChild& child : placedChildren) {
                if (child.colSpan == 1 && child.colStart == c) {
                    maxAuto = std::max(maxAuto, child.measuredWidth);
                }
            }
            colWidths[static_cast<std::size_t>(c)] = maxAuto;
            nonFrTotal += maxAuto;
        } else if (track.type == GridTrackType::Fraction) {
            sumFr += track.value;
        }
    }

    const float freeColSpace = std::max(0.0f, innerW - nonFrTotal - totalColGaps);
    if (sumFr > 0.0f) {
        for (int c = 0; c < numCols; ++c) {
            if (colTracks[static_cast<std::size_t>(c)].type == GridTrackType::Fraction) {
                colWidths[static_cast<std::size_t>(c)] =
                    freeColSpace * (colTracks[static_cast<std::size_t>(c)].value / sumFr);
            }
        }
    }

    float totalColsWidth = 0.0f;
    for (float cw : colWidths) {
        totalColsWidth += cw;
    }
    totalColsWidth += totalColGaps;
    const float naturalGridWidth = padL + padR + totalColsWidth;

    if (forcedWidth > 0.0f) {
        resolvedW = forcedWidth;
    } else if (!style.width.empty() && style.width != "auto") {
        resolvedW = ParseDimension(style.width, containerWidth, naturalGridWidth, scale);
    } else {
        resolvedW = std::min(containerWidth, naturalGridWidth);
    }
    resolvedW = std::max(style.minWidth * scale, std::min(resolvedW, style.maxWidth * scale));

    // Resolve row heights
    std::vector<GridTrack> rowTracks = style.templateRows;
    if (rowTracks.empty() && !style.rowsString.empty()) {
        rowTracks = ParseGridTracks(style.rowsString);
    }

    std::vector<float> rowHeights(static_cast<std::size_t>(numRows), 0.0f);
    for (int r = 0; r < numRows; ++r) {
        if (r < static_cast<int>(rowTracks.size()) && rowTracks[static_cast<std::size_t>(r)].type == GridTrackType::Pixel) {
            rowHeights[static_cast<std::size_t>(r)] = rowTracks[static_cast<std::size_t>(r)].value * scale;
        } else {
            float maxRowH = 36.0f * scale;
            for (const PlacedGridChild& child : placedChildren) {
                if (child.rowSpan == 1 && child.rowStart == r) {
                    maxRowH = std::max(maxRowH, child.measuredHeight);
                }
            }
            rowHeights[static_cast<std::size_t>(r)] = maxRowH;
        }
    }

    // Compute track start coordinates
    std::vector<float> colOffsets(static_cast<std::size_t>(numCols), 0.0f);
    float colCursor = padL;
    for (int c = 0; c < numCols; ++c) {
        colOffsets[static_cast<std::size_t>(c)] = colCursor;
        colCursor += colWidths[static_cast<std::size_t>(c)] + colGap;
    }

    std::vector<float> rowOffsets(static_cast<std::size_t>(numRows), 0.0f);
    float rowCursor = padT;
    for (int r = 0; r < numRows; ++r) {
        rowOffsets[static_cast<std::size_t>(r)] = rowCursor;
        rowCursor += rowHeights[static_cast<std::size_t>(r)] + rowGap;
    }

    const float totalRowsHeight = (numRows > 0) ? (rowCursor - rowGap - padT) : 0.0f;
    const float naturalGridHeight = padT + padB + totalRowsHeight;
    float resolvedH = naturalGridHeight;
    if (forcedHeight > 0.0f) {
        resolvedH = forcedHeight;
    } else if (style.hasHeightLength && !style.heightLength.IsAuto()) {
        resolvedH = style.heightLength.Resolve(containerWidth / scale, 1.0f) * scale;
    } else if (!style.height.empty() && style.height != "auto") {
        resolvedH = ParseDimension(style.height, 0.0f, naturalGridHeight, scale);
    }
    resolvedH = std::max(style.minHeight * scale, std::min(resolvedH, style.maxHeight * scale));

    // Place and align children within cells
    for (PlacedGridChild& child : placedChildren) {
        const GridItemProps& props = items[child.itemIndex].props;
        const float cellX = colOffsets[static_cast<std::size_t>(child.colStart)];
        float cellW = 0.0f;
        for (int c = child.colStart; c < child.colStart + child.colSpan; ++c) {
            cellW += colWidths[static_cast<std::size_t>(c)];
        }
        if (child.colSpan > 1) {
            cellW += static_cast<float>(child.colSpan - 1) * colGap;
        }

        const float cellY = rowOffsets[static_cast<std::size_t>(child.rowStart)];
        float cellH = 0.0f;
        for (int r = child.rowStart; r < child.rowStart + child.rowSpan; ++r) {
            cellH += rowHeights[static_cast<std::size_t>(r)];
        }
        if (child.rowSpan > 1) {
            cellH += static_cast<float>(child.rowSpan - 1) * rowGap;
        }

        GridAlignment justify = props.justifySelf != GridAlignment::Auto ? props.justifySelf : style.justifyItems;
        GridAlignment align = props.alignSelf != GridAlignment::Auto ? props.alignSelf : style.alignItems;

        float itemW = cellW;
        if (justify == GridAlignment::Stretch) {
            itemW = cellW;
        } else {
            itemW = std::min(cellW, child.measuredWidth);
        }
        if (items[child.itemIndex].kind == GridItemKind::Scrollbar &&
            items[child.itemIndex].scrollbar->GetOrientation() == ScrollbarOrientation::Vertical) {
            itemW = std::min(cellW, child.measuredWidth);
        }
        if (PropHasWidth(props.hasMaxWidthLength, props.maxWidthLength, props.maxWidth)) {
            itemW = std::min(itemW, ResolvePropLength(props.hasMaxWidthLength, props.maxWidthLength, props.maxWidth, innerW, itemW, scale));
        }
        if (PropHasWidth(props.hasMinWidthLength, props.minWidthLength, props.minWidth)) {
            itemW = std::max(itemW, ResolvePropLength(props.hasMinWidthLength, props.minWidthLength, props.minWidth, innerW, itemW, scale));
        }

        float itemX = cellX;
        if (justify == GridAlignment::Center) {
            itemX = cellX + std::max(0.0f, (cellW - itemW) * 0.5f);
        } else if (justify == GridAlignment::End) {
            itemX = cellX + std::max(0.0f, cellW - itemW);
        }

        float itemH = cellH;
        if (align == GridAlignment::Stretch) {
            itemH = cellH;
        } else {
            itemH = std::min(cellH, child.measuredHeight);
        }
        if (items[child.itemIndex].kind == GridItemKind::Scrollbar &&
            items[child.itemIndex].scrollbar->GetOrientation() == ScrollbarOrientation::Horizontal) {
            itemH = std::min(cellH, child.measuredHeight);
        }
        if (PropHasHeight(props.hasMaxHeightLength, props.maxHeightLength, props.maxHeight)) {
            itemH = std::min(itemH, ResolvePropLength(props.hasMaxHeightLength, props.maxHeightLength, props.maxHeight, 0.0f, itemH, scale));
        }
        if (PropHasHeight(props.hasMinHeightLength, props.minHeightLength, props.minHeight)) {
            itemH = std::max(itemH, ResolvePropLength(props.hasMinHeightLength, props.minHeightLength, props.minHeight, 0.0f, itemH, scale));
        }

        float itemY = cellY;
        if (align == GridAlignment::Center) {
            itemY = cellY + std::max(0.0f, (cellH - itemH) * 0.5f);
        } else if (align == GridAlignment::End) {
            itemY = cellY + std::max(0.0f, cellH - itemH);
        }

        child.finalX = itemX;
        child.finalY = itemY;
        child.finalWidth = itemW;
        child.finalHeight = itemH;
    }

    ComputedGridLayout layout;
    layout.containerWidth = resolvedW;
    layout.containerHeight = resolvedH;
    layout.children = std::move(placedChildren);
    return layout;
}

}  // namespace

GridMeasureResult GridPainter::Measure(const FontEngine& fonts,
                                      Grid& grid,
                                      float containerWidth,
                                      float pixelScale,
                                      float forcedWidth,
                                      float forcedHeight) {
    const float scale = std::max(0.5f, pixelScale);
    if (grid.HasLayoutCache(containerWidth, scale, forcedWidth, forcedHeight)) {
        GridMeasureResult cached;
        cached.width = grid.LayoutCache().containerWidth;
        cached.height = grid.LayoutCache().containerHeight;
        return cached;
    }
    ComputedGridLayout layout =
        ComputeLayout(fonts, grid, containerWidth, pixelScale, forcedWidth, forcedHeight);
    GridMeasureResult result;
    result.width = layout.containerWidth;
    result.height = layout.containerHeight;
    grid.StoreLayoutCache(containerWidth, scale, forcedWidth, forcedHeight, std::move(layout));
    return result;
}

GridPaintResult GridPainter::Paint(FrameBuffer& buffer,
                                  const FontEngine& fonts,
                                  Grid& grid,
                                  float originX,
                                  float originY,
                                  float containerWidth,
                                  float pixelScale,
                                  float forcedWidth,
                                  float forcedHeight) {
    const SelectableScope selection(&grid, grid.GetStyle().selectable);
    const float scale = std::max(0.5f, pixelScale);
    GridStyle style = grid.GetStyle();
    Resolve(style);

    const ComputedGridLayout* layoutPtr = nullptr;
    ComputedGridLayout layoutOwned;
    if (grid.HasLayoutCache(containerWidth, scale, forcedWidth, forcedHeight)) {
        layoutPtr = &grid.LayoutCache();
    } else {
        layoutOwned = ComputeLayout(fonts, grid, containerWidth, scale, forcedWidth, forcedHeight);
        grid.StoreLayoutCache(containerWidth, scale, forcedWidth, forcedHeight, std::move(layoutOwned));
        layoutPtr = &grid.LayoutCache();
    }
    const ComputedGridLayout& layout = *layoutPtr;
    grid.SetNeedsPaint(false);

    const float containerX = originX;
    const float containerY = originY;
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

    std::vector<GridItem>& items = grid.GetItems();

    for (const PlacedGridChild& child : layout.children) {
        GridItem& item = items[child.itemIndex];
        const float childX = containerX + child.finalX;
        const float childY = containerY + child.finalY;
        const float childW = child.finalWidth;
        const float childH = child.finalHeight;

        switch (item.kind) {
            case GridItemKind::Text: {
                TextPainter::Paint(buffer, fonts, *item.text, childX, childY, childW, scale);
                break;
            }
            case GridItemKind::Svg: {
                SvgPainter::Paint(buffer, *item.svg, childX, childY, childW, scale);
                break;
            }
            case GridItemKind::BrandLogo: {
                BrandLogoPainter::Paint(buffer, fonts, *item.brandLogo, childX, childY, scale);
                break;
            }
            case GridItemKind::TitleBar: {
                TitleBarPainter::Paint(buffer, fonts, *item.titleBar, childX, childY, childW, scale);
                break;
            }
            case GridItemKind::Sidebar: {
                SidebarPainter::Paint(buffer, fonts, *item.sidebar, childX, childY, childH, scale, 1.0f);
                break;
            }
            case GridItemKind::ThemeToggle: {
                ThemeTogglePainter::Paint(buffer, fonts, *item.themeToggle, childX, childY, childW, scale);
                break;
            }
            case GridItemKind::ThemeIconToggle: {
                ThemeIconTogglePainter::Paint(buffer, *item.themeIconToggle, childX, childY, scale);
                break;
            }
            case GridItemKind::SidebarItem: {
                SidebarPainter::PaintItem(buffer, fonts, *item.sidebarItem, childX, childY, childW, childH, scale, false);
                break;
            }
            case GridItemKind::Separator: {
                SeparatorPainter::Paint(buffer, fonts, *item.separator, childX, childY, childW, childH, scale);
                break;
            }
            case GridItemKind::MotionShape: {
                item.motionShape->Paint(buffer, childX, childY, childW, childH, scale);
                break;
            }
            case GridItemKind::Button: {
                ButtonPainter::Paint(buffer,
                                     fonts,
                                     *item.button,
                                     childX,
                                     childY,
                                     childW,
                                     scale,
                                     childW,
                                     childH);
                break;
            }
            case GridItemKind::Input: {
                InputPainter::Paint(buffer,
                                    fonts,
                                    *item.input,
                                    childX,
                                    childY,
                                    childW,
                                    scale,
                                    childW,
                                    childH);
                break;
            }
            case GridItemKind::Scrollbar: {
                const bool vertical =
                    item.scrollbar->GetOrientation() == ScrollbarOrientation::Vertical;
                ScrollbarPainter::Paint(buffer,
                                        *item.scrollbar,
                                        childX,
                                        childY,
                                        childW,
                                        scale,
                                        vertical ? -1.0f : childW,
                                        vertical ? childH : -1.0f);
                break;
            }
            case GridItemKind::Container: {
                if (item.container) {
                    const float cw = (PropHasWidth(item.props.hasWidthLength, item.props.widthLength, item.props.width) && item.props.width != "auto") ? childW : -1.0f;
                    const float ch = (PropHasHeight(item.props.hasHeightLength, item.props.heightLength, item.props.height) && item.props.height != "auto") ? childH : -1.0f;
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
            case GridItemKind::Flexbox: {
                if (item.flexbox) {
                    const float fw = (PropHasWidth(item.props.hasWidthLength, item.props.widthLength, item.props.width) && item.props.width != "auto") ? childW : -1.0f;
                    const float fh = (PropHasHeight(item.props.hasHeightLength, item.props.heightLength, item.props.height) && item.props.height != "auto") ? childH : -1.0f;
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
            case GridItemKind::Grid: {
                if (item.grid) {
                    const float gw = (PropHasWidth(item.props.hasWidthLength, item.props.widthLength, item.props.width) && item.props.width != "auto") ? childW : -1.0f;
                    const float gh = (PropHasHeight(item.props.hasHeightLength, item.props.heightLength, item.props.height) && item.props.height != "auto") ? childH : -1.0f;
                    GridPainter::Paint(buffer,
                                       fonts,
                                       *item.grid,
                                       childX,
                                       childY,
                                       childW,
                                       scale,
                                       gw,
                                       gh);
                }
                break;
            }
        }
    }

    GridBounds bounds;
    bounds.x = containerX;
    bounds.y = containerY;
    bounds.width = containerW;
    bounds.height = containerH;
    grid.SetBounds(bounds);

    GridPaintResult result;
    result.bounds = bounds;
    result.advanceY = containerH + (style.marginBottom + 16.0f) * scale;
    return result;
}

}  // namespace Robo
