#pragma once

#include "Svg.hpp"

#include <string>

namespace Robo {

// The DevKit mark from the reference brand logo.
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

private:
    float size_ = 20.0f;
    BrandLogoVariant variant_ = BrandLogoVariant::Mark;
    std::string primary_;
    std::string secondary_;
};

BrandLogoVariant ParseBrandLogoVariant(const std::string& value);

}  // namespace Robo
