#include "Robo/ContextMenuPainter.hpp"

#include "Robo/SvgPainter.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace Robo {

namespace {

void PaintIcon(FrameBuffer& buffer, Svg& graphic, float x, float y, float size, const Color& color, float opacity, float scale) {
    graphic.GetStyle().width = size / scale;
    graphic.GetStyle().height = size / scale;
    graphic.GetStyle().widthSpec.clear();
    graphic.GetStyle().heightSpec.clear();
    graphic.GetStyle().color = color;
    graphic.GetStyle().opacity = opacity;
    SvgPainter::Paint(buffer, graphic, x, y, size, scale);
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
    const float textWidth = fonts.MeasureWidth(text, fontSize, FontFaceId::Medium, 0.0f);
    const float ascent = fonts.GetAscent(fontSize, FontFaceId::Medium);
    const float descent = fonts.GetDescent(fontSize, FontFaceId::Medium);
    const float baseline = y + (height - (ascent + descent)) * 0.5f + ascent;
    buffer.PushClipRect(x, y, width, height);
    fonts.DrawString(buffer, x, baseline, text, fontSize, FontFaceId::Medium, color, opacity, 0.0f, true);
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
    const Color surface = dark ? Color::FromHex("#2C2C2E") : Color::FromHex("#F5F5F7");
    const Color ink = dark ? Color::FromHex("#F5F5F7") : Color::FromHex("#1D1D1F");
    const Color shortcutInk = dark ? Color::FromHex("#98989D") : Color::FromHex("#8E8E93");
    const Color highlight = dark ? Color::FromHex("#0A84FF") : Color::FromHex("#007AFF");
    const Color pressedFill = dark ? Color::FromHex("#0866CC") : Color::FromHex("#0063CC");
    const Color highlightInk = Color::FromHex("#FFFFFF");
    const Color border = dark ? Color::FromRgba(255, 255, 255, 255) : Color::FromRgba(80, 85, 98, 255);
    const Color separator = border;
    const float surfaceAlpha = (dark ? 0.92f : 0.94f) * cover * amount;
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

    bool anyIcon = false;
    for (const ContextMenuItem& item : items) {
        if (item.HasIcon()) {
            anyIcon = true;
        }
    }
    const float iconSize = 15.0f * scale;
    const float lead = anyIcon ? iconSize + 8.0f * scale : 0.0f;
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
        const bool hot = (hover == index || focus == index) && !item.IsDisabled();
        const bool down = pressed == index && hot;
        const Color text = hot ? highlightInk : ink;
        const Color trailing = hot ? highlightInk : shortcutInk;
        const float textOpacity = amount * (item.IsDisabled() ? 0.45f : 1.0f);
        if (hot) {
            buffer.FillRoundedRect(bounds.x, bounds.y, bounds.width, bounds.height, rowRadius, down ? pressedFill : highlight, amount);
        }
        float cursor = bounds.x + pad;
        if (anyIcon) {
            if (item.HasIcon()) {
                PaintIcon(buffer, item.Icon(), cursor, bounds.y + (bounds.height - iconSize) * 0.5f, iconSize, text, textOpacity, scale);
            }
            cursor += lead;
        }
        float trailingWidth = 0.0f;
        if (item.HasChevron()) {
            trailingWidth += iconSize;
        } else if (item.IsChecked() && item.HasCheckGraphic()) {
            trailingWidth += iconSize;
        }
        trailingWidth += item.ShortcutWidth(fonts, fontSize, iconSize);
        DrawLabel(buffer, fonts, item.GetLabel(), cursor, bounds.y,
                  std::max(1.0f, bounds.width - (cursor - bounds.x) - pad - trailingWidth),
                  bounds.height, fontSize, text, textOpacity);
        float markX = bounds.x + bounds.width - pad;
        if (item.HasChevron()) {
            markX -= iconSize;
            PaintIcon(buffer, item.Chevron(), markX, bounds.y + (bounds.height - iconSize) * 0.5f, iconSize, trailing, textOpacity, scale);
            markX -= 6.0f * scale;
        } else if (item.IsChecked() && item.HasCheckGraphic()) {
            markX -= iconSize;
            PaintIcon(buffer, item.CheckGraphic(), markX, bounds.y + (bounds.height - iconSize) * 0.5f, iconSize, trailing, textOpacity, scale);
            markX -= 6.0f * scale;
        }
        const std::vector<ContextMenuGlyph>& shortcut = item.Shortcut();
        for (int part = static_cast<int>(shortcut.size()) - 1; part >= 0; --part) {
            const ContextMenuGlyph& glyph = shortcut[static_cast<std::size_t>(part)];
            if (glyph.text) {
                const float textWidth = fonts.MeasureWidth(glyph.label, fontSize, FontFaceId::Medium, 0.0f);
                markX -= textWidth;
                DrawLabel(buffer, fonts, glyph.label, markX, bounds.y, textWidth, bounds.height, fontSize, trailing, textOpacity);
            } else {
                markX -= iconSize;
                Svg graphic = glyph.graphic;
                PaintIcon(buffer, graphic, markX, bounds.y + (bounds.height - iconSize) * 0.5f, iconSize, trailing, textOpacity, scale);
            }
            markX -= 2.0f * scale;
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
