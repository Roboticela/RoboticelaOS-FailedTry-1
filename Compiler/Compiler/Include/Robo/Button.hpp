#pragma once

#include "Color.hpp"
#include "FontStyle.hpp"
#include "Selectable.hpp"
#include "Svg.hpp"
#include "TextAlign.hpp"

#include <map>
#include <string>

namespace Robo {

enum class ButtonVariant {
    Filled,
    Tinted,
    Gray,
    Plain,
    Destructive,
    Outline
};

enum class ButtonInteraction {
    Normal,
    Hovered,
    Pressed
};

struct ButtonStyle {
    ButtonVariant variant = ButtonVariant::Filled;
    double fontSize = 17.0;
    FontWeight fontWeight = FontWeight::Medium;
    FontStyleKind fontStyle = FontStyleKind::Normal;
    Color textColor = Color::FromHex("#FFFFFF");
    Color backgroundColor = Color::FromHex("#007AFF");
    double cornerRadius = 14.0;
    double paddingX = 20.0;
    double paddingY = 12.0;
    double letterSpacing = 0.0;
    double opacity = 1.0;
    SelectableMode selectable = SelectableMode::Inherit;
    TextAlign align = TextAlign::Left;
    bool hasCustomTextColor = false;
    bool hasCustomBackground = false;
    std::map<std::string, std::string> tokens;
};

struct ButtonBounds {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    bool Contains(float pointX, float pointY) const;
};

class Button {
public:
    Button();

    void SetLabel(const std::string& label);
    const std::string& GetLabel() const;

    void SetStyle(const ButtonStyle& style);
    const ButtonStyle& GetStyle() const;

    void SetInteraction(ButtonInteraction interaction);
    ButtonInteraction GetInteraction() const;

    void SetBounds(const ButtonBounds& bounds);
    const ButtonBounds& GetBounds() const;

    void ApplyVariantDefaults();

    void SetDisabled(bool disabled);
    bool IsDisabled() const;
    void SetLoading(bool loading);
    bool IsLoading() const;
    bool CanActivate() const;

    void SetFocused(bool focused);
    bool IsFocused() const;
    void SetFocusVisible(bool visible);
    bool IsFocusVisible() const;

    void SetIcon(Svg icon);
    bool HasIcon() const;
    Svg& GetIcon();
    const Svg& GetIcon() const;
    void SetIconOnly(bool iconOnly);
    bool IsIconOnly() const;

    Color ResolveBackgroundColor() const;
    Color ResolveTextColor() const;
    Color ResolveFocusColor() const;
    float ResolveOpacity() const;

private:
    std::string label_;
    ButtonStyle style_;
    ButtonInteraction interaction_ = ButtonInteraction::Normal;
    ButtonBounds bounds_;
    Svg icon_;
    bool hasIcon_ = false;
    bool iconOnly_ = false;
    bool disabled_ = false;
    bool loading_ = false;
    bool focused_ = false;
    bool focusVisible_ = false;
};

ButtonVariant ParseButtonVariant(const std::string& value);

}  // namespace Robo
