#include "Robo/DesktopPainter.hpp"

#include "Robo/Foundation.hpp"
#include "Robo/SvgPainter.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace Robo {

namespace {

constexpr float MENU_HEIGHT = 30.0f;
constexpr float MENU_PAD = 21.0f;
constexpr float MENU_APP_X = 62.0f;
constexpr float MENU_FONT = 16.0f;
constexpr float MENU_GAP = 22.0f;
constexpr float MENU_CLOCK = 15.0f;
constexpr float MENU_PILL = 22.0f;
constexpr float MENU_PILL_PAD = 8.0f;
constexpr float APPLE_W = 18.0f;
constexpr float APPLE_H = 21.0f;
constexpr float WIFI_W = 20.0f;
constexpr float WIFI_H = 16.0f;
constexpr float BATTERY_W = 27.0f;
constexpr float BATTERY_H = 16.0f;
constexpr float SEARCH_SIZE = 17.0f;
constexpr float CONTROL_SIZE = 18.0f;
constexpr float STATUS_GAP = 14.0f;
constexpr float CLOCK_GAP = 16.0f;
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

SvgCommand CubicTo(float c1x, float c1y, float c2x, float c2y, float x, float y) {
    return SvgCommand::Cubic(c1x, c1y, c2x, c2y, x, y);
}

SvgShape Filled(std::vector<SvgCommand> commands) {
    SvgShape shape;
    shape.commands = std::move(commands);
    shape.hasFill = true;
    shape.hasStroke = false;
    shape.currentFill = true;
    shape.fillRule = SvgFillRule::NonZero;
    return shape;
}

SvgShape Stroked(std::vector<SvgCommand> commands, float width) {
    SvgShape shape;
    shape.commands = std::move(commands);
    shape.hasFill = false;
    shape.hasStroke = true;
    shape.currentStroke = true;
    shape.strokeWidth = width;
    return shape;
}

void AddCircle(std::vector<SvgCommand>& commands, float cx, float cy, float radius) {
    const float k = 0.55228475f * radius;
    commands.push_back(SvgCommand::Move(cx + radius, cy));
    commands.push_back(CubicTo(cx + radius, cy + k, cx + k, cy + radius, cx, cy + radius));
    commands.push_back(CubicTo(cx - k, cy + radius, cx - radius, cy + k, cx - radius, cy));
    commands.push_back(CubicTo(cx - radius, cy - k, cx - k, cy - radius, cx, cy - radius));
    commands.push_back(CubicTo(cx + k, cy - radius, cx + radius, cy - k, cx + radius, cy));
    commands.push_back(SvgCommand::Close());
}

void AddArc(std::vector<SvgCommand>& commands, float cx, float cy, float radius, float start, float sweep) {
    const int steps = 2;
    const float step = sweep / static_cast<float>(steps);
    auto pointX = [&](float angle) { return cx + radius * std::cos(angle); };
    auto pointY = [&](float angle) { return cy - radius * std::sin(angle); };
    commands.push_back(SvgCommand::Move(pointX(start), pointY(start)));
    for (int stepIndex = 0; stepIndex < steps; ++stepIndex) {
        const float from = start + step * static_cast<float>(stepIndex);
        const float to = from + step;
        const float quarter = step * 0.25f;
        const float handle = (4.0f / 3.0f) * (std::sin(quarter) / std::cos(quarter)) * radius;
        commands.push_back(CubicTo(pointX(from) - std::sin(from) * handle,
                                   pointY(from) - std::cos(from) * handle,
                                   pointX(to) + std::sin(to) * handle,
                                   pointY(to) + std::cos(to) * handle,
                                   pointX(to),
                                   pointY(to)));
    }
}

Svg AppleMark() {
    Svg graphic;
    SvgStyle style;
    style.viewWidth = 17.0f;
    style.viewHeight = 20.0f;
    style.width = APPLE_W;
    style.height = APPLE_H;
    graphic.SetStyle(style);
    graphic.AddShape(Filled({
        SvgCommand::Move(13.9f, 10.6f),
        CubicTo(13.9f, 8.2f, 15.9f, 7.0f, 16.0f, 6.9f),
        CubicTo(14.9f, 5.2f, 13.1f, 5.0f, 12.5f, 5.0f),
        CubicTo(11.0f, 4.8f, 9.6f, 5.9f, 8.9f, 5.9f),
        CubicTo(8.2f, 5.9f, 7.0f, 5.1f, 5.7f, 5.1f),
        CubicTo(4.1f, 5.1f, 2.6f, 6.1f, 1.7f, 7.5f),
        CubicTo(0.0f, 10.4f, 1.3f, 14.8f, 2.9f, 17.2f),
        CubicTo(3.7f, 18.4f, 4.7f, 19.7f, 5.9f, 19.6f),
        CubicTo(7.1f, 19.5f, 7.6f, 18.8f, 9.0f, 18.8f),
        CubicTo(10.4f, 18.8f, 10.9f, 19.6f, 12.2f, 19.6f),
        CubicTo(13.5f, 19.6f, 14.3f, 18.4f, 15.1f, 17.3f),
        CubicTo(16.0f, 16.0f, 16.4f, 14.7f, 16.4f, 14.6f),
        CubicTo(16.3f, 14.6f, 13.9f, 13.6f, 13.9f, 10.7f),
        SvgCommand::Close(),
        SvgCommand::Move(11.6f, 4.2f),
        CubicTo(12.3f, 3.4f, 12.7f, 2.3f, 12.6f, 1.1f),
        CubicTo(11.6f, 1.2f, 10.5f, 1.7f, 9.8f, 2.6f),
        CubicTo(9.2f, 3.3f, 8.6f, 4.5f, 8.8f, 5.6f),
        CubicTo(8.9f, 5.6f, 8.9f, 5.6f, 9.0f, 5.6f),
        CubicTo(10.1f, 5.6f, 11.0f, 5.0f, 11.6f, 4.2f),
        SvgCommand::Close(),
    }));
    return graphic;
}

Svg SearchMark() {
    Svg graphic;
    SvgStyle style;
    style.viewWidth = 17.0f;
    style.viewHeight = 17.0f;
    style.width = SEARCH_SIZE;
    style.height = SEARCH_SIZE;
    graphic.SetStyle(style);
    std::vector<SvgCommand> ring;
    AddCircle(ring, 7.1f, 7.1f, 4.7f);
    graphic.AddShape(Stroked(std::move(ring), 1.6f));
    graphic.AddShape(Stroked({SvgCommand::Move(10.6f, 10.6f), SvgCommand::Line(15.2f, 15.2f)}, 1.7f));
    return graphic;
}

Svg WifiMark() {
    Svg graphic;
    SvgStyle style;
    style.viewWidth = 20.0f;
    style.viewHeight = 16.0f;
    style.width = WIFI_W;
    style.height = WIFI_H;
    graphic.SetStyle(style);
    const float cx = 10.0f;
    const float cy = 13.4f;
    const float start = 0.70f;
    const float sweep = 1.74f;
    for (float radius : {4.0f, 7.15f, 10.3f}) {
        std::vector<SvgCommand> arc;
        AddArc(arc, cx, cy, radius, start, sweep);
        graphic.AddShape(Stroked(std::move(arc), 1.45f));
    }
    std::vector<SvgCommand> dot;
    AddCircle(dot, cx, cy, 1.25f);
    graphic.AddShape(Filled(std::move(dot)));
    return graphic;
}

Svg BatteryMark() {
    Svg graphic;
    SvgStyle style;
    style.viewWidth = 27.0f;
    style.viewHeight = 16.0f;
    style.width = BATTERY_W;
    style.height = BATTERY_H;
    graphic.SetStyle(style);
    std::vector<SvgCommand> body;
    const float x = 0.9f;
    const float y = 3.1f;
    const float w = 21.4f;
    const float h = 9.8f;
    const float r = 2.2f;
    body.push_back(SvgCommand::Move(x + r, y));
    body.push_back(SvgCommand::Line(x + w - r, y));
    body.push_back(CubicTo(x + w, y, x + w, y, x + w, y + r));
    body.push_back(SvgCommand::Line(x + w, y + h - r));
    body.push_back(CubicTo(x + w, y + h, x + w, y + h, x + w - r, y + h));
    body.push_back(SvgCommand::Line(x + r, y + h));
    body.push_back(CubicTo(x, y + h, x, y + h, x, y + h - r));
    body.push_back(SvgCommand::Line(x, y + r));
    body.push_back(CubicTo(x, y, x, y, x + r, y));
    body.push_back(SvgCommand::Close());
    graphic.AddShape(Stroked(std::move(body), 1.35f));
    graphic.AddShape(Filled({
        SvgCommand::Move(23.0f, 5.6f),
        SvgCommand::Line(25.2f, 5.6f),
        CubicTo(26.2f, 5.6f, 26.2f, 5.6f, 26.2f, 6.6f),
        SvgCommand::Line(26.2f, 9.4f),
        CubicTo(26.2f, 10.4f, 26.2f, 10.4f, 25.2f, 10.4f),
        SvgCommand::Line(23.0f, 10.4f),
        SvgCommand::Close(),
    }));
    graphic.AddShape(Filled({
        SvgCommand::Move(3.0f, 5.3f),
        SvgCommand::Line(16.4f, 5.3f),
        SvgCommand::Line(16.4f, 10.7f),
        SvgCommand::Line(3.0f, 10.7f),
        SvgCommand::Close(),
    }));
    return graphic;
}

Svg ControlMark() {
    Svg graphic;
    SvgStyle style;
    style.viewWidth = 18.0f;
    style.viewHeight = 18.0f;
    style.width = CONTROL_SIZE;
    style.height = CONTROL_SIZE;
    graphic.SetStyle(style);
    graphic.AddShape(Filled({
        SvgCommand::Move(1.0f, 3.6f),
        SvgCommand::Line(16.4f, 3.6f),
        SvgCommand::Line(16.4f, 6.2f),
        SvgCommand::Line(1.0f, 6.2f),
        SvgCommand::Close(),
    }));
    std::vector<SvgCommand> topKnob;
    AddCircle(topKnob, 13.2f, 4.9f, 2.7f);
    graphic.AddShape(Filled(std::move(topKnob)));
    graphic.AddShape(Filled({
        SvgCommand::Move(1.6f, 11.6f),
        SvgCommand::Line(17.0f, 11.6f),
        SvgCommand::Line(17.0f, 14.2f),
        SvgCommand::Line(1.6f, 14.2f),
        SvgCommand::Close(),
    }));
    std::vector<SvgCommand> bottomKnob;
    AddCircle(bottomKnob, 4.8f, 12.9f, 2.7f);
    graphic.AddShape(Filled(std::move(bottomKnob)));
    return graphic;
}

void PaintGlyph(FrameBuffer& buffer, Svg graphic, float x, float y, const Color& color, float pixelScale) {
    SvgStyle style = graphic.GetStyle();
    style.color = color;
    graphic.SetStyle(style);
    SvgPainter::Paint(buffer, graphic, x, y, style.width * pixelScale, pixelScale);
}

void FrostMenuBar(FrameBuffer& buffer, int height) {
    const int width = buffer.GetWidth();
    const std::uint8_t* pixels = buffer.GetPixels();
    if (width <= 0 || height <= 0 || pixels == nullptr) {
        return;
    }
    std::vector<std::uint8_t> source(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3u);
    for (int y = 0; y < height; ++y) {
        const std::uint8_t* row = pixels + static_cast<std::size_t>(y * width) * 4u;
        std::uint8_t* dest = source.data() + static_cast<std::size_t>(y * width) * 3u;
        for (int x = 0; x < width; ++x) {
            dest[x * 3] = row[x * 4];
            dest[x * 3 + 1] = row[x * 4 + 1];
            dest[x * 3 + 2] = row[x * 4 + 2];
        }
    }

    const int block = 8;
    const float veil = 0.52f;
    const float skyRed = 174.0f;
    const float skyGreen = 223.0f;
    const float skyBlue = 246.0f;
    for (int y0 = 0; y0 < height; y0 += block) {
        for (int x0 = 0; x0 < width; x0 += block) {
            int red = 0;
            int green = 0;
            int blue = 0;
            int count = 0;
            const int top = std::max(0, y0 - block);
            const int left = std::max(0, x0 - block);
            const int bottom = std::min(height, y0 + block * 2);
            const int right = std::min(width, x0 + block * 2);
            for (int y = top; y < bottom; y += 2) {
                const std::uint8_t* row = source.data() + static_cast<std::size_t>(y * width) * 3u;
                for (int x = left; x < right; x += 2) {
                    red += row[x * 3];
                    green += row[x * 3 + 1];
                    blue += row[x * 3 + 2];
                    ++count;
                }
            }
            if (count == 0) {
                continue;
            }
            const float inv = (1.0f - veil) / static_cast<float>(count);
            const int mixedRed = static_cast<int>(static_cast<float>(red) * inv + skyRed * veil);
            const int mixedGreen = static_cast<int>(static_cast<float>(green) * inv + skyGreen * veil);
            const int mixedBlue = static_cast<int>(static_cast<float>(blue) * inv + skyBlue * veil);
            buffer.FillRect(x0, y0, std::min(block, width - x0), std::min(block, height - y0),
                            Color::FromRgba(static_cast<std::uint8_t>(mixedRed),
                                            static_cast<std::uint8_t>(mixedGreen),
                                            static_cast<std::uint8_t>(mixedBlue),
                                            255));
        }
    }
}

MenuBounds Slot(float x, float y, float width, float height) {
    MenuBounds bounds;
    bounds.x = x;
    bounds.y = y;
    bounds.width = width;
    bounds.height = height;
    return bounds;
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

DesktopChrome DesktopPainter::Layout(float surfaceWidth,
                                   float surfaceHeight,
                                   float pixelScale,
                                   bool hasMenu,
                                   bool hasShell,
                                   bool hasDock) {
    const float scale = std::max(0.5f, pixelScale);
    DesktopChrome chrome;
    chrome.menuHeight = hasMenu ? MENU_HEIGHT * scale : 0.0f;
    chrome.shellHeight = hasShell ? SHELL_HEIGHT * scale : 0.0f;
    chrome.dockHeight = hasDock ? (DOCK_ITEM_HEIGHT + DOCK_PAD * 2.0f + DOCK_MARGIN) * scale : 0.0f;
    chrome.dockY = std::max(0.0f, surfaceHeight - chrome.dockHeight);
    chrome.contentY = chrome.menuHeight + chrome.shellHeight;
    chrome.contentHeight = std::max(1.0f, surfaceHeight - chrome.contentY - chrome.dockHeight);
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

    if (desktop.HasTopBarMenu()) {
        TopBarMenu& menu = desktop.GetTopBarMenu();
        const float barHeight = chrome.menuHeight;
        menu.SetBar(0.0f, 0.0f, static_cast<float>(buffer.GetWidth()), barHeight);
        const float pad = MENU_PAD * scale;
        const float appX = MENU_APP_X * scale;
        const float fontSize = MENU_FONT * scale;
        const float gap = MENU_GAP * scale;
        const float pillHeight = MENU_PILL * scale;
        const float pillPad = MENU_PILL_PAD * scale;
        const float pillY = (barHeight - pillHeight) * 0.5f;
        const Color activeFill = Color::FromHex("#007AFF");
        const Color activeInk = Color::FromHex("#FFFFFF");
        const Color hoverFill = Color::FromRgba(0, 0, 0, 255);
        const int active = menu.Active();
        const int hovered = menu.Hover();

        float statusRight = static_cast<float>(buffer.GetWidth()) - pad;
        float clockX = statusRight;
        const float clockSize = MENU_CLOCK * scale;
        if (!menu.GetClock().empty()) {
            const float clockWidth = fonts.MeasureWidth(menu.GetClock(), clockSize, FontFaceId::Regular, 0.0f);
            clockX = statusRight - clockWidth;
            statusRight = clockX - CLOCK_GAP * scale;
        }
        const float controlX = statusRight - CONTROL_SIZE * scale;
        statusRight = controlX - STATUS_GAP * scale;
        const float batteryX = statusRight - BATTERY_W * scale;
        statusRight = batteryX - STATUS_GAP * scale;
        const float wifiX = statusRight - WIFI_W * scale;
        statusRight = wifiX - STATUS_GAP * scale;
        const float searchX = statusRight - SEARCH_SIZE * scale;
        const float statusLeft = searchX - gap;

        const float appleW = APPLE_W * scale;
        const float appleH = APPLE_H * scale;
        const float appleX = pad;
        const float appleY = (barHeight - appleH) * 0.5f;
        const float applePillW = appleW + pillPad * 2.0f;
        menu.SetAppleBounds(Slot(appleX - pillPad, 0.0f, applePillW, barHeight));

        const float appWidth = fonts.MeasureWidth(menu.GetApp(), fontSize, FontFaceId::SemiBold, 0.0f);
        menu.SetAppBounds(Slot(appX - pillPad, 0.0f, appWidth + pillPad * 2.0f, barHeight));

        float labelX = appX + appWidth + gap;
        std::vector<TopBarMenuItem>& labels = menu.Items();
        for (TopBarMenuItem& item : labels) {
            const float textWidth = fonts.MeasureWidth(item.GetLabel(), fontSize, FontFaceId::Regular, 0.0f);
            const float slotWidth = textWidth + pillPad * 2.0f;
            if (labelX + textWidth > statusLeft) {
                item.SetBounds(Slot(0.0f, 0.0f, 0.0f, 0.0f));
                continue;
            }
            item.SetBounds(Slot(labelX - pillPad, 0.0f, slotWidth, barHeight));
            labelX += textWidth + gap;
        }

        const bool paintBar = barHeight > 0.0f &&
            !buffer.MissesClip(0, 0, buffer.GetWidth(), std::max(1, static_cast<int>(std::ceil(barHeight))));
        if (paintBar) {
            buffer.Pump();
            FrostMenuBar(buffer, std::max(1, static_cast<int>(std::round(barHeight))));
            buffer.FillRect(0, std::max(0, static_cast<int>(std::round(barHeight)) - 1), buffer.GetWidth(), 1,
                            Color::FromHex("#1D1D1F"), 0.12f);

            auto pill = [&](float x, float width, bool open, bool hot) {
                if (width <= 0.0f) {
                    return;
                }
                if (open) {
                    buffer.FillRoundedRect(x, pillY, width, pillHeight, 6.0f * scale, activeFill);
                } else if (hot) {
                    buffer.FillRoundedRect(x, pillY, width, pillHeight, 6.0f * scale, hoverFill, 0.08f);
                }
            };
            pill(appleX - pillPad, applePillW, active == 0, hovered == 0);
            pill(appX - pillPad, appWidth + pillPad * 2.0f, active == 1, hovered == 1);
            for (int index = 0; index < static_cast<int>(labels.size()); ++index) {
                const MenuBounds& bounds = labels[static_cast<std::size_t>(index)].GetBounds();
                pill(bounds.x, bounds.width, active == index + 2, hovered == index + 2);
            }

            const Color appleColor = active == 0 ? activeInk : ink;
            PaintGlyph(buffer, AppleMark(), appleX, appleY, appleColor, scale);
            DrawLine(buffer, fonts, menu.GetApp(), appX, 0.0f, appWidth, barHeight, fontSize,
                     FontFaceId::SemiBold, active == 1 ? activeInk : ink, false);
            for (int index = 0; index < static_cast<int>(labels.size()); ++index) {
                const MenuBounds& bounds = labels[static_cast<std::size_t>(index)].GetBounds();
                if (bounds.width <= 0.0f) {
                    continue;
                }
                const Color labelColor = active == index + 2 ? activeInk : ink;
                DrawLine(buffer, fonts, labels[static_cast<std::size_t>(index)].GetLabel(), bounds.x + pillPad, 0.0f,
                         bounds.width - pillPad * 2.0f, barHeight, fontSize, FontFaceId::Regular, labelColor, false);
            }

            const Color status = ink;
            const float searchY = (barHeight - SEARCH_SIZE * scale) * 0.5f;
            const float wifiY = (barHeight - WIFI_H * scale) * 0.5f;
            const float batteryY = (barHeight - BATTERY_H * scale) * 0.5f;
            const float controlY = (barHeight - CONTROL_SIZE * scale) * 0.5f;
            PaintGlyph(buffer, SearchMark(), searchX, searchY, status, scale);
            PaintGlyph(buffer, WifiMark(), wifiX, wifiY, status, scale);
            PaintGlyph(buffer, BatteryMark(), batteryX, batteryY, status, scale);
            PaintGlyph(buffer, ControlMark(), controlX, controlY, status, scale);
            if (!menu.GetClock().empty()) {
                DrawLine(buffer, fonts, menu.GetClock(), clockX, 0.0f,
                         static_cast<float>(buffer.GetWidth()) - pad - clockX, barHeight, clockSize, FontFaceId::Regular,
                         ink, false);
            }
        }
    }

    const int menuPixels = static_cast<int>(std::lround(chrome.menuHeight));
    if (desktop.HasAppShell() && chrome.shellHeight > 0.0f &&
        !buffer.MissesClip(0, menuPixels, buffer.GetWidth(),
                           std::max(1, static_cast<int>(std::ceil(chrome.shellHeight))))) {
        buffer.Pump();
        buffer.FillRect(0, menuPixels, buffer.GetWidth(), static_cast<int>(std::lround(chrome.shellHeight)), panel);
        DrawLine(buffer, fonts, desktop.GetAppShell().GetTitle(), 20.0f * scale, chrome.menuHeight,
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
    const bool paintDock = !buffer.MissesClip(static_cast<int>(std::floor(barX)) - 2,
                                               static_cast<int>(std::floor(barY)) - 2,
                                               static_cast<int>(std::ceil(barWidth)) + 4,
                                               static_cast<int>(std::ceil(barHeight)) + 4);
    if (paintDock) {
        buffer.Pump();
        buffer.FillRoundedRect(barX, barY, barWidth, barHeight, DOCK_RADIUS * scale, panel, 0.94f);
    }

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
        if (!paintDock) {
            continue;
        }
        buffer.Pump();

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
