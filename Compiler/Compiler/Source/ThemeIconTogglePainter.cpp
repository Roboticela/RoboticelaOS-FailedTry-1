#include "Robo/ThemeIconTogglePainter.hpp"

#include "Robo/AppearanceGlyph.hpp"
#include "Robo/Foundation.hpp"
#include "Robo/Svg.hpp"
#include "Robo/SvgPainter.hpp"

#include <algorithm>

namespace Robo {

namespace {

float Css(float value, float scale) {
    return value * scale;
}

Color LerpColor(const Color& a, const Color& b, float t) {
    auto channel = [t](std::uint8_t from, std::uint8_t to) {
        return static_cast<std::uint8_t>(from + (static_cast<float>(to) - from) * t);
    };
    return Color::FromRgba(channel(a.red, b.red), channel(a.green, b.green), channel(a.blue, b.blue), channel(a.alpha, b.alpha));
}

Color TokenOn(const char* hex, const char* token, Theme theme) {
    Color color = Color::FromHex(hex);
    CurrentFoundation().LookupColor(token, theme, color);
    return color;
}

Color Mix(const Color& base, const Color& tint, float tintAmount) {
    const float keep = 1.0f - tintAmount;
    return Color::FromRgba(static_cast<std::uint8_t>(base.red * keep + tint.red * tintAmount),
                           static_cast<std::uint8_t>(base.green * keep + tint.green * tintAmount),
                           static_cast<std::uint8_t>(base.blue * keep + tint.blue * tintAmount),
                           255);
}

bool Single(ThemeIconToggleVariant variant) {
    return variant == ThemeIconToggleVariant::Ghost || variant == ThemeIconToggleVariant::Subtle ||
           variant == ThemeIconToggleVariant::Outline;
}

void StrokeRound(FrameBuffer& buffer, float x, float y, float width, float height, float radius, const Color& color, float thickness, float opacity) {
    Svg svg;
    SvgStyle style;
    style.viewWidth = width;
    style.viewHeight = height;
    style.width = width;
    style.height = height;
    style.color = color;
    style.opacity = opacity;
    svg.SetStyle(style);
    const float inset = thickness * 0.5f;
    const float right = width - inset;
    const float bottom = height - inset;
    const float rad = std::max(0.0f, radius - inset);
    const float k = rad * 0.55228475f;
    SvgShape shape;
    shape.hasFill = false;
    shape.hasStroke = true;
    shape.currentStroke = true;
    shape.strokeWidth = thickness;
    shape.commands.push_back(SvgCommand::Move(inset + rad, inset));
    shape.commands.push_back(SvgCommand::Line(right - rad, inset));
    shape.commands.push_back(SvgCommand::Cubic(right - rad + k, inset, right, inset + rad - k, right, inset + rad));
    shape.commands.push_back(SvgCommand::Line(right, bottom - rad));
    shape.commands.push_back(SvgCommand::Cubic(right, bottom - rad + k, right - rad + k, bottom, right - rad, bottom));
    shape.commands.push_back(SvgCommand::Line(inset + rad, bottom));
    shape.commands.push_back(SvgCommand::Cubic(inset + rad - k, bottom, inset, bottom - rad + k, inset, bottom - rad));
    shape.commands.push_back(SvgCommand::Line(inset, inset + rad));
    shape.commands.push_back(SvgCommand::Cubic(inset, inset + rad - k, inset + rad - k, inset, inset + rad, inset));
    shape.commands.push_back(SvgCommand::Close());
    svg.AddShape(shape);
    SvgPainter::Paint(buffer, svg, x, y, width, 1.0f);
}

ThemeToggleBounds Box(float x, float y, float width, float height) {
    ThemeToggleBounds bounds;
    bounds.x = x;
    bounds.y = y;
    bounds.width = width;
    bounds.height = height;
    return bounds;
}

}  // namespace

ThemeIconToggleMeasureResult ThemeIconTogglePainter::Measure(const ThemeIconToggle& toggle, float pixelScale) {
    const float scale = std::max(0.5f, pixelScale);
    ThemeIconToggleMeasureResult result;
    if (Single(toggle.GetVariant())) {
        result.width = Css(32.0f, scale);
        result.height = Css(32.0f, scale);
    } else {
        const bool small = toggle.GetSize() == ThemeIconToggleSize::Small;
        const float segment = Css(small ? 36.0f : 44.0f, scale);
        result.width = Css(4.0f, scale) + segment * 2.0f;
        result.height = Css(small ? 36.0f : 44.0f, scale);
    }
    result.advanceY = result.height + Css(12.0f, scale);
    return result;
}

ThemeIconTogglePaintResult ThemeIconTogglePainter::Paint(FrameBuffer& buffer,
                                                         ThemeIconToggle& toggle,
                                                         float originX,
                                                         float originY,
                                                         float pixelScale) {
    const float scale = std::max(0.5f, pixelScale);
    const float opacity = toggle.IsDisabled() ? 0.55f : 1.0f;
    const float mix = toggle.ThemeMix();
    const ThemeIconToggleMeasureResult measured = Measure(toggle, scale);
    const Color primary = LerpColor(TokenOn("#1D1D1F", "color.text.primary", Theme::Light), TokenOn("#1D1D1F", "color.text.primary", Theme::Dark), mix);
    const Color secondary = LerpColor(TokenOn("#6E6E73", "color.text.secondary", Theme::Light), TokenOn("#6E6E73", "color.text.secondary", Theme::Dark), mix);
    const Color hover = LerpColor(TokenOn("#F5F5F7", "color.bg.hover", Theme::Light), TokenOn("#F5F5F7", "color.bg.hover", Theme::Dark), mix);
    const Color bgSecondary = LerpColor(TokenOn("#F5F5F7", "color.bg.secondary", Theme::Light), TokenOn("#F5F5F7", "color.bg.secondary", Theme::Dark), mix);
    const Color tertiary = LerpColor(TokenOn("#EDEDEF", "color.bg.tertiary", Theme::Light), TokenOn("#EDEDEF", "color.bg.tertiary", Theme::Dark), mix);
    const Color elevated = LerpColor(TokenOn("#FFFFFF", "color.bg.elevated", Theme::Light), TokenOn("#FFFFFF", "color.bg.elevated", Theme::Dark), mix);
    const Color border = LerpColor(TokenOn("#D2D2D7", "color.border.default", Theme::Light), TokenOn("#D2D2D7", "color.border.default", Theme::Dark), mix);
    const Color white = Color::FromHex("#FFFFFF");
    const Color lightLifted = TokenOn("#FFFFFF", "color.bg.elevated", Theme::Light);
    const Color darkLifted = Mix(TokenOn("#EDEDEF", "color.bg.tertiary", Theme::Dark), white, 0.28f);
    const Color lifted = LerpColor(lightLifted, darkLifted, mix);

    if (Single(toggle.GetVariant())) {
        const bool hot = !toggle.IsDisabled() && toggle.GetHot() != ThemeIconHot::None;
        const float radius = measured.width * 0.5f;
        if (toggle.GetVariant() == ThemeIconToggleVariant::Subtle || hot) {
            const Color fill = hot ? hover : bgSecondary;
            buffer.FillRoundedRect(originX, originY, measured.width, measured.height, radius, fill, opacity);
        }
        if (toggle.GetVariant() == ThemeIconToggleVariant::Outline) {
            StrokeRound(buffer, originX, originY, measured.width, measured.height, radius, border, std::max(1.0f, scale), opacity);
        }
        const float iconCss = toggle.GetSize() == ThemeIconToggleSize::Small ? 15.0f : 16.0f;
        const float icon = Css(iconCss, scale);
        const Color ink = hot ? primary : secondary;
        const float iconX = originX + (measured.width - icon) * 0.5f;
        const float iconY = originY + (measured.height - icon) * 0.5f;
        if (mix < 0.999f) {
            PaintAppearanceMark(buffer, AppearanceMark::Moon, iconX, iconY, iconCss, 1.6f, ink, opacity * (1.0f - mix), scale);
        }
        if (mix > 0.001f) {
            PaintAppearanceMark(buffer, AppearanceMark::Sun, iconX, iconY, iconCss, 1.6f, ink, opacity * mix, scale);
        }
        toggle.SetBounds(Box(originX, originY, measured.width, measured.height));
        toggle.SetSegmentBounds({}, {});
    } else {
        const bool small = toggle.GetSize() == ThemeIconToggleSize::Small;
        const float pad = Css(2.0f, scale);
        const float trackRadius = toggle.GetVariant() == ThemeIconToggleVariant::Pair ? measured.height * 0.5f : Css(small ? 10.0f : 12.0f, scale);
        const float thumbRadius = toggle.GetVariant() == ThemeIconToggleVariant::Pair ? (measured.height - pad * 2.0f) * 0.5f : Css(small ? 8.0f : 10.0f, scale);
        Color track = tertiary;
        Color thumb = lifted;
        if (toggle.GetVariant() == ThemeIconToggleVariant::Segmented) {
            track = Color::FromRgba(0, 0, 0, 0);
            thumb = LerpColor(TokenOn("#F5F5F7", "color.bg.secondary", Theme::Light), darkLifted, mix);
        } else if (toggle.GetVariant() == ThemeIconToggleVariant::Split) {
            track = Color::FromRgba(0, 0, 0, 0);
            thumb = LerpColor(TokenOn("#EDEDEF", "color.bg.tertiary", Theme::Light), darkLifted, mix);
        }
        if (track.alpha > 0) {
            buffer.FillRoundedRect(originX, originY, measured.width, measured.height, trackRadius, track, opacity);
        }
        if (toggle.GetVariant() == ThemeIconToggleVariant::Segmented) {
            StrokeRound(buffer, originX, originY, measured.width, measured.height, trackRadius, border, std::max(1.0f, scale), opacity);
        }
        const float thumbWidth = (measured.width - pad * 2.0f) * 0.5f;
        const float thumbHeight = measured.height - pad * 2.0f;
        const float thumbX = originX + pad + thumbWidth * mix;
        buffer.FillRoundedRect(thumbX, originY + pad, thumbWidth, thumbHeight, thumbRadius, thumb, opacity);

        const float iconCss = small ? 15.0f : 16.0f;
        const float icon = Css(iconCss, scale);
        const float segmentWidth = thumbWidth;
        const ThemeIconHot hot = toggle.IsDisabled() ? ThemeIconHot::None : toggle.GetHot();
        const float iconY = originY + (measured.height - icon) * 0.5f;
        const Color sunInk = LerpColor(primary, hot == ThemeIconHot::Light ? primary : secondary, mix);
        const Color moonInk = LerpColor(hot == ThemeIconHot::Dark ? primary : secondary, primary, mix);
        PaintAppearanceMark(buffer, AppearanceMark::Sun, originX + pad + (segmentWidth - icon) * 0.5f, iconY, iconCss, 1.6f, sunInk, opacity, scale);
        PaintAppearanceMark(buffer, AppearanceMark::Moon, originX + pad + segmentWidth + (segmentWidth - icon) * 0.5f, iconY, iconCss, 1.6f, moonInk, opacity, scale);
        toggle.SetBounds(Box(originX, originY, measured.width, measured.height));
        toggle.SetSegmentBounds(Box(originX, originY, measured.width * 0.5f, measured.height),
                                Box(originX + measured.width * 0.5f, originY, measured.width * 0.5f, measured.height));
    }

    ThemeIconTogglePaintResult result;
    result.advanceY = measured.advanceY;
    return result;
}

}  // namespace Robo
