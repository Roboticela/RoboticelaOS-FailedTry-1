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
    return layoutCache_.valid && std::fabs(layoutCache_.containerWidth - containerWidth) < 0.5f &&
           std::fabs(layoutCache_.pixelScale - pixelScale) < 0.001f;
}

void Text::ReadMeasureCache(float& width, float& height, float& advanceY) const {
    width = layoutCache_.width;
    height = layoutCache_.height;
    advanceY = layoutCache_.advanceY;
}

void Text::WriteMeasureCache(float containerWidth, float pixelScale, float width, float height,
                             float advanceY) const {
    measureCacheValid_ = true;
    measureCacheContainer_ = containerWidth;
    measureCacheScale_ = pixelScale;
    measureCacheWidth_ = width;
    measureCacheHeight_ = height;
    measureCacheAdvanceY_ = advanceY;
    layoutCache_.width = width;
    layoutCache_.height = height;
    layoutCache_.advanceY = advanceY;
    layoutCache_.containerWidth = containerWidth;
    layoutCache_.pixelScale = pixelScale;
    layoutCache_.valid = true;
    needsLayout_ = false;
}

void Text::InvalidateMeasureCache() const {
    measureCacheValid_ = false;
    layoutCache_.valid = false;
    layoutCache_.lines.clear();
    needsLayout_ = true;
}

TextLayoutCache& Text::LayoutCache() const {
    return layoutCache_;
}

bool Text::NeedsLayout() const {
    return needsLayout_ || !layoutCache_.valid;
}

void Text::SetNeedsLayout(bool needs) const {
    needsLayout_ = needs;
    if (needs) {
        layoutCache_.valid = false;
        measureCacheValid_ = false;
    }
}

}  // namespace Robo
