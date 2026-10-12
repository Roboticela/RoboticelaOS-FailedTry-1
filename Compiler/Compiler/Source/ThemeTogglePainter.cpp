#include "Robo/ThemeTogglePainter.hpp"

#include "Robo/AppearanceGlyph.hpp"
#include "Robo/Foundation.hpp"
#include "Robo/Svg.hpp"
#include "Robo/SvgPainter.hpp"

#include <algorithm>
#include <vector>

namespace Robo {

namespace {

void AddLine(std::vector<SvgCommand>& commands, float x, float y) {
    commands.push_back(SvgCommand::Line(x, y));
}

void AddCircle(std::vector<SvgCommand>& commands, float cx, float cy, float radius) {
    const float k = radius * 0.55228475f;
    commands.push_back(SvgCommand::Move(cx + radius, cy));
    commands.push_back(SvgCommand::Cubic(cx + radius, cy + k, cx + k, cy + radius, cx, cy + radius));
    commands.push_back(SvgCommand::Cubic(cx - k, cy + radius, cx - radius, cy + k, cx - radius, cy));
    commands.push_back(SvgCommand::Cubic(cx - radius, cy - k, cx - k, cy - radius, cx, cy - radius));
    commands.push_back(SvgCommand::Cubic(cx + k, cy - radius, cx + radius, cy - k, cx + radius, cy));
    commands.push_back(SvgCommand::Close());
}

SvgShape StrokeOf(std::vector<SvgCommand> commands, float strokeWidth) {
    SvgShape shape;
    shape.commands = std::move(commands);
    shape.hasFill = false;
    shape.hasStroke = true;
    shape.currentStroke = true;
    shape.strokeWidth = strokeWidth;
    return shape;
}

struct ModeBox {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

struct ToggleLayout {
    float width = 0.0f;
    float height = 0.0f;
    ModeBox light;
    ModeBox dark;
    ModeBox track;
    bool stacked = false;
    bool showLabel = false;
    bool showIcon = false;
    bool compactTrack = false;
    float iconCss = 16.0f;
};

float Css(float value, float scale) {
    return value * scale;
}

float LabelWidth(const FontEngine& fonts, const char* text, float scale) {
    return fonts.MeasureWidth(text, Css(12.0f, scale), FontFaceId::Regular, 0.0f);
}

ToggleLayout LayoutOf(const FontEngine& fonts, const ThemeToggle& toggle, float availableWidth, float scale) {
    const ThemeToggleVariant variant = toggle.GetVariant();
    const bool stacked = variant == ThemeToggleVariant::Default;
    const bool switchOnly = variant == ThemeToggleVariant::Switch;
    const bool showLabel = variant != ThemeToggleVariant::Icons && !switchOnly;
    const bool showIcon = variant != ThemeToggleVariant::Labels && !switchOnly;
    const bool compactTrack = !stacked;
    const float iconCss = stacked ? 18.0f : 16.0f;
    const float lightText = showLabel ? LabelWidth(fonts, "Light", scale) : 0.0f;
    const float darkText = showLabel ? LabelWidth(fonts, "Dark", scale) : 0.0f;
    const float icon = showIcon ? Css(iconCss, scale) : 0.0f;
    const float rowGap = (showIcon && showLabel) ? Css(4.0f, scale) : 0.0f;

    ToggleLayout layout;
    layout.stacked = stacked;
    layout.showLabel = showLabel;
    layout.showIcon = showIcon;
    layout.compactTrack = compactTrack;
    layout.iconCss = iconCss;

    if (switchOnly) {
        layout.width = Css(64.0f, scale);
        layout.height = Css(28.0f, scale);
        layout.track = {0.0f, 0.0f, layout.width, layout.height};
        return layout;
    }

    float buttonWidth = 0.0f;
    float buttonHeight = 0.0f;
    if (stacked) {
        const float line = Css(12.0f * 1.33f, scale);
        buttonWidth = Css(48.0f, scale);
        buttonHeight = icon + rowGap + line;
    } else {
        const float text = std::max(lightText, darkText);
        buttonWidth = icon + rowGap + (showLabel ? (showIcon ? std::max(lightText, 0.0f) : 0.0f) : 0.0f);
        if (showLabel && !showIcon) {
            buttonWidth = 0.0f;
        }
        buttonHeight = Css(44.0f, scale);
        layout.light.width = showLabel && showIcon ? icon + rowGap + lightText : (showLabel ? lightText : icon);
        layout.dark.width = showLabel && showIcon ? icon + rowGap + darkText : (showLabel ? darkText : icon);
        if (!showLabel && !showIcon) {
            layout.light.width = 0.0f;
            layout.dark.width = 0.0f;
        }
        (void)buttonWidth;
        (void)text;
    }

    if (stacked) {
        const float padX = Css(8.0f, scale);
        const float padTop = Css(4.0f, scale);
        const float padBottom = Css(8.0f, scale);
        const float gap = Css(10.0f, scale);
        const float minimum = padX * 2.0f + buttonWidth * 2.0f + gap * 2.0f + Css(72.0f, scale);
        layout.width = availableWidth >= minimum && availableWidth < Css(4000.0f, scale) ? availableWidth : std::max(minimum, Css(280.0f, scale));
        const float trackWidth = std::max(Css(72.0f, scale), layout.width - padX * 2.0f - buttonWidth * 2.0f - gap * 2.0f);
        const float trackHeight = Css(34.0f, scale);
        const float rowHeight = std::max(buttonHeight, trackHeight);
        layout.height = padTop + rowHeight + padBottom;
        const float rowY = padTop + (rowHeight - buttonHeight) * 0.5f;
        const float trackY = padTop + (rowHeight - trackHeight) * 0.5f;
        layout.light = {padX, rowY, buttonWidth, buttonHeight};
        layout.track = {padX + buttonWidth + gap, trackY, trackWidth, trackHeight};
        layout.dark = {layout.track.x + trackWidth + gap, rowY, buttonWidth, buttonHeight};
        return layout;
    }

    const float gap = Css(8.0f, scale);
    const float trackWidth = Css(64.0f, scale);
    const float trackHeight = Css(28.0f, scale);
    layout.height = buttonHeight;
    layout.light.height = buttonHeight;
    layout.dark.height = buttonHeight;
    layout.light.y = 0.0f;
    layout.dark.y = (buttonHeight - layout.dark.height) * 0.5f;
    layout.light.x = 0.0f;
    layout.track = {layout.light.width + gap, (buttonHeight - trackHeight) * 0.5f, trackWidth, trackHeight};
    layout.dark.x = layout.track.x + trackWidth + gap;
    layout.width = layout.dark.x + layout.dark.width;
    return layout;
}

void DrawCaption(FrameBuffer& buffer,
                 const FontEngine& fonts,
                 const char* text,
                 float x,
                 float y,
                 float lineHeight,
                 const Color& color,
                 float opacity,
                 float scale) {
    const float fontSize = Css(12.0f, scale);
    const float ascent = fonts.GetAscent(fontSize, FontFaceId::Regular);
    const float descent = fonts.GetDescent(fontSize, FontFaceId::Regular);
    const float baseline = y + (lineHeight - (ascent + descent)) * 0.5f + ascent;
    fonts.DrawString(buffer, x, baseline, text, fontSize, FontFaceId::Regular, color, opacity, 0.0f);
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

void DrawMode(FrameBuffer& buffer,
              const FontEngine& fonts,
              const ModeBox& box,
              bool light,
              bool stacked,
              bool showIcon,
              bool showLabel,
              float iconCss,
              const Color& ink,
              float opacity,
              float scale) {
    if (box.width <= 0.0f) {
        return;
    }
    const float icon = showIcon ? Css(iconCss, scale) : 0.0f;
    const float gap = (showIcon && showLabel) ? Css(4.0f, scale) : 0.0f;
    const char* label = light ? "Light" : "Dark";
    const float textWidth = showLabel ? fonts.MeasureWidth(label, Css(12.0f, scale), FontFaceId::Regular, 0.0f) : 0.0f;
    const float line = Css(stacked ? 12.0f * 1.33f : 12.0f, scale);
    if (stacked) {
        const float stack = icon + gap + line;
        const float top = box.y + (box.height - stack) * 0.5f;
        if (showIcon) {
            PaintAppearanceMark(buffer, light ? AppearanceMark::Sun : AppearanceMark::Moon, box.x + (box.width - icon) * 0.5f, top, iconCss, 1.5f, ink, opacity, scale);
        }
        if (showLabel) {
            DrawCaption(buffer, fonts, label, box.x + (box.width - textWidth) * 0.5f, top + icon + gap, line, ink, opacity, scale);
        }
        return;
    }
    const float group = icon + gap + textWidth;
    float cursor = box.x + (box.width - group) * 0.5f;
    if (showIcon) {
        PaintAppearanceMark(buffer, light ? AppearanceMark::Sun : AppearanceMark::Moon, cursor, box.y + (box.height - icon) * 0.5f, iconCss, 1.5f, ink, opacity, scale);
        cursor += icon + gap;
    }
    if (showLabel) {
        DrawCaption(buffer, fonts, label, cursor, box.y + (box.height - line) * 0.5f, line, ink, opacity, scale);
    }
}

void DrawSwitch(FrameBuffer& buffer, const ModeBox& track, float mix, bool compact, float opacity) {
    const Color off = TokenOn("#D2D2D7", "color.border.default", Theme::Light);
    const Color brand = TokenOn("#0E7AFF", "color.brand.primary", Theme::Dark);
    const Color trackColor = LerpColor(off, brand, mix);
    buffer.FillRoundedRect(track.x, track.y, track.width, track.height, track.height * 0.5f, trackColor, opacity);

    const float inset = compact ? 3.0f : 4.0f;
    const float knob = compact ? 22.0f : 26.0f;
    const float parked = compact ? 25.0f : 30.0f;
    const float scale = track.height / (compact ? 28.0f : 34.0f);
    const float knobSize = knob * scale;
    const float knobX = track.x + inset * scale + (track.width - parked * scale - inset * scale) * mix;
    const float knobY = track.y + inset * scale;
    const Color knobColor = ResolveColor(Color::FromHex("#FFFFFF"), "color.text.on-brand");
    buffer.FillRoundedRect(knobX, knobY, knobSize, knobSize, knobSize * 0.5f, knobColor, opacity);
    const float dot = (compact ? 5.0f : 6.0f) * scale;
    const Color dotColor = ResolveColor(Color::FromHex("#C7C7CC"), "color.text.disabled");
    buffer.FillRoundedRect(knobX + (knobSize - dot) * 0.5f, knobY + (knobSize - dot) * 0.5f, dot, dot, dot * 0.5f, dotColor, opacity);
}

ModeBox Shift(ModeBox box, float x, float y) {
    box.x += x;
    box.y += y;
    return box;
}

ThemeToggleBounds ToBounds(const ModeBox& box) {
    ThemeToggleBounds bounds;
    bounds.x = box.x;
    bounds.y = box.y;
    bounds.width = box.width;
    bounds.height = box.height;
    return bounds;
}

}  // namespace

void PaintAppearanceMark(FrameBuffer& buffer,
                         AppearanceMark mark,
                         float x,
                         float y,
                         float cssSize,
                         float strokeCss,
                         const Color& color,
                         float opacity,
                         float pixelScale) {
    Svg svg;
    SvgStyle style;
    style.viewX = 0.0f;
    style.viewY = 0.0f;
    style.viewWidth = 24.0f;
    style.viewHeight = 24.0f;
    style.width = cssSize;
    style.height = cssSize;
    style.color = color;
    style.opacity = opacity;
    svg.SetStyle(style);
    const float stroke = strokeCss * 24.0f / std::max(1.0f, cssSize);
    if (mark == AppearanceMark::Sun) {
        std::vector<SvgCommand> circle;
        AddCircle(circle, 12.0f, 12.0f, 3.5f);
        svg.AddShape(StrokeOf(std::move(circle), stroke));
        std::vector<SvgCommand> rays;
        const float pairs[8][4] = {
            {12.0f, 3.0f, 12.0f, 5.5f},
            {12.0f, 18.5f, 12.0f, 21.0f},
            {3.0f, 12.0f, 5.5f, 12.0f},
            {18.5f, 12.0f, 21.0f, 12.0f},
            {5.6f, 5.6f, 7.5f, 7.5f},
            {16.5f, 16.5f, 18.4f, 18.4f},
            {18.4f, 5.6f, 16.5f, 7.5f},
            {7.5f, 16.5f, 5.6f, 18.4f},
        };
        for (const auto& ray : pairs) {
            rays.push_back(SvgCommand::Move(ray[0], ray[1]));
            AddLine(rays, ray[2], ray[3]);
        }
        svg.AddShape(StrokeOf(std::move(rays), stroke));
    } else {
        std::vector<SvgCommand> moon;
        moon.push_back(SvgCommand::Move(16.0f, 5.2f));
        moon.push_back(SvgCommand::Cubic(14.2f, 6.4f, 13.0f, 8.5f, 13.0f, 11.0f));
        moon.push_back(SvgCommand::Cubic(13.0f, 15.1f, 16.4f, 18.5f, 20.5f, 18.5f));
        moon.push_back(SvgCommand::Cubic(18.4f, 20.6f, 15.5f, 21.5f, 12.5f, 21.5f));
        moon.push_back(SvgCommand::Cubic(7.8f, 21.5f, 4.0f, 17.7f, 4.0f, 13.0f));
        moon.push_back(SvgCommand::Cubic(4.0f, 8.3f, 7.8f, 4.5f, 12.5f, 4.5f));
        moon.push_back(SvgCommand::Cubic(13.8f, 4.5f, 15.0f, 4.8f, 16.0f, 5.2f));
        moon.push_back(SvgCommand::Close());
        svg.AddShape(StrokeOf(std::move(moon), stroke));
    }
    SvgPainter::Paint(buffer, svg, x, y, cssSize * pixelScale, pixelScale);
}

ThemeToggleMeasureResult ThemeTogglePainter::Measure(const FontEngine& fonts,
                                                     const ThemeToggle& toggle,
                                                     float availableWidth,
                                                     float pixelScale) {
    const float scale = std::max(0.5f, pixelScale);
    const ToggleLayout layout = LayoutOf(fonts, toggle, availableWidth, scale);
    ThemeToggleMeasureResult result;
    result.width = layout.width;
    result.height = layout.height;
    result.advanceY = layout.height + Css(12.0f, scale);
    return result;
}

ThemeTogglePaintResult ThemeTogglePainter::Paint(FrameBuffer& buffer,
                                                 const FontEngine& fonts,
                                                 ThemeToggle& toggle,
                                                 float originX,
                                                 float originY,
                                                 float availableWidth,
                                                 float pixelScale) {
    const float scale = std::max(0.5f, pixelScale);
    const ToggleLayout layout = LayoutOf(fonts, toggle, availableWidth, scale);
    const float opacity = toggle.IsDisabled() ? 0.55f : 1.0f;
    const float mix = toggle.ThemeMix();
    const Color lightInk = LerpColor(TokenOn("#1D1D1F", "color.text.primary", Theme::Light),
                                     TokenOn("#86868B", "color.text.tertiary", Theme::Dark),
                                     mix);
    const Color darkInk = LerpColor(TokenOn("#86868B", "color.text.tertiary", Theme::Light),
                                    TokenOn("#1D1D1F", "color.text.primary", Theme::Dark),
                                    mix);
    if (layout.light.width > 0.0f) {
        DrawMode(buffer, fonts, Shift(layout.light, originX, originY), true, layout.stacked, layout.showIcon, layout.showLabel, layout.iconCss, lightInk, opacity, scale);
        DrawMode(buffer, fonts, Shift(layout.dark, originX, originY), false, layout.stacked, layout.showIcon, layout.showLabel, layout.iconCss, darkInk, opacity, scale);
    }
    DrawSwitch(buffer, Shift(layout.track, originX, originY), mix, layout.compactTrack, opacity);

    ThemeToggleBounds bounds;
    bounds.x = originX;
    bounds.y = originY;
    bounds.width = layout.width;
    bounds.height = layout.height;
    toggle.SetBounds(bounds);
    toggle.SetPartBounds(ThemeTogglePart::Light, ToBounds(Shift(layout.light, originX, originY)));
    toggle.SetPartBounds(ThemeTogglePart::Dark, ToBounds(Shift(layout.dark, originX, originY)));
    toggle.SetPartBounds(ThemeTogglePart::Switch, ToBounds(Shift(layout.track, originX, originY)));

    ThemeTogglePaintResult result;
    result.advanceY = layout.height + Css(12.0f, scale);
    return result;
}

}  // namespace Robo
