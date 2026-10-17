#pragma once

#include "Length.hpp"

#include <cstdlib>
#include <string>

namespace Robo {

inline float ParsePropDimension(const std::string& value, float parentSize, float fallback, float scale = 1.0f) {
    if (value.empty() || value == "auto") {
        return fallback;
    }
    if (value.size() > 1 && value.back() == '%') {
        const float percent = static_cast<float>(std::atof(value.c_str()));
        return parentSize * (percent / 100.0f);
    }
    if (value.size() > 3 && value.compare(value.size() - 3, 3, "rem") == 0) {
        return static_cast<float>(std::atof(value.c_str()) * 16.0f) * scale;
    }
    if (value.size() > 2 && value.compare(value.size() - 2, 2, "px") == 0) {
        return static_cast<float>(std::atof(value.c_str())) * scale;
    }
    return static_cast<float>(std::atof(value.c_str())) * scale;
}

inline float ResolvePropLength(bool hasLength, const Length& length, const std::string& legacy, float parentSize,
                               float fallback, float scale) {
    if (hasLength && !length.IsAuto()) {
        return length.Resolve(parentSize / scale, 1.0f) * scale;
    }
    if (!legacy.empty() && legacy != "auto") {
        return ParsePropDimension(legacy, parentSize, fallback, scale);
    }
    return fallback;
}

inline bool PropHasWidth(bool hasLength, const Length& length, const std::string& legacy) {
    return (hasLength && !length.IsAuto()) || (!legacy.empty() && legacy != "auto");
}

inline bool PropHasHeight(bool hasLength, const Length& length, const std::string& legacy) {
    return PropHasWidth(hasLength, length, legacy);
}

}  // namespace Robo
