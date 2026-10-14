#include "Robo/ContextMenuPainter.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace Robo {

namespace {

void DrawChevron(FrameBuffer& buffer, float originX, float originY, float size, const Color& color, float opacity) {
    const int steps = std::max(3, static_cast<int>(std::round(size)));
    for (int step = 0; step <= steps; ++step) {
        const float t = static_cast<float>(step) / static_cast<float>(steps);
        const float x = originX + t * size;
        const float upper = originY + t * size * 0.5f;
        const float lower = originY + size - t * size * 0.5f;
        buffer.FillRect(static_cast<int>(std::round(x)), static_cast<int>(std::round(upper)), 2, 2, color, opacity);
        buffer.FillRect(static_cast<int>(std::round(x)), static_cast<int>(std::round(lower)) - 1, 2, 2, color, opacity);
    }
}

void DrawCheck(FrameBuffer& buffer, float x, float y, float size, const Color& color, float opacity) {
    const int left = static_cast<int>(std::round(x));
    const int top = static_cast<int>(std::round(y + size * 0.45f));
    buffer.FillRect(left, top, std::max(1, static_cast<int>(size * 0.35f)), 2, color, opacity);
    buffer.FillRect(left + static_cast<int>(size * 0.28f), top - static_cast<int>(size * 0.28f), 2,
                    std::max(2, static_cast<int>(size * 0.55f)), color, opacity);
}

void DrawLabel(FrameBuffer& buffer,
               const FontEngine& fonts,
               const std::string& text,
               float x,
               float y,
               float width,
               float height,
               float fontSize,
               const Color& color,
               float opacity) {
    const float textWidth = fonts.MeasureWidth(text, fontSize, FontFaceId::Regular, 0.0f);
    const float ascent = fonts.GetAscent(fontSize, FontFaceId::Regular);
    const float descent = fonts.GetDescent(fontSize, FontFaceId::Regular);
    const float baseline = y + (height - (ascent + descent)) * 0.5f + ascent;
    buffer.PushClipRect(x, y, width, height);
    fonts.DrawString(buffer, x, baseline, text, fontSize, FontFaceId::Regular, color, opacity, 0.0f);
    buffer.PopClip();
    (void)textWidth;
}

void PaintPanel(FrameBuffer& buffer,
                const FontEngine& fonts,
                std::vector<ContextMenuItem>& items,
                const ContextMenuBounds& panel,
                float scale,
                bool dark,
                float cover,
                float amount,
                int hover,
                int pressed,
                int focus) {
    if (panel.width <= 1.0f || panel.height <= 1.0f || amount <= 0.0f) {
        return;
    }
    const float radius = 13.0f * scale;
    const float rowRadius = 6.0f * scale;
    const float fontSize = 13.0f * scale;
    const float pad = 10.0f * scale;
    const Color surface = dark ? Color::FromRgba(35, 36, 42, 255) : Color::FromRgba(248, 249, 252, 255);
    const Color ink = dark ? Color::FromHex("#F5F5F7") : Color::FromHex("#202126");
    const Color highlight = dark ? Color::FromHex("#2A4A72") : Color::FromHex("#D5E7FF");
    const Color pressedFill = dark ? Color::FromHex("#1A3558") : Color::FromHex("#B7D4FF");
    const Color highlightInk = dark ? Color::FromHex("#F5F5F7") : Color::FromHex("#172A44");
    const Color border = dark ? Color::FromRgba(255, 255, 255, 255) : Color::FromRgba(80, 85, 98, 255);
    const Color separator = border;
    const float surfaceAlpha = (dark ? 0.82f : 0.84f) * cover * amount;
    const float borderAlpha = (dark ? 0.16f : 0.20f) * amount * std::max(0.35f, cover);
    const float separatorAlpha = (dark ? 0.12f : 0.18f) * amount * std::max(0.35f, cover);
    const float shadow = 0.16f * amount;

    buffer.FillRoundedRect(panel.x, panel.y + 10.0f * scale, panel.width, panel.height, radius, Color::FromHex("#000000"), shadow * 0.55f);
    buffer.FillRoundedRect(panel.x + 1.0f, panel.y + 2.0f, panel.width, panel.height, radius, Color::FromHex("#000000"), shadow * 0.35f);
    buffer.FillRoundedRect(panel.x, panel.y, panel.width, panel.height, radius, border, borderAlpha);
    buffer.FillRoundedRect(panel.x + 1.0f, panel.y + 1.0f, panel.width - 2.0f, panel.height - 2.0f, std::max(1.0f, radius - 1.0f), surface, surfaceAlpha);
    buffer.FillRect(static_cast<int>(std::round(panel.x + radius)), static_cast<int>(std::round(panel.y + 1.0f)),
                    std::max(1, static_cast<int>(panel.width - radius * 2.0f)), 1,
                    Color::FromHex("#FFFFFF"), (dark ? 0.18f : 0.55f) * amount);

    for (int index = 0; index < static_cast<int>(items.size()); ++index) {
        ContextMenuItem& item = items[static_cast<std::size_t>(index)];
        const ContextMenuBounds& bounds = item.GetBounds();
        if (bounds.width <= 0.0f || bounds.height <= 0.0f) {
            continue;
        }
        if (item.IsSeparator()) {
            const float inset = 4.0f * scale;
            const int lineY = static_cast<int>(std::round(bounds.y + bounds.height * 0.5f));
            buffer.FillRect(static_cast<int>(std::round(bounds.x + inset)), lineY,
                            std::max(1, static_cast<int>(bounds.width - inset * 2.0f)), 1, separator, separatorAlpha);
            continue;
        }
        const bool hot = hover == index || focus == index;
        const bool down = pressed == index && hot;
        Color text = item.IsDisabled() ? ink : (hot ? highlightInk : ink);
        float textOpacity = amount * (item.IsDisabled() ? 0.45f : 1.0f);
        if (hot && !item.IsDisabled()) {
            buffer.FillRoundedRect(bounds.x, bounds.y, bounds.width, bounds.height, rowRadius, down ? pressedFill : highlight, amount);
        }
        const float mark = item.HasSubmenu() ? 14.0f * scale : (item.IsCheckable() ? 16.0f * scale : 0.0f);
        DrawLabel(buffer, fonts, item.GetLabel(), bounds.x + pad, bounds.y, std::max(1.0f, bounds.width - pad * 2.0f - mark),
                  bounds.height, fontSize, text, textOpacity);
        if (item.IsChecked()) {
            DrawCheck(buffer, bounds.x + bounds.width - pad - 12.0f * scale, bounds.y + (bounds.height - 10.0f * scale) * 0.5f,
                      10.0f * scale, text, textOpacity);
        }
        if (item.HasSubmenu()) {
            const float chevron = 7.0f * scale;
            DrawChevron(buffer, bounds.x + bounds.width - pad - chevron, bounds.y + (bounds.height - chevron) * 0.5f, chevron, text, textOpacity);
        }
    }
}

}  // namespace

