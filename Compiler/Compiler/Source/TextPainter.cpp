#include "Robo/TextPainter.hpp"
#include "Robo/Foundation.hpp"
#include "Robo/TextSelection.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

namespace Robo {

namespace {

struct ResolvedRun {
    std::string content;
    float fontSize = 16.0f;
    Color color;
    FontFaceId faceId = FontFaceId::Regular;
    TextDecoration decoration = TextDecoration::None;
    float letterSpacing = 0.0f;
    float opacity = 1.0f;
};

ResolvedRun ResolveRun(const TextRun& run, const TextStyle& style, const FontEngine& fonts, float pixelScale) {
    ResolvedRun resolved;
    resolved.content = run.content;
    resolved.fontSize = static_cast<float>(run.hasFontSize ? run.fontSize : style.fontSize) * pixelScale;
    resolved.color = run.hasColor ? run.color : style.color;
    const FontWeight weight = run.hasFontWeight ? run.fontWeight : style.fontWeight;
    const FontStyleKind fontStyle = run.hasFontStyle ? run.fontStyle : style.fontStyle;
    resolved.faceId = fonts.ResolveFace(weight, fontStyle);
    resolved.decoration = run.hasTextDecoration ? run.textDecoration : style.textDecoration;
    resolved.letterSpacing =
        static_cast<float>(run.hasLetterSpacing ? run.letterSpacing : style.letterSpacing) * pixelScale;
    resolved.opacity = static_cast<float>(run.hasOpacity ? run.opacity : style.opacity);
    return resolved;
}

bool IsXmlWhitespace(unsigned char ch) {
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\f' || ch == '\v';
}

std::string NormalizeXmlText(const std::string& input) {
    if (input.empty()) {
        return {};
    }

    bool hasNonWhitespace = false;
    for (unsigned char ch : input) {
        if (!IsXmlWhitespace(ch)) {
            hasNonWhitespace = true;
            break;
        }
    }
    if (!hasNonWhitespace) {
        return " ";
    }

    const bool leadSpace = IsXmlWhitespace(static_cast<unsigned char>(input.front()));
    const bool trailSpace = IsXmlWhitespace(static_cast<unsigned char>(input.back()));

    std::string output;
    output.reserve(input.size() + 2);
    bool pendingSpace = false;
    for (unsigned char ch : input) {
        if (IsXmlWhitespace(ch)) {
            pendingSpace = true;
            continue;
        }
        if (pendingSpace && !output.empty()) {
            output.push_back(' ');
        }
        pendingSpace = false;
        output.push_back(static_cast<char>(ch));
    }

    if (leadSpace) {
        output.insert(output.begin(), ' ');
    }
    if (trailSpace) {
        output.push_back(' ');
    }
    return output;
}

std::vector<std::string> SplitTokens(const std::string& text) {
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : text) {
        if (ch == ' ') {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            tokens.push_back(" ");
            continue;
        }
        current.push_back(ch);
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

TextLayoutSpan MakeSpan(const std::string& token, const ResolvedRun& run, float width, bool isSpace) {
    TextLayoutSpan span;
    span.text = token;
    span.width = width;
    span.isSpace = isSpace;
    span.fontSize = run.fontSize;
    span.color = run.color;
    span.faceId = run.faceId;
    span.decoration = run.decoration;
    span.letterSpacing = run.letterSpacing;
    span.opacity = run.opacity;
    return span;
}

void PushSpan(TextLayoutLine& line, const TextLayoutSpan& span) {
    if (span.text.empty()) {
        return;
    }
    line.spans.push_back(span);
    line.width += span.width;
}

void TrimLineEdges(TextLayoutLine& line) {
    while (!line.spans.empty() && line.spans.front().isSpace) {
        line.width -= line.spans.front().width;
        line.spans.erase(line.spans.begin());
    }
    while (!line.spans.empty() && line.spans.back().isSpace) {
        line.width -= line.spans.back().width;
        line.spans.pop_back();
    }
}

}  // namespace

float TextPainter::ResolveWidthValue(const std::string& value, float referenceWidth) {
    if (value.empty()) {
        return referenceWidth;
    }
    if (value == "100%") {
        return referenceWidth;
    }
    if (value.size() > 1 && value.back() == '%') {
        const float percent = static_cast<float>(std::atof(value.c_str()));
        return referenceWidth * (percent / 100.0f);
    }
    if (value.size() > 3 && value.compare(value.size() - 3, 3, "rem") == 0) {
        return static_cast<float>(std::atof(value.c_str()) * 16.0);
    }
    if (value.size() > 2 && value.compare(value.size() - 2, 2, "px") == 0) {
        return static_cast<float>(std::atof(value.c_str()));
    }
    return static_cast<float>(std::atof(value.c_str()));
}

float TextPainter::ResolveWidth(const TextStyle& style, float referenceWidth, float pixelScale) {
    const float scale = std::max(0.5f, pixelScale);
    const float logicalWidth = referenceWidth / scale;
    float blockWidth;
    if (style.hasWidthLength) {
        blockWidth = style.widthLength.Resolve(logicalWidth, 1.0f) * scale;
    } else {
        blockWidth = ResolveWidthValue(style.width, logicalWidth) * scale;
    }
    if (style.hasMaxWidthLength && !style.maxWidthLength.IsAuto()) {
        blockWidth = std::min(blockWidth, style.maxWidthLength.Resolve(logicalWidth, 1.0f) * scale);
    } else if (!style.maxWidth.empty()) {
        blockWidth = std::min(blockWidth, ResolveWidthValue(style.maxWidth, logicalWidth) * scale);
    }
    return std::max(1.0f, blockWidth);
}

const TextLayoutCache& TextPainter::EnsureLayout(const FontEngine& fonts,
                                                const Text& text,
                                                float containerWidth,
                                                float pixelScale) {
    const float scale = std::max(0.5f, pixelScale);
    TextLayoutCache& cache = text.LayoutCache();
    if (cache.valid && std::fabs(cache.containerWidth - containerWidth) < 0.5f &&
        std::fabs(cache.pixelScale - scale) < 0.001f && !text.NeedsLayout()) {
        return cache;
    }

    TextStyle style = text.GetStyle();
    Resolve(style);
    const float blockWidth = ResolveWidth(style, containerWidth, scale);

    std::vector<ResolvedRun> runs;
    for (const TextRun& run : text.GetRuns()) {
        if (run.content.empty()) {
            continue;
        }
        TextRun runStyle = run;
        Resolve(runStyle);
        ResolvedRun resolved = ResolveRun(runStyle, style, fonts, scale);
        resolved.content = NormalizeXmlText(run.content);
        if (!resolved.content.empty()) {
            runs.push_back(resolved);
        }
    }

    cache = TextLayoutCache{};
    cache.blockWidth = blockWidth;
    cache.align = style.align;
    cache.styleOpacity = static_cast<float>(style.opacity);
    cache.containerWidth = containerWidth;
    cache.pixelScale = scale;

    if (runs.empty()) {
        cache.height = static_cast<float>(style.fontSize) * static_cast<float>(style.lineHeight) * scale;
        cache.width = 0.0f;
        cache.advanceY = cache.height + 12.0f * scale;
        cache.baseLineHeight = cache.height;
        cache.valid = true;
        text.SetNeedsLayout(false);
        text.WriteMeasureCache(containerWidth, scale, cache.width, cache.height, cache.advanceY);
        return cache;
    }

    TextLayoutLine currentLine;

    auto BreakLongToken = [&](const std::string& token, const ResolvedRun& runStyle) {
        std::string built;
        for (unsigned char byte : token) {
            std::string candidate = built;
            candidate.push_back(static_cast<char>(byte));
            const float width =
                fonts.MeasureWidth(candidate, runStyle.fontSize, runStyle.faceId, runStyle.letterSpacing);
            if (!built.empty() && style.wrap && currentLine.width + width > blockWidth &&
                !currentLine.spans.empty()) {
                PushSpan(currentLine,
                         MakeSpan(built, runStyle,
                                  fonts.MeasureWidth(built, runStyle.fontSize, runStyle.faceId,
                                                     runStyle.letterSpacing),
                                  false));
                cache.lines.push_back(std::move(currentLine));
                currentLine = TextLayoutLine();
                built.assign(1, static_cast<char>(byte));
            } else {
                built = candidate;
            }
        }
        if (!built.empty()) {
            TextLayoutSpan span =
                MakeSpan(built, runStyle,
                         fonts.MeasureWidth(built, runStyle.fontSize, runStyle.faceId, runStyle.letterSpacing),
                         false);
            if (style.wrap && !currentLine.spans.empty() && currentLine.width + span.width > blockWidth) {
                cache.lines.push_back(std::move(currentLine));
                currentLine = TextLayoutLine();
            }
            PushSpan(currentLine, span);
        }
    };

    for (const ResolvedRun& run : runs) {
        const std::vector<std::string> tokens = SplitTokens(run.content);
        for (const std::string& token : tokens) {
            const bool isSpace = (token == " ");
            TextLayoutSpan span = MakeSpan(
                token, run, fonts.MeasureWidth(token, run.fontSize, run.faceId, run.letterSpacing), isSpace);

            if (!style.wrap) {
                PushSpan(currentLine, span);
                continue;
            }

            if (!span.isSpace && !currentLine.spans.empty() && currentLine.width + span.width > blockWidth) {
                cache.lines.push_back(std::move(currentLine));
                currentLine = TextLayoutLine();
            }

            if (!span.isSpace && span.width > blockWidth) {
                BreakLongToken(token, run);
            } else if (!(span.isSpace && currentLine.spans.empty())) {
                PushSpan(currentLine, span);
            }
        }
    }
    cache.lines.push_back(std::move(currentLine));

    float maxAscent = 0.0f;
    float maxDescent = 0.0f;
    for (const ResolvedRun& run : runs) {
        maxAscent = std::max(maxAscent, fonts.GetAscent(run.fontSize, run.faceId));
        maxDescent = std::max(maxDescent, fonts.GetDescent(run.fontSize, run.faceId));
    }
    if (maxAscent <= 0.0f) {
        maxAscent = static_cast<float>(style.fontSize) * 0.8f;
    }
    const float baseLineHeight =
        std::max(maxAscent + maxDescent, static_cast<float>(style.fontSize)) *
        static_cast<float>(style.lineHeight);

    float maxLineWidth = 0.0f;
    std::size_t nonEmptyLines = 0;
    for (TextLayoutLine& line : cache.lines) {
        TrimLineEdges(line);
        if (!line.spans.empty()) {
            ++nonEmptyLines;
            maxLineWidth = std::max(maxLineWidth, line.width);
        }
    }

    cache.maxAscent = maxAscent;
    cache.baseLineHeight = baseLineHeight;
    cache.width = maxLineWidth;
    cache.height = std::max(baseLineHeight, static_cast<float>(nonEmptyLines) * baseLineHeight);
    cache.advanceY = cache.height + 12.0f * scale;
    cache.valid = true;
    text.SetNeedsLayout(false);
    text.WriteMeasureCache(containerWidth, scale, cache.width, cache.height, cache.advanceY);
    return cache;
}

TextMeasureResult TextPainter::Measure(const FontEngine& fonts,
                                       const Text& text,
                                       float containerWidth,
                                       float pixelScale) {
    const TextLayoutCache& cache = EnsureLayout(fonts, text, containerWidth, pixelScale);
    TextMeasureResult result;
    result.width = cache.width;
    result.height = cache.height;
    result.advanceY = cache.advanceY;
    return result;
}

float TextPainter::Paint(FrameBuffer& buffer,
                         const FontEngine& fonts,
                         const Text& text,
                         float originX,
                         float originY,
                         float containerWidth,
                         float pixelScale) {
    const SelectableScope selection(&text, text.GetStyle().selectable);
    const float scale = std::max(0.5f, pixelScale);
    const TextLayoutCache& cache = EnsureLayout(fonts, text, containerWidth, scale);

    if (cache.lines.empty()) {
        return cache.advanceY;
    }

    const float blockWidth = cache.blockWidth;
    const float maxAscent = cache.maxAscent;
    const float baseLineHeight = cache.baseLineHeight;
    const float opacity = cache.styleOpacity;
    float cursorY = originY;

    // Retained capacity across paints — no mid-frame malloc growth after warm-up.
    // Single painter thread (AGENTS); avoid thread_local for freestanding kernels.
    static std::vector<TextGlyphDraw> paintDraws;

    for (std::size_t lineIndex = 0; lineIndex < cache.lines.size(); ++lineIndex) {
        const TextLayoutLine& line = cache.lines[lineIndex];
        if (line.spans.empty()) {
            continue;
        }

        float startX = originX;
        float extraGap = 0.0f;
        int gapCount = 0;
        const bool isLastLine = lineIndex + 1 >= cache.lines.size();

        if (cache.align == TextAlign::Center) {
            startX = originX + std::max(0.0f, (blockWidth - line.width) * 0.5f);
        } else if (cache.align == TextAlign::Right) {
            startX = originX + std::max(0.0f, blockWidth - line.width);
        } else if (cache.align == TextAlign::Justify && !isLastLine) {
            for (const TextLayoutSpan& span : line.spans) {
                if (span.isSpace) {
                    ++gapCount;
                }
            }
            if (gapCount > 0 && line.width >= blockWidth * 0.75f && blockWidth > line.width) {
                extraGap = (blockWidth - line.width) / static_cast<float>(gapCount);
            }
        }

        buffer.Pump();
        const int lineX = static_cast<int>(std::floor(originX));
        const int lineY = static_cast<int>(std::floor(cursorY));
        const int lineWidth = std::max(1, static_cast<int>(std::ceil(blockWidth)) + 2);
        const int lineHeight = std::max(1, static_cast<int>(std::ceil(baseLineHeight)) + 2);
        if (buffer.MissesClip(lineX, lineY, lineWidth, lineHeight)) {
            cursorY += baseLineHeight;
            continue;
        }

        float penX = startX;
        const float baseline = cursorY + maxAscent;
        paintDraws.clear();
        if (paintDraws.capacity() < line.spans.size() * 8) {
            paintDraws.reserve(line.spans.size() * 8);
        }

        for (const TextLayoutSpan& span : line.spans) {
            fonts.RecordString(paintDraws,
                               penX,
                               baseline,
                               span.text,
                               span.fontSize,
                               span.faceId,
                               span.color,
                               span.opacity * opacity,
                               span.letterSpacing);
            penX += span.width;
            if (span.isSpace) {
                penX += extraGap;
            }
        }

        FontEngine::PaintSelectionBands(buffer, paintDraws);
        fonts.PaintGlyphDraws(buffer, paintDraws);

        penX = startX;
        for (const TextLayoutSpan& span : line.spans) {
            if (span.decoration == TextDecoration::Underline) {
                const int y = static_cast<int>(std::round(baseline + std::max(1.0f, span.fontSize * 0.12f)));
                buffer.FillRect(static_cast<int>(std::floor(penX)),
                                y,
                                std::max(1, static_cast<int>(std::ceil(span.width))),
                                std::max(1, static_cast<int>(span.fontSize * 0.07f)),
                                span.color,
                                span.opacity * opacity);
            } else if (span.decoration == TextDecoration::LineThrough) {
                const int y = static_cast<int>(std::round(baseline - maxAscent * 0.35f));
                buffer.FillRect(static_cast<int>(std::floor(penX)),
                                y,
                                std::max(1, static_cast<int>(std::ceil(span.width))),
                                std::max(1, static_cast<int>(span.fontSize * 0.07f)),
                                span.color,
                                span.opacity * opacity);
            }
            penX += span.width;
            if (span.isSpace) {
                penX += extraGap;
            }
        }

        cursorY += baseLineHeight;
    }

    return cache.advanceY;
}

}  // namespace Robo
