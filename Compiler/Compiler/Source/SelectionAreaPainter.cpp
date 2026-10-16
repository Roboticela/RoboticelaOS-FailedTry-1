#include "Robo/SelectionAreaPainter.hpp"
#include "Robo/SvgPainter.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace Robo {

namespace {

std::size_t NextCodepoint(const std::string& text, std::size_t index) {
    if (index >= text.size()) {
        return text.size();
    }
    const unsigned char lead = static_cast<unsigned char>(text[index]);
    std::size_t size = 1;
    if ((lead & 0xE0) == 0xC0) {
        size = 2;
    } else if ((lead & 0xF0) == 0xE0) {
        size = 3;
    } else if ((lead & 0xF8) == 0xF0) {
        size = 4;
    }
    return std::min(text.size(), index + size);
}

struct LabelLine {
    std::string text;
    float width = 0.0f;
};

void WrapLabel(const FontEngine& fonts,
               const std::string& text,
               float maxWidth,
               float fontSize,
               FontFaceId face,
               float letterSpacing,
               bool wrap,
               int maxLines,
               std::vector<LabelLine>& lines) {
    lines.clear();
    const int limit = std::max(1, std::min(maxLines, 4));
    std::string word;
    std::string line;
    auto widthOf = [&](const std::string& value) {
        return fonts.MeasureWidth(value, fontSize, face, letterSpacing);
    };
    auto pushLine = [&]() {
        if (line.empty() || static_cast<int>(lines.size()) >= limit) {
            return;
        }
        LabelLine entry;
        entry.text = line;
        entry.width = widthOf(line);
        lines.push_back(entry);
        line.clear();
    };
    auto addWord = [&](const std::string& piece) {
        if (piece.empty()) {
            return;
        }
        if (!wrap) {
            if (!line.empty()) {
                line += " ";
            }
            line += piece;
            return;
        }
        const std::string trial = line.empty() ? piece : line + " " + piece;
        if (widthOf(trial) <= maxWidth || line.empty()) {
            line = trial;
            return;
        }
        pushLine();
        if (static_cast<int>(lines.size()) >= limit) {
            return;
        }
        if (widthOf(piece) <= maxWidth) {
            line = piece;
            return;
        }
        std::string broken;
        for (std::size_t index = 0; index < piece.size();) {
            const std::size_t next = NextCodepoint(piece, index);
            const std::string grown = broken + piece.substr(index, next - index);
            if (!broken.empty() && widthOf(grown) > maxWidth) {
                line = broken;
                pushLine();
                broken.clear();
                if (static_cast<int>(lines.size()) >= limit) {
                    return;
                }
            }
            broken += piece.substr(index, next - index);
            index = next;
        }
        line = broken;
    };
    for (std::size_t index = 0; index <= text.size(); ++index) {
        const bool end = index == text.size();
        const char ch = end ? ' ' : text[index];
        if (ch == ' ' || ch == '\n') {
            addWord(word);
            word.clear();
            if (ch == '\n') {
                pushLine();
            }
            if (static_cast<int>(lines.size()) >= limit) {
                break;
            }
            continue;
        }
        word.push_back(ch);
    }
    if (static_cast<int>(lines.size()) < limit && !line.empty()) {
        pushLine();
    }
    if (lines.empty() && !text.empty()) {
        LabelLine entry;
        entry.text = text;
        entry.width = widthOf(text);
        lines.push_back(entry);
    }
}

void DrawLabelLine(FrameBuffer& buffer,
                   const FontEngine& fonts,
                   const std::string& text,
                   float x,
                   float baseline,
                   float fontSize,
                   FontFaceId face,
                   const Color& color,
                   float letterSpacing,
                   float opacity) {
    fonts.DrawString(buffer, x, baseline, text, fontSize, face, color, opacity, letterSpacing, true);
}

void DrawLabelShadow(FrameBuffer& buffer,
                     const FontEngine& fonts,
                     const std::string& text,
                     float x,
                     float baseline,
                     float fontSize,
                     FontFaceId face,
                     const SelectionLabelStyle& style,
                     float scale,
                     float letterSpacing) {
    const float blur = std::max(0.0f, style.shadowBlur) * scale;
    const float originX = x + style.shadowX * scale;
    const float originY = baseline + style.shadowY * scale;
    const float alpha = static_cast<float>(style.shadowColor.alpha) / 255.0f;
    if (alpha <= 0.0f) {
        return;
    }
    if (blur < 0.5f) {
        DrawLabelLine(buffer, fonts, text, originX, originY, fontSize, face, style.shadowColor, letterSpacing, 1.0f);
        return;
    }
    const float taps[][3] = {
        {0.0f, 0.0f, 0.42f},  {-1.0f, 0.0f, 0.12f}, {1.0f, 0.0f, 0.12f}, {0.0f, -1.0f, 0.10f},
        {0.0f, 1.0f, 0.12f},  {-1.0f, -1.0f, 0.03f}, {1.0f, -1.0f, 0.03f}, {-1.0f, 1.0f, 0.03f},
        {1.0f, 1.0f, 0.03f},
    };
    for (const auto& tap : taps) {
        DrawLabelLine(buffer, fonts, text, originX + tap[0] * blur, originY + tap[1] * blur, fontSize, face,
                      style.shadowColor, letterSpacing, tap[2]);
    }
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
                                 bool dark,
                                 Svg* folderIcon) {
    SelectionBounds bounds;
    bounds.x = x;
    bounds.y = y;
    bounds.width = width;
    bounds.height = height;
    area.SetBounds(bounds);
    area.SetPixelScale(pixelScale);

    const Color accent = dark ? Color::FromHex("#0A84FF") : Color::FromHex("#007AFF");
    const Color selectedInk = Color::FromHex("#FFFFFF");
    const Color folder = dark ? Color::FromHex("#2C2C2E") : Color::FromHex("#FFFFFF");
    const Color folderEdge = dark ? Color::FromHex("#454548") : Color::FromHex("#E5E5EA");
    const Color folderMark = dark ? Color::FromHex("#F5F5F7") : Color::FromHex("#1D1D1F");
    const float scale = pixelScale > 0.0f ? pixelScale : 1.0f;

    int itemIndex = 0;
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
        const SelectionLabelStyle& labelStyle = item.HasLabelStyle() ? item.GetLabelStyle() : area.GetLabelStyle();
        const float labelGap = std::max(0.0f, labelStyle.iconGap) * scale;
        const float labelY = iconY + iconSize + labelGap;
        const bool selected = item.IsSelected();
        const bool focused = item.IsFocused() && area.HasKeyboardFocus();

        const bool iconPlate = selected || area.Hover() == itemIndex;
        if (iconPlate) {
            const float padX = 8.0f * scale;
            const float padY = 6.0f * scale;
            buffer.FillRoundedRect(iconX - padX, iconY - padY, iconSize + padX * 2.0f, iconSize + padY * 2.0f, 12.0f * scale,
                                   Color::FromHex("#FFFFFF"), 0.28f);
        }
        const bool paintedFolder = folderIcon != nullptr && !folderIcon->GetShapes().empty();
        if (paintedFolder) {
            SvgStyle iconStyle = folderIcon->GetStyle();
            iconStyle.width = 52.0f;
            iconStyle.height = 52.0f;
            iconStyle.opacity = 1.0f;
            folderIcon->SetStyle(iconStyle);
            SvgPainter::Paint(buffer, *folderIcon, iconX, iconY, iconSize, scale);
        } else {
            buffer.FillRoundedRect(iconX, iconY + 10.0f * scale, iconSize, iconSize - 10.0f * scale, 6.0f * scale,
                                   selected ? Color::FromHex("#FFFFFF") : folder);
            buffer.FillRoundedRect(iconX, iconY + 10.0f * scale, iconSize, 2.0f * scale, 1.0f * scale, selected ? accent : folderEdge);
            buffer.FillRoundedRect(iconX + 4.0f * scale, iconY + 2.0f * scale, 22.0f * scale, 12.0f * scale, 3.0f * scale,
                                   selected ? accent : folderMark);
        }
        const FontFaceId labelFace = fonts.ResolveFace(labelStyle.fontWeight, FontStyleKind::Normal);
        const float cssSize = std::max(1.0f, labelStyle.fontSize) * scale;
        const float labelSize = std::max(1.0f, fonts.PixelSizeForCss(cssSize, labelFace));
        const float letterSpacing = labelStyle.letterSpacing * scale;
        const float lineStep = cssSize * std::max(1.0f, labelStyle.lineHeight);
        const float labelWidth = std::max(1.0f, std::min(itemWidth, labelStyle.maxWidth * scale));
        const float labelX = itemX + (itemWidth - labelWidth) * 0.5f;
        std::vector<LabelLine> lines;
        WrapLabel(fonts, item.GetLabel(), labelWidth, labelSize, labelFace, letterSpacing, labelStyle.wrap,
                  labelStyle.maxLines, lines);
        float widest = 0.0f;
        for (const LabelLine& line : lines) {
            widest = std::max(widest, line.width);
        }
        const float chipPadX = 6.0f * scale;
        const float chipPadY = 1.0f * scale;
        const float chipWidth = std::min(labelWidth, widest + chipPadX * 2.0f);
        const float chipHeight = lineStep * static_cast<float>(std::max(1, static_cast<int>(lines.size()))) + chipPadY * 2.0f;
        const float chipX = labelX + (labelWidth - chipWidth) * 0.5f;
        if (selected && !lines.empty()) {
            buffer.FillRoundedRect(chipX, labelY - chipPadY, chipWidth, chipHeight, labelStyle.selectionRadius * scale,
                                   labelStyle.selectionColor);
        }
        const float ascent = fonts.GetAscent(labelSize, labelFace);
        const Color labelColor = selected ? selectedInk : labelStyle.color;
        for (int index = 0; index < static_cast<int>(lines.size()); ++index) {
            const LabelLine& line = lines[static_cast<std::size_t>(index)];
            float textX = labelX;
            if (labelStyle.align == TextAlign::Center) {
                textX = labelX + std::max(0.0f, (labelWidth - line.width) * 0.5f);
            } else if (labelStyle.align == TextAlign::Right) {
                textX = labelX + std::max(0.0f, labelWidth - line.width);
            }
            const float baseline = labelY + ascent + lineStep * static_cast<float>(index);
            if (!selected) {
                DrawLabelShadow(buffer, fonts, line.text, textX, baseline, labelSize, labelFace, labelStyle, scale,
                                letterSpacing);
            }
            DrawLabelLine(buffer, fonts, line.text, textX, baseline, labelSize, labelFace, labelColor, letterSpacing, 1.0f);
        }
        const float labelHeight = std::max(lineStep, chipHeight);
        if (focused && !selected) {
            DrawOutline(buffer, itemX + 2.0f * scale, itemY + 2.0f * scale, itemWidth - 4.0f * scale, itemHeight - 4.0f * scale,
                        accent);
        } else if (focused && selected && !lines.empty()) {
            DrawOutline(buffer, chipX + 1.0f * scale, labelY, chipWidth - 2.0f * scale, std::max(1.0f, labelHeight - 2.0f * scale),
                        Color::FromHex("#FFFFFF"));
        }
        itemIndex += 1;
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
