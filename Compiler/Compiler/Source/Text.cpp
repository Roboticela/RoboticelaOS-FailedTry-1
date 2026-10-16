#include "Robo/Text.hpp"

#include <cmath>

namespace Robo {

Text::Text() = default;

void Text::SetStyle(const TextStyle& style) {
    style_ = style;
    InvalidateMeasureCache();
}

const TextStyle& Text::GetStyle() const {
    return style_;
}

void Text::ClearRuns() {
    runs_.clear();
    InvalidateMeasureCache();
}

void Text::AddRun(const TextRun& run) {
    runs_.push_back(run);
    InvalidateMeasureCache();
}

void Text::AddPlain(const std::string& content) {
    TextRun run;
    run.content = content;
    runs_.push_back(run);
    InvalidateMeasureCache();
}

const std::vector<TextRun>& Text::GetRuns() const {
    return runs_;
}

bool Text::HasMeasureCache(float containerWidth, float pixelScale) const {
    return measureCacheValid_ && std::fabs(measureCacheContainer_ - containerWidth) < 0.5f &&
           std::fabs(measureCacheScale_ - pixelScale) < 0.001f;
}

void Text::ReadMeasureCache(float& width, float& height, float& advanceY) const {
    width = measureCacheWidth_;
    height = measureCacheHeight_;
    advanceY = measureCacheAdvanceY_;
}

void Text::WriteMeasureCache(float containerWidth, float pixelScale, float width, float height,
                             float advanceY) const {
    measureCacheValid_ = true;
    measureCacheContainer_ = containerWidth;
    measureCacheScale_ = pixelScale;
    measureCacheWidth_ = width;
    measureCacheHeight_ = height;
    measureCacheAdvanceY_ = advanceY;
}

void Text::InvalidateMeasureCache() const {
    measureCacheValid_ = false;
}

}  // namespace Robo
