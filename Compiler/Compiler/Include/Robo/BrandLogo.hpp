#pragma once

#include "Svg.hpp"

#include <string>

namespace Robo {

// Brand mark from the reference logo.
// mark and icon are the glyph. text adds the product name.
// lockup adds the company name and the product name.
enum class BrandLogoVariant {
    Mark,
    Icon,
    Text,
    Lockup
};

class BrandLogo {
public:
    void SetSize(float size);
    float GetSize() const;

    void SetVariant(BrandLogoVariant variant);
    BrandLogoVariant GetVariant() const;

    void SetPrimary(const std::string& primary);
    void SetSecondary(const std::string& secondary);

    const std::string& Title() const;
    const std::string& Subtitle() const;
    bool ShowsLabel() const;

    Svg Mark() const;

    struct MeasureCache {
        float pixelScale = -1.0f;
        float width = 0.0f;
        float height = 0.0f;
        float advanceY = 0.0f;
        float markWidth = 0.0f;
        float markHeight = 0.0f;
        float titleWidth = 0.0f;
        float spaceWidth = 0.0f;
        float subtitleWidth = 0.0f;
        bool valid = false;
    };
    bool HasMeasureCache(float pixelScale) const;
    const MeasureCache& GetMeasureCache() const;
    void StoreMeasureCache(MeasureCache cache) const;
    void InvalidateMeasureCache() const;

private:
    float size_ = 20.0f;
    BrandLogoVariant variant_ = BrandLogoVariant::Mark;
    std::string primary_;
    std::string secondary_;
    mutable MeasureCache measureCache_{};
};

BrandLogoVariant ParseBrandLogoVariant(const std::string& value);

}  // namespace Robo
