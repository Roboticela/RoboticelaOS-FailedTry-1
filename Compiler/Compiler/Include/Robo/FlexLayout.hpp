#pragma once

#include <cstddef>
#include <vector>

namespace Robo {

struct MeasuredFlexChild {
    std::size_t itemIndex = 0;
    float baseWidth = 0.0f;
    float baseHeight = 0.0f;
    float baseMain = 0.0f;
    float baseCross = 0.0f;
    float allocatedMain = 0.0f;
    float allocatedCross = 0.0f;
    float finalX = 0.0f;
    float finalY = 0.0f;
    float finalWidth = 0.0f;
    float finalHeight = 0.0f;
};

struct FlexLine {
    std::vector<MeasuredFlexChild> children;
    float lineCrossSize = 0.0f;
};

struct ComputedFlexLayout {
    float containerWidth = 0.0f;
    float containerHeight = 0.0f;
    std::vector<FlexLine> lines;
};

struct FlexLayoutCacheKey {
    float containerWidth = -1.0f;
    float pixelScale = -1.0f;
    float forcedWidth = -1.0f;
    float forcedHeight = -1.0f;
};

struct FlexLayoutCache {
    ComputedFlexLayout layout;
    FlexLayoutCacheKey key;
    bool valid = false;
};

}  // namespace Robo
