#include "Robo/SidebarPainter.hpp"

#include "Robo/Foundation.hpp"
#include "Robo/ScrollbarPainter.hpp"
#include "Robo/Svg.hpp"
#include "Robo/SvgPainter.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace Robo {

namespace {

std::string LabelFirstLine(const std::string& label) {
    std::string out;
    out.reserve(std::min<std::size_t>(label.size(), 64u));
    for (char ch : label) {
        if (ch == '\n' || ch == '\r') {
            break;
        }
        out.push_back(ch);
        if (out.size() >= 64u) {
            break;
        }
    }
    return out;
}

const float ROW_HEIGHT = 36.0f;
const float SECTION_HEIGHT = 40.0f;
const float ICON_SIZE = 20.0f;
const float ROW_PAD = 8.0f;
const float LABEL_GAP = 10.0f;
const float LABEL_SIZE = 14.0f;
const float SECTION_SIZE = 12.0f;

float LabelFade(float openAmount) {
    return std::max(0.0f, std::min(1.0f, (openAmount - 0.08f) / 0.42f));
}

float SegmentDistance(float px, float py, float ax, float ay, float bx, float by) {
    const float vx = bx - ax;
    const float vy = by - ay;
    const float lengthSq = vx * vx + vy * vy;
    float t = 0.0f;
    if (lengthSq > 0.0001f) {
        t = std::max(0.0f, std::min(1.0f, ((px - ax) * vx + (py - ay) * vy) / lengthSq));
    }
    const float dx = px - (ax + vx * t);
    const float dy = py - (ay + vy * t);
    return std::sqrt(dx * dx + dy * dy);
}

void PaintStroke(FrameBuffer& buffer, float x0, float y0, float x1, float y1, float thickness, const Color& color) {
    const float radius = std::max(0.5f, thickness * 0.5f);
    const int left = static_cast<int>(std::floor(std::min(x0, x1) - radius));
    const int top = static_cast<int>(std::floor(std::min(y0, y1) - radius));
    const int right = static_cast<int>(std::ceil(std::max(x0, x1) + radius));
    const int bottom = static_cast<int>(std::ceil(std::max(y0, y1) + radius));
    for (int py = top; py < bottom; ++py) {
        for (int px = left; px < right; ++px) {
            const float distance = SegmentDistance(static_cast<float>(px) + 0.5f, static_cast<float>(py) + 0.5f, x0, y0, x1, y1);
            if (distance >= radius) {
                continue;
            }
            const float coverage = std::min(1.0f, radius - distance);
            buffer.BlendPixel(px, py, color, coverage);
        }
    }
}

void PaintRailChevron(FrameBuffer& buffer, float centerX, float centerY, float size, bool up, const Color& color) {
    const float half = size * 0.42f;
    const float rise = size * 0.28f;
    const float yOuter = up ? centerY + rise : centerY - rise;
    const float yInner = up ? centerY - rise : centerY + rise;
    const float thickness = std::max(1.5f, size * 0.16f);
    PaintStroke(buffer, centerX - half, yOuter, centerX, yInner, thickness, color);
    PaintStroke(buffer, centerX, yInner, centerX + half, yOuter, thickness, color);
}

void PaintRailArrowButton(FrameBuffer& buffer,
                          Sidebar& sidebar,
                          float x,
                          float y,
                          float width,
                          float height,
                          float scale,
                          bool up) {
    const SidebarRailArrow arrow = up ? SidebarRailArrow::Up : SidebarRailArrow::Down;
    const bool pressed = sidebar.PressedRailArrow() == arrow;
    const bool hot = sidebar.HotRailArrow() == arrow;
    const Color hairline = ResolveColor(Color::FromHex("#E5E5E7"), "color.border.subtle");
    Color fill = ResolveColor(Color::FromHex("#F5F5F7"), "color.bg.secondary");
    if (pressed) {
        fill = ResolveColor(Color::FromHex("#EDEDEF"), "color.bg.tertiary");
        fill.red = static_cast<std::uint8_t>(fill.red * 0.92f);
        fill.green = static_cast<std::uint8_t>(fill.green * 0.92f);
        fill.blue = static_cast<std::uint8_t>(fill.blue * 0.92f);
    } else if (hot) {
        fill = ResolveColor(Color::FromHex("#EDEDEF"), "color.bg.tertiary");
    }
    const int line = std::max(1, static_cast<int>(std::round(scale)));
    const int left = static_cast<int>(std::floor(x));
    const int top = static_cast<int>(std::floor(y));
    const int boxW = std::max(1, static_cast<int>(std::ceil(width)));
    const int boxH = std::max(1, static_cast<int>(std::ceil(height)));
    buffer.FillRect(left, top, boxW, boxH, hairline, 1.0f);
    buffer.FillRect(left + line, top + line, std::max(1, boxW - line * 2), std::max(1, boxH - line * 2), fill, 1.0f);
    const Color ink = ResolveColor(Color::FromHex("#6E6E73"), "color.text.secondary");
    PaintRailChevron(buffer, x + width * 0.5f, y + height * 0.5f, 10.0f * scale, up, ink);
}

void PaintFocusBorder(FrameBuffer& buffer, float x, float y, float width, float height, float radius, const Color& color, float thickness) {
    Svg svg;
    SvgStyle style;
    style.viewWidth = width;
    style.viewHeight = height;
    style.width = width;
    style.height = height;
    style.color = color;
    style.opacity = 1.0f;
    svg.SetStyle(style);
    const float inset = thickness * 0.5f;
    const float right = width - inset;
    const float bottom = height - inset;
    const float rad = std::max(0.0f, radius - inset);
    const float curve = rad * 0.55228475f;
    SvgShape shape;
    shape.hasFill = false;
    shape.hasStroke = true;
    shape.currentStroke = true;
    shape.strokeWidth = thickness;
    shape.commands.push_back(SvgCommand::Move(inset + rad, inset));
    shape.commands.push_back(SvgCommand::Line(right - rad, inset));
    shape.commands.push_back(SvgCommand::Cubic(right - rad + curve, inset, right, inset + rad - curve, right, inset + rad));
    shape.commands.push_back(SvgCommand::Line(right, bottom - rad));
    shape.commands.push_back(SvgCommand::Cubic(right, bottom - rad + curve, right - rad + curve, bottom, right - rad, bottom));
    shape.commands.push_back(SvgCommand::Line(inset + rad, bottom));
    shape.commands.push_back(SvgCommand::Cubic(inset + rad - curve, bottom, inset, bottom - rad + curve, inset, bottom - rad));
    shape.commands.push_back(SvgCommand::Line(inset, inset + rad));
    shape.commands.push_back(SvgCommand::Cubic(inset, inset + rad - curve, inset + rad - curve, inset, inset + rad, inset));
    shape.commands.push_back(SvgCommand::Close());
    svg.AddShape(shape);
    SvgPainter::Paint(buffer, svg, x, y, width, 1.0f);
}

void PaintIcon(FrameBuffer& buffer, SidebarItem& item, float x, float y, float size, const Color& ink, float opacity, float scale) {
    if (!item.HasIcon()) {
        return;
    }
    Svg& icon = item.Icon();
    icon.GetStyle().width = ICON_SIZE;
    icon.GetStyle().height = ICON_SIZE;
    icon.GetStyle().widthSpec.clear();
    icon.GetStyle().heightSpec.clear();
    icon.GetStyle().color = ink;
    icon.GetStyle().opacity = opacity;
    SvgPainter::Paint(buffer, icon, x, y, size, scale);
}

void PaintRow(FrameBuffer& buffer,
              const FontEngine& fonts,
              SidebarItem& item,
              float x,
              float y,
              float width,
              float height,
              float scale,
              bool selected,
              float labelOpacity) {
    SidebarItemBounds bounds;
    bounds.x = x;
    bounds.y = y;
    bounds.width = width;
    bounds.height = height;
    item.SetBounds(bounds);
    if (buffer.MissesClip(static_cast<int>(x), static_cast<int>(y), static_cast<int>(width), static_cast<int>(height))) {
        return;
    }
    const bool enabled = item.CanActivate();
    const Color accent = ResolveColor(Color::FromHex("#0E7AFF"), "color.brand.primary");
    const Color quiet = ResolveColor(Color::FromHex("#6E6E73"), "color.text.secondary");
    const Color disabledInk = ResolveColor(Color::FromHex("#C7C7CC"), "color.text.disabled");
    const Color ink = !enabled ? disabledInk : (selected ? accent : quiet);
    const float opacity = item.IsLoading() ? 0.45f : 1.0f;
    const float inset = 6.0f * scale;
    const float pillX = x + inset;
    const float pillY = y + 2.0f * scale;
    const float pillW = std::max(1.0f, width - inset * 2.0f);
    const float pillH = std::max(1.0f, height - 4.0f * scale);
    const float pillR = 8.0f * scale;
    if (item.IsFocusVisible() && enabled) {
        const float ring = 2.0f * scale;
        const Color focus = ResolveColor(Color::FromHex("#0E7AFF"), "color.brand.primary");
        PaintFocusBorder(buffer, pillX - ring, pillY - ring, pillW + ring * 2.0f, pillH + ring * 2.0f, pillR + ring, focus, ring);
    }
    if (enabled) {
        const SidebarItemInteraction interaction = item.GetInteraction();
        if (interaction == SidebarItemInteraction::Pressed) {
            if (selected) {
                buffer.FillRoundedRect(pillX, pillY, pillW, pillH, pillR, accent, 0.30f);
            } else {
                Color fill = ResolveColor(Color::FromHex("#EDEDEF"), "color.bg.tertiary");
                fill.red = static_cast<std::uint8_t>(fill.red * 0.92f);
                fill.green = static_cast<std::uint8_t>(fill.green * 0.92f);
                fill.blue = static_cast<std::uint8_t>(fill.blue * 0.92f);
                buffer.FillRoundedRect(pillX, pillY, pillW, pillH, pillR, fill, 1.0f);
            }
        } else if (interaction == SidebarItemInteraction::Hovered) {
            if (selected) {
                buffer.FillRoundedRect(pillX, pillY, pillW, pillH, pillR, accent, 0.22f);
            } else {
                const Color fill = ResolveColor(Color::FromHex("#EDEDEF"), "color.bg.tertiary");
                buffer.FillRoundedRect(pillX, pillY, pillW, pillH, pillR, fill, 1.0f);
            }
        } else if (selected) {
            buffer.FillRoundedRect(pillX, pillY, pillW, pillH, pillR, accent, 0.14f);
        }
    }
    const float icon = ICON_SIZE * scale;
    const float rail = SIDEBAR_RAIL * scale;
    const float iconX = width + 0.5f >= rail ? x + (rail - icon) * 0.5f : x + std::max(0.0f, (width - icon) * 0.5f);
    const float iconY = y + std::max(0.0f, (height - icon) * 0.5f);
    PaintIcon(buffer, item, iconX, iconY, icon, ink, opacity, scale);
    if (labelOpacity <= 0.01f || item.GetLabel().empty()) {
        return;
    }
    const float fontSize = LABEL_SIZE * scale;
    const float ascent = fonts.GetAscent(fontSize, FontFaceId::Medium);
    const float descent = fonts.GetDescent(fontSize, FontFaceId::Medium);
    const float baseline = y + (height - fontSize) * 0.5f + (fontSize - (ascent + descent)) * 0.5f + ascent;
    const float textX = iconX + icon + LABEL_GAP * scale;
    buffer.PushClipRect(textX, y, std::max(1.0f, x + width - textX - 8.0f * scale), height);
    fonts.DrawString(buffer, textX, baseline, LabelFirstLine(item.GetLabel()), fontSize, FontFaceId::Medium, ink, labelOpacity * opacity, -0.01f * fontSize, true);
    buffer.PopClip();
}

void PaintSection(FrameBuffer& buffer,
                  const FontEngine& fonts,
                  SidebarItem& item,
                  float x,
                  float y,
                  float width,
                  float height,
                  float scale,
                  float opacity,
                  bool drawRule) {
    SidebarItemBounds bounds;
    bounds.x = x;
    bounds.y = y;
    bounds.width = width;
    bounds.height = height;
    item.SetBounds(bounds);
    if (height < 1.0f || opacity <= 0.01f) {
        return;
    }
    buffer.PushClipRect(x, y, std::max(1.0f, width), height);
    if (drawRule) {
        const Color hairline = ResolveColor(Color::FromHex("#E5E5E7"), "color.border.subtle");
        const int line = std::max(1, static_cast<int>(std::round(scale)));
        buffer.FillRect(static_cast<int>(std::floor(x + 12.0f * scale)),
                        static_cast<int>(std::floor(y + 6.0f * scale)),
                        std::max(1, static_cast<int>(std::ceil(width - 24.0f * scale))),
                        line,
                        hairline,
                        opacity);
    }
    if (!item.GetLabel().empty()) {
        const float fontSize = SECTION_SIZE * scale;
        const float ascent = fonts.GetAscent(fontSize, FontFaceId::Medium);
        const Color title = ResolveColor(Color::FromHex("#6E6E73"), "color.text.secondary");
        fonts.DrawString(buffer, x + 16.0f * scale, y + 24.0f * scale + ascent * 0.15f, LabelFirstLine(item.GetLabel()), fontSize, FontFaceId::Medium, title, opacity, 0.02f * fontSize, true);
    }
    buffer.PopClip();
}

}  // namespace

