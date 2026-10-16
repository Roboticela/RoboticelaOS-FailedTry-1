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

struct GlyphSpan {
    std::string text;
    ResolvedRun style;
    float width = 0.0f;
    bool isSpace = false;
};

struct LayoutLine {
    std::vector<GlyphSpan> spans;
    float width = 0.0f;
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
    // Whitespace-only run between styled spans must stay as a real space.
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

    // Keep boundary spaces so "word <Span>x</Span> word" does not become "wordxword".
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

void PushSpan(LayoutLine& line, const GlyphSpan& span) {
    if (span.text.empty()) {
        return;
    }
    line.spans.push_back(span);
    line.width += span.width;
}

void TrimLineEdges(LayoutLine& line) {
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

TextMeasureResult TextPainter::Measure(const FontEngine& fonts,
                                       const Text& text,
                                       float containerWidth,
                                       float pixelScale) {
    const float scale = std::max(0.5f, pixelScale);
    if (text.HasMeasureCache(containerWidth, scale)) {
        TextMeasureResult cached;
        text.ReadMeasureCache(cached.width, cached.height, cached.advanceY);
        return cached;
    }
    TextStyle style = text.GetStyle();
    Resolve(style);
    const float logicalWidth = containerWidth / scale;
    float blockWidth = ResolveWidthValue(style.width, logicalWidth) * scale;
    if (!style.maxWidth.empty()) {
        blockWidth = std::min(blockWidth, ResolveWidthValue(style.maxWidth, logicalWidth) * scale);
    }
    blockWidth = std::max(1.0f, blockWidth);

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

    TextMeasureResult result;
    if (runs.empty()) {
        result.height = static_cast<float>(style.fontSize) * static_cast<float>(style.lineHeight) * scale;
        result.width = 0.0f;
        result.advanceY = result.height + 12.0f * scale;
        return result;
    }

    std::vector<LayoutLine> lines;
    LayoutLine currentLine;

    auto BreakLongToken = [&](const std::string& token, const ResolvedRun& runStyle) {
        std::string built;
        for (unsigned char byte : token) {
            std::string candidate = built;
            candidate.push_back(static_cast<char>(byte));
            const float width = fonts.MeasureWidth(candidate, runStyle.fontSize, runStyle.faceId, runStyle.letterSpacing);
            if (!built.empty() && style.wrap && currentLine.width + width > blockWidth && !currentLine.spans.empty()) {
                GlyphSpan span;
                span.text = built;
                span.style = runStyle;
                span.width = fonts.MeasureWidth(built, runStyle.fontSize, runStyle.faceId, runStyle.letterSpacing);
                span.isSpace = false;
                PushSpan(currentLine, span);
                lines.push_back(currentLine);
                currentLine = LayoutLine();
                built.assign(1, static_cast<char>(byte));
            } else {
                built = candidate;
            }
        }
        if (!built.empty()) {
            GlyphSpan span;
            span.text = built;
            span.style = runStyle;
            span.width = fonts.MeasureWidth(built, runStyle.fontSize, runStyle.faceId, runStyle.letterSpacing);
            span.isSpace = false;
            if (style.wrap && !currentLine.spans.empty() && currentLine.width + span.width > blockWidth) {
                lines.push_back(currentLine);
                currentLine = LayoutLine();
            }
            PushSpan(currentLine, span);
        }
    };

    for (std::size_t runIndex = 0; runIndex < runs.size(); ++runIndex) {
        const ResolvedRun& run = runs[runIndex];
        const std::vector<std::string> tokens = SplitTokens(run.content);
        for (const std::string& token : tokens) {
            GlyphSpan span;
            span.text = token;
            span.style = run;
            span.isSpace = (token == " ");
            span.width = fonts.MeasureWidth(token, run.fontSize, run.faceId, run.letterSpacing);

            if (!style.wrap) {
                PushSpan(currentLine, span);
                continue;
            }

            if (!span.isSpace && !currentLine.spans.empty() && currentLine.width + span.width > blockWidth) {
                lines.push_back(currentLine);
                currentLine = LayoutLine();
            }

            if (!span.isSpace && span.width > blockWidth) {
                BreakLongToken(token, run);
            } else if (!(span.isSpace && currentLine.spans.empty())) {
                PushSpan(currentLine, span);
            }
        }
    }
    lines.push_back(currentLine);

    float maxAscent = 0.0f;
    float maxDescent = 0.0f;
    for (const ResolvedRun& run : runs) {
        maxAscent = std::max(maxAscent, fonts.GetAscent(run.fontSize, run.faceId));
        maxDescent = std::max(maxDescent, fonts.GetDescent(run.fontSize, run.faceId));
    }
    if (maxAscent <= 0.0f) {
        maxAscent = static_cast<float>(style.fontSize) * 0.8f;
    }
    const float baseLineHeight = std::max(maxAscent + maxDescent, static_cast<float>(style.fontSize)) *
                                 static_cast<float>(style.lineHeight);

    float maxLineWidth = 0.0f;
    std::size_t nonEmptyLines = 0;
    for (auto& line : lines) {
        TrimLineEdges(line);
        if (!line.spans.empty()) {
            ++nonEmptyLines;
            maxLineWidth = std::max(maxLineWidth, line.width);
        }
    }

    result.width = maxLineWidth;
    result.height = std::max(baseLineHeight, static_cast<float>(nonEmptyLines) * baseLineHeight);
    result.advanceY = result.height + 12.0f * scale;
    text.WriteMeasureCache(containerWidth, scale, result.width, result.height, result.advanceY);
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
    TextStyle style = text.GetStyle();
    Resolve(style);
    const float logicalWidth = containerWidth / scale;
    float blockWidth = ResolveWidthValue(style.width, logicalWidth) * scale;
    if (!style.maxWidth.empty()) {
        blockWidth = std::min(blockWidth, ResolveWidthValue(style.maxWidth, logicalWidth) * scale);
    }
    blockWidth = std::max(1.0f, blockWidth);

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

    if (runs.empty()) {
        return static_cast<float>(style.fontSize) * static_cast<float>(style.lineHeight) * scale + 12.0f * scale;
    }

    // Re-join adjacent runs with spaces only between word boundaries handled per-token.
    std::vector<LayoutLine> lines;
    LayoutLine currentLine;

    auto BreakLongToken = [&](const std::string& token, const ResolvedRun& runStyle) {
        std::string built;
        for (unsigned char byte : token) {
            std::string candidate = built;
            candidate.push_back(static_cast<char>(byte));
            const float width = fonts.MeasureWidth(candidate, runStyle.fontSize, runStyle.faceId, runStyle.letterSpacing);
            if (!built.empty() && style.wrap && currentLine.width + width > blockWidth && !currentLine.spans.empty()) {
                GlyphSpan span;
                span.text = built;
                span.style = runStyle;
                span.width = fonts.MeasureWidth(built, runStyle.fontSize, runStyle.faceId, runStyle.letterSpacing);
                span.isSpace = false;
                PushSpan(currentLine, span);
                lines.push_back(currentLine);
                currentLine = LayoutLine();
                built.assign(1, static_cast<char>(byte));
            } else {
                built = candidate;
            }
        }
        if (!built.empty()) {
            GlyphSpan span;
            span.text = built;
            span.style = runStyle;
            span.width = fonts.MeasureWidth(built, runStyle.fontSize, runStyle.faceId, runStyle.letterSpacing);
            span.isSpace = false;
            if (style.wrap && !currentLine.spans.empty() && currentLine.width + span.width > blockWidth) {
                lines.push_back(currentLine);
                currentLine = LayoutLine();
            }
            PushSpan(currentLine, span);
        }
    };

    for (std::size_t runIndex = 0; runIndex < runs.size(); ++runIndex) {
        const ResolvedRun& run = runs[runIndex];
        const std::vector<std::string> tokens = SplitTokens(run.content);
        for (const std::string& token : tokens) {
            GlyphSpan span;
            span.text = token;
            span.style = run;
            span.isSpace = (token == " ");
            span.width = fonts.MeasureWidth(token, run.fontSize, run.faceId, run.letterSpacing);

            if (!style.wrap) {
                PushSpan(currentLine, span);
                continue;
            }

            if (!span.isSpace && !currentLine.spans.empty() && currentLine.width + span.width > blockWidth) {
                lines.push_back(currentLine);
                currentLine = LayoutLine();
            }

            if (!span.isSpace && span.width > blockWidth) {
                BreakLongToken(token, run);
            } else if (!(span.isSpace && currentLine.spans.empty())) {
                PushSpan(currentLine, span);
            }
        }
    }
    lines.push_back(currentLine);

    float maxAscent = 0.0f;
    float maxDescent = 0.0f;
    for (const ResolvedRun& run : runs) {
        maxAscent = std::max(maxAscent, fonts.GetAscent(run.fontSize, run.faceId));
        maxDescent = std::max(maxDescent, fonts.GetDescent(run.fontSize, run.faceId));
    }
    if (maxAscent <= 0.0f) {
        maxAscent = static_cast<float>(style.fontSize) * 0.8f;
    }
    const float baseLineHeight = std::max(maxAscent + maxDescent, static_cast<float>(style.fontSize)) *
                                 static_cast<float>(style.lineHeight);
    float cursorY = originY;

    for (std::size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
        LayoutLine& line = lines[lineIndex];
        TrimLineEdges(line);
        if (line.spans.empty()) {
            continue;
        }

        float startX = originX;
        float extraGap = 0.0f;
        int gapCount = 0;
        const bool isLastLine = lineIndex + 1 >= lines.size();

        if (style.align == TextAlign::Center) {
            startX = originX + std::max(0.0f, (blockWidth - line.width) * 0.5f);
        } else if (style.align == TextAlign::Right) {
            startX = originX + std::max(0.0f, blockWidth - line.width);
        } else if (style.align == TextAlign::Justify && !isLastLine) {
            for (const GlyphSpan& span : line.spans) {
                if (span.isSpace) {
                    ++gapCount;
                }
            }
            // Only justify lines that nearly fill the block; avoids huge gaps on short lines.
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
        const float opacity = static_cast<float>(style.opacity);
        std::vector<TextGlyphDraw> draws;
        draws.reserve(64);

        for (const GlyphSpan& span : line.spans) {
            fonts.RecordString(draws,
                               penX,
                               baseline,
                               span.text,
                               span.style.fontSize,
                               span.style.faceId,
                               span.style.color,
                               span.style.opacity * opacity,
                               span.style.letterSpacing);
            penX += span.width;
            if (span.isSpace) {
                penX += extraGap;
            }
        }

        FontEngine::PaintSelectionBands(buffer, draws);
        fonts.PaintGlyphDraws(buffer, draws);

        penX = startX;
        for (const GlyphSpan& span : line.spans) {
            if (span.style.decoration == TextDecoration::Underline) {
                const int y = static_cast<int>(std::round(baseline + std::max(1.0f, span.style.fontSize * 0.12f)));
                buffer.FillRect(static_cast<int>(std::floor(penX)),
                                y,
                                std::max(1, static_cast<int>(std::ceil(span.width))),
                                std::max(1, static_cast<int>(span.style.fontSize * 0.07f)),
                                span.style.color,
                                span.style.opacity * opacity);
            } else if (span.style.decoration == TextDecoration::LineThrough) {
                const int y = static_cast<int>(std::round(baseline - maxAscent * 0.35f));
                buffer.FillRect(static_cast<int>(std::floor(penX)),
                                y,
                                std::max(1, static_cast<int>(std::ceil(span.width))),
                                std::max(1, static_cast<int>(span.style.fontSize * 0.07f)),
                                span.style.color,
                                span.style.opacity * opacity);
            }
            penX += span.width;
            if (span.isSpace) {
                penX += extraGap;
            }
        }

        cursorY += baseLineHeight;
    }

    return std::max(baseLineHeight, cursorY - originY) + 12.0f * scale;
}

}  // namespace Robo
