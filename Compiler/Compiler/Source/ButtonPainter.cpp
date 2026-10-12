#include "Robo/ButtonPainter.hpp"
#include "Robo/Foundation.hpp"
#include "Robo/Svg.hpp"
#include "Robo/SvgPainter.hpp"
#include "Robo/TextSelection.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace Robo {

namespace {

void StrokeRoundedRect(FrameBuffer& buffer, float x, float y, float width, float height, float radius, const Color& color, float thickness, float opacity) {
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

std::string CollapseToSingleLine(const std::string& input) {
    std::string output;
    output.reserve(input.size());
    bool pendingSpace = false;
    for (unsigned char ch : input) {
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\f' || ch == '\v') {
            pendingSpace = true;
            continue;
        }
        if (pendingSpace && !output.empty()) {
            output.push_back(' ');
        }
        pendingSpace = false;
        output.push_back(static_cast<char>(ch));
    }
    return output;
}

void FillRoundedRect(FrameBuffer& buffer,
                     float x,
                     float y,
                     float width,
                     float height,
                     float radius,
                     const Color& color,
                     float opacity) {
    buffer.FillRoundedRect(x, y, width, height, radius, color, opacity);
    return;
    if (width <= 0.0f || height <= 0.0f) {
        return;
    }

    const float cappedRadius = std::max(0.0f, std::min({radius, width * 0.5f, height * 0.5f}));
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const int x1 = static_cast<int>(std::ceil(x + width));
    const int y1 = static_cast<int>(std::ceil(y + height));
    const float left = x + cappedRadius;
    const float right = x + width - cappedRadius;
    const float top = y + cappedRadius;
    const float bottom = y + height - cappedRadius;
    const float radiusSq = cappedRadius * cappedRadius;

    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            const float fx = static_cast<float>(px) + 0.5f;
            const float fy = static_cast<float>(py) + 0.5f;
            if (fx < x || fx > x + width || fy < y || fy > y + height) {
                continue;
            }

            bool inside = true;
            if (cappedRadius > 0.0f) {
                float cx = fx;
                float cy = fy;
                if (fx < left && fy < top) {
                    cx = left;
                    cy = top;
                } else if (fx > right && fy < top) {
                    cx = right;
                    cy = top;
                } else if (fx < left && fy > bottom) {
                    cx = left;
                    cy = bottom;
                } else if (fx > right && fy > bottom) {
                    cx = right;
                    cy = bottom;
                }

                if (cx != fx || cy != fy) {
                    const float dx = fx - cx;
                    const float dy = fy - cy;
                    inside = (dx * dx + dy * dy) <= radiusSq;
                }
            }

            if (inside) {
                buffer.BlendPixel(px, py, color, opacity);
            }
        }
    }
}

}  // namespace

ButtonMeasureResult ButtonPainter::Measure(const FontEngine& fonts,
                                           const Button& button,
                                           float pixelScale) {
    const float scale = std::max(0.5f, pixelScale);
    ButtonStyle style = button.GetStyle();
    Resolve(style);
    const std::string label = CollapseToSingleLine(button.GetLabel());
    const FontFaceId faceId = fonts.ResolveFace(style.fontWeight, style.fontStyle);
    const float fontSize = static_cast<float>(style.fontSize) * scale;
    const float letterSpacing = static_cast<float>(style.letterSpacing) * scale;

    const bool showLabel = !button.IsIconOnly();
    const std::string visibleLabel = showLabel ? label : std::string();
    const float textWidth = visibleLabel.empty() ? 0.0f : fonts.MeasureWidth(visibleLabel, fontSize, faceId, letterSpacing);
    const float ascent = fonts.GetAscent(fontSize, faceId);
    const float descent = fonts.GetDescent(fontSize, faceId);
    const float textHeight = std::max(fontSize, ascent + descent);
    const float iconSize = button.HasIcon() ? 20.0f * scale : 0.0f;
    const float iconGap = button.HasIcon() && !visibleLabel.empty() ? 8.0f * scale : 0.0f;

    const float paddingX = static_cast<float>(style.paddingX) * scale;
    const float paddingY = static_cast<float>(style.paddingY) * scale;
    const float minTarget = 44.0f * scale;

    ButtonMeasureResult result;
    result.width = button.IsIconOnly() ? minTarget : std::max(minTarget, textWidth + iconSize + iconGap + paddingX * 2.0f);
    result.height = button.IsIconOnly() ? minTarget : std::max(minTarget, textHeight + paddingY * 2.0f);
    result.advanceY = result.height + 16.0f * scale;
    return result;
}

