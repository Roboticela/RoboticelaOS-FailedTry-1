#include "Robo/TitleBarPainter.hpp"

#include "Robo/BrandLogoPainter.hpp"
#include "Robo/ButtonPainter.hpp"
#include "Robo/Foundation.hpp"
#include "Robo/SvgPainter.hpp"
#include "Robo/ThemeIconTogglePainter.hpp"

#include <algorithm>

namespace Robo {

namespace {

const float TITLE_SIZE = 17.0f;
const float TITLE_TRACKING = -0.022f;
const float SIDE_INSET = 16.0f;
const float CLUSTER_GAP = 10.0f;
const float SIDEBAR_PAD = 9.0f;
const float SIDEBAR_ICON_WIDTH = 24.0f;
const float SIDEBAR_ICON_HEIGHT = 30.0f * 16.5f / 28.5f;
const float SIDEBAR_BUTTON_WIDTH = SIDEBAR_ICON_WIDTH + SIDEBAR_PAD * 2.0f;
const float SIDEBAR_BUTTON_HEIGHT = SIDEBAR_ICON_HEIGHT + SIDEBAR_PAD * 2.0f;

void PaintSidebarIcon(FrameBuffer& buffer, Svg& icon, float x, float y, float width, const Color& ink, float opacity, float scale) {
    if (opacity <= 0.01f) {
        return;
    }
    icon.GetStyle().viewX = 1.75f;
    icon.GetStyle().viewY = 3.75f;
    icon.GetStyle().viewWidth = 28.5f;
    icon.GetStyle().viewHeight = 16.5f;
    icon.GetStyle().width = SIDEBAR_ICON_WIDTH;
    icon.GetStyle().height = SIDEBAR_ICON_HEIGHT;
    icon.GetStyle().widthSpec.clear();
    icon.GetStyle().heightSpec.clear();
    icon.GetStyle().color = ink;
    icon.GetStyle().opacity = opacity;
    SvgPainter::Paint(buffer, icon, x, y, width, scale);
}

}  // namespace

TitleBarMeasureResult TitleBarPainter::Measure(const FontEngine& fonts,
                                               const TitleBar& bar,
                                               float containerWidth,
                                               float pixelScale) {
    (void)fonts;
    (void)bar;
    const float scale = std::max(0.5f, pixelScale);
    TitleBarMeasureResult result;
    result.width = std::max(0.0f, containerWidth);
    result.height = TITLE_BAR_HEIGHT * scale;
    result.advanceY = result.height + 12.0f * scale;
    return result;
}

TitleBarPaintResult TitleBarPainter::Paint(FrameBuffer& buffer,
                                           const FontEngine& fonts,
                                           TitleBar& bar,
                                           float originX,
                                           float originY,
                                           float containerWidth,
                                           float pixelScale) {
    const float scale = std::max(0.5f, pixelScale);
    const TitleBarMeasureResult measured = Measure(fonts, bar, containerWidth, scale);
    const Color background = ResolveColor(Color::FromHex("#F5F5F7"), "color.bg.secondary");
    const Color hairline = ResolveColor(Color::FromHex("#E5E5E7"), "color.border.subtle");
    buffer.FillRect(static_cast<int>(std::floor(originX)),
                    static_cast<int>(std::floor(originY)),
                    std::max(1, static_cast<int>(std::ceil(measured.width))),
                    std::max(1, static_cast<int>(std::ceil(measured.height))),
                    background,
                    1.0f);
    const int line = std::max(1, static_cast<int>(std::round(scale)));
    buffer.FillRect(static_cast<int>(std::floor(originX)),
                    static_cast<int>(std::floor(originY + measured.height)) - line,
                    std::max(1, static_cast<int>(std::ceil(measured.width))),
                    line,
                    hairline,
                    1.0f);

    const float inset = SIDE_INSET * scale;
    const BrandLogoMeasureResult logo = BrandLogoPainter::Measure(fonts, bar.Logo(), scale);
    const float logoY = originY + std::max(0.0f, (measured.height - logo.height) * 0.5f);
    BrandLogoPainter::Paint(buffer, fonts, bar.Logo(), originX + inset, logoY, scale);

    const float buttonW = SIDEBAR_BUTTON_WIDTH * scale;
    const float buttonH = SIDEBAR_BUTTON_HEIGHT * scale;
    const float buttonX = originX + inset + logo.width + CLUSTER_GAP * scale;
    const float buttonY = originY + std::max(0.0f, (measured.height - buttonH) * 0.5f);
    ButtonPainter::Paint(buffer, fonts, bar.SidebarButton(), buttonX, buttonY, buttonW, scale, buttonW, buttonH);
    if (bar.HasSidebarIcons()) {
        const ButtonBounds& buttonBounds = bar.SidebarButton().GetBounds();
        const float pad = SIDEBAR_PAD * scale;
        const float glyphW = SIDEBAR_ICON_WIDTH * scale;
        const float glyphX = buttonBounds.x + pad;
        const float glyphY = buttonBounds.y + pad;
        const float open = bar.SidebarAmount();
        const Color ink = ResolveColor(Color::FromHex("#6E6E73"), "color.text.secondary");
        buffer.PushClipRect(buttonBounds.x, buttonBounds.y, buttonBounds.width, buttonBounds.height);
        PaintSidebarIcon(buffer, bar.HiddenIcon(), glyphX, glyphY, glyphW, ink, 1.0f - open, scale);
        PaintSidebarIcon(buffer, bar.ShownIcon(), glyphX, glyphY, glyphW, ink, open, scale);
        buffer.PopClip();
    }

    const ThemeIconToggleMeasureResult icon = ThemeIconTogglePainter::Measure(bar.ThemeIcon(), scale);
    const float iconX = originX + std::max(inset, measured.width - inset - icon.width);
    const float iconY = originY + std::max(0.0f, (measured.height - icon.height) * 0.5f);
    ThemeIconTogglePainter::Paint(buffer, bar.ThemeIcon(), iconX, iconY, scale);

    if (!bar.GetTitle().empty()) {
        const float fontSize = TITLE_SIZE * scale;
        const float tracking = TITLE_TRACKING * fontSize;
        const float textWidth = fonts.MeasureWidth(bar.GetTitle(), fontSize, FontFaceId::SemiBold, tracking);
        const float ascent = fonts.GetAscent(fontSize, FontFaceId::SemiBold);
        const float descent = fonts.GetDescent(fontSize, FontFaceId::SemiBold);
        const float lineTop = originY + (measured.height - fontSize) * 0.5f;
        const float baseline = lineTop + (fontSize - (ascent + descent)) * 0.5f + ascent;
        const float textX = originX + std::max(0.0f, (measured.width - textWidth) * 0.5f);
        const Color ink = ResolveColor(Color::FromHex("#1D1D1F"), "color.text.primary");
        fonts.DrawString(buffer, textX, baseline, bar.GetTitle(), fontSize, FontFaceId::SemiBold, ink, 1.0f, tracking);
    }

    TitleBarPaintResult result;
    result.advanceY = measured.advanceY;
    result.height = measured.height;
    return result;
}

}  // namespace Robo
