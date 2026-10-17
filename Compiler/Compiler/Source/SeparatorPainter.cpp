#include "Robo/SeparatorPainter.hpp"
#include "Robo/Foundation.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace Robo {

namespace {

float Css(float value, float scale) {
    return value * std::max(0.5f, scale);
}

float MarginCss(const Separator& separator) {
    if (separator.IsFlush()) {
        return 0.0f;
    }
    if (separator.GetSize() == SeparatorSize::Small) {
        return 16.0f;
    }
    if (separator.GetSize() == SeparatorSize::Large) {
        return 32.0f;
    }
    return 24.0f;
}

float LabelCss(const Separator& separator) {
    if (separator.GetSize() == SeparatorSize::Small) {
        return 11.0f;
    }
    if (separator.GetSize() == SeparatorSize::Large) {
        return 13.0f;
    }
    return 12.0f;
}

float GapCss(const Separator& separator) {
    return separator.GetSize() == SeparatorSize::Small ? 12.0f : 16.0f;
}

Color ToneColor(SeparatorTone tone, bool quiet) {
    const char* token = "color.border.default";
    Color fallback = Color::FromHex("#D2D2D7");
    switch (tone) {
        case SeparatorTone::Brand:
            token = "color.brand.primary";
            fallback = Color::FromHex("#0E7AFF");
            break;
        case SeparatorTone::Success:
            token = "color.success";
            fallback = Color::FromHex("#30D158");
            break;
        case SeparatorTone::Warning:
            token = "color.warning";
            fallback = Color::FromHex("#FF9F0A");
            break;
        case SeparatorTone::Danger:
            token = "color.danger";
            fallback = Color::FromHex("#FF3B30");
            break;
        case SeparatorTone::Default:
            if (quiet) {
                token = "color.border.subtle";
                fallback = Color::FromHex("#E5E5E7");
            }
            break;
    }
    Color color = ResolveColor(fallback, token);
    if (quiet && tone != SeparatorTone::Default) {
        color.alpha = static_cast<std::uint8_t>(std::round(color.alpha * 0.4f));
    }
    return color;
}

Color LabelColor(SeparatorTone tone, bool disabled) {
    if (disabled) {
        return ResolveColor(Color::FromHex("#C7C7CC"), "color.text.disabled");
    }
    switch (tone) {
        case SeparatorTone::Brand:
            return ResolveColor(Color::FromHex("#0E7AFF"), "color.brand.primary");
        case SeparatorTone::Success:
            return ResolveColor(Color::FromHex("#30D158"), "color.success");
        case SeparatorTone::Warning:
            return ResolveColor(Color::FromHex("#FF9F0A"), "color.warning");
        case SeparatorTone::Danger:
            return ResolveColor(Color::FromHex("#FF3B30"), "color.danger");
        case SeparatorTone::Default:
            return ResolveColor(Color::FromHex("#6E6E73"), "color.text.secondary");
    }
    return ResolveColor(Color::FromHex("#6E6E73"), "color.text.secondary");
}

enum class FadeEdge { None, In, Out, Both };

float FadeAlpha(float t, FadeEdge edge) {
    const float clamped = std::max(0.0f, std::min(1.0f, t));
    if (edge == FadeEdge::In) {
        return clamped;
    }
    if (edge == FadeEdge::Out) {
        return 1.0f - clamped;
    }
    if (edge == FadeEdge::Both) {
        const float dist = std::abs(clamped - 0.5f) * 2.0f;
        if (dist < 0.2f) {
            return 1.0f;
        }
        return std::max(0.0f, 1.0f - (dist - 0.2f) / 0.8f);
    }
    return 1.0f;
}

void DrawRule(FrameBuffer& buffer,
              float origin,
              float cross,
              float length,
              float thickness,
              bool horizontal,
              SeparatorVariant variant,
              FadeEdge fade,
              const Color& color,
              float opacity) {
    if (length <= 0.5f) {
        return;
    }
    const int thick = std::max(1, static_cast<int>(std::round(thickness)));
    const int span = std::max(1, static_cast<int>(std::round(length)));
    const int start = static_cast<int>(std::floor(origin));
    const int across = static_cast<int>(std::floor(cross - thickness * 0.5f));

    auto fill = [&](int along, int amount, float alpha) {
        if (amount <= 0 || alpha <= 0.001f) {
            return;
        }
        if (horizontal) {
            buffer.FillRect(along, across, amount, thick, color, opacity * alpha);
        } else {
            buffer.FillRect(across, along, thick, amount, color, opacity * alpha);
        }
    };

    if (variant == SeparatorVariant::Dashed) {
        const int dash = std::max(2, static_cast<int>(std::round(thickness > 1.5f ? 6.0f : 4.0f)));
        const int gap = std::max(3, static_cast<int>(std::round(6.0f)));
        for (int cursor = 0; cursor < span; cursor += dash + gap) {
            const float t = span <= 1 ? 1.0f : static_cast<float>(cursor) / static_cast<float>(span - 1);
            fill(start + cursor, std::min(dash, span - cursor), FadeAlpha(t, fade));
        }
        return;
    }

    if (variant == SeparatorVariant::Dotted) {
        const int dot = std::max(2, static_cast<int>(std::round(2.0f)));
        const int step = std::max(dot + 2, static_cast<int>(std::round(8.0f)));
        for (int cursor = 0; cursor < span; cursor += step) {
            const float t = span <= 1 ? 0.5f : static_cast<float>(cursor) / static_cast<float>(span - 1);
            const float radius = static_cast<float>(dot) * 0.5f;
            if (horizontal) {
                buffer.FillRoundedRect(static_cast<float>(start + cursor),
                                       cross - radius,
                                       static_cast<float>(dot),
                                       static_cast<float>(dot),
                                       radius,
                                       color,
                                       opacity * FadeAlpha(t, fade));
            } else {
                buffer.FillRoundedRect(cross - radius,
                                       static_cast<float>(start + cursor),
                                       static_cast<float>(dot),
                                       static_cast<float>(dot),
                                       radius,
                                       color,
                                       opacity * FadeAlpha(t, fade));
            }
        }
        return;
    }

    if (fade != FadeEdge::None || variant == SeparatorVariant::Fade) {
        const FadeEdge edge = fade == FadeEdge::None ? FadeEdge::Both : fade;
        for (int cursor = 0; cursor < span; ++cursor) {
            const float t = span <= 1 ? 1.0f : static_cast<float>(cursor) / static_cast<float>(span - 1);
            fill(start + cursor, 1, FadeAlpha(t, edge));
        }
        return;
    }

    fill(start, span, 1.0f);
}

}  // namespace

