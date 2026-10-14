#include "Robo/SelectionAreaPainter.hpp"

#include <algorithm>
#include <cmath>

namespace Robo {

namespace {

void DrawLabel(FrameBuffer& buffer,
               const FontEngine& fonts,
               const std::string& text,
               float x,
               float y,
               float width,
               float height,
               float fontSize,
               const Color& color) {
    const float textWidth = fonts.MeasureWidth(text, fontSize, FontFaceId::Medium, 0.0f);
    const float ascent = fonts.GetAscent(fontSize, FontFaceId::Medium);
    const float descent = fonts.GetDescent(fontSize, FontFaceId::Medium);
    const float lineTop = y + std::max(0.0f, (height - fontSize) * 0.5f);
    const float baseline = lineTop + (fontSize - (ascent + descent)) * 0.5f + ascent;
    const float textX = x + std::max(0.0f, (width - textWidth) * 0.5f);
    fonts.DrawString(buffer, textX, baseline, text, fontSize, FontFaceId::Medium, color, 1.0f, 0.0f);
}

void DrawOutline(FrameBuffer& buffer, float x, float y, float width, float height, const Color& color) {
    const int left = static_cast<int>(std::round(x));
    const int top = static_cast<int>(std::round(y));
    const int boxWidth = std::max(1, static_cast<int>(std::round(width)));
    const int boxHeight = std::max(1, static_cast<int>(std::round(height)));
    buffer.FillRect(left, top, boxWidth, 1, color);
    buffer.FillRect(left, top + boxHeight - 1, boxWidth, 1, color);
    buffer.FillRect(left, top, 1, boxHeight, color);
    buffer.FillRect(left + boxWidth - 1, top, 1, boxHeight, color);
}

SelectionBounds ClipToArea(const SelectionBounds& rect, const SelectionBounds& area) {
    const float left = std::max(rect.x, area.x);
    const float top = std::max(rect.y, area.y);
    const float right = std::min(rect.x + rect.width, area.x + area.width);
    const float bottom = std::min(rect.y + rect.height, area.y + area.height);
    SelectionBounds clipped;
    clipped.x = left;
    clipped.y = top;
    clipped.width = std::max(0.0f, right - left);
    clipped.height = std::max(0.0f, bottom - top);
    return clipped;
}

}  // namespace

void SelectionAreaPainter::Paint(FrameBuffer& buffer,
                                 const FontEngine& fonts,
                                 SelectionArea& area,
                                 float x,
                                 float y,
                                 float width,
                                 float height,
                                 float pixelScale,
                                 bool dark) {
    SelectionBounds bounds;
    bounds.x = x;
    bounds.y = y;
    bounds.width = width;
    bounds.height = height;
    area.SetBounds(bounds);
    area.SetPixelScale(pixelScale);

    const Color accent = dark ? Color::FromHex("#0A84FF") : Color::FromHex("#007AFF");
    const Color labelInk = dark ? Color::FromHex("#F5F5F7") : Color::FromHex("#1D1D1F");
    const Color selectedInk = Color::FromHex("#FFFFFF");
    const Color folder = dark ? Color::FromHex("#2C2C2E") : Color::FromHex("#FFFFFF");
    const Color folderEdge = dark ? Color::FromHex("#454548") : Color::FromHex("#E5E5EA");
    const Color folderMark = dark ? Color::FromHex("#F5F5F7") : Color::FromHex("#1D1D1F");
    const float scale = pixelScale > 0.0f ? pixelScale : 1.0f;

    for (SelectionAreaItem& item : area.Items()) {
        const float itemX = x + item.GetX() * scale;
        const float itemY = y + item.GetY() * scale;
        const float itemWidth = item.GetWidth() * scale;
        const float itemHeight = item.GetHeight() * scale;
        SelectionBounds itemBounds;
        itemBounds.x = itemX;
        itemBounds.y = itemY;
        itemBounds.width = itemWidth;
        itemBounds.height = itemHeight;
        item.SetBounds(itemBounds);

        const float iconSize = 52.0f * scale;
        const float iconX = itemX + (itemWidth - iconSize) * 0.5f;
        const float iconY = itemY + 8.0f * scale;
        const float labelY = iconY + iconSize + 6.0f * scale;
        const float labelHeight = 18.0f * scale;
        const bool selected = item.IsSelected();
        const bool focused = item.IsFocused() && area.HasKeyboardFocus();

        if (selected) {
            buffer.FillRoundedRect(iconX - 6.0f * scale, iconY - 4.0f * scale, iconSize + 12.0f * scale,
                                   iconSize + 8.0f * scale, 8.0f * scale, accent, 0.28f);
            buffer.FillRoundedRect(itemX + 6.0f * scale, labelY, itemWidth - 12.0f * scale, labelHeight, 4.0f * scale, accent);
        }
        buffer.FillRoundedRect(iconX, iconY + 10.0f * scale, iconSize, iconSize - 10.0f * scale, 6.0f * scale,
                               selected ? Color::FromHex("#FFFFFF") : folder);
        buffer.FillRoundedRect(iconX, iconY + 10.0f * scale, iconSize, 2.0f * scale, 1.0f * scale, selected ? accent : folderEdge);
        buffer.FillRoundedRect(iconX + 4.0f * scale, iconY + 2.0f * scale, 22.0f * scale, 12.0f * scale, 3.0f * scale,
                               selected ? accent : folderMark);
        DrawLabel(buffer, fonts, item.GetLabel(), itemX, labelY, itemWidth, labelHeight, 12.0f * scale,
                  selected ? selectedInk : labelInk);
        if (focused && !selected) {
            DrawOutline(buffer, itemX + 2.0f * scale, itemY + 2.0f * scale, itemWidth - 4.0f * scale, itemHeight - 4.0f * scale,
                        accent);
        } else if (focused && selected) {
            DrawOutline(buffer, itemX + 8.0f * scale, labelY + 1.0f * scale, itemWidth - 16.0f * scale, labelHeight - 2.0f * scale,
                        Color::FromHex("#FFFFFF"));
        }
    }

    if (area.HasMarquee()) {
        const SelectionBounds rect = ClipToArea(area.MarqueeRect(), bounds);
        if (rect.width >= 1.0f || rect.height >= 1.0f) {
            buffer.FillRoundedRect(rect.x, rect.y, std::max(1.0f, rect.width), std::max(1.0f, rect.height), 2.0f * scale, accent,
                                   0.22f);
            DrawOutline(buffer, rect.x, rect.y, std::max(1.0f, rect.width), std::max(1.0f, rect.height), accent);
        }
    }
}

}  // namespace Robo