void ContextMenuPainter::Paint(FrameBuffer& buffer,
                               const FontEngine& fonts,
                               ContextMenu& menu,
                               float pixelScale,
                               float viewWidth,
                               float viewHeight,
                               float topInset,
                               float bottomInset) {
    if (!menu.IsVisible()) {
        return;
    }
    const float scale = std::max(0.5f, pixelScale);
    menu.Layout(fonts, scale, viewWidth, viewHeight, topInset, bottomInset);
    const ContextMenuBounds damage = menu.DamageBounds();
    if (buffer.MissesClip(static_cast<int>(std::floor(damage.x)) - 28,
                          static_cast<int>(std::floor(damage.y)) - 8,
                          static_cast<int>(std::ceil(damage.width)) + 56,
                          static_cast<int>(std::ceil(damage.height)) + 48)) {
        return;
    }
    const float cover = 1.0f - menu.GetTransparency();
    const float amount = std::max(0.0f, std::min(1.0f, menu.OpenAmount()));
    const int hover = menu.HoverChild() < 0 ? menu.Hover() : -1;
    const int focus = menu.FocusChild() < 0 ? menu.Focus() : -1;
    const int pressed = menu.PressedChild() < 0 ? menu.Pressed() : -1;
    PaintPanel(buffer, fonts, menu.Items(), menu.Panel(), scale, menu.IsDark(), cover, amount, hover, pressed, focus);
    if (menu.Submenu() >= 0 && menu.Submenu() < static_cast<int>(menu.Items().size())) {
        ContextMenuItem& parent = menu.Items()[static_cast<std::size_t>(menu.Submenu())];
        const int childHover = menu.Hover() == menu.Submenu() ? menu.HoverChild() : -1;
        const int childFocus = menu.Focus() == menu.Submenu() ? menu.FocusChild() : -1;
        const int childPressed = menu.Pressed() == menu.Submenu() ? menu.PressedChild() : -1;
        PaintPanel(buffer, fonts, parent.Items(), menu.SubmenuPanel(), scale, menu.IsDark(), cover, amount,
                   childHover, childPressed, childFocus);
    }
}

}  // namespace Robo
