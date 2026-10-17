#include "Robo/Input.hpp"
#include "Robo/Foundation.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

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

std::size_t PrevUtf8Boundary(const std::string& text, std::size_t index) {
    if (index == 0 || index > text.size()) {
        return 0;
    }
    std::size_t i = index - 1;
    while (i > 0 && (static_cast<unsigned char>(text[i]) & 0xC0) == 0x80) {
        --i;
    }
    return i;
}

std::size_t NextUtf8Boundary(const std::string& text, std::size_t index);

int CodepointAt(const std::string& text, std::size_t index, std::size_t& next) {
    if (index >= text.size()) {
        next = text.size();
        return -1;
    }
    const unsigned char lead = static_cast<unsigned char>(text[index]);
    std::size_t size = 1;
    int codepoint = lead;
    if ((lead & 0x80u) == 0) {
        size = 1;
        codepoint = lead;
    } else if ((lead & 0xE0u) == 0xC0u && index + 1u < text.size()) {
        size = 2;
        codepoint = ((lead & 0x1Fu) << 6) | (static_cast<unsigned char>(text[index + 1u]) & 0x3Fu);
    } else if ((lead & 0xF0u) == 0xE0u && index + 2u < text.size()) {
        size = 3;
        codepoint = ((lead & 0x0Fu) << 12) | ((static_cast<unsigned char>(text[index + 1u]) & 0x3Fu) << 6) |
                    (static_cast<unsigned char>(text[index + 2u]) & 0x3Fu);
    } else if ((lead & 0xF8u) == 0xF0u && index + 3u < text.size()) {
        size = 4;
        codepoint = ((lead & 0x07u) << 18) | ((static_cast<unsigned char>(text[index + 1u]) & 0x3Fu) << 12) |
                    ((static_cast<unsigned char>(text[index + 2u]) & 0x3Fu) << 6) |
                    (static_cast<unsigned char>(text[index + 3u]) & 0x3Fu);
    }
    next = index + size;
    return codepoint;
}

bool IsCombiningMark(int codepoint) {
    return (codepoint >= 0x0300 && codepoint <= 0x036F) || (codepoint >= 0x1AB0 && codepoint <= 0x1AFF) ||
           (codepoint >= 0x1DC0 && codepoint <= 0x1DFF) || (codepoint >= 0x20D0 && codepoint <= 0x20FF) ||
           (codepoint >= 0xFE20 && codepoint <= 0xFE2F);
}

std::size_t NextGrapheme(const std::string& text, std::size_t index) {
    std::size_t next = NextUtf8Boundary(text, index);
    while (next < text.size()) {
        std::size_t after = next;
        const int codepoint = CodepointAt(text, next, after);
        if (!IsCombiningMark(codepoint)) {
            break;
        }
        next = after;
    }
    return next;
}

std::size_t PrevGrapheme(const std::string& text, std::size_t index) {
    if (index == 0) {
        return 0;
    }
    std::size_t at = PrevUtf8Boundary(text, index);
    while (at > 0) {
        std::size_t after = at;
        const int codepoint = CodepointAt(text, at, after);
        if (!IsCombiningMark(codepoint)) {
            break;
        }
        at = PrevUtf8Boundary(text, at);
    }
    return at;
}

std::size_t NextUtf8Boundary(const std::string& text, std::size_t index) {
    if (index >= text.size()) {
        return text.size();
    }
    unsigned char lead = static_cast<unsigned char>(text[index]);
    std::size_t size = 1;
    if ((lead & 0x80) == 0) {
        size = 1;
    } else if ((lead & 0xE0) == 0xC0) {
        size = 2;
    } else if ((lead & 0xF0) == 0xE0) {
        size = 3;
    } else if ((lead & 0xF8) == 0xF0) {
        size = 4;
    }
    return std::min(text.size(), index + size);
}

std::size_t CountUtf8Codepoints(const std::string& text) {
    std::size_t count = 0;
    for (std::size_t i = 0; i < text.size();) {
        i = NextUtf8Boundary(text, i);
        ++count;
    }
    return count;
}

}  // namespace

bool InputBounds::Contains(float pointX, float pointY) const {
    return pointX >= x && pointY >= y && pointX < x + width && pointY < y + height;
}

Input::Input() = default;

