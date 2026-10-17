#pragma once

#include "Color.hpp"
#include "FontStyle.hpp"
#include "Length.hpp"
#include "Selectable.hpp"
#include "TextAlign.hpp"

#include <cstddef>
#include <map>
#include <string>

namespace Robo {

enum class InputVariant {
    Filled,
    Bordered,
    Plain
};

enum class InputInteraction {
    Normal,
    Hovered,
    Focused
};

struct InputStyle {
    InputVariant variant = InputVariant::Filled;
    double fontSize = 17.0;
    FontWeight fontWeight = FontWeight::Regular;
    FontStyleKind fontStyle = FontStyleKind::Normal;
    Color textColor = Color::FromHex("#1C1C1E");
    Color placeholderColor = Color::FromHex("#8E8E93");
    Color backgroundColor = Color::FromHex("#F2F2F7");
    Color borderColor = Color::FromHex("#C6C6C8");
    Color focusBorderColor = Color::FromHex("#007AFF");
    Color caretColor = Color::FromHex("#007AFF");
    double cornerRadius = 10.0;
    double paddingX = 12.0;
    double paddingY = 10.0;
    double borderWidth = 1.0;
    double focusBorderWidth = 2.0;
    double letterSpacing = 0.0;
    double opacity = 1.0;
    SelectableMode selectable = SelectableMode::Inherit;
    double minWidth = 200.0;
    std::string width = "100%";
    Length widthLength = Length::Percent(100.0f);
    bool hasWidthLength = false;
    TextAlign align = TextAlign::Left;
    int maxLength = 0;
    bool hasCustomTextColor = false;
    bool hasCustomPlaceholderColor = false;
    bool hasCustomBackground = false;
    bool hasCustomBorder = false;
    std::map<std::string, std::string> tokens;
};

struct InputBounds {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    bool Contains(float pointX, float pointY) const;
};

class Input {
public:
    Input();

    void SetValue(const std::string& value);
    const std::string& GetValue() const;

    void SetPlaceholder(const std::string& placeholder);
    const std::string& GetPlaceholder() const;

    void SetStyle(const InputStyle& style);
    const InputStyle& GetStyle() const;

    void SetInteraction(InputInteraction interaction);
    InputInteraction GetInteraction() const;

    void SetBounds(const InputBounds& bounds);
    const InputBounds& GetBounds() const;

    void SetCaretIndex(std::size_t index);
    std::size_t GetCaretIndex() const;

    void ApplyVariantDefaults();

    Color ResolveBackgroundColor() const;
    Color ResolveBorderColor() const;
    float ResolveBorderWidth() const;
    float ResolveOpacity() const;

    bool InsertText(const std::string& utf8);
    bool DeleteBackward();
    bool DeleteForward();
    bool MoveCaretLeft();
    bool MoveCaretRight();
    void ClampCaret();

    struct MeasureCache {
        float containerWidth = -1.0f;
        float pixelScale = -1.0f;
        float width = 0.0f;
        float height = 0.0f;
        float advanceY = 0.0f;
        float ascent = 0.0f;
        float textHeight = 0.0f;
        bool valid = false;
    };
    bool HasMeasureCache(float containerWidth, float pixelScale) const;
    const MeasureCache& GetMeasureCache() const;
    void StoreMeasureCache(MeasureCache cache) const;
    void InvalidateMeasureCache() const;

private:
    std::string value_;
    std::string placeholder_;
    InputStyle style_;
    InputInteraction interaction_ = InputInteraction::Normal;
    InputBounds bounds_;
    std::size_t caretIndex_ = 0;
    mutable MeasureCache measureCache_{};
};

InputVariant ParseInputVariant(const std::string& value);

}  // namespace Robo
