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
    previousLineCount_ = 0;
    const int limit = static_cast<int>(layoutCache_.lines.size());
    for (int index = 0; index < limit && previousLineCount_ < 128; ++index) {
        previousLineY_[previousLineCount_] = layoutCache_.lines[static_cast<std::size_t>(index)].y;
        previousLineH_[previousLineCount_] = layoutCache_.lines[static_cast<std::size_t>(index)].height;
        previousLineW_[previousLineCount_] = layoutCache_.lines[static_cast<std::size_t>(index)].width;
        previousLineCount_ += 1;
    }
    measureCacheValid_ = false;
    layoutCache_.valid = false;
    layoutCache_.lines.clear();
    needsLayout_ = true;
}

void Text::NoteRelayout() const {
    dirtyLineFirst_ = -1;
    dirtyLineLast_ = -1;
    if (previousLineCount_ == 0) {
        return;
    }
    const int count = static_cast<int>(layoutCache_.lines.size());
    const int shared = count < previousLineCount_ ? count : previousLineCount_;
    for (int index = 0; index < shared; ++index) {
        const TextLayoutLine& line = layoutCache_.lines[static_cast<std::size_t>(index)];
        const float oldY = previousLineY_[index];
        const float oldH = previousLineH_[index];
        if (std::fabs(line.y - oldY) > 0.5f || std::fabs(line.height - oldH) > 0.5f ||
            std::fabs(line.width - previousLineW_[index]) > 0.5f) {
            if (dirtyLineFirst_ < 0) {
                dirtyLineFirst_ = index;
            }
            dirtyLineLast_ = index;
        }
    }
    if (count != previousLineCount_) {
        const int from = shared;
        const int to = (count > previousLineCount_ ? count : previousLineCount_) - 1;
        if (to >= from) {
            if (dirtyLineFirst_ < 0) {
                dirtyLineFirst_ = from;
            }
            dirtyLineLast_ = to;
        }
    }
    previousLineCount_ = 0;
}

bool Text::TakeDirtyLines(int& first, int& last) const {
    if (dirtyLineFirst_ < 0 || dirtyLineLast_ < dirtyLineFirst_) {
        return false;
    }
    first = dirtyLineFirst_;
    last = dirtyLineLast_;
    dirtyLineFirst_ = -1;
    dirtyLineLast_ = -1;
    return true;
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
