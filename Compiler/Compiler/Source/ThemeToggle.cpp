#include "Robo/ThemeToggle.hpp"

#include "Robo/Foundation.hpp"
#include "Robo/Motion.hpp"

#include <cmath>

namespace Robo {

bool ThemeToggleBounds::Contains(float pointX, float pointY) const {
    return width > 0.0f && height > 0.0f && pointX >= x && pointY >= y && pointX < x + width && pointY < y + height;
}

void ThemeToggle::SetVariant(ThemeToggleVariant variant) {
    variant_ = variant;
}

ThemeToggleVariant ThemeToggle::GetVariant() const {
    return variant_;
}

void ThemeToggle::SetDisabled(bool disabled) {
    disabled_ = disabled;
}

bool ThemeToggle::IsDisabled() const {
    return disabled_;
}

void ThemeToggle::SetBounds(const ThemeToggleBounds& bounds) {
    bounds_ = bounds;
}

const ThemeToggleBounds& ThemeToggle::GetBounds() const {
    return bounds_;
}

void ThemeToggle::SetPartBounds(ThemeTogglePart part, const ThemeToggleBounds& bounds) {
    if (part == ThemeTogglePart::Light) {
        light_ = bounds;
    } else if (part == ThemeTogglePart::Dark) {
        dark_ = bounds;
    } else if (part == ThemeTogglePart::Switch) {
        track_ = bounds;
    }
}

ThemeTogglePart ThemeToggle::HitPart(float x, float y) const {
    if (light_.Contains(x, y)) {
        return ThemeTogglePart::Light;
    }
    if (dark_.Contains(x, y)) {
        return ThemeTogglePart::Dark;
    }
    if (track_.Contains(x, y)) {
        return ThemeTogglePart::Switch;
    }
    return ThemeTogglePart::None;
}

namespace {

void ScaleOne(ThemeToggleBounds& bounds, float factor) {
    bounds.x *= factor;
    bounds.y *= factor;
    bounds.width *= factor;
    bounds.height *= factor;
}

}  // namespace

void ThemeToggle::ScaleBounds(float factor) {
    ScaleOne(bounds_, factor);
    ScaleOne(light_, factor);
    ScaleOne(dark_, factor);
    ScaleOne(track_, factor);
}

void ThemeToggle::Tick(float deltaMs) {
    AdvanceThemeMix(deltaMs, 200.0f, primed_, mixing_, mix_, mixFrom_, mixTo_, mixElapsed_);
}

bool ThemeToggle::NeedsMotion() const {
    const float target = CurrentTheme() == Theme::Dark ? 1.0f : 0.0f;
    return !primed_ || mixing_ || std::fabs(mix_ - target) > 0.001f;
}

float ThemeToggle::ThemeMix() const {
    return mix_;
}

ThemeToggleVariant ParseThemeToggleVariant(const std::string& value) {
    if (value == "compact") {
        return ThemeToggleVariant::Compact;
    }
    if (value == "labels") {
        return ThemeToggleVariant::Labels;
    }
    if (value == "icons") {
        return ThemeToggleVariant::Icons;
    }
    if (value == "switch") {
        return ThemeToggleVariant::Switch;
    }
    return ThemeToggleVariant::Default;
}

}  // namespace Robo
