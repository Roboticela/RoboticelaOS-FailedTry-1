#pragma once

#include <cstddef>
#include <vector>

namespace Robo {

// Child frames relative to the Position container origin (0,0), including padding.
struct PlacedPositionChild {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

struct ComputedPositionLayout {
    float containerWidth = 0.0f;
    float containerHeight = 0.0f;
    std::vector<PlacedPositionChild> children;
};

struct PositionLayoutCacheKey {
    float availableWidth = -1.0f;
    float pixelScale = -1.0f;
    float forcedWidth = -1.0f;
    float forcedHeight = -1.0f;
};

struct PositionLayoutCache {
    ComputedPositionLayout layout;
    PositionLayoutCacheKey key;
    bool valid = false;
};

}  // namespace Robo
