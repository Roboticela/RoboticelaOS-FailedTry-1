#include "Robo/Scrollbar.hpp"

#include <algorithm>
#include <cmath>

namespace Robo {

namespace {

double Clamp01(double value) {
    return std::max(0.0, std::min(1.0, value));
}

// The two arrow ends are square caps sized to the track, so they line up with
// the thickness of the bar instead of floating in an oversized box.
constexpr double MIN_ARROW_CROSS = 6.0;
constexpr double MIN_SCROLL_STEP = 0.02;
constexpr double VISIBILITY_EPSILON = 0.0001;

}  // namespace

bool ScrollbarBounds::Contains(float pointX, float pointY) const {
    return pointX >= x && pointY >= y && pointX < x + width && pointY < y + height;
}

Scrollbar::Scrollbar() = default;

void Scrollbar::SetStyle(const ScrollbarStyle& style) {
    style_ = style;
}

const ScrollbarStyle& Scrollbar::GetStyle() const {
    return style_;
}

ScrollbarStyle& Scrollbar::GetStyle() {
    return style_;
}

void Scrollbar::SetOrientation(ScrollbarOrientation orientation) {
    style_.orientation = orientation;
}

ScrollbarOrientation Scrollbar::GetOrientation() const {
    return style_.orientation;
}

void Scrollbar::SetContentSize(double contentSize) {
    contentSize_ = std::max(0.0, contentSize);
}

double Scrollbar::GetContentSize() const {
    return contentSize_;
}

void Scrollbar::SetViewportSize(double viewportSize) {
    viewportSize_ = std::max(0.0, viewportSize);
}

double Scrollbar::GetViewportSize() const {
    return viewportSize_;
}

void Scrollbar::SetValue(double value) {
    value_ = Clamp01(value);
    targetValue_ = value_;
}

double Scrollbar::GetValue() const {
    return value_;
}

void Scrollbar::ScrollTo(double value) {
    SetValue(value);
}

bool Scrollbar::ScrollBy(double deltaRatio) {
    const double next = Clamp01(targetValue_ + deltaRatio);
    if (next == targetValue_) {
        return false;
    }
    targetValue_ = next;
    return true;
}

void Scrollbar::SetThumbRatio(double ratio) {
    thumbRatio_ = Clamp01(ratio);
    hasExplicitThumbRatio_ = true;
}

double Scrollbar::GetThumbRatio() const {
    return ResolveThumbRatio();
}

bool Scrollbar::HasExplicitThumbRatio() const {
    return hasExplicitThumbRatio_;
}

double Scrollbar::ResolveLineStep() const {
    const double range = contentSize_ - viewportSize_;
    if (range > 1.0) {
        return std::min(0.35, 56.0 / range);
    }
    return 0.05;
}

bool Scrollbar::ScrollByLines(double lines) {
    if (lines == 0.0) {
        return false;
    }
    // Scroll attempts reveal the control even at the very ends of the range.
    NotifyInteraction();
    return ScrollBy(lines * ResolveLineStep());
}

bool Scrollbar::ScrollByPage(double pages) {
    if (pages == 0.0) {
        return false;
    }
    NotifyInteraction();
    return ScrollBy(pages * ResolveScrollStep());
}

bool Scrollbar::MoveByPixels(double pixels, bool animate) {
    if (pixels == 0.0) {
        return false;
    }
    NotifyInteraction();
    const double range = contentSize_ - viewportSize_;
    double delta = 0.0;
    if (range > 1.0) {
        delta = pixels / range;
    } else if (contentSize_ > 0.0) {
        delta = pixels / contentSize_;
    } else {
        delta = (pixels / 40.0) * ResolveLineStep();
    }
    if (!animate) {
        const double next = Clamp01(targetValue_ + delta);
        if (next == targetValue_ && next == value_) {
            return false;
        }
        targetValue_ = next;
        value_ = next;
        return true;
    }
    return ScrollBy(delta);
}

bool Scrollbar::ScrollByPixels(double pixels) {
    return MoveByPixels(pixels, true);
}

bool Scrollbar::ScrollByPixelsImmediate(double pixels) {
    return MoveByPixels(pixels, false);
}

bool Scrollbar::ScrollToStart() {
    if (targetValue_ == 0.0) {
        return false;
    }
    targetValue_ = 0.0;
    NotifyInteraction();
    return true;
}

bool Scrollbar::ScrollToEnd() {
    if (targetValue_ == 1.0) {
        return false;
    }
    targetValue_ = 1.0;
    NotifyInteraction();
    return true;
}

void Scrollbar::SetFocused(bool focused) {
    if (focused_ == focused) {
        return;
    }
    focused_ = focused;
    if (focused_) {
        NotifyInteraction();
    }
}

bool Scrollbar::IsFocused() const {
    return focused_;
}

void Scrollbar::SetInteraction(ScrollbarInteraction interaction) {
    interaction_ = interaction;
}

ScrollbarInteraction Scrollbar::GetInteraction() const {
    return interaction_;
}

void Scrollbar::SetBounds(const ScrollbarBounds& bounds) {
    bounds_ = bounds;
}

const ScrollbarBounds& Scrollbar::GetBounds() const {
    return bounds_;
}

void Scrollbar::SetPixelScale(float pixelScale) {
    pixelScale_ = std::max(0.5f, pixelScale);
}

float Scrollbar::GetPixelScale() const {
    return pixelScale_;
}

void Scrollbar::ApplyVariantDefaults() {
    if (!style_.hasCustomThumbColor) {
        style_.thumbColor = Color::FromHex("#C7C7CC");
    }
    if (!style_.hasCustomThumbHoverColor) {
        style_.thumbHoverColor = style_.hasCustomThumbColor ? style_.thumbColor : Color::FromHex("#AEAEB2");
    }
    if (!style_.hasCustomThumbActiveColor) {
        style_.thumbActiveColor = style_.hasCustomThumbColor ? style_.thumbColor : Color::FromHex("#8E8E93");
    }
    if (!style_.hasCustomArrowColor) {
        style_.arrowColor = Color::FromHex("#8E8E93");
    }
    if (!style_.hasCustomArrowHoverColor) {
        style_.arrowHoverColor = Color::FromHex("#1C1C1E");
    }
    if (!style_.hasCustomArrowActiveColor) {
        style_.arrowActiveColor = Color::FromHex("#1C1C1E");
    }
    if (style_.showArrows && style_.arrowSize <= 0.0) {
        style_.arrowSize = 16.0;
    }
    if (style_.cornerRadius <= 0.0) {
        style_.cornerRadius = style_.thickness * 0.5;
    }
    if (style_.hiddenOpacity < 0.0) {
        style_.hiddenOpacity = 0.0;
    } else if (style_.hiddenOpacity > 1.0) {
        style_.hiddenOpacity = 1.0;
    }
    if (style_.autoHideDelay < 0.0) {
        style_.autoHideDelay = 0.0;
    }
    if (style_.fadeDuration < 1.0) {
        style_.fadeDuration = 1.0;
    }
    if (style_.autoHide) {
        reveal_ = static_cast<float>(style_.hiddenOpacity);
        revealTarget_ = reveal_;
        idleMs_ = style_.autoHideDelay;
    }
}

double Scrollbar::TrackLength() const {
    return (style_.orientation == ScrollbarOrientation::Vertical) ? static_cast<double>(bounds_.height)
                                                                  : static_cast<double>(bounds_.width);
}

double Scrollbar::CrossSize() const {
    return (style_.orientation == ScrollbarOrientation::Vertical) ? static_cast<double>(bounds_.width)
                                                                  : static_cast<double>(bounds_.height);
}

double Scrollbar::ResolveThumbRatio() const {
    if (hasExplicitThumbRatio_) {
        return Clamp01(thumbRatio_);
    }
    if (contentSize_ <= 0.0 || viewportSize_ <= 0.0) {
        return 1.0;
    }
    if (contentSize_ <= viewportSize_) {
        return 1.0;
    }
    return Clamp01(viewportSize_ / contentSize_);
}

bool Scrollbar::IsOverflowing() const {
    if (hasExplicitThumbRatio_) {
        return thumbRatio_ < 1.0 - VISIBILITY_EPSILON;
    }
    if (contentSize_ <= 0.0 || viewportSize_ <= 0.0) {
        return true;
    }
    return contentSize_ > viewportSize_ + VISIBILITY_EPSILON;
}

bool Scrollbar::IsVisible() const {
    if (style_.visibility == ScrollbarVisibility::Auto) {
        return IsOverflowing();
    }
    return true;
}

double Scrollbar::ResolveArrowSize() const {
    if (!style_.showArrows) {
        return 0.0;
    }
    const double base = style_.arrowSize > 0.0 ? style_.arrowSize : std::max(style_.thickness, MIN_ARROW_CROSS);
    return base * static_cast<double>(pixelScale_);
}

double Scrollbar::ResolveArrowCap() const {
    const double arrow = ResolveArrowSize();
    if (arrow <= 0.0) {
        return 0.0;
    }
    return arrow + std::max(0.0, style_.arrowGap) * static_cast<double>(pixelScale_);
}

double Scrollbar::ResolveThumbTrackStart() const {
    return ResolveArrowCap();
}

double Scrollbar::ResolveThumbTrackLength() const {
    return std::max(0.0, TrackLength() - 2.0 * ResolveArrowCap());
}

double Scrollbar::ResolveThumbLength(double trackLength) const {
    if (trackLength <= 0.0) {
        return 0.0;
    }
    const double minLength =
        std::min(static_cast<double>(style_.minThumbLength) * static_cast<double>(pixelScale_), trackLength);
    const double length = ResolveThumbRatio() * trackLength;
    return std::max(minLength, std::min(length, trackLength));
}

double Scrollbar::ResolveThumbOffset(double trackLength) const {
    const double thumbLength = ResolveThumbLength(trackLength);
    const double maxStart = std::max(0.0, trackLength - thumbLength);
    return Clamp01(value_) * maxStart;
}

ScrollbarBounds Scrollbar::ResolveThumbBounds() const {
    ScrollbarBounds thumb;
    const double trackLength = ResolveThumbTrackLength();
    const double thumbLength = ResolveThumbLength(trackLength);
    const double thumbOffset = ResolveThumbOffset(trackLength);
    const double trackStart = ResolveThumbTrackStart();

    if (style_.orientation == ScrollbarOrientation::Vertical) {
        thumb.x = bounds_.x;
        thumb.y = bounds_.y + static_cast<float>(trackStart + thumbOffset);
        thumb.width = bounds_.width;
        thumb.height = static_cast<float>(thumbLength);
    } else {
        thumb.x = bounds_.x + static_cast<float>(trackStart + thumbOffset);
        thumb.y = bounds_.y;
        thumb.width = static_cast<float>(thumbLength);
        thumb.height = bounds_.height;
    }
    return thumb;
}

ScrollbarBounds Scrollbar::ResolveArrowBounds(ScrollbarArrow arrow) const {
    ScrollbarBounds result;
    const double arrowSize = ResolveArrowSize();
    const double arrowCap = ResolveArrowCap();
    if (arrow == ScrollbarArrow::None || arrowSize <= 0.0 || arrowCap <= 0.0) {
        return result;
    }

    if (style_.orientation == ScrollbarOrientation::Vertical) {
        result.x = bounds_.x;
        result.width = bounds_.width;
        result.height = static_cast<float>(arrowSize);
        result.y = (arrow == ScrollbarArrow::Start)
                       ? bounds_.y
                       : bounds_.y + bounds_.height - static_cast<float>(arrowSize);
    } else {
        result.y = bounds_.y;
        result.height = bounds_.height;
        result.width = static_cast<float>(arrowSize);
        result.x = (arrow == ScrollbarArrow::Start)
                       ? bounds_.x
                       : bounds_.x + bounds_.width - static_cast<float>(arrowSize);
    }
    return result;
}

bool Scrollbar::ArrowContains(ScrollbarArrow arrow, float x, float y) const {
    if (arrow == ScrollbarArrow::None) {
        return false;
    }
    return ResolveArrowBounds(arrow).Contains(x, y);
}

ScrollbarArrow Scrollbar::FindArrowAt(float x, float y) const {
    if (!style_.showArrows || !IsVisible()) {
        return ScrollbarArrow::None;
    }
    if (ArrowContains(ScrollbarArrow::Start, x, y)) {
        return ScrollbarArrow::Start;
    }
    if (ArrowContains(ScrollbarArrow::End, x, y)) {
        return ScrollbarArrow::End;
    }
    return ScrollbarArrow::None;
}

void Scrollbar::SetHotArrow(ScrollbarArrow arrow) {
    hotArrow_ = arrow;
    if (hotArrow_ != ScrollbarArrow::None) {
        NotifyInteraction();
    }
}

ScrollbarArrow Scrollbar::GetHotArrow() const {
    return hotArrow_;
}

void Scrollbar::SetPressedArrow(ScrollbarArrow arrow) {
    if (pressedArrow_ != arrow) {
        arrowRepeatMs_ = 0.0;
        arrowRepeatArmed_ = false;
    }
    pressedArrow_ = arrow;
}

ScrollbarArrow Scrollbar::GetPressedArrow() const {
    return pressedArrow_;
}

double Scrollbar::ResolveScrollStep() const {
    if (contentSize_ > 0.0 && viewportSize_ > 0.0 && contentSize_ > viewportSize_) {
        return std::max(MIN_SCROLL_STEP, viewportSize_ / contentSize_);
    }
    return 0.1;
}

bool Scrollbar::StepByArrow(ScrollbarArrow arrow) {
    if (arrow == ScrollbarArrow::None) {
        return false;
    }
    const double pixels = arrow == ScrollbarArrow::Start ? -96.0 : 96.0;
    return ScrollByPixels(pixels);
}

void Scrollbar::SetPointerInside(bool pointerInside) {
    pointerInside_ = pointerInside;
    if (pointerInside_) {
        NotifyInteraction();
    }
}

bool Scrollbar::IsPointerInside() const {
    return pointerInside_;
}

void Scrollbar::NotifyInteraction() {
    // Any interaction clears the inactivity countdown and reveals the control.
    idleMs_ = 0.0;
    revealTarget_ = 1.0f;
}

bool Scrollbar::Tick(double deltaMs) {
    const double delta = std::max(0.0, deltaMs);
    bool changed = false;

    if (pressedArrow_ != ScrollbarArrow::None) {
        arrowRepeatMs_ += delta;
        const double interval = arrowRepeatArmed_ ? 70.0 : 280.0;
        if (arrowRepeatMs_ >= interval) {
            arrowRepeatMs_ = 0.0;
            arrowRepeatArmed_ = true;
            if (StepByArrow(pressedArrow_)) {
                changed = true;
            }
        }
    } else {
        arrowRepeatMs_ = 0.0;
        arrowRepeatArmed_ = false;
    }

    if (!dragging_) {
        const double gap = targetValue_ - value_;
        if (std::fabs(gap) > 0.00005) {
            const double blend = 1.0 - std::exp(-delta / 70.0);
            value_ += gap * std::max(0.0, blend);
            if (std::fabs(targetValue_ - value_) < 0.00005) {
                value_ = targetValue_;
            }
            changed = true;
        }
    }

    float target = 1.0f;
    if (!IsVisible()) {
        // A bar that is simply not needed stays ready. Only auto-hide rests at
        // the hidden opacity, and it comes back the next time it is visible.
        target = style_.autoHide ? 0.0f : 1.0f;
    } else if (style_.autoHide) {
        const bool active =
            pointerInside_ || dragging_ || focused_ ||
            interaction_ == ScrollbarInteraction::Dragging ||
            pressedArrow_ != ScrollbarArrow::None || hotArrow_ != ScrollbarArrow::None;
        if (active) {
            idleMs_ = 0.0;
        } else {
            idleMs_ += delta;
            if (idleMs_ >= style_.autoHideDelay) {
                target = static_cast<float>(style_.hiddenOpacity);
            }
        }
    }
    revealTarget_ = target;

    const float step = static_cast<float>(delta / std::max(1.0, style_.fadeDuration));
    const float previous = reveal_;
    if (reveal_ < revealTarget_) {
        reveal_ = std::min(revealTarget_, reveal_ + step);
    } else if (reveal_ > revealTarget_) {
        reveal_ = std::max(revealTarget_, reveal_ - step);
    }
    return changed || std::fabs(reveal_ - previous) > 0.0005f;
}

bool Scrollbar::NeedsAnimation() const {
    if (std::fabs(targetValue_ - value_) > 0.00008) {
        return true;
    }
    if (pressedArrow_ != ScrollbarArrow::None) {
        return true;
    }
    if (std::fabs(reveal_ - revealTarget_) > 0.002f) {
        return true;
    }
    if (style_.autoHide && idleMs_ < style_.autoHideDelay) {
        const bool active =
            pointerInside_ || dragging_ || focused_ ||
            interaction_ == ScrollbarInteraction::Dragging ||
            pressedArrow_ != ScrollbarArrow::None || hotArrow_ != ScrollbarArrow::None;
        if (!active) {
            return true;  // Still counting down towards the fade-out.
        }
    }
    return false;
}

float Scrollbar::GetRevealOpacity() const {
    return reveal_;
}

Color Scrollbar::ResolveThumbColor() const {
    if (interaction_ == ScrollbarInteraction::Dragging) {
        return style_.thumbActiveColor;
    }
    if (interaction_ == ScrollbarInteraction::Hovered) {
        return style_.thumbHoverColor;
    }
    return style_.thumbColor;
}

Color Scrollbar::ResolveTrackColor() const {
    return style_.trackColor;
}

Color Scrollbar::ResolveArrowColor(ScrollbarArrow arrow) const {
    if (arrow != ScrollbarArrow::None && arrow == pressedArrow_) {
        return style_.arrowActiveColor;
    }
    if (arrow != ScrollbarArrow::None && arrow == hotArrow_) {
        return style_.arrowHoverColor;
    }
    return style_.arrowColor;
}

float Scrollbar::ResolveOpacity() const {
    return static_cast<float>(style_.opacity);
}

float Scrollbar::ResolveEffectiveOpacity() const {
    if (!IsVisible()) {
        return 0.0f;
    }
    return static_cast<float>(style_.opacity) * reveal_;
}

bool Scrollbar::ThumbContains(float x, float y) const {
    return ResolveThumbBounds().Contains(x, y);
}

double Scrollbar::TrackCoordinate(float x, float y) const {
    if (style_.orientation == ScrollbarOrientation::Vertical) {
        return static_cast<double>(y) - static_cast<double>(bounds_.y);
    }
    return static_cast<double>(x) - static_cast<double>(bounds_.x);
}

void Scrollbar::JumpTo(float x, float y) {
    BeginDrag(x, y);
}

void Scrollbar::BeginDrag(float x, float y) {
    targetValue_ = value_;
    const double trackLength = ResolveThumbTrackLength();
    const double thumbLength = ResolveThumbLength(trackLength);
    const double maxStart = std::max(0.0, trackLength - thumbLength);
    const double coordinate = TrackCoordinate(x, y) - ResolveThumbTrackStart();

    double start = ResolveThumbOffset(trackLength);
    if (!ThumbContains(x, y) && maxStart > 0.0) {
        start = std::max(0.0, std::min(coordinate - thumbLength * 0.5, maxStart));
        value_ = start / maxStart;
        targetValue_ = value_;
    }

    dragGrabOffset_ = coordinate - start;
    dragging_ = true;
    interaction_ = ScrollbarInteraction::Dragging;
    NotifyInteraction();
}

void Scrollbar::UpdateDrag(float x, float y) {
    if (!dragging_) {
        return;
    }
    const double trackLength = ResolveThumbTrackLength();
    const double thumbLength = ResolveThumbLength(trackLength);
    const double maxStart = std::max(0.0, trackLength - thumbLength);
    if (maxStart <= 0.0) {
        return;
    }
    const double coordinate = TrackCoordinate(x, y) - ResolveThumbTrackStart();
    const double start = std::max(0.0, std::min(coordinate - dragGrabOffset_, maxStart));
    value_ = start / maxStart;
    targetValue_ = value_;
    NotifyInteraction();
}

void Scrollbar::EndDrag() {
    dragging_ = false;
    if (interaction_ == ScrollbarInteraction::Dragging) {
        interaction_ = ScrollbarInteraction::Normal;
    }
}

bool Scrollbar::IsDragging() const {
    return dragging_;
}

ScrollbarOrientation ParseScrollbarOrientation(const std::string& value) {
    if (value == "horizontal" || value == "h" || value == "row") {
        return ScrollbarOrientation::Horizontal;
    }
    return ScrollbarOrientation::Vertical;
}

ScrollbarVisibility ParseScrollbarVisibility(const std::string& value) {
    if (value == "auto" || value == "overflow" || value == "automatic") {
        return ScrollbarVisibility::Auto;
    }
    return ScrollbarVisibility::Always;
}

bool ParseScrollbarFlag(const std::string& value, bool fallback) {
    if (value == "true" || value == "1" || value == "yes") {
        return true;
    }
    if (value == "false" || value == "0" || value == "no") {
        return false;
    }
    return fallback;
}

std::string ScrollbarVisibilityToCss(ScrollbarVisibility visibility) {
    return visibility == ScrollbarVisibility::Auto ? "auto" : "scroll";
}

}  // namespace Robo
