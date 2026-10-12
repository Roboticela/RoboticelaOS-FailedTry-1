#pragma once

#include "ThemeToggle.hpp"

#include <string>

namespace Robo {

enum class ThemeIconToggleVariant {
    Ghost,
    Subtle,
    Outline,
    Pair,
    Segmented,
    Split
};

enum class ThemeIconToggleSize {
    Small,
    Medium
};

enum class ThemeIconHot {
    None,
    Button,
    Light,
    Dark
};

class ThemeIconToggle {
public:
    void SetVariant(ThemeIconToggleVariant variant);
    ThemeIconToggleVariant GetVariant() const;

    void SetSize(ThemeIconToggleSize size);
    ThemeIconToggleSize GetSize() const;

    void SetDisabled(bool disabled);
    bool IsDisabled() const;

    void SetBounds(const ThemeToggleBounds& bounds);
    const ThemeToggleBounds& GetBounds() const;

    void SetSegmentBounds(const ThemeToggleBounds& light, const ThemeToggleBounds& dark);
    ThemeTogglePart HitPart(float x, float y) const;

    void SetHot(ThemeIconHot hot);
    ThemeIconHot GetHot() const;

    void ScaleBounds(float factor);

    void Tick(float deltaMs);
    bool NeedsMotion() const;
    float ThemeMix() const;

private:
    ThemeIconToggleVariant variant_ = ThemeIconToggleVariant::Ghost;
    ThemeIconToggleSize size_ = ThemeIconToggleSize::Medium;
    bool disabled_ = false;
    bool primed_ = false;
    bool mixing_ = false;
    float mix_ = 0.0f;
    float mixFrom_ = 0.0f;
    float mixTo_ = 0.0f;
    float mixElapsed_ = 0.0f;
    ThemeToggleBounds bounds_;
    ThemeToggleBounds light_;
    ThemeToggleBounds dark_;
    ThemeIconHot hot_ = ThemeIconHot::None;
};

ThemeIconToggleVariant ParseThemeIconToggleVariant(const std::string& value);
ThemeIconToggleSize ParseThemeIconToggleSize(const std::string& value);

}  // namespace Robo