void Input::SetValue(const std::string& value) {
    value_ = value;
    ClampCaret();
    InvalidateMeasureCache();
}

const std::string& Input::GetValue() const {
    return value_;
}

void Input::SetPlaceholder(const std::string& placeholder) {
    placeholder_ = placeholder;
}

const std::string& Input::GetPlaceholder() const {
    return placeholder_;
}

void Input::SetStyle(const InputStyle& style) {
    style_ = style;
    InvalidateMeasureCache();
}

const InputStyle& Input::GetStyle() const {
    return style_;
}

void Input::SetInteraction(InputInteraction interaction) {
    interaction_ = interaction;
}

InputInteraction Input::GetInteraction() const {
    return interaction_;
}

void Input::SetBounds(const InputBounds& bounds) {
    bounds_ = bounds;
}

const InputBounds& Input::GetBounds() const {
    return bounds_;
}

void Input::SetCaretIndex(std::size_t index) {
    caretIndex_ = std::min(index, value_.size());
    ClampCaret();
    InvalidateCaretCache();
}

std::size_t Input::GetCaretIndex() const {
    return caretIndex_;
}

void Input::ApplyVariantDefaults() {
    if (style_.tokens.find("fontSize") == style_.tokens.end()) {
        style_.tokens["fontSize"] = "type.body.size";
    }
    if (style_.tokens.find("fontWeight") == style_.tokens.end()) {
        style_.tokens["fontWeight"] = "type.body.weight";
    }
    if (style_.tokens.find("paddingX") == style_.tokens.end()) {
        style_.tokens["paddingX"] = "space.3";
    }
    if (style_.tokens.find("paddingY") == style_.tokens.end()) {
        style_.tokens["paddingY"] = "space.2";
    }
    if (style_.tokens.find("focusBorderColor") == style_.tokens.end()) {
        style_.tokens["focusBorderColor"] = "color.border.focus";
    }
    if (style_.tokens.find("caretColor") == style_.tokens.end()) {
        style_.tokens["caretColor"] = "color.brand.primary";
    }

    switch (style_.variant) {
        case InputVariant::Bordered:
            if (!style_.hasCustomBackground) {
                style_.tokens["backgroundColor"] = "color.bg.elevated";
            }
            if (!style_.hasCustomBorder) {
                style_.tokens["borderColor"] = "color.border.default";
            }
            style_.borderWidth = 1.0;
            if (style_.tokens.find("cornerRadius") == style_.tokens.end()) {
                style_.tokens["cornerRadius"] = "radius.control";
            }
            break;
        case InputVariant::Plain:
            if (!style_.hasCustomBackground) {
                style_.backgroundColor = Color::FromRgba(0, 0, 0, 0);
                style_.tokens["backgroundColor"] = "";
            }
            if (!style_.hasCustomBorder) {
                style_.borderColor = Color::FromRgba(0, 0, 0, 0);
                style_.tokens["borderColor"] = "";
            }
            style_.borderWidth = 0.0;
            if (style_.tokens.find("cornerRadius") == style_.tokens.end()) {
                style_.cornerRadius = 0.0;
                style_.tokens["cornerRadius"] = "";
            }
            break;
        case InputVariant::Filled:
        default:
            if (!style_.hasCustomBackground) {
                style_.tokens["backgroundColor"] = "color.bg.secondary";
            }
            if (!style_.hasCustomBorder) {
                style_.tokens["borderColor"] = "color.bg.secondary";
            }
            style_.borderWidth = 0.0;
            if (style_.tokens.find("cornerRadius") == style_.tokens.end()) {
                style_.tokens["cornerRadius"] = "radius.control";
            }
            break;
    }

    if (!style_.hasCustomTextColor) {
        style_.tokens["textColor"] = "color.text.primary";
    }
    if (!style_.hasCustomPlaceholderColor) {
        style_.tokens["placeholderColor"] = "color.text.tertiary";
    }
}

Color Input::ResolveBackgroundColor() const {
    InputStyle style = style_;
    Resolve(style);
    Color background = style.backgroundColor;
    if (interaction_ == InputInteraction::Hovered) {
        if (style.variant == InputVariant::Filled) {
            return MixToward(background, Color::FromHex("#000000"), 0.04f);
        }
        if (style.variant == InputVariant::Bordered) {
            return MixToward(background, ResolveColor(background, "color.bg.secondary"), 0.5f);
        }
    }
    if (interaction_ == InputInteraction::Focused && style.variant == InputVariant::Filled) {
        return MixToward(background, Color::FromHex("#FFFFFF"), 0.35f);
    }
    return background;
}

