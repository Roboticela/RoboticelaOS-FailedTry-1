#pragma once

#include "Color.hpp"
#include "FontStyle.hpp"
#include "Length.hpp"
#include "Selectable.hpp"
#include "TextAlign.hpp"
#include "TextLayout.hpp"
#include "TextRun.hpp"

#include <map>
#include <string>
#include <vector>

namespace Robo {

struct TextStyle {
    double fontSize = 16.0;
    Color color = Color::FromHex("#1A1A1A");
    FontWeight fontWeight = FontWeight::Regular;
    FontStyleKind fontStyle = FontStyleKind::Normal;
    TextDecoration textDecoration = TextDecoration::None;
    TextAlign align = TextAlign::Left;
    bool wrap = true;
    double lineHeight = 1.4;
    double letterSpacing = 0.0;
    double opacity = 1.0;
    SelectableMode selectable = SelectableMode::Inherit;
    // Legacy string widths (codegen migration); prefer widthLength/maxWidthLength.
    std::string width = "100%";
    std::string maxWidth;
    Length widthLength = Length::Percent(100.0f);
    Length maxWidthLength = Length::Auto();
    bool hasWidthLength = false;
    bool hasMaxWidthLength = false;
    // Empty token value means the attribute was set to a custom literal.
    std::map<std::string, std::string> tokens;
};

class Text {
public:
    Text();

    void SetStyle(const TextStyle& style);
    const TextStyle& GetStyle() const;

    void ClearRuns();
    void AddRun(const TextRun& run);
    void AddPlain(const std::string& content);
    const std::vector<TextRun>& GetRuns() const;

    // Layout cache: Measure fills retained lines; Paint stamps without re-wrapping.
    bool HasMeasureCache(float containerWidth, float pixelScale) const;
    void ReadMeasureCache(float& width, float& height, float& advanceY) const;
    void WriteMeasureCache(float containerWidth, float pixelScale, float width, float height,
                           float advanceY) const;
    void InvalidateMeasureCache() const;

    TextLayoutCache& LayoutCache() const;
    bool NeedsLayout() const;
    void SetNeedsLayout(bool needs) const;
    void NoteRelayout() const;
    bool TakeDirtyLines(int& first, int& last) const;

private:
    TextStyle style_;
    std::vector<TextRun> runs_;
    mutable bool measureCacheValid_ = false;
    mutable float measureCacheContainer_ = -1.0f;
    mutable float measureCacheScale_ = -1.0f;
    mutable float measureCacheWidth_ = 0.0f;
    mutable float measureCacheHeight_ = 0.0f;
    mutable float measureCacheAdvanceY_ = 0.0f;
    mutable TextLayoutCache layoutCache_{};
    mutable bool needsLayout_ = true;
    mutable int previousLineCount_ = 0;
    mutable float previousLineY_[128]{};
    mutable float previousLineH_[128]{};
    mutable float previousLineW_[128]{};
    mutable int dirtyLineFirst_ = -1;
    mutable int dirtyLineLast_ = -1;
};

}  // namespace Robo
