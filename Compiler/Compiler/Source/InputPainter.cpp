#include "Robo/InputPainter.hpp"
#include "Robo/Foundation.hpp"
#include "Robo/TextSelection.hpp"
#include "Robo/TextPainter.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace Robo {

namespace {

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
    if (width <= 0.0f || height <= 0.0f || color.alpha == 0) {
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

void StrokeRoundedRect(FrameBuffer& buffer,
                       float x,
                       float y,
                       float width,
                       float height,
                       float radius,
                       float strokeWidth,
                       const Color& color,
                       float opacity) {
    if (width <= 0.0f || height <= 0.0f || strokeWidth <= 0.0f) {
        return;
    }

    const float cappedRadius = std::max(0.0f, std::min({radius, width * 0.5f, height * 0.5f}));
    const float half = strokeWidth * 0.5f;
    const int x0 = static_cast<int>(std::floor(x - half));
    const int y0 = static_cast<int>(std::floor(y - half));
    const int x1 = static_cast<int>(std::ceil(x + width + half));
    const int y1 = static_cast<int>(std::ceil(y + height + half));
    const float left = x + cappedRadius;
    const float right = x + width - cappedRadius;
    const float top = y + cappedRadius;
    const float bottom = y + height - cappedRadius;

    auto DistanceToRoundedRect = [&](float fx, float fy) -> float {
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
            return std::sqrt(dx * dx + dy * dy) - cappedRadius;
        }

        const float dx = std::max(x - fx, fx - (x + width));
        const float dy = std::max(y - fy, fy - (y + height));
        return std::max(dx, dy);
    };

    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            const float fx = static_cast<float>(px) + 0.5f;
            const float fy = static_cast<float>(py) + 0.5f;
            const float distance = DistanceToRoundedRect(fx, fy);
            if (std::abs(distance) <= half) {
                const float edge = 1.0f - std::min(1.0f, std::abs(distance) / std::max(0.5f, half));
                buffer.BlendPixel(px, py, color, opacity * edge);
            }
        }
    }
}

void DrawPlainUnderline(FrameBuffer& buffer,
                        float x,
                        float y,
                        float width,
                        float thickness,
                        const Color& color,
                        float opacity) {
    buffer.FillRect(static_cast<int>(std::floor(x)),
                    static_cast<int>(std::floor(y)),
                    std::max(1, static_cast<int>(std::ceil(width))),
                    std::max(1, static_cast<int>(std::ceil(thickness))),
                    color,
                    opacity);
}

}  // namespace

InputMeasureResult InputPainter::Measure(const FontEngine& fonts,
                                        const Input& input,
                                        float containerWidth,
                                        float pixelScale) {
    const float scale = std::max(0.5f, pixelScale);
    if (input.HasMeasureCache(containerWidth, scale)) {
        const Input::MeasureCache& cached = input.GetMeasureCache();
        InputMeasureResult result;
        result.width = cached.width;
        result.height = cached.height;
        result.advanceY = cached.advanceY;
        return result;
    }
    InputStyle style = input.GetStyle();
    Resolve(style);
    const FontFaceId faceId = fonts.ResolveFace(style.fontWeight, style.fontStyle);
    const float fontSize = static_cast<float>(style.fontSize) * scale;

    const float ascent = fonts.GetAscent(fontSize, faceId);
    const float descent = fonts.GetDescent(fontSize, faceId);
    const float textHeight = std::max(fontSize, ascent + descent);
    const float paddingY = static_cast<float>(style.paddingY) * scale;

    const float logicalContainer = containerWidth / scale;
    float fieldWidth;
    if (style.hasWidthLength) {
        fieldWidth = style.widthLength.Resolve(logicalContainer, 1.0f) * scale;
    } else {
        fieldWidth = TextPainter::ResolveWidthValue(style.width, logicalContainer) * scale;
    }
    fieldWidth = std::max(fieldWidth, static_cast<float>(style.minWidth) * scale);
    if (containerWidth > 0.0f) {
        fieldWidth = std::min(fieldWidth, containerWidth);
    }
    const float fieldHeight = textHeight + paddingY * 2.0f;

    Input::MeasureCache cache;
    cache.containerWidth = containerWidth;
    cache.pixelScale = scale;
    cache.width = fieldWidth;
    cache.height = fieldHeight;
    cache.advanceY = fieldHeight + 16.0f * scale;
    cache.ascent = ascent;
    cache.textHeight = textHeight;
    cache.valid = true;
    input.StoreMeasureCache(std::move(cache));

    InputMeasureResult result;
    result.width = fieldWidth;
    result.height = fieldHeight;
    result.advanceY = fieldHeight + 16.0f * scale;
    return result;
}