SeparatorMeasureResult SeparatorPainter::Measure(const FontEngine& fonts,
                                                 const Separator& separator,
                                                 float availableWidth,
                                                 float availableHeight,
                                                 float pixelScale) {
    const float scale = std::max(0.5f, pixelScale);
    const bool horizontal = separator.GetOrientation() == SeparatorOrientation::Horizontal;
    const float margin = Css(MarginCss(separator), scale);
    const float line = Css(separator.GetSize() == SeparatorSize::Large ? 2.0f : 1.0f, scale);
    const bool labeled = !separator.GetLabel().empty();
    float labelSize = 0.0f;
    if (labeled) {
        const float fontSize = Css(LabelCss(separator), scale);
        const FontFaceId face = fonts.ResolveFace(FontWeight::Medium, FontStyleKind::Normal);
        labelSize = std::max(fontSize, fonts.GetAscent(fontSize, face) + fonts.GetDescent(fontSize, face));
    }
    const float content = std::max(line, labelSize);

    SeparatorMeasureResult result;
    if (horizontal) {
        result.width = std::max(line, availableWidth);
        result.height = content + margin * 2.0f;
    } else {
        result.width = std::max(line, labeled ? content : line) + margin * 2.0f;
        const float minimum = Css(labeled ? 48.0f : 16.0f, scale);
        result.height = availableHeight > 1.0f ? std::max(minimum, availableHeight) : minimum;
    }
    return result;
}

