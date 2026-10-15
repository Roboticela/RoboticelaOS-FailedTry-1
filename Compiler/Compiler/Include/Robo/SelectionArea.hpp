#pragma once

#include "Color.hpp"
#include "FontStyle.hpp"
#include "TextAlign.hpp"

#include <string>
#include <vector>

namespace Robo {

// How a marquee item paints the name under its icon.
struct SelectionLabelStyle {
    float fontSize = 12.0f;
    FontWeight fontWeight = FontWeight::Regular;
    Color color = Color::FromHex("#FFFFFF");
    TextAlign align = TextAlign::Center;
    float lineHeight = 1.25f;
    float letterSpacing = 0.0f;
    bool wrap = true;
    float maxWidth = 100.0f;
    int maxLines = 2;
    float shadowX = 0.0f;
    float shadowY = 1.0f;
    float shadowBlur = 2.0f;
    Color shadowColor = Color::FromHex("#000000D9");
    float iconGap = 5.0f;
    float selectionRadius = 4.0f;
    Color selectionColor = Color::FromHex("#007AFF");
};

enum class SelectionInclusion {
    Intersect,
    Contain
};

enum class SelectionCombine {
    Replace,
    Add,
    Subtract,
    Toggle
};

enum class SelectionGesture {
    Idle,
    Pending,
    Marquee,
    Move
};

struct SelectionBounds {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    bool Contains(float pointX, float pointY) const;
    bool Intersects(const SelectionBounds& other) const;
    bool ContainsRect(const SelectionBounds& other) const;
};

// One object the marquee can select. Also called a marquee, rubber-band, or drag-box item.
class SelectionAreaItem {
public:
    void SetLabel(const std::string& label);
    const std::string& GetLabel() const;

    void SetPosition(float x, float y);
    float GetX() const;
    float GetY() const;

    void SetSize(float width, float height);
    float GetWidth() const;
    float GetHeight() const;

    void SetSelected(bool selected);
    bool IsSelected() const;

    void SetFocused(bool focused);
    bool IsFocused() const;

    void SetLocked(bool locked);
    bool IsLocked() const;

    void SetBounds(const SelectionBounds& bounds);
    const SelectionBounds& GetBounds() const;

    void SetLabelStyle(const SelectionLabelStyle& style);
    bool HasLabelStyle() const;
    const SelectionLabelStyle& GetLabelStyle() const;

private:
    std::string label_;
    float x_ = 0.0f;
    float y_ = 0.0f;
    float width_ = 104.0f;
    float height_ = 108.0f;
    bool selected_ = false;
    bool focused_ = false;
    bool locked_ = false;
    bool hasLabelStyle_ = false;
    SelectionLabelStyle labelStyle_{};
    SelectionBounds bounds_{};
};

// A container that selects its items with a drag rectangle.
// MarqueeSelection, RubberBandSelection, and DragBoxSelection are the same component.
class SelectionArea {
public:
    void SetInclusion(SelectionInclusion inclusion);
    void SetCombine(SelectionCombine combine);
    void SetThreshold(float threshold);
    void SetKeyboardFocus(bool focused);
    bool HasKeyboardFocus() const;

    void AddItem(SelectionAreaItem item);
    std::vector<SelectionAreaItem>& Items();
    const std::vector<SelectionAreaItem>& Items() const;

    void SetBounds(const SelectionBounds& bounds);
    const SelectionBounds& GetBounds() const;
    void SetPixelScale(float pixelScale);

    void SetLabelStyle(const SelectionLabelStyle& style);
    const SelectionLabelStyle& GetLabelStyle() const;

    SelectionGesture GetGesture() const;
    bool HasMarquee() const;
    SelectionBounds MarqueeRect() const;
    int SelectedCount() const;

    bool PointerDown(float x, float y, bool fromTouch, int clicks, int modifiers);
    bool PointerMove(float x, float y);
    bool PointerUp(float x, float y);
    bool LongPress(float x, float y);
    bool Cancel();
    bool KeyDown(const std::string& key, bool shift, int modifiers);

private:
    struct ItemSnapshot {
        bool selected = false;
        float x = 0.0f;
        float y = 0.0f;
        float boundsX = 0.0f;
        float boundsY = 0.0f;
    };

    bool Hit(float x, float y, int& index) const;
    bool Eligible(int index) const;
    SelectionCombine CombineFor(int modifiers) const;
    void ReplaceWith(int index);
    void ToggleIndex(int index);
    void SelectRange(int from, int to);
    void SelectAll();
    void ClearSelection();
    void ApplyMarquee();
    SelectionBounds NormalizedRect() const;
    bool MoveFocus(int direction, bool extend);
    void FocusOnly(int index);
    void TakeSnapshot();
    void RestoreSnapshot();

    std::vector<SelectionAreaItem> items_;
    SelectionInclusion inclusion_ = SelectionInclusion::Intersect;
    SelectionCombine combine_ = SelectionCombine::Replace;
    float threshold_ = 4.0f;
    bool keyboardFocus_ = false;
    SelectionBounds bounds_{};
    float pixelScale_ = 1.0f;
    SelectionLabelStyle labelStyle_{};

    SelectionGesture gesture_ = SelectionGesture::Idle;
    SelectionCombine dragCombine_ = SelectionCombine::Replace;
    float anchorX_ = 0.0f;
    float anchorY_ = 0.0f;
    float pointerX_ = 0.0f;
    float pointerY_ = 0.0f;
    float moveAnchorX_ = 0.0f;
    float moveAnchorY_ = 0.0f;
    int pressedItem_ = -1;
    int focus_ = -1;
    int rangeAnchor_ = -1;
    bool fromTouch_ = false;
    std::vector<ItemSnapshot> snapshot_;
};

}  // namespace Robo
