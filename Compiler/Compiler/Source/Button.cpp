#include "Robo/Button.hpp"
#include "Robo/Foundation.hpp"

#include <algorithm>

namespace Robo {

namespace {

Color MixToward(const Color& from, const Color& toward, float amount) {
    const float t = std::max(0.0f, std::min(1.0f, amount));
    Color result;
    result.red = static_cast<std::uint8_t>(from.red + (toward.red - from.red) * t + 0.5f);
    result.green = static_cast<std::uint8_t>(from.green + (toward.green - from.green) * t + 0.5f);
    result.blue = static_cast<std::uint8_t>(from.blue + (toward.blue - from.blue) * t + 0.5f);
    result.alpha = static_cast<std::uint8_t>(from.alpha + (toward.alpha - from.alpha) * t + 0.5f);
    return result;
}

}  // namespace

bool ButtonBounds::Contains(float pointX, float pointY) const {
    return pointX >= x && pointY >= y && pointX < x + width && pointY < y + height;
}

Button::Button() = default;

void Button::SetLabel(const std::string& label) {
    label_ = label;
}

const std::string& Button::GetLabel() const {
    return label_;
}

void Button::SetStyle(const ButtonStyle& style) {
    style_ = style;
}

const ButtonStyle& Button::GetStyle() const {
    return style_;
}

void Button::SetInteraction(ButtonInteraction interaction) {
    interaction_ = interaction;
}

ButtonInteraction Button::GetInteraction() const {
    return interaction_;
}

void Button::SetBounds(const ButtonBounds& bounds) {
    bounds_ = bounds;
}

const ButtonBounds& Button::GetBounds() const {
    return bounds_;
}

void Button::ApplyVariantDefaults() {
    if (style_.tokens.find("fontSize") == style_.tokens.end()) {
        style_.tokens["fontSize"] = "type.button.size";
    }
    if (style_.tokens.find("fontWeight") == style_.tokens.end()) {
        style_.tokens["fontWeight"] = "type.button.weight";
    }
    if (style_.tokens.find("letterSpacing") == style_.tokens.end()) {
        style_.tokens["letterSpacing"] = "type.button.tracking";
    }
    if (style_.tokens.find("cornerRadius") == style_.tokens.end()) {
        style_.tokens["cornerRadius"] = "radius.pill";
    }
    if (style_.tokens.find("paddingX") == style_.tokens.end()) {
        style_.tokens["paddingX"] = "space.4";
    }
    if (style_.tokens.find("paddingY") == style_.tokens.end()) {
        style_.tokens["paddingY"] = "space.3";
    }

    if (iconOnly_) {
        style_.paddingX = 0.0;
        style_.paddingY = 0.0;
        style_.tokens["paddingX"] = "";
        style_.tokens["paddingY"] = "";
        if (style_.tokens.find("cornerRadius") != style_.tokens.end()) {
            style_.tokens["cornerRadius"] = "radius.pill";
        }
    }

    switch (style_.variant) {
        case ButtonVariant::Tinted:
            if (!style_.hasCustomBackground) {
                style_.tokens["backgroundColor"] = "color.brand.primary.subtle";
            }
            if (!style_.hasCustomTextColor) {
                style_.tokens["textColor"] = "color.brand.primary";
            }
            break;
        case ButtonVariant::Gray:
            if (!style_.hasCustomBackground) {
                style_.tokens["backgroundColor"] = "color.bg.tertiary";
            }
            if (!style_.hasCustomTextColor) {
                style_.tokens["textColor"] = "color.text.primary";
            }
            break;
        case ButtonVariant::Plain:
            if (!style_.hasCustomBackground) {
                style_.backgroundColor = Color::FromRgba(0, 0, 0, 0);
                style_.tokens["backgroundColor"] = "";
            }
            if (!style_.hasCustomTextColor) {
                style_.tokens["textColor"] = "color.brand.primary";
            }
            break;
        case ButtonVariant::Destructive:
            if (!style_.hasCustomBackground) {
                style_.tokens["backgroundColor"] = "color.danger";
            }
            if (!style_.hasCustomTextColor) {
                style_.tokens["textColor"] = "color.text.on-brand";
            }
            break;
        case ButtonVariant::Outline:
            if (!style_.hasCustomBackground) {
                style_.backgroundColor = Color::FromRgba(0, 0, 0, 0);
                style_.tokens["backgroundColor"] = "";
            }
            if (!style_.hasCustomTextColor) {
                style_.tokens["textColor"] = "color.text.primary";
            }
            break;
        case ButtonVariant::Filled:
        default:
            if (!style_.hasCustomBackground) {
                style_.tokens["backgroundColor"] = "color.brand.primary";
            }
            if (!style_.hasCustomTextColor) {
                style_.tokens["textColor"] = "color.text.on-brand";
            }
            break;
    }
}

void Button::SetDisabled(bool disabled) {
    disabled_ = disabled;
}

bool Button::IsDisabled() const {
    return disabled_;
}

void Button::SetLoading(bool loading) {
    loading_ = loading;
}

bool Button::IsLoading() const {
    return loading_;
}

bool Button::CanActivate() const {
    return !disabled_ && !loading_;
}

void Button::SetFocused(bool focused) {
    focused_ = focused;
    if (!focused_) {
        focusVisible_ = false;
    }
}

bool Button::IsFocused() const {
    return focused_;
}

void Button::SetFocusVisible(bool visible) {
    focusVisible_ = visible && focused_;
}

bool Button::IsFocusVisible() const {
    return focusVisible_;
}

void Button::SetIcon(Svg icon) {
    icon_ = std::move(icon);
    hasIcon_ = true;
}

bool Button::HasIcon() const {
    return hasIcon_;
}

Svg& Button::GetIcon() {
    return icon_;
}

const Svg& Button::GetIcon() const {
    return icon_;
}

void Button::SetIconOnly(bool iconOnly) {
    iconOnly_ = iconOnly;
}

bool Button::IsIconOnly() const {
    return iconOnly_;
}

Color Button::ResolveBackgroundColor() const {
    ButtonStyle style = style_;
    Resolve(style);
    const Color background = style.backgroundColor;
    const Color subtle = ResolveColor(Color::FromHex("#0E7AFF1A"), "color.brand.primary.subtle");
    const Color disabledFill = ResolveColor(Color::FromHex("#B8D9FF"), "color.brand.primary.disabled");
    const Color quiet = ResolveColor(Color::FromHex("#EDEDEF"), "color.bg.tertiary");

    if (!CanActivate()) {
        if (style.variant == ButtonVariant::Filled) {
            return disabledFill;
        }
        if (style.variant == ButtonVariant::Plain || style.variant == ButtonVariant::Outline) {
            return Color::FromRgba(0, 0, 0, 0);
        }
        return quiet;
    }

    if (style.variant == ButtonVariant::Plain) {
        if (interaction_ == ButtonInteraction::Pressed) {
            return MixToward(subtle, ResolveColor(subtle, "color.brand.primary"), 0.18f);
        }
        if (interaction_ == ButtonInteraction::Hovered) {
            return subtle;
        }
        return background;
    }

    if (interaction_ == ButtonInteraction::Hovered || interaction_ == ButtonInteraction::Pressed) {
        const bool pressed = interaction_ == ButtonInteraction::Pressed;
        const char* token = "color.bg.hover";
        Color fallback = Color::FromHex("#F5F5F7");
        if (style.variant == ButtonVariant::Filled) {
            token = pressed ? "color.brand.primary.active" : "color.brand.primary.hover";
            fallback = pressed ? Color::FromHex("#0757C7") : Color::FromHex("#0A6AE8");
        } else if (style.variant == ButtonVariant::Destructive) {
            const Color danger = ResolveColor(Color::FromHex("#FF3B30"), "color.danger");
            if (pressed) {
                return MixToward(danger, Color::FromHex("#000000"), 0.16f);
            }
            return ResolveColor(Color::FromHex("#E0342A"), "color.danger.hover");
        } else if (style.variant == ButtonVariant::Tinted) {
            return MixToward(background, ResolveColor(background, "color.brand.primary"), pressed ? 0.22f : 0.12f);
        } else if (pressed) {
            token = "color.bg.active";
            fallback = Color::FromHex("#EDEDEF");
        }
        return ResolveColor(fallback, token);
    }

    return background;
}

Color Button::ResolveTextColor() const {
    ButtonStyle style = style_;
    Resolve(style);
    if (!CanActivate() && style.variant != ButtonVariant::Filled) {
        return ResolveColor(Color::FromHex("#C7C7CC"), "color.text.disabled");
    }
    if (style.variant == ButtonVariant::Plain && interaction_ == ButtonInteraction::Pressed && CanActivate()) {
        return ResolveColor(style.textColor, "color.brand.primary.active");
    }
    return style.textColor;
}

Color Button::ResolveFocusColor() const {
    if (style_.variant == ButtonVariant::Destructive) {
        return ResolveColor(Color::FromHex("#FF3B30"), "color.danger");
    }
    return ResolveColor(Color::FromHex("#0E7AFF"), "color.brand.primary");
}

float Button::ResolveOpacity() const {
    return static_cast<float>(style_.opacity);
}

ButtonVariant ParseButtonVariant(const std::string& value) {
    if (value == "tinted") {
        return ButtonVariant::Tinted;
    }
    if (value == "gray" || value == "grey" || value == "secondary") {
        return ButtonVariant::Gray;
    }
    if (value == "plain" || value == "tertiary") {
        return ButtonVariant::Plain;
    }
    if (value == "destructive" || value == "danger") {
        return ButtonVariant::Destructive;
    }
    if (value == "outline") {
        return ButtonVariant::Outline;
    }
    return ButtonVariant::Filled;
}

}  // namespace Robo
