#include "Robo/BrandLogoPainter.hpp"

#include "Robo/Foundation.hpp"
#include "Robo/SvgPainter.hpp"

#include <algorithm>

namespace Robo {

namespace {

const float LABEL_SIZE = 13.0f;
const float LABEL_GAP = 8.0f;
const float LABEL_TRACKING = -0.01f;

float LabelWidth(const FontEngine& fonts, const std::string& text, FontFaceId face, float fontSize) {
    if (text.empty()) {
        return 0.0f;
    }
    return fonts.MeasureWidth(text, fontSize, face, LABEL_TRACKING * fontSize);
}

}  // namespace

BrandLogoMeasureResult BrandLogoPainter::Measure(const FontEngine& fonts, const BrandLogo& logo, float pixelScale) {
    const float scale = std::max(0.5f, pixelScale);
    Svg mark = logo.Mark();
    const SvgMeasureResult graphic = SvgPainter::Measure(mark, scale);
    BrandLogoMeasureResult result;
    result.width = graphic.width;
    result.height = graphic.height;
    if (logo.ShowsLabel()) {
        const float fontSize = LABEL_SIZE * scale;
        const float titleWidth = LabelWidth(fonts, logo.Title(), FontFaceId::SemiBold, fontSize);
        float labelWidth = titleWidth;
        if (logo.GetVariant() == BrandLogoVariant::Lockup) {
            const float spaceWidth = LabelWidth(fonts, " ", FontFaceId::SemiBold, fontSize);
            const float subtitleWidth = LabelWidth(fonts, logo.Subtitle(), FontFaceId::Medium, fontSize);
            labelWidth += spaceWidth + subtitleWidth;
        }
        result.width += LABEL_GAP * scale + labelWidth;
        result.height = std::max(result.height, fontSize);
    }
    result.advanceY = result.height + 12.0f * scale;
    return result;
}

BrandLogoPaintResult BrandLogoPainter::Paint(FrameBuffer& buffer,
                                             const FontEngine& fonts,
                                             const BrandLogo& logo,
                                             float originX,
                                             float originY,
                                             float pixelScale) {
    const float scale = std::max(0.5f, pixelScale);
    const BrandLogoMeasureResult measured = Measure(fonts, logo, scale);
    Svg mark = logo.Mark();
    const SvgMeasureResult graphic = SvgPainter::Measure(mark, scale);
    const float markY = originY + std::max(0.0f, (measured.height - graphic.height) * 0.5f);
    SvgPainter::Paint(buffer, mark, originX, markY, graphic.width, scale);
    if (logo.ShowsLabel()) {
        const float fontSize = LABEL_SIZE * scale;
        const float ascent = fonts.GetAscent(fontSize, FontFaceId::SemiBold);
        const float descent = fonts.GetDescent(fontSize, FontFaceId::SemiBold);
        const float lineTop = originY + (measured.height - fontSize) * 0.5f;
        const float baseline = lineTop + (fontSize - (ascent + descent)) * 0.5f + ascent;
        const float textX = originX + graphic.width + LABEL_GAP * scale;
        const Color primary = ResolveColor(Color::FromHex("#1D1D1F"), "color.text.primary");
        const Color secondary = ResolveColor(Color::FromHex("#6E6E73"), "color.text.secondary");
        const float tracking = LABEL_TRACKING * fontSize;
        fonts.DrawString(buffer, textX, baseline, logo.Title(), fontSize, FontFaceId::SemiBold, primary, 1.0f, tracking);
        if (logo.GetVariant() == BrandLogoVariant::Lockup) {
            const float titleWidth = LabelWidth(fonts, logo.Title(), FontFaceId::SemiBold, fontSize);
            const float spaceWidth = LabelWidth(fonts, " ", FontFaceId::SemiBold, fontSize);
            fonts.DrawString(buffer,
                             textX + titleWidth + spaceWidth,
                             baseline,
                             logo.Subtitle(),
                             fontSize,
                             FontFaceId::Medium,
                             secondary,
                             1.0f,
                             tracking);
        }
    }
    BrandLogoPaintResult result;
    result.advanceY = measured.advanceY;
    return result;
}

}  // namespace Robo
