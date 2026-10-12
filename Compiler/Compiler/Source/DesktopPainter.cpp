#include "Robo/DesktopPainter.hpp"

#include "Robo/Foundation.hpp"
#include "Robo/SvgPainter.hpp"

#include <algorithm>
#include <string>

namespace Robo {

namespace {

constexpr float SHELL_HEIGHT = 48.0f;
constexpr float SHELL_TITLE = 15.0f;
constexpr float DOCK_ITEM_WIDTH = 72.0f;
constexpr float DOCK_ITEM_HEIGHT = 64.0f;
constexpr float DOCK_GAP = 8.0f;
constexpr float DOCK_PAD = 10.0f;
constexpr float DOCK_MARGIN = 12.0f;
constexpr float DOCK_RADIUS = 18.0f;
constexpr float ITEM_RADIUS = 14.0f;

std::string DockMark(const std::string& label) {
    if (label.empty()) {
        return "?";
    }
    const unsigned char lead = static_cast<unsigned char>(label[0]);
    if (lead < 128) {
        std::string mark(1, static_cast<char>(lead));
        if (mark[0] >= 'a' && mark[0] <= 'z') {
            mark[0] = static_cast<char>(mark[0] - 'a' + 'A');
        }
        return mark;
    }
    return label.substr(0, 1);
}

void DrawLine(FrameBuffer& buffer,
              const FontEngine& fonts,
              const std::string& text,
              float originX,
              float originY,
              float width,
              float height,
              float fontSize,
              FontFaceId face,
              const Color& color,
              bool center) {
    const float textWidth = fonts.MeasureWidth(text, fontSize, face, 0.0f);
    const float ascent = fonts.GetAscent(fontSize, face);
    const float descent = fonts.GetDescent(fontSize, face);
    const float lineTop = originY + std::max(0.0f, (height - fontSize) * 0.5f);
    const float baseline = lineTop + (fontSize - (ascent + descent)) * 0.5f + ascent;
    const float textX = center ? originX + std::max(0.0f, (width - textWidth) * 0.5f) : originX;
    fonts.DrawString(buffer, textX, baseline, text, fontSize, face, color, 1.0f, 0.0f);
}

}  // namespace

DesktopChrome DesktopPainter::Layout(float surfaceWidth, float surfaceHeight, float pixelScale, bool hasShell, bool hasDock) {
    const float scale = std::max(0.5f, pixelScale);
    DesktopChrome chrome;
    chrome.shellHeight = hasShell ? SHELL_HEIGHT * scale : 0.0f;
    chrome.dockHeight = hasDock ? (DOCK_ITEM_HEIGHT + DOCK_PAD * 2.0f + DOCK_MARGIN) * scale : 0.0f;
    chrome.dockY = std::max(0.0f, surfaceHeight - chrome.dockHeight);
    chrome.contentY = chrome.shellHeight;
    chrome.contentHeight = std::max(1.0f, surfaceHeight - chrome.shellHeight - chrome.dockHeight);
    (void)surfaceWidth;
    return chrome;
}

void DesktopPainter::Paint(FrameBuffer& buffer,
                           const FontEngine& fonts,
                           Desktop& desktop,
                           const DesktopChrome& chrome,
                           float pixelScale,
                           int hoveredIndex) {
    const float scale = std::max(0.5f, pixelScale);
    const Color ink = ResolveColor(Color::FromHex("#1D1D1F"), "color.text.primary");
    const Color quiet = ResolveColor(Color::FromHex("#6E6E73"), "color.text.secondary");
    const Color panel = ResolveColor(Color::FromHex("#F5F5F7"), "color.bg.secondary");
    const Color tile = ResolveColor(Color::FromHex("#FFFFFF"), "color.bg.primary");
    const Color selected = ResolveColor(Color::FromHex("#0E7AFF"), "color.brand.primary");

    if (desktop.HasAppShell()) {
        buffer.FillRect(0, 0, buffer.GetWidth(), static_cast<int>(chrome.shellHeight), panel);
        DrawLine(buffer, fonts, desktop.GetAppShell().GetTitle(), 20.0f * scale, 0.0f,
                 static_cast<float>(buffer.GetWidth()) - 40.0f * scale, chrome.shellHeight, SHELL_TITLE * scale,
                 FontFaceId::SemiBold, ink, false);
    }

    std::vector<DockItem>& items = desktop.Dock();
    if (items.empty()) {
        desktop.SetDockBar(0.0f, 0.0f, 0.0f, 0.0f);
        return;
    }

    const float itemWidth = DOCK_ITEM_WIDTH * scale;
    const float itemHeight = DOCK_ITEM_HEIGHT * scale;
    const float gap = DOCK_GAP * scale;
    const float pad = DOCK_PAD * scale;
    const float barHeight = itemHeight + pad * 2.0f;
    const float barWidth = static_cast<float>(items.size()) * itemWidth
        + static_cast<float>(items.size() - 1) * gap + pad * 2.0f;
    const float barX = std::max(pad, (static_cast<float>(buffer.GetWidth()) - barWidth) * 0.5f);
    const float barY = static_cast<float>(buffer.GetHeight()) - barHeight - DOCK_MARGIN * scale;
    desktop.SetDockBar(barX, barY, barWidth, barHeight);
    buffer.FillRoundedRect(barX, barY, barWidth, barHeight, DOCK_RADIUS * scale, panel, 0.94f);

    for (int index = 0; index < static_cast<int>(items.size()); ++index) {
        DockItem& item = items[static_cast<std::size_t>(index)];
        const float itemX = barX + pad + static_cast<float>(index) * (itemWidth + gap);
        const float itemY = barY + pad;
        DockItemBounds bounds;
        bounds.x = itemX;
        bounds.y = itemY;
        bounds.width = itemWidth;
        bounds.height = itemHeight;
        item.SetBounds(bounds);

        Color fill = tile;
        if (item.IsSelected()) {
            fill = selected;
        } else if (index == hoveredIndex) {
            fill = ResolveColor(Color::FromHex("#E5E5E7"), "color.border.subtle");
        }
        buffer.FillRoundedRect(itemX, itemY, itemWidth, itemHeight, ITEM_RADIUS * scale, fill);

        const Color mark = item.IsSelected() ? Color::FromHex("#FFFFFF") : ink;
        if (item.HasIcon()) {
            Svg& icon = item.Icon();
            SvgStyle style = icon.GetStyle();
            style.width = 22.0f;
            style.height = 22.0f;
            style.color = mark;
            icon.SetStyle(style);
            const float iconSize = 22.0f * scale;
            SvgPainter::Paint(buffer, icon, itemX + (itemWidth - iconSize) * 0.5f, itemY + 8.0f * scale, iconSize, scale);
        } else {
            DrawLine(buffer, fonts, DockMark(item.GetLabel()), itemX, itemY + 6.0f * scale, itemWidth, 28.0f * scale,
                     16.0f * scale, FontFaceId::SemiBold, mark, true);
        }
        DrawLine(buffer, fonts, item.GetLabel(), itemX, itemY + 34.0f * scale, itemWidth, 22.0f * scale, 11.0f * scale,
                 FontFaceId::Medium, item.IsSelected() ? Color::FromHex("#FFFFFF") : quiet, true);
    }
}

}  // namespace Robo