SeparatorPaintResult SeparatorPainter::Paint(FrameBuffer& buffer,
                                             const FontEngine& fonts,
                                             const Separator& separator,
                                             float originX,
                                             float originY,
                                             float boxWidth,
                                             float boxHeight,
                                             float pixelScale) {
    const float scale = std::max(0.5f, pixelScale);
    const bool horizontal = separator.GetOrientation() == SeparatorOrientation::Horizontal;
    const float margin = Css(MarginCss(separator), scale);
    const float thickness = Css(separator.GetSize() == SeparatorSize::Large || separator.GetVariant() == SeparatorVariant::Dotted
                                    ? (separator.GetVariant() == SeparatorVariant::Dotted ? 3.0f : 2.0f)
                                    : 1.0f,
                                scale);
    const float opacity = separator.IsDisabled() ? 0.55f : 1.0f;
    const bool quiet = separator.GetVariant() == SeparatorVariant::Subtle;
    const Color color = ToneColor(separator.GetTone(), quiet);
    const std::string label = separator.GetLabel();
    const bool labeled = !label.empty();
    const float inset = horizontal && separator.IsInset() ? Css(16.0f, scale) : 0.0f;

    float boxX = originX + (horizontal ? inset : 0.0f);
    float boxY = originY;
    float boxW = std::max(1.0f, boxWidth - inset);
    float boxH = std::max(1.0f, boxHeight);

    const float contentX = boxX + (horizontal ? 0.0f : margin);
    const float contentY = boxY + (horizontal ? margin : 0.0f);
    const float contentW = std::max(1.0f, boxW - (horizontal ? 0.0f : margin * 2.0f));
    const float contentH = std::max(1.0f, boxH - (horizontal ? margin * 2.0f : 0.0f));
    const float midX = contentX + contentW * 0.5f;
    const float midY = contentY + contentH * 0.5f;

    auto rule = [&](float origin, float cross, float length, bool across, FadeEdge fade) {
        DrawRule(buffer, origin, cross, length, thickness, across, separator.GetVariant(), fade, color, opacity);
    };

    if (!labeled) {
        if (horizontal) {
            rule(contentX, midY, contentW, true, separator.GetVariant() == SeparatorVariant::Fade ? FadeEdge::Both : FadeEdge::None);
        } else {
            rule(contentY, midX, contentH, false, separator.GetVariant() == SeparatorVariant::Fade ? FadeEdge::Both : FadeEdge::None);
        }
    } else {
        const float fontSize = Css(LabelCss(separator), scale);
        const FontFaceId face = fonts.ResolveFace(FontWeight::Medium, FontStyleKind::Normal);
        const float letterSpacing = 0.0f;
        float textWidth = 0.0f;
        if (separator.HasLabelCache(fontSize)) {
            textWidth = separator.LabelCacheWidth();
        } else {
            textWidth = fonts.MeasureWidth(label, fontSize, face, letterSpacing);
            separator.StoreLabelCache(fontSize, textWidth);
        }
        const float ascent = fonts.GetAscent(fontSize, face);
        const float descent = fonts.GetDescent(fontSize, face);
        const float textHeight = std::max(fontSize, ascent + descent);
        const float gap = Css(GapCss(separator), scale);
        const SeparatorAlign align = separator.GetAlign();
        const bool fade = separator.GetVariant() == SeparatorVariant::Fade;

        if (horizontal) {
            const float limit = contentW * 0.7f;
            if (textWidth > limit) {
                textWidth = limit;
            }
            float textX = contentX + (contentW - textWidth) * 0.5f;
            if (align == SeparatorAlign::Start) {
                textX = contentX;
            } else if (align == SeparatorAlign::End) {
                textX = contentX + contentW - textWidth;
            }
            const float left = textX - contentX - gap;
            const float right = contentX + contentW - (textX + textWidth) - gap;
            if (align != SeparatorAlign::Start && left > 1.0f) {
                rule(contentX, midY, left, true, fade ? FadeEdge::In : FadeEdge::None);
            }
            if (align != SeparatorAlign::End && right > 1.0f) {
                rule(textX + textWidth + gap, midY, right, true, fade ? FadeEdge::Out : FadeEdge::None);
            }
            const float baseline = midY - textHeight * 0.5f + ascent;
            fonts.DrawString(buffer, textX, baseline, label, fontSize, face, LabelColor(separator.GetTone(), separator.IsDisabled()), opacity, letterSpacing, true);
        } else {
            const float limit = contentH * 0.7f;
            float textBlock = std::min(textHeight, limit);
            float textY = contentY + (contentH - textBlock) * 0.5f;
            if (align == SeparatorAlign::Start) {
                textY = contentY;
            } else if (align == SeparatorAlign::End) {
                textY = contentY + contentH - textBlock;
            }
            const float above = textY - contentY - gap;
            const float below = contentY + contentH - (textY + textBlock) - gap;
            if (align != SeparatorAlign::Start && above > 1.0f) {
                rule(contentY, midX, above, false, fade ? FadeEdge::In : FadeEdge::None);
            }
            if (align != SeparatorAlign::End && below > 1.0f) {
                rule(textY + textBlock + gap, midX, below, false, fade ? FadeEdge::Out : FadeEdge::None);
            }
            const float baseline = textY + (textBlock - textHeight) * 0.5f + ascent;
            const float textX = midX - textWidth * 0.5f;
            fonts.DrawString(buffer, textX, baseline, label, fontSize, face, LabelColor(separator.GetTone(), separator.IsDisabled()), opacity, letterSpacing, true);
        }
    }

    SeparatorPaintResult result;
    result.advanceY = boxH;
    result.width = boxW;
    result.height = boxH;
    return result;
}

}  // namespace Robo
