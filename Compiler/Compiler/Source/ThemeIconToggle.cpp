#include "Robo/ThemeIconToggle.hpp"

#include "Robo/Foundation.hpp"
#include "Robo/Motion.hpp"

#include <cmath>

namespace Robo {

void ThemeIconToggle::SetVariant(ThemeIconToggleVariant variant) {
    variant_ = variant;
}

ThemeIconToggleVariant ThemeIconToggle::GetVariant() const {
    return variant_;
}

void ThemeIconToggle::SetSize(ThemeIconToggleSize size) {
    size_ = size;
}

ThemeIconToggleSize ThemeIconToggle::GetSize() const {
    return size_;
}

void ThemeIconToggle::SetDisabled(bool disabled) {
    disabled_ = disabled;
}

bool ThemeIconToggle::IsDisabled() const {
    return disabled_;
}

void ThemeIconToggle::SetBounds(const ThemeToggleBounds& bounds) {
    bounds_ = bounds;
}

const ThemeToggleBounds& ThemeIconToggle::GetBounds() const {
    return bounds_;
}

void ThemeIconToggle::SetSegmentBounds(const ThemeToggleBounds& light, const ThemeToggleBounds& dark) {
    light_ = light;
    dark_ = dark;
}

ThemeTogglePart ThemeIconToggle::HitPart(float x, float y) const {
    if (variant_ == ThemeIconToggleVariant::Ghost || variant_ == ThemeIconToggleVariant::Subtle ||
        variant_ == ThemeIconToggleVariant::Outline) {
        return bounds_.Contains(x, y) ? ThemeTogglePart::Switch : ThemeTogglePart::None;
    }
    if (light_.Contains(x, y)) {
        return ThemeTogglePart::Light;
    }
    if (dark_.Contains(x, y)) {
        return ThemeTogglePart::Dark;
    }
    return ThemeTogglePart::None;
}

void ThemeIconToggle::SetHot(ThemeIconHot hot) {
    hot_ = hot;
}

ThemeIconHot ThemeIconToggle::GetHot() const {
    return hot_;
}

void ThemeIconToggle::Tick(float deltaMs) {
    AdvanceThemeMix(deltaMs, 300.0f, primed_, mixing_, mix_, mixFrom_, mixTo_, mixElapsed_);
}

bool ThemeIconToggle::NeedsMotion() const {
    const float target = CurrentTheme() == Theme::Dark ? 1.0f : 0.0f;
    return !primed_ || mixing_ || std::fabs(mix_ - target) > 0.001f;
}

float ThemeIconToggle::ThemeMix() const {
    return mix_;
}

void ThemeIconToggle::ScaleBounds(float factor) {
    bounds_.x *= factor;
    bounds_.y *= factor;
    bounds_.width *= factor;
    bounds_.height *= factor;
    light_.x *= factor;
    light_.y *= factor;
    light_.width *= factor;
    light_.height *= factor;
    dark_.x *= factor;
    dark_.y *= factor;
    dark_.width *= factor;
    dark_.height *= factor;
}

ThemeIconToggleVariant ParseThemeIconToggleVariant(const std::string& value) {
    if (value == "subtle") {
        return ThemeIconToggleVariant::Subtle;
    }
    if (value == "outline") {
        return ThemeIconToggleVariant::Outline;
    }
    if (value == "pair") {
        return ThemeIconToggleVariant::Pair;
    }
    if (value == "segmented") {
        return ThemeIconToggleVariant::Segmented;
    }
    if (value == "split") {
        return ThemeIconToggleVariant::Split;
    }
    return ThemeIconToggleVariant::Ghost;
}

ThemeIconToggleSize ParseThemeIconToggleSize(const std::string& value) {
    if (value == "sm" || value == "small") {
        return ThemeIconToggleSize::Small;
    }
    return ThemeIconToggleSize::Medium;
}

}  // namespace Robo