ButtonPaintResult ButtonPainter::Paint(FrameBuffer& buffer,
                                       const FontEngine& fonts,
                                       Button& button,
                                       float originX,
                                       float originY,
                                       float containerWidth,
                                       float pixelScale,
                                       float forcedWidth,
                                       float forcedHeight) {
    const SelectableScope selection(&button, button.GetStyle().selectable);
    const float scale = std::max(0.5f, pixelScale);
    ButtonStyle style = button.GetStyle();
    Resolve(style);
    const std::string label = CollapseToSingleLine(button.GetLabel());
    const FontFaceId faceId = fonts.ResolveFace(style.fontWeight, style.fontStyle);
    const float fontSize = static_cast<float>(style.fontSize) * scale;
    const float letterSpacing = static_cast<float>(style.letterSpacing) * scale;
    const float opacity = button.ResolveOpacity();
    const Color backgroundColor = button.ResolveBackgroundColor();
    const Color textColor = button.ResolveTextColor();

    const bool showLabel = !button.IsIconOnly();
    const std::string visibleLabel = showLabel ? label : std::string();
    const float textWidth = visibleLabel.empty() ? 0.0f : fonts.MeasureWidth(visibleLabel, fontSize, faceId, letterSpacing);
    const float ascent = fonts.GetAscent(fontSize, faceId);
    const float descent = fonts.GetDescent(fontSize, faceId);
    const float textHeight = std::max(fontSize, ascent + descent);
    const float iconSize = 20.0f * scale;
    const float iconGap = button.HasIcon() && !visibleLabel.empty() ? 8.0f * scale : 0.0f;
    const float contentWidth = (button.HasIcon() ? iconSize : 0.0f) + iconGap + textWidth;

    const float paddingX = static_cast<float>(style.paddingX) * scale;
    const float paddingY = static_cast<float>(style.paddingY) * scale;
    const float minTarget = 44.0f * scale;
    const float naturalWidth = button.IsIconOnly() ? minTarget : std::max(minTarget, contentWidth + paddingX * 2.0f);
    const float naturalHeight = button.IsIconOnly() ? minTarget : std::max(minTarget, textHeight + paddingY * 2.0f);

    const float buttonWidth = forcedWidth > 0.0f ? forcedWidth : naturalWidth;
    const float buttonHeight = forcedHeight > 0.0f ? forcedHeight : naturalHeight;

    float buttonX = originX;
    if (forcedWidth <= 0.0f) {
        if (style.align == TextAlign::Center) {
            buttonX = originX + std::max(0.0f, (containerWidth - buttonWidth) * 0.5f);
        } else if (style.align == TextAlign::Right) {
            buttonX = originX + std::max(0.0f, containerWidth - buttonWidth);
        }
    }

    const float buttonY = originY;
    ButtonPaintResult result;
    result.bounds.x = buttonX;
    result.bounds.y = buttonY;
    result.bounds.width = buttonWidth;
    result.bounds.height = buttonHeight;
    result.advanceY = buttonHeight + 16.0f * scale;
    button.SetBounds(result.bounds);
    buffer.Pump();
    if (buffer.MissesClip(static_cast<int>(std::floor(buttonX)) - 4,
                          static_cast<int>(std::floor(buttonY)) - 4,
                          static_cast<int>(std::ceil(buttonWidth)) + 8,
                          static_cast<int>(std::ceil(buttonHeight)) + 8)) {
        return result;
    }
    const float radius = static_cast<float>(style.cornerRadius) * scale;
    if (button.IsFocusVisible() && button.CanActivate()) {
        const float ring = 2.0f * scale;
        StrokeRoundedRect(buffer,
                          buttonX - ring,
                          buttonY - ring,
                          buttonWidth + ring * 2.0f,
                          buttonHeight + ring * 2.0f,
                          radius + ring,
                          button.ResolveFocusColor(),
                          ring,
                          1.0f);
    }

    if (backgroundColor.alpha > 0) {
        FillRoundedRect(buffer, buttonX, buttonY, buttonWidth, buttonHeight, radius, backgroundColor, opacity);
    }
    if (style.variant == ButtonVariant::Outline) {
        const Color border = ResolveColor(Color::FromHex("#D2D2D7"), "color.border.default");
        const float thickness = std::max(1.0f, std::round(scale));
        StrokeRoundedRect(buffer, buttonX, buttonY, buttonWidth, buttonHeight, radius, border, thickness, opacity);
    }

    float contentX = buttonX + (buttonWidth - contentWidth) * 0.5f;
    if (!button.IsIconOnly() && style.align == TextAlign::Left && buttonWidth > contentWidth + paddingX * 2.0f) {
        contentX = buttonX + paddingX;
    } else if (!button.IsIconOnly() && style.align == TextAlign::Right) {
        contentX = buttonX + buttonWidth - paddingX - contentWidth;
    }

    if (button.HasIcon()) {
        Svg& icon = button.GetIcon();
        icon.GetStyle().width = 20.0f;
        icon.GetStyle().height = 20.0f;
        icon.GetStyle().widthSpec.clear();
        icon.GetStyle().heightSpec.clear();
        icon.GetStyle().color = textColor;
        icon.GetStyle().opacity = opacity;
        const float iconY = buttonY + (buttonHeight - iconSize) * 0.5f;
        buffer.PushClipRect(buttonX, buttonY, buttonWidth, buttonHeight);
        SvgPainter::Paint(buffer, icon, contentX, iconY, iconSize, scale);
        buffer.PopClip();
        contentX += iconSize + iconGap;
    }

    if (!visibleLabel.empty()) {
        const float baseline = buttonY + (buttonHeight - textHeight) * 0.5f + ascent;
        fonts.DrawString(buffer, contentX, baseline, visibleLabel, fontSize, faceId, textColor, opacity, letterSpacing);
    }

    return result;
}

}  // namespace Robo
