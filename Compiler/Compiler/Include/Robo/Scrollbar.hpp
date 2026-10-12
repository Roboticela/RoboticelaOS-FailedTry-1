#pragma once

#include "Color.hpp"

#include <map>
#include <string>

namespace Robo {

enum class ScrollbarOrientation {
    Vertical,
    Horizontal
};

enum class ScrollbarInteraction {
    Normal,
    Hovered,
    Dragging
};

// Controls when the control is drawn at all.
enum class ScrollbarVisibility {
    Always,  // Drawn regardless of the content/viewport relationship.
    Auto     // Drawn only while the content is larger than the viewport.
};

// The two ends of the track, used for the filled arrow buttons.
enum class ScrollbarArrow {
    None,
    Start,
    End
};

struct ScrollbarStyle {
    ScrollbarOrientation orientation = ScrollbarOrientation::Vertical;
    double thickness = 14.0;
    double length = 220.0;
    double cornerRadius = 7.0;
    double minThumbLength = 28.0;
    Color trackColor = Color::FromRgba(0, 0, 0, 0);
    Color thumbColor = Color::FromRgba(60, 60, 67, 120);
    Color thumbHoverColor = Color::FromRgba(60, 60, 67, 170);
    Color thumbActiveColor = Color::FromRgba(28, 28, 30, 200);
    double opacity = 1.0;
    bool hasCustomThumbColor = false;
    bool hasCustomThumbHoverColor = false;
    bool hasCustomThumbActiveColor = false;
    bool hasCustomTrackColor = false;

    // Filled arrowheads at both ends of the track.
    bool showArrows = true;
    double arrowSize = 16.0;  // 0 derives the cap from the thickness.
    double arrowGap = 2.0;
    Color arrowColor = Color::FromHex("#636366");
    Color arrowHoverColor = Color::FromHex("#1C1C1E");
    Color arrowActiveColor = Color::FromHex("#1C1C1E");
    bool hasCustomArrowColor = false;
    bool hasCustomArrowHoverColor = false;
    bool hasCustomArrowActiveColor = false;

    // Auto-hide: the control stays out of the way until the user interacts.
    bool autoHide = false;
    double autoHideDelay = 900.0;   // Milliseconds of inactivity before fading out.
    double fadeDuration = 160.0;    // Milliseconds for one fade step.
    double hiddenOpacity = 0.0;     // Resting opacity while auto-hidden.
    ScrollbarVisibility visibility = ScrollbarVisibility::Always;
    std::map<std::string, std::string> tokens;
};

struct ScrollbarBounds {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    bool Contains(float pointX, float pointY) const;
};

class Scrollbar {
public:
    Scrollbar();

    void SetStyle(const ScrollbarStyle& style);
    const ScrollbarStyle& GetStyle() const;
    ScrollbarStyle& GetStyle();

    void SetOrientation(ScrollbarOrientation orientation);
    ScrollbarOrientation GetOrientation() const;

    void SetContentSize(double contentSize);
    double GetContentSize() const;

    void SetViewportSize(double viewportSize);
    double GetViewportSize() const;

    void SetValue(double value);
    double GetValue() const;
    void ScrollTo(double value);
    bool ScrollBy(double deltaRatio);

    // Keyboard / pointer-device scrolling. One line is a wheel notch or an arrow
    // key, one page is a viewport, and the ratio steps map onto the value.
    double ResolveLineStep() const;
    double ResolveScrollStep() const;
    bool ScrollByLines(double lines);
    bool ScrollByPage(double pages);
    bool ScrollByPixels(double pixels);
    // Touch drags stay 1:1 with the finger. Wheel and keys glide toward the target.
    bool ScrollByPixelsImmediate(double pixels);
    bool ScrollToStart();
    bool ScrollToEnd();

    void SetFocused(bool focused);
    bool IsFocused() const;

    void SetThumbRatio(double ratio);
    double GetThumbRatio() const;
    bool HasExplicitThumbRatio() const;

    void SetInteraction(ScrollbarInteraction interaction);
    ScrollbarInteraction GetInteraction() const;

    void SetBounds(const ScrollbarBounds& bounds);
    const ScrollbarBounds& GetBounds() const;
    void SetPixelScale(float pixelScale);
    float GetPixelScale() const;

    void ApplyVariantDefaults();

    // True while the content is larger than the viewport, i.e. a scrollbar is
    // actually useful. Explicit thumb ratios below 1 also count as overflowing.
    bool IsOverflowing() const;
    // False only for visibility=auto when nothing overflows.
    bool IsVisible() const;

    double TrackLength() const;
    double CrossSize() const;
    double ResolveThumbLength(double trackLength) const;
    double ResolveThumbOffset(double trackLength) const;
    ScrollbarBounds ResolveThumbBounds() const;

    double ResolveArrowSize() const;
    double ResolveArrowCap() const;
    double ResolveThumbTrackStart() const;
    double ResolveThumbTrackLength() const;
    ScrollbarBounds ResolveArrowBounds(ScrollbarArrow arrow) const;
    bool ArrowContains(ScrollbarArrow arrow, float x, float y) const;
    ScrollbarArrow FindArrowAt(float x, float y) const;

    void SetHotArrow(ScrollbarArrow arrow);
    ScrollbarArrow GetHotArrow() const;
    void SetPressedArrow(ScrollbarArrow arrow);
    ScrollbarArrow GetPressedArrow() const;
    bool StepByArrow(ScrollbarArrow arrow);

    // Auto-hide plumbing.
    void SetPointerInside(bool pointerInside);
    bool IsPointerInside() const;
    void NotifyInteraction();
    // Advances the reveal animation; returns true when the appearance changed.
    bool Tick(double deltaMs);
    bool NeedsAnimation() const;
    float GetRevealOpacity() const;
    float ResolveEffectiveOpacity() const;

    Color ResolveThumbColor() const;
    Color ResolveTrackColor() const;
    Color ResolveArrowColor(ScrollbarArrow arrow) const;
    float ResolveOpacity() const;

    bool ThumbContains(float x, float y) const;

    void JumpTo(float x, float y);
    void BeginDrag(float x, float y);
    void UpdateDrag(float x, float y);
    void EndDrag();
    bool IsDragging() const;

private:
    double TrackCoordinate(float x, float y) const;
    double ResolveThumbRatio() const;

    bool MoveByPixels(double pixels, bool animate);

    ScrollbarStyle style_;
    ScrollbarInteraction interaction_ = ScrollbarInteraction::Normal;
    ScrollbarBounds bounds_;
    double value_ = 0.0;
    double targetValue_ = 0.0;
    double arrowRepeatMs_ = 0.0;
    bool arrowRepeatArmed_ = false;
    double contentSize_ = 0.0;
    double viewportSize_ = 0.0;
    double thumbRatio_ = 0.0;
    bool hasExplicitThumbRatio_ = false;
    double dragGrabOffset_ = 0.0;
    bool dragging_ = false;
    bool focused_ = false;
    float pixelScale_ = 1.0f;

    ScrollbarArrow hotArrow_ = ScrollbarArrow::None;
    ScrollbarArrow pressedArrow_ = ScrollbarArrow::None;
    bool pointerInside_ = false;
    double idleMs_ = 0.0;
    float reveal_ = 1.0f;
    float revealTarget_ = 1.0f;
};

ScrollbarOrientation ParseScrollbarOrientation(const std::string& value);
ScrollbarVisibility ParseScrollbarVisibility(const std::string& value);
bool ParseScrollbarFlag(const std::string& value, bool fallback);
std::string ScrollbarVisibilityToCss(ScrollbarVisibility visibility);

}  // namespace Robo