InputPaintResult InputPainter::Paint(FrameBuffer& buffer,
                                     const FontEngine& fonts,
                                     Input& input,
                                     float originX,
                                     float originY,
                                     float containerWidth,
                                     float pixelScale,
                                     float forcedWidth,
                                     float forcedHeight) {
    const SelectableScope selection(&input, input.GetStyle().selectable);
    const float scale = std::max(0.5f, pixelScale);
    InputPainter::Measure(fonts, input, containerWidth, scale);
    InputStyle style = input.GetStyle();
    Resolve(style);
    const Input::MeasureCache& measured = input.GetMeasureCache();
    const FontFaceId faceId = fonts.ResolveFace(style.fontWeight, style.fontStyle);
    const float fontSize = static_cast<float>(style.fontSize) * scale;
    const float letterSpacing = static_cast<float>(style.letterSpacing) * scale;
    const float opacity = input.ResolveOpacity();
    const Color backgroundColor = input.ResolveBackgroundColor();
    const Color borderColor = input.ResolveBorderColor();
    const float borderWidth = input.ResolveBorderWidth() * scale;

    const float ascent = measured.ascent;
    const float textHeight = measured.textHeight;
    const float paddingX = static_cast<float>(style.paddingX) * scale;
    const float paddingY = static_cast<float>(style.paddingY) * scale;

    const float naturalWidth = measured.width;
    const float naturalHeight = measured.height;

    const float fieldWidth = forcedWidth > 0.0f ? forcedWidth : naturalWidth;
    const float fieldHeight = forcedHeight > 0.0f ? forcedHeight : naturalHeight;

    float fieldX = originX;
    if (forcedWidth <= 0.0f) {
        if (style.align == TextAlign::Center) {
            fieldX = originX + std::max(0.0f, (containerWidth - fieldWidth) * 0.5f);
        } else if (style.align == TextAlign::Right) {
            fieldX = originX + std::max(0.0f, containerWidth - fieldWidth);
        }
    }
    const float fieldY = originY;

    if (backgroundColor.alpha > 0) {
        FillRoundedRect(buffer,
                        fieldX,
                        fieldY,
                        fieldWidth,
                        fieldHeight,
                        static_cast<float>(style.cornerRadius) * scale,
                        backgroundColor,
                        opacity);
    }

    if (style.variant == InputVariant::Plain) {
        const Color lineColor =
            input.GetInteraction() == InputInteraction::Focused ? style.focusBorderColor : borderColor;
        const float thickness =
            input.GetInteraction() == InputInteraction::Focused ? 2.0f * scale : 1.0f * scale;
        DrawPlainUnderline(buffer,
                           fieldX,
                           fieldY + fieldHeight - thickness,
                           fieldWidth,
                           thickness,
                           lineColor,
                           opacity);
    } else if (borderWidth > 0.0f) {
        StrokeRoundedRect(buffer,
                          fieldX,
                          fieldY,
                          fieldWidth,
                          fieldHeight,
                          static_cast<float>(style.cornerRadius) * scale,
                          borderWidth,
                          borderColor,
                          opacity);
    }

    const std::string& value = input.GetValue();
    const bool showPlaceholder = value.empty();
    const std::string& display = showPlaceholder ? input.GetPlaceholder() : value;
    const Color textColor = showPlaceholder ? style.placeholderColor : style.textColor;

    const float textX = fieldX + paddingX;
    const float baseline = fieldY + paddingY + ascent;
    const float maxTextWidth = std::max(0.0f, fieldWidth - paddingX * 2.0f);

    float caretX = textX;
    if (!display.empty()) {
        std::string visible;
        float caretOffset = 0.0f;
        if (!showPlaceholder && input.HasCaretCache(maxTextWidth, scale)) {
            const Input::CaretCache& cached = input.GetCaretCache();
            visible = cached.visible;
            caretOffset = cached.caretOffset;
        } else {
            visible = display;
            float textWidth = fonts.MeasureWidth(visible, fontSize, faceId, letterSpacing);
            if (textWidth > maxTextWidth && !showPlaceholder) {
                while (!visible.empty() && textWidth > maxTextWidth) {
                    visible = visible.substr(1);
                    textWidth = fonts.MeasureWidth(visible, fontSize, faceId, letterSpacing);
                }
            }
            if (!showPlaceholder) {
                const std::size_t caret = input.GetCaretIndex();
                const std::string before = value.substr(0, std::min(caret, value.size()));
                float beforeWidth = fonts.MeasureWidth(before, fontSize, faceId, letterSpacing);
                caretOffset = beforeWidth > maxTextWidth ? maxTextWidth : beforeWidth;
                Input::CaretCache cache;
                cache.maxTextWidth = maxTextWidth;
                cache.pixelScale = scale;
                cache.caretIndex = caret;
                cache.value = value;
                cache.visible = visible;
                cache.caretOffset = caretOffset;
                input.StoreCaretCache(std::move(cache));
            }
        }

        fonts.DrawString(buffer,
                         textX,
                         baseline,
                         visible,
                         fontSize,
                         faceId,
                         textColor,
                         opacity,
                         letterSpacing);

        if (!showPlaceholder) {
            caretX = textX + caretOffset;
        }
    }

    if (input.GetInteraction() == InputInteraction::Focused && !showPlaceholder) {
        const float caretHeight = textHeight * 0.9f;
        const float caretTop = baseline - ascent + (textHeight - caretHeight) * 0.5f;
        buffer.FillRect(static_cast<int>(std::floor(caretX)),
                        static_cast<int>(std::floor(caretTop)),
                        std::max(1, static_cast<int>(std::round(2.0f * scale))),
                        std::max(1, static_cast<int>(std::ceil(caretHeight))),
                        style.caretColor,
                        opacity);
    } else if (input.GetInteraction() == InputInteraction::Focused && showPlaceholder) {
        const float caretHeight = textHeight * 0.9f;
        const float caretTop = baseline - ascent + (textHeight - caretHeight) * 0.5f;
        buffer.FillRect(static_cast<int>(std::floor(textX)),
                        static_cast<int>(std::floor(caretTop)),
                        std::max(1, static_cast<int>(std::round(2.0f * scale))),
                        std::max(1, static_cast<int>(std::ceil(caretHeight))),
                        style.caretColor,
                        opacity);
    }

    InputPaintResult result;
    result.bounds.x = fieldX;
    result.bounds.y = fieldY;
    result.bounds.width = fieldWidth;
    result.bounds.height = fieldHeight;
    result.advanceY = fieldHeight + 16.0f * scale;
    input.SetBounds(result.bounds);
    return result;
}

}  // namespace Robo