SidebarMeasureResult SidebarPainter::Measure(const Sidebar& sidebar, float availableHeight, float pixelScale, float openAmount) {
    const float scale = std::max(0.5f, pixelScale);
    SidebarMeasureResult result;
    result.width = sidebar.LayoutWidth(openAmount) * scale;
    result.height = std::max(0.0f, availableHeight);
    return result;
}

SidebarItemMeasureResult SidebarPainter::MeasureItem(const FontEngine& fonts, const SidebarItem& item, float pixelScale) {
    const float scale = std::max(0.5f, pixelScale);
    SidebarItemMeasureResult result;
    std::string label;
    label.reserve(std::min<std::size_t>(item.GetLabel().size(), 64u));
    for (char ch : item.GetLabel()) {
        if (ch == '\n' || ch == '\r') {
            break;
        }
        label.push_back(ch);
        if (label.size() >= 64u) {
            break;
        }
    }
    if (item.IsSection()) {
        const float text = label.empty() ? 0.0f
                                         : fonts.MeasureWidth(label, SECTION_SIZE * scale, FontFaceId::Medium,
                                                              0.02f * SECTION_SIZE * scale);
        result.width = 16.0f * scale + text + 16.0f * scale;
        result.height = SECTION_HEIGHT * scale;
        return result;
    }
    const float text = label.empty() ? 0.0f
                                     : fonts.MeasureWidth(label, LABEL_SIZE * scale, FontFaceId::Medium,
                                                          -0.01f * LABEL_SIZE * scale);
    result.width = 14.0f * scale + ICON_SIZE * scale + (text > 0.0f ? LABEL_GAP * scale + text + 12.0f * scale : 14.0f * scale);
    result.height = ROW_HEIGHT * scale;
    return result;
}

