#include "Robo/BrandLogoPainter.hpp"

#include "Robo/Foundation.hpp"
#include "Robo/SvgPainter.hpp"

#include <algorithm>
#include <utility>

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
    if (logo.HasMeasureCache(scale)) {
        const BrandLogo::MeasureCache& cached = logo.GetMeasureCache();
        BrandLogoMeasureResult result;
        result.width = cached.width;
        result.height = cached.height;
        result.advanceY = cached.advanceY;
        return result;
    }

    Svg mark = logo.Mark();
    const SvgMeasureResult graphic = SvgPainter::Measure(mark, scale);
    BrandLogo::MeasureCache cache;
    cache.pixelScale = scale;
    cache.markWidth = graphic.width;
    cache.markHeight = graphic.height;
    cache.width = graphic.width;
    cache.height = graphic.height;
    if (logo.ShowsLabel()) {
        const float fontSize = LABEL_SIZE * scale;
        cache.titleWidth = LabelWidth(fonts, logo.Title(), FontFaceId::SemiBold, fontSize);
        float labelWidth = cache.titleWidth;
        if (logo.GetVariant() == BrandLogoVariant::Lockup) {
            cache.spaceWidth = LabelWidth(fonts, " ", FontFaceId::SemiBold, fontSize);
            cache.subtitleWidth = LabelWidth(fonts, logo.Subtitle(), FontFaceId::Medium, fontSize);
            labelWidth += cache.spaceWidth + cache.subtitleWidth;
        }
        cache.width += LABEL_GAP * scale + labelWidth;
        cache.height = std::max(cache.height, fontSize);
    }
    cache.advanceY = cache.height + 12.0f * scale;
    cache.valid = true;
    logo.StoreMeasureCache(std::move(cache));

    BrandLogoMeasureResult result;
    result.width = logo.GetMeasureCache().width;
    result.height = logo.GetMeasureCache().height;
    result.advanceY = logo.GetMeasureCache().advanceY;
    return result;
}

BrandLogoPaintResult BrandLogoPainter::Paint(FrameBuffer& buffer,
                                             const FontEngine& fonts,
                                             const BrandLogo& logo,
                                             float originX,
                                             float originY,
                                             float pixelScale) {
    const float scale = std::max(0.5f, pixelScale);
    Measure(fonts, logo, scale);
    const BrandLogo::MeasureCache& measured = logo.GetMeasureCache();
    Svg mark = logo.Mark();
    const float markY = originY + std::max(0.0f, (measured.height - measured.markHeight) * 0.5f);
    SvgPainter::Paint(buffer, mark, originX, markY, measured.markWidth, scale);
    if (logo.ShowsLabel()) {
        const float fontSize = LABEL_SIZE * scale;
        const float ascent = fonts.GetAscent(fontSize, FontFaceId::SemiBold);
        const float descent = fonts.GetDescent(fontSize, FontFaceId::SemiBold);
        const float lineTop = originY + (measured.height - fontSize) * 0.5f;
        const float baseline = lineTop + (fontSize - (ascent + descent)) * 0.5f + ascent;
        const float textX = originX + measured.markWidth + LABEL_GAP * scale;
        const Color primary = ResolveColor(Color::FromHex("#1D1D1F"), "color.text.primary");
        const Color secondary = ResolveColor(Color::FromHex("#6E6E73"), "color.text.secondary");
        const float tracking = LABEL_TRACKING * fontSize;
        fonts.DrawString(buffer, textX, baseline, logo.Title(), fontSize, FontFaceId::SemiBold, primary, 1.0f, tracking, true);
        if (logo.GetVariant() == BrandLogoVariant::Lockup) {
            fonts.DrawString(buffer,
                             textX + measured.titleWidth + measured.spaceWidth,
                             baseline,
                             logo.Subtitle(),
                             fontSize,
                             FontFaceId::Medium,
                             secondary,
                             1.0f,
                             tracking,
                             true);
        }
    }
    BrandLogoPaintResult result;
    result.advanceY = measured.advanceY;
    return result;
}

}  // namespace Robo
