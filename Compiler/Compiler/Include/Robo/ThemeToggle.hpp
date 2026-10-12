#pragma once

#include <string>

namespace Robo {

enum class ThemeToggleVariant {
    Default,
    Compact,
    Labels,
    Icons,
    Switch
};

enum class ThemeTogglePart {
    None,
    Light,
    Dark,
    Switch
};

struct ThemeToggleBounds {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    bool Contains(float pointX, float pointY) const;
};

class ThemeToggle {
public:
    void SetVariant(ThemeToggleVariant variant);
    ThemeToggleVariant GetVariant() const;

    void SetDisabled(bool disabled);
    bool IsDisabled() const;

    void SetBounds(const ThemeToggleBounds& bounds);
    const ThemeToggleBounds& GetBounds() const;

    void SetPartBounds(ThemeTogglePart part, const ThemeToggleBounds& bounds);
    ThemeTogglePart HitPart(float x, float y) const;

    void ScaleBounds(float factor);

    void Tick(float deltaMs);
    bool NeedsMotion() const;
    float ThemeMix() const;

private:
    ThemeToggleVariant variant_ = ThemeToggleVariant::Default;
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
    ThemeToggleBounds track_;
};

ThemeToggleVariant ParseThemeToggleVariant(const std::string& value);

}  // namespace Robo