Color Input::ResolveBorderColor() const {
    InputStyle style = style_;
    Resolve(style);
    if (interaction_ == InputInteraction::Focused) {
        return style.focusBorderColor;
    }
    return style.borderColor;
}

float Input::ResolveBorderWidth() const {
    if (interaction_ == InputInteraction::Focused) {
        return static_cast<float>(style_.focusBorderWidth);
    }
    if (style_.variant == InputVariant::Plain) {
        return 1.0f;
    }
    return static_cast<float>(style_.borderWidth);
}

float Input::ResolveOpacity() const {
    return static_cast<float>(style_.opacity);
}

void Input::ClampCaret() {
    caretIndex_ = std::min(caretIndex_, value_.size());
    while (caretIndex_ > 0 && (static_cast<unsigned char>(value_[caretIndex_]) & 0xC0) == 0x80) {
        --caretIndex_;
    }
}

bool Input::InsertText(const std::string& utf8) {
    if (utf8.empty()) {
        return false;
    }
    if (style_.maxLength > 0) {
        const std::size_t current = CountUtf8Codepoints(value_);
        const std::size_t incoming = CountUtf8Codepoints(utf8);
        if (current + incoming > static_cast<std::size_t>(style_.maxLength)) {
            return false;
        }
    }
    ClampCaret();
    value_.insert(caretIndex_, utf8);
    caretIndex_ += utf8.size();
    ClampCaret();
    return true;
}

bool Input::DeleteBackward() {
    ClampCaret();
    if (caretIndex_ == 0) {
        return false;
    }
    const std::size_t start = PrevGrapheme(value_, caretIndex_);
    value_.erase(start, caretIndex_ - start);
    caretIndex_ = start;
    return true;
}

bool Input::DeleteForward() {
    ClampCaret();
    if (caretIndex_ >= value_.size()) {
        return false;
    }
    const std::size_t end = NextGrapheme(value_, caretIndex_);
    value_.erase(caretIndex_, end - caretIndex_);
    return true;
}

bool Input::MoveCaretLeft() {
    ClampCaret();
    if (caretIndex_ == 0) {
        return false;
    }
    caretIndex_ = PrevGrapheme(value_, caretIndex_);
    return true;
}

bool Input::MoveCaretRight() {
    ClampCaret();
    if (caretIndex_ >= value_.size()) {
        return false;
    }
    caretIndex_ = NextGrapheme(value_, caretIndex_);
    return true;
}

bool Input::HasMeasureCache(float containerWidth, float pixelScale) const {
    return measureCache_.valid && std::fabs(measureCache_.containerWidth - containerWidth) < 0.5f &&
           std::fabs(measureCache_.pixelScale - pixelScale) < 0.001f;
}

const Input::MeasureCache& Input::GetMeasureCache() const {
    return measureCache_;
}

void Input::StoreMeasureCache(MeasureCache cache) const {
    measureCache_ = std::move(cache);
    measureCache_.valid = true;
}

void Input::InvalidateMeasureCache() const {
    measureCache_.valid = false;
    InvalidateCaretCache();
}

bool Input::HasCaretCache(float maxTextWidth, float pixelScale) const {
    return caretCache_.valid && caretCache_.value == value_ && caretCache_.caretIndex == caretIndex_ &&
           std::fabs(caretCache_.maxTextWidth - maxTextWidth) < 0.5f &&
           std::fabs(caretCache_.pixelScale - pixelScale) < 0.001f;
}

const Input::CaretCache& Input::GetCaretCache() const {
    return caretCache_;
}

void Input::StoreCaretCache(CaretCache cache) const {
    caretCache_ = std::move(cache);
    caretCache_.valid = true;
}

void Input::InvalidateCaretCache() const {
    caretCache_.valid = false;
}

InputVariant ParseInputVariant(const std::string& value) {
    if (value == "bordered" || value == "border") {
        return InputVariant::Bordered;
    }
    if (value == "plain") {
        return InputVariant::Plain;
    }
    return InputVariant::Filled;
}

}  // namespace Robo
