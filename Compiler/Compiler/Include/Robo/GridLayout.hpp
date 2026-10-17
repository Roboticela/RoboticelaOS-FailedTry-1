#pragma once

#include <cstddef>
#include <vector>

namespace Robo {

struct PlacedGridChild {
    std::size_t itemIndex = 0;
    int colStart = 0;
    int colSpan = 1;
    int rowStart = 0;
    int rowSpan = 1;

    float measuredWidth = 0.0f;
    float measuredHeight = 0.0f;

    float finalX = 0.0f;
    float finalY = 0.0f;
    float finalWidth = 0.0f;
    float finalHeight = 0.0f;
};

struct ComputedGridLayout {
    float containerWidth = 0.0f;
    float containerHeight = 0.0f;
    std::vector<PlacedGridChild> children;
};

struct GridLayoutCacheKey {
    float containerWidth = -1.0f;
    float pixelScale = -1.0f;
    float forcedWidth = -1.0f;
    float forcedHeight = -1.0f;
};

struct GridLayoutCache {
    ComputedGridLayout layout;
    GridLayoutCacheKey key;
    bool valid = false;
};

}  // namespace Robo
