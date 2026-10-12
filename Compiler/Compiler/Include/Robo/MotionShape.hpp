#pragma once

#include "Color.hpp"
#include "FrameBuffer.hpp"
#include "Motion.hpp"
#include "Svg.hpp"

#include <string>
#include <vector>

namespace Robo {

struct MotionShapeBounds {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    bool Contains(float pointX, float pointY) const;
};

// A drawn rect, ellipse, or path. Theme, hover, and keyframe motion
// interpolate the same fields the page sets in XML.
class MotionShape {
public:
    enum class Kind {
        Rect,
        Ellipse,
        Path
    };

    void SetKind(Kind kind);
    void SetX(float value);
    void SetY(float value);
    void SetDarkX(float value);
    void SetDarkY(float value);
    void SetBottom(float value);
    void SetDarkBottom(float value);
    void SetWidth(float value);
    void SetHeight(float value);
    void SetRadius(float value);
    void SetHole(float value);
    void SetFill(const Color& color);
    void SetFillEnd(const Color& color);
    void SetDarkFill(const Color& color);
    void SetDarkFillEnd(const Color& color);
    void SetHoverFill(const Color& color);
    void SetOpacity(float value);
    void SetDarkOpacity(float value);
    void SetScale(float value);
    void SetDarkScale(float value);
    void SetRotate(float value);
    void SetDarkRotate(float value);
    void SetHoverX(float value);
    void SetDarkHoverX(float value);
    void SetHoverY(float value);
    void SetHoverScale(float value);
    void SetHoverRotate(float value);
    void SetHoverWithin(bool value);
    void SetClip(bool value);
    void SetToggleTheme(bool value);
    void SetHorizontal(bool value);
    void SetTransition(float durationMs, const MotionEasing& easing);
    void SetAnimation(const std::string& name, float delayMs);
    void SetViewBox(float x, float y, float width, float height);
    void AddCommand(SvgCommand command);
    void AddChild(MotionShape child);

    Kind GetKind() const;
    float Width() const;
    float Height() const;
    bool TogglesTheme() const;
    const MotionShapeBounds& Bounds() const;

    void Tick(float deltaMs, bool parentHot);
    bool NeedsMotion() const;
    bool UpdatePointer(float x, float y);
    void ScaleBounds(float factor);

    void Paint(FrameBuffer& buffer, float originX, float originY, float parentWidth, float parentHeight, float pixelScale) const;

private:
    void ShiftBounds(float dx, float dy) const;
    Kind kind_ = Kind::Rect;
    float x_ = 0.0f;
    float y_ = 0.0f;
    float darkX_ = 0.0f;
    float darkY_ = 0.0f;
    float bottom_ = 0.0f;
    float darkBottom_ = 0.0f;
    float width_ = 0.0f;
    float height_ = 0.0f;
    float radius_ = 0.0f;
    float hole_ = 0.0f;
    Color fill_ = Color::FromRgba(0, 0, 0, 0);
    Color fillEnd_ = Color::FromRgba(0, 0, 0, 0);
    Color darkFill_ = Color::FromRgba(0, 0, 0, 0);
    Color darkFillEnd_ = Color::FromRgba(0, 0, 0, 0);
    Color hoverFill_ = Color::FromRgba(0, 0, 0, 0);
    float opacity_ = 1.0f;
    float darkOpacity_ = 1.0f;
    float scale_ = 1.0f;
    float darkScale_ = 1.0f;
    float rotate_ = 0.0f;
    float darkRotate_ = 0.0f;
    float hoverX_ = 0.0f;
    float darkHoverX_ = 0.0f;
    float hoverY_ = 0.0f;
    float hoverScale_ = 1.0f;
    float hoverRotate_ = 0.0f;
    float transitionMs_ = 0.0f;
    float hoverDurationMs_ = 300.0f;
    float animationDelayMs_ = 0.0f;
    float viewX_ = 0.0f;
    float viewY_ = 0.0f;
    float viewWidth_ = 0.0f;
    float viewHeight_ = 0.0f;
    bool hasDarkX_ = false;
    bool hasDarkY_ = false;
    bool hasBottom_ = false;
    bool hasDarkBottom_ = false;
    bool hasDarkFill_ = false;
    bool hasDarkOpacity_ = false;
    bool hasDarkScale_ = false;
    bool hasDarkRotate_ = false;
    bool hasHoverX_ = false;
    bool hasDarkHoverX_ = false;
    bool hasHoverY_ = false;
    bool hasHoverScale_ = false;
    bool hasHoverRotate_ = false;
    bool hasHoverFill_ = false;
    bool hoverWithin_ = false;
    bool clip_ = false;
    bool toggleTheme_ = false;
    bool horizontal_ = false;
    MotionEasing transitionEasing_;
    std::string animation_;
    std::vector<SvgCommand> commands_;
    std::vector<MotionShape> children_;

    bool primed_ = false;
    bool mixing_ = false;
    float mix_ = 0.0f;
    float mixFrom_ = 0.0f;
    float mixTo_ = 0.0f;
    float mixElapsed_ = 0.0f;
    bool hoverPrimed_ = false;
    float hover_ = 0.0f;
    float hoverFrom_ = 0.0f;
    float hoverTo_ = 0.0f;
    float hoverElapsed_ = 0.0f;
    float hoverTarget_ = 0.0f;
    float elapsedMs_ = 0.0f;
    bool hot_ = false;
    mutable MotionShapeBounds bounds_;
};

}  // namespace Robo