void SidebarPainter::PaintItem(FrameBuffer& buffer,
                               const FontEngine& fonts,
                               SidebarItem& item,
                               float originX,
                               float originY,
                               float width,
                               float height,
                               float pixelScale,
                               bool selected) {
    const float scale = std::max(0.5f, pixelScale);
    const float rowHeight = height > 0.0f ? height : (item.IsSection() ? SECTION_HEIGHT : ROW_HEIGHT) * scale;
    if (item.IsSection()) {
        PaintSection(buffer, fonts, item, originX, originY, width, rowHeight, scale, 1.0f, false);
        return;
    }
    const bool showLabel = width > (SIDEBAR_RAIL + 24.0f) * scale;
    PaintRow(buffer, fonts, item, originX, originY, width, rowHeight, scale, selected, showLabel ? 1.0f : 0.0f);
}

void SidebarPainter::Paint(FrameBuffer& buffer,
                           const FontEngine& fonts,
                           Sidebar& sidebar,
                           float originX,
                           float originY,
                           float height,
                           float pixelScale,
                           float openAmount) {
    const float scale = std::max(0.5f, pixelScale);
    const float width = std::max(1.0f, sidebar.LayoutWidth(openAmount) * scale);
    const int paintHeight = std::max(1, static_cast<int>(std::ceil(height)));
    const Color background = ResolveColor(Color::FromHex("#F5F5F7"), "color.bg.secondary");
    const Color hairline = ResolveColor(Color::FromHex("#E5E5E7"), "color.border.subtle");
    const int left = static_cast<int>(std::floor(originX));
    buffer.FillRect(left, static_cast<int>(std::floor(originY)), std::max(1, static_cast<int>(std::ceil(width))), paintHeight, background, 1.0f);
    const int line = std::max(1, static_cast<int>(std::round(scale)));
    buffer.FillRect(left + std::max(1, static_cast<int>(std::ceil(width))) - line,
                    static_cast<int>(std::floor(originY)),
                    line,
                    paintHeight,
                    hairline,
                    1.0f);

    const float row = ROW_HEIGHT * scale;
    const float rail = SIDEBAR_RAIL * scale;
    const bool activity = sidebar.GetVariant() == SidebarVariant::Activity && !sidebar.Items().empty();
    const float labelOpacity = activity ? 0.0f : LabelFade(openAmount);
    std::vector<SidebarItem>& items = sidebar.Items();
    float railContent = ROW_PAD * scale;
    if (activity) {
        for (const SidebarItem& item : items) {
            if (!item.IsSection()) {
                railContent += row;
            }
        }
        railContent += ROW_PAD * scale;
        sidebar.SetRail(rail, railContent, height, 96.0f * scale);
        buffer.PushClipRect(originX, originY, rail, height);
    } else {
        sidebar.SetRailArrows(SidebarItemBounds{}, SidebarItemBounds{});
    }
    float cursor = originY + ROW_PAD * scale - (activity ? sidebar.RailScroll() : 0.0f);
    for (int i = 0; i < static_cast<int>(items.size()); ++i) {
        SidebarItem& item = items[static_cast<std::size_t>(i)];
        if (item.IsSection()) {
            const float sectionHeight = SECTION_HEIGHT * scale * (activity ? 0.0f : openAmount);
            const bool drawRule = i > 0;
            PaintSection(buffer, fonts, item, originX, cursor, width, sectionHeight, scale, labelOpacity, drawRule);
            cursor += sectionHeight;
            continue;
        }
        const float rowWidth = activity ? rail : width;
        PaintRow(buffer, fonts, item, originX, cursor, rowWidth, row, scale, i == sidebar.Selected() || item.IsSelected(), labelOpacity);
        cursor += row;
    }
    if (activity) {
        buffer.PopClip();
        const float buttonH = 22.0f * scale;
        SidebarItemBounds up{};
        SidebarItemBounds down{};
        if (sidebar.CanScrollRailUp()) {
            up.x = originX;
            up.y = originY;
            up.width = rail;
            up.height = buttonH;
            PaintRailArrowButton(buffer, sidebar, up.x, up.y, rail, buttonH, scale, true);
        }
        if (sidebar.CanScrollRailDown()) {
            down.x = originX;
            down.y = originY + height - buttonH;
            down.width = rail;
            down.height = buttonH;
            PaintRailArrowButton(buffer, sidebar, down.x, down.y, rail, buttonH, scale, false);
        }
        sidebar.SetRailArrows(up, down);
    }
    if (!activity || openAmount < 0.08f) {
        for (SidebarItem& item : items) {
            for (SidebarItem& child : item.Children()) {
                child.SetBounds(SidebarItemBounds{});
            }
        }
    }

    if (!activity || openAmount < 0.08f || items.empty()) {
        return;
    }
    const int selected = std::max(0, std::min(sidebar.Selected(), static_cast<int>(items.size()) - 1));
    for (int i = 0; i < static_cast<int>(items.size()); ++i) {
        if (i == selected) {
            continue;
        }
        for (SidebarItem& child : items[static_cast<std::size_t>(i)].Children()) {
            child.SetBounds(SidebarItemBounds{});
        }
    }
    SidebarItem& current = items[static_cast<std::size_t>(selected)];
    const float panelX = originX + rail;
    const float panelW = std::max(0.0f, width - rail);
    buffer.FillRect(static_cast<int>(std::floor(panelX)),
                    static_cast<int>(std::floor(originY)),
                    line,
                    paintHeight,
                    hairline,
                    1.0f);
    if (!current.GetLabel().empty()) {
        const float fontSize = 13.0f * scale;
        const float ascent = fonts.GetAscent(fontSize, FontFaceId::SemiBold);
        const Color title = ResolveColor(Color::FromHex("#6E6E73"), "color.text.secondary");
        fonts.DrawString(buffer, panelX + 16.0f * scale, originY + 22.0f * scale + ascent * 0.2f,
                         LabelFirstLine(current.GetLabel()), fontSize, FontFaceId::SemiBold, title, 1.0f,
                         0.04f * fontSize, true);
    }
    const float header = 44.0f * scale;
    float content = 0.0f;
    for (const SidebarItem& child : current.Children()) {
        content += child.IsSection() ? SECTION_HEIGHT * scale : row;
    }
    const float viewHeight = std::max(0.0f, height - header);
    sidebar.SetPanel(originX, originY, width, height, originY + header, viewHeight, content);
    float rowY = originY + header - sidebar.Scroll();
    const float clipTop = originY + header;
    const float clipBottom = clipTop + viewHeight;
    buffer.PushClipRect(panelX, clipTop, std::max(1.0f, panelW), viewHeight);
    bool firstRow = true;
    for (SidebarItem& child : current.Children()) {
        const float rowH = child.IsSection() ? SECTION_HEIGHT * scale : row;
        if (rowY + rowH >= clipTop && rowY <= clipBottom) {
            if (child.IsSection()) {
                PaintSection(buffer, fonts, child, panelX, rowY, panelW, rowH, scale, 1.0f, !firstRow);
            } else {
                PaintRow(buffer, fonts, child, panelX, rowY, panelW, rowH, scale, child.IsSelected(), 1.0f);
            }
        } else {
            child.SetBounds(SidebarItemBounds{});
        }
        rowY += rowH;
        firstRow = false;
    }
    buffer.PopClip();
    Scrollbar& bar = sidebar.PanelScrollbar();
    bar.SetContentSize(static_cast<double>(content));
    bar.SetViewportSize(static_cast<double>(viewHeight));
    if (content > viewHeight + 1.0f) {
        const float thickness = static_cast<float>(bar.GetStyle().thickness) * scale;
        ScrollbarPainter::Paint(buffer,
                                bar,
                                panelX + panelW - thickness,
                                originY + header,
                                thickness,
                                scale,
                                thickness,
                                viewHeight);
    } else {
        bar.SetBounds({});
    }
}

}  // namespace Robo
