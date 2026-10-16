#pragma once

#include <cstdlib>
#include <cstring>
#include <string>

namespace Robo {

enum class LengthKind {
    Auto,
    Px,
    Percent,
    Rem
};

// Compiled CSS-like length. Prefer this over runtime string parse in paint/measure.
struct Length {
    LengthKind kind = LengthKind::Auto;
    float value = 0.0f;

    static Length Auto() {
        Length length;
        length.kind = LengthKind::Auto;
        return length;
    }

    static Length Px(float px) {
        Length length;
        length.kind = LengthKind::Px;
        length.value = px;
        return length;
    }

    static Length Percent(float percent) {
        Length length;
        length.kind = LengthKind::Percent;
        length.value = percent;
        return length;
    }

    static Length Rem(float rem) {
        Length length;
        length.kind = LengthKind::Rem;
        length.value = rem;
        return length;
    }

    static Length Parse(const std::string& text) {
        if (text.empty() || text == "auto") {
            return Auto();
        }
        if (text.size() > 1 && text.back() == '%') {
            return Percent(static_cast<float>(std::atof(text.c_str())));
        }
        if (text.size() > 3 && text.compare(text.size() - 3, 3, "rem") == 0) {
            return Rem(static_cast<float>(std::atof(text.c_str())));
        }
        if (text.size() > 2 && text.compare(text.size() - 2, 2, "px") == 0) {
            return Px(static_cast<float>(std::atof(text.c_str())));
        }
        return Px(static_cast<float>(std::atof(text.c_str())));
    }

    float Resolve(float reference, float scale = 1.0f) const {
        switch (kind) {
            case LengthKind::Px:
                return value * scale;
            case LengthKind::Percent:
                return reference * (value / 100.0f);
            case LengthKind::Rem:
                return value * 16.0f * scale;
            case LengthKind::Auto:
            default:
                return reference;
        }
    }

    bool IsAuto() const {
        return kind == LengthKind::Auto;
    }
};

}  // namespace Robo
