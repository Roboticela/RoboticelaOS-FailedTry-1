#include "Robo/MotionShape.hpp"

#include "Robo/Foundation.hpp"
#include "Robo/SvgPainter.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace Robo {

namespace {

float Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

Color LerpColor(const Color& a, const Color& b, float t) {
    return Color::FromRgba(static_cast<std::uint8_t>(Lerp(a.red, b.red, t)),
                           static_cast<std::uint8_t>(Lerp(a.green, b.green, t)),
                           static_cast<std::uint8_t>(Lerp(a.blue, b.blue, t)),
                           static_cast<std::uint8_t>(Lerp(a.alpha, b.alpha, t)));
}

float MixValue(float light, float dark, bool hasDark, float mix) {
    return hasDark ? Lerp(light, dark, mix) : light;
}

void FillGradient(FrameBuffer& buffer,
                  float x,
                  float y,
                  float width,
                  float height,
                  const Color& start,
                  const Color& end,
                  bool horizontal,
                  float opacity) {
    if (width <= 0.0f || height <= 0.0f || opacity <= 0.0f) {
        return;
    }
    if (end.alpha == 0 && start.alpha == 0) {
        return;
    }
    if (end.alpha == 0 || (start.red == end.red && start.green == end.green && start.blue == end.blue && start.alpha == end.alpha)) {
        buffer.FillRect(static_cast<int>(std::floor(x)), static_cast<int>(std::floor(y)),
                        static_cast<int>(std::ceil(width)), static_cast<int>(std::ceil(height)), start, opacity);
        return;
    }
    const int steps = std::max(1, static_cast<int>(horizontal ? width : height));
    for (int step = 0; step < steps; ++step) {
        const float t = steps == 1 ? 0.0f : static_cast<float>(step) / static_cast<float>(steps - 1);
        const Color color = LerpColor(start, end, t);
        if (horizontal) {
            buffer.FillRect(static_cast<int>(std::floor(x + step)), static_cast<int>(std::floor(y)), 1,
                            static_cast<int>(std::ceil(height)), color, opacity);
        } else {
            buffer.FillRect(static_cast<int>(std::floor(x)), static_cast<int>(std::floor(y + step)),
                            static_cast<int>(std::ceil(width)), 1, color, opacity);
        }
    }
}

void FillAnnulus(FrameBuffer& buffer, float x, float y, float width, float height, float innerDiameter, const Color& color, float opacity) {
    if (width <= 0.0f || height <= 0.0f || opacity <= 0.0f || color.alpha == 0) {
        return;
    }
    const float outer = std::min(width, height) * 0.5f;
    const float inner = std::max(0.0f, std::min(innerDiameter * 0.5f, outer));
    const float cx = x + width * 0.5f;
    const float cy = y + height * 0.5f;
    const int x0 = std::max(0, static_cast<int>(std::floor(x - 1.0f)));
    const int y0 = std::max(0, static_cast<int>(std::floor(y - 1.0f)));
    const int x1 = std::min(buffer.GetWidth(), static_cast<int>(std::ceil(x + width + 1.0f)));
    const int y1 = std::min(buffer.GetHeight(), static_cast<int>(std::ceil(y + height + 1.0f)));
    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            const float dx = static_cast<float>(px) + 0.5f - cx;
            const float dy = static_cast<float>(py) + 0.5f - cy;
            const float distance = std::sqrt(dx * dx + dy * dy);
            const float outerCoverage = std::max(0.0f, std::min(1.0f, outer - distance + 0.5f));
            const float innerCoverage = std::max(0.0f, std::min(1.0f, distance - inner + 0.5f));
            const float coverage = outerCoverage * innerCoverage;
            if (coverage > 0.0f) {
                buffer.BlendPixel(px, py, color, opacity * coverage);
            }
        }
    }
}

float RoundedCoverage(float px, float py, float width, float height, float radius) {
    radius = std::max(0.0f, std::min(radius, std::min(width, height) * 0.5f));
    const float cx = width * 0.5f;
    const float cy = height * 0.5f;
    const float halfW = width * 0.5f - radius;
    const float halfH = height * 0.5f - radius;
    const float qx = std::fabs(px - cx) - halfW;
    const float qy = std::fabs(py - cy) - halfH;
    const float outsideX = std::max(qx, 0.0f);
    const float outsideY = std::max(qy, 0.0f);
    const float distance = std::sqrt(outsideX * outsideX + outsideY * outsideY) + std::min(std::max(qx, qy), 0.0f) - radius;
    return std::max(0.0f, std::min(1.0f, 0.5f - distance));
}

void BlitMasked(FrameBuffer& destination,
                const FrameBuffer& source,
                float originX,
                float originY,
                float radius,
                float scale,
                float degrees,
                float opacity) {
    if (scale <= 0.0f || opacity <= 0.0f) {
        return;
    }
    const float srcW = static_cast<float>(source.GetWidth());
    const float srcH = static_cast<float>(source.GetHeight());
    const float destW = srcW * scale;
    const float destH = srcH * scale;
    const float radians = degrees * 0.0174532925f;
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    const float centerX = originX + srcW * 0.5f;
    const float centerY = originY + srcH * 0.5f;
    const int x0 = static_cast<int>(std::floor(centerX - destW * 0.5f - 2.0f));
    const int y0 = static_cast<int>(std::floor(centerY - destH * 0.5f - 2.0f));
    const int x1 = static_cast<int>(std::ceil(centerX + destW * 0.5f + 2.0f));
    const int y1 = static_cast<int>(std::ceil(centerY + destH * 0.5f + 2.0f));
    const std::uint8_t* pixels = source.GetPixels();
    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            const float dx = static_cast<float>(px) + 0.5f - centerX;
            const float dy = static_cast<float>(py) + 0.5f - centerY;
            const float localX = (cosine * dx + sine * dy) / scale + srcW * 0.5f;
            const float localY = (-sine * dx + cosine * dy) / scale + srcH * 0.5f;
            if (localX < 0.0f || localY < 0.0f || localX >= srcW || localY >= srcH) {
                continue;
            }
            const float coverage = radius > 0.0f ? RoundedCoverage(localX, localY, srcW, srcH, radius) : 1.0f;
            if (coverage <= 0.0f) {
                continue;
            }
            const int sx = std::min(source.GetWidth() - 1, std::max(0, static_cast<int>(localX)));
            const int sy = std::min(source.GetHeight() - 1, std::max(0, static_cast<int>(localY)));
            const std::size_t index = static_cast<std::size_t>((sy * source.GetWidth() + sx) * 4);
            const float sourceAlpha = pixels[index + 3] / 255.0f;
            if (sourceAlpha <= 0.0f) {
                continue;
            }
            destination.BlendPixel(px, py,
                                   Color::FromRgba(pixels[index], pixels[index + 1], pixels[index + 2], 255),
                                   opacity * coverage * sourceAlpha);
        }
    }
}

}  // namespace

bool MotionShapeBounds::Contains(float pointX, float pointY) const {
    return width > 0.0f && height > 0.0f && pointX >= x && pointY >= y && pointX < x + width && pointY < y + height;
}

void MotionShape::SetKind(Kind kind) { kind_ = kind; }
void MotionShape::SetX(float value) { x_ = value; }
void MotionShape::SetY(float value) { y_ = value; }
void MotionShape::SetDarkX(float value) { darkX_ = value; hasDarkX_ = true; }
void MotionShape::SetDarkY(float value) { darkY_ = value; hasDarkY_ = true; }
void MotionShape::SetBottom(float value) { bottom_ = value; hasBottom_ = true; }
void MotionShape::SetDarkBottom(float value) { darkBottom_ = value; hasDarkBottom_ = true; }
void MotionShape::SetWidth(float value) { width_ = value; }
void MotionShape::SetHeight(float value) { height_ = value; }
void MotionShape::SetRadius(float value) { radius_ = value; }
void MotionShape::SetHole(float value) { hole_ = value; }
void MotionShape::SetFill(const Color& color) { fill_ = color; }
void MotionShape::SetFillEnd(const Color& color) { fillEnd_ = color; }
void MotionShape::SetDarkFill(const Color& color) { darkFill_ = color; hasDarkFill_ = true; }
void MotionShape::SetDarkFillEnd(const Color& color) { darkFillEnd_ = color; }
void MotionShape::SetHoverFill(const Color& color) { hoverFill_ = color; hasHoverFill_ = true; }
void MotionShape::SetOpacity(float value) { opacity_ = value; }
void MotionShape::SetDarkOpacity(float value) { darkOpacity_ = value; hasDarkOpacity_ = true; }
void MotionShape::SetScale(float value) { scale_ = value; }
void MotionShape::SetDarkScale(float value) { darkScale_ = value; hasDarkScale_ = true; }
void MotionShape::SetRotate(float value) { rotate_ = value; }
void MotionShape::SetDarkRotate(float value) { darkRotate_ = value; hasDarkRotate_ = true; }
void MotionShape::SetHoverX(float value) { hoverX_ = value; hasHoverX_ = true; }
void MotionShape::SetDarkHoverX(float value) { darkHoverX_ = value; hasDarkHoverX_ = true; }
void MotionShape::SetHoverY(float value) { hoverY_ = value; hasHoverY_ = true; }
void MotionShape::SetHoverScale(float value) { hoverScale_ = value; hasHoverScale_ = true; }
void MotionShape::SetHoverRotate(float value) { hoverRotate_ = value; hasHoverRotate_ = true; }
void MotionShape::SetHoverWithin(bool value) { hoverWithin_ = value; }
void MotionShape::SetClip(bool value) { clip_ = value; }
void MotionShape::SetToggleTheme(bool value) { toggleTheme_ = value; }
void MotionShape::SetHorizontal(bool value) { horizontal_ = value; }
void MotionShape::SetTransition(float durationMs, const MotionEasing& easing) {
    transitionMs_ = durationMs;
    transitionEasing_ = easing;
}
void MotionShape::SetAnimation(const std::string& name, float delayMs) {
    animation_ = name;
    animationDelayMs_ = delayMs;
}
void MotionShape::SetViewBox(float x, float y, float width, float height) {
    viewX_ = x;
    viewY_ = y;
    viewWidth_ = width;
    viewHeight_ = height;
}
void MotionShape::AddCommand(SvgCommand command) { commands_.push_back(std::move(command)); }
void MotionShape::AddChild(MotionShape child) { children_.push_back(std::move(child)); }
MotionShape::Kind MotionShape::GetKind() const { return kind_; }
float MotionShape::Width() const { return width_; }
float MotionShape::Height() const { return height_; }
bool MotionShape::TogglesTheme() const { return toggleTheme_; }
const MotionShapeBounds& MotionShape::Bounds() const { return bounds_; }

void MotionShape::Tick(float deltaMs, bool parentHot) {
    const float target = CurrentTheme() == Theme::Dark ? 1.0f : 0.0f;
    if (!primed_) {
        mix_ = target;
        mixTo_ = target;
        primed_ = true;
    } else if (std::fabs(mixTo_ - target) > 0.0001f || std::fabs(mix_ - target) > 0.0001f) {
        if (!mixing_ || std::fabs(mixTo_ - target) > 0.0001f) {
            mixFrom_ = mix_;
            mixTo_ = target;
            mixElapsed_ = 0.0f;
            mixing_ = transitionMs_ > 0.0f;
        }
        if (!mixing_) {
            mix_ = target;
        } else {
            mixElapsed_ += deltaMs;
            const float progress = std::min(1.0f, mixElapsed_ / std::max(1.0f, transitionMs_));
            mix_ = Lerp(mixFrom_, mixTo_, Ease(progress, transitionEasing_));
            if (progress >= 1.0f) {
                mixing_ = false;
                mix_ = target;
            }
        }
    }
    const bool hot = hoverWithin_ ? parentHot : hot_;
    const float hoverTarget = hot ? 1.0f : 0.0f;
    hoverTarget_ = hoverTarget;
    if (!hoverPrimed_) {
        hover_ = hoverTarget;
        hoverFrom_ = hoverTarget;
        hoverTo_ = hoverTarget;
        hoverPrimed_ = true;
    } else if (std::fabs(hoverTo_ - hoverTarget) > 0.0001f) {
        hoverFrom_ = hover_;
        hoverTo_ = hoverTarget;
        hoverElapsed_ = 0.0f;
    }
    if (std::fabs(hover_ - hoverTo_) > 0.0001f) {
        const float duration = transitionMs_ > 0.0f ? transitionMs_ : hoverDurationMs_;
        const MotionEasing easing = transitionMs_ > 0.0f ? transitionEasing_ : ParseEasing("ease");
        hoverElapsed_ += deltaMs;
        const float progress = std::min(1.0f, hoverElapsed_ / std::max(1.0f, duration));
        hover_ = Lerp(hoverFrom_, hoverTo_, Ease(progress, easing));
        if (progress >= 1.0f) {
            hover_ = hoverTo_;
        }
    }
    elapsedMs_ += deltaMs;
    const bool childHot = hot_ || parentHot;
    for (MotionShape& child : children_) {
        child.Tick(deltaMs, childHot);
    }
}

bool MotionShape::NeedsMotion() const {
    const float target = CurrentTheme() == Theme::Dark ? 1.0f : 0.0f;
    if (!primed_ || std::fabs(mix_ - target) > 0.001f) {
        return transitionMs_ > 0.0f || !primed_;
    }
    if (std::fabs(hover_ - hoverTarget_) > 0.001f) {
        return true;
    }
    if (!hoverWithin_ && std::fabs(hover_ - (hot_ ? 1.0f : 0.0f)) > 0.001f) {
        return true;
    }
    if (!animation_.empty()) {
        const MotionClip* clip = FindMotion(animation_);
        if (clip != nullptr && clip->infinite && mix_ > 0.02f) {
            return true;
        }
        if (clip != nullptr && !clip->infinite && elapsedMs_ < clip->delayMs + clip->durationMs + animationDelayMs_) {
            return true;
        }
    }
    for (const MotionShape& child : children_) {
        if (child.NeedsMotion()) {
            return true;
        }
    }
    return false;
}

bool MotionShape::UpdatePointer(float x, float y) {
    const bool next = bounds_.Contains(x, y);
    bool changed = next != hot_;
    hot_ = next;
    for (MotionShape& item : children_) {
        changed = item.UpdatePointer(x, y) || changed;
    }
    return changed;
}

void MotionShape::ShiftBounds(float dx, float dy) const {
    bounds_.x += dx;
    bounds_.y += dy;
    for (const MotionShape& child : children_) {
        child.ShiftBounds(dx, dy);
    }
}

void MotionShape::ScaleBounds(float factor) {
    bounds_.x *= factor;
    bounds_.y *= factor;
    bounds_.width *= factor;
    bounds_.height *= factor;
    for (MotionShape& child : children_) {
        child.ScaleBounds(factor);
    }
}

void MotionShape::Paint(FrameBuffer& buffer, float originX, float originY, float parentWidth, float parentHeight, float pixelScale) const {
    const float scale = std::max(0.5f, pixelScale);
    const MotionSample motion = SampleMotion(animation_, elapsedMs_, animationDelayMs_);
    const float hoverMix = hover_;
    const float theme = mix_;
    const float cssX = MixValue(x_, darkX_, hasDarkX_, theme) + MixValue(hasHoverX_ ? hoverX_ : 0.0f, hasDarkHoverX_ ? darkHoverX_ : hoverX_, hasDarkHoverX_, theme) * hoverMix;
    float cssY = MixValue(y_, darkY_, hasDarkY_, theme);
    if (hasBottom_) {
        const float bottom = MixValue(bottom_, darkBottom_, hasDarkBottom_, theme);
        cssY = (parentHeight / scale) - height_ - bottom;
    }
    cssY += (hasHoverY_ ? hoverY_ : 0.0f) * hoverMix;
    if (motion.hasX) {
        cssY += 0.0f;
    }
    const float drawX = originX + (cssX + (motion.hasX ? motion.x : 0.0f)) * scale;
    const float drawY = originY + (cssY + (motion.hasY ? motion.y : 0.0f)) * scale;
    const float drawW = std::max(1.0f, width_ * scale);
    const float drawH = std::max(1.0f, height_ * scale);
    const float visualScale = MixValue(scale_, darkScale_, hasDarkScale_, theme) * (motion.hasScale ? motion.scale : 1.0f) *
                              (hasHoverScale_ ? Lerp(1.0f, hoverScale_, hoverMix) : 1.0f);
    const float visualRotate = MixValue(rotate_, darkRotate_, hasDarkRotate_, theme) + (motion.hasRotate ? motion.rotate : 0.0f) +
                               (hasHoverRotate_ ? hoverRotate_ * hoverMix : 0.0f);
    float opacity = MixValue(opacity_, darkOpacity_, hasDarkOpacity_, theme);
    if (motion.hasOpacity) {
        opacity *= motion.opacity;
    }
    Color fill = fill_;
    Color fillEnd = fillEnd_;
    if (hasDarkFill_) {
        fill = LerpColor(fill_, darkFill_, theme);
        if (darkFillEnd_.alpha > 0 || fillEnd_.alpha > 0) {
            fillEnd = LerpColor(fillEnd_, darkFillEnd_.alpha > 0 ? darkFillEnd_ : fillEnd_, theme);
        }
    }
    if (hasHoverFill_ && hoverMix > 0.0f) {
        fill = LerpColor(fill, hoverFill_, hoverMix);
    }
    bounds_.x = drawX;
    bounds_.y = drawY;
    bounds_.width = drawW;
    bounds_.height = drawH;
    if (opacity <= 0.001f) {
        return;
    }

    const bool spin = std::fabs(visualRotate) > 0.1f;
    const bool zoom = std::fabs(visualScale - 1.0f) > 0.001f;
    const bool fade = opacity < 0.999f;
    // Scale alone stays on the direct path so a cloud bank can grow without
    // being cropped to the small element box.
    const bool isolate = clip_ || spin || fade || (zoom && kind_ == Kind::Path);
    if (!isolate) {
        const float centerX = drawX + drawW * 0.5f;
        const float centerY = drawY + drawH * 0.5f;
        const float paintScale = scale * visualScale;
        const float paintX = centerX + (drawX - centerX) * visualScale;
        const float paintY = centerY + (drawY - centerY) * visualScale;
        const float paintW = drawW * visualScale;
        const float paintH = drawH * visualScale;
        if (kind_ == Kind::Path && !commands_.empty()) {
            Svg svg;
            SvgStyle style;
            style.viewX = viewX_;
            style.viewY = viewY_;
            style.viewWidth = viewWidth_ > 0.0f ? viewWidth_ : width_;
            style.viewHeight = viewHeight_ > 0.0f ? viewHeight_ : height_;
            style.width = width_;
            style.height = height_;
            style.opacity = opacity;
            style.color = fill;
            svg.SetStyle(style);
            SvgShape shape;
            shape.commands = commands_;
            shape.hasFill = true;
            shape.currentFill = true;
            shape.hasStroke = false;
            shape.fillRule = SvgFillRule::EvenOdd;
            svg.AddShape(std::move(shape));
            SvgPainter::Paint(buffer, svg, paintX, paintY, paintW, paintScale);
        } else if (fill.alpha > 0 || fillEnd.alpha > 0) {
            if (kind_ == Kind::Ellipse && hole_ > 0.0f) {
                FillAnnulus(buffer, paintX, paintY, paintW, paintH, hole_ * paintScale, fill, opacity);
            } else if (kind_ == Kind::Ellipse || radius_ > 0.0f) {
                const float radius = kind_ == Kind::Ellipse ? std::min(paintW, paintH) * 0.5f : radius_ * paintScale;
                if (fillEnd.alpha == 0) {
                    buffer.FillRoundedRect(paintX, paintY, paintW, paintH, radius, fill, opacity);
                } else {
                    FrameBuffer layer(std::max(1, static_cast<int>(std::ceil(paintW))), std::max(1, static_cast<int>(std::ceil(paintH))));
                    layer.Clear(Color::FromRgba(0, 0, 0, 0));
                    FillGradient(layer, 0.0f, 0.0f, paintW, paintH, fill, fillEnd, horizontal_, 1.0f);
                    BlitMasked(buffer, layer, paintX, paintY, radius, 1.0f, 0.0f, opacity);
                }
            } else {
                FillGradient(buffer, paintX, paintY, paintW, paintH, fill, fillEnd, horizontal_, opacity);
            }
        }
        for (const MotionShape& child : children_) {
            child.Paint(buffer, paintX, paintY, paintW, paintH, paintScale);
        }
        return;
    }

    if (clip_ && kind_ == Kind::Rect && radius_ > 0.0f) {
        const float shadowRadius = std::min(radius_ * scale, std::min(drawW, drawH) * 0.5f);
        const float lip = 1.0f - theme;
        if (lip > 0.04f) {
            buffer.FillRoundedRect(drawX, drawY + 1.4f * scale, drawW, drawH, shadowRadius, Color::FromRgba(255, 255, 255, 255), 0.16f * lip);
        }
        buffer.FillRoundedRect(drawX, drawY - 1.2f * scale, drawW, drawH, shadowRadius, Color::FromRgba(0, 0, 0, 255), 0.18f);
    }

    const int layerW = std::max(1, static_cast<int>(std::ceil(drawW)));
    const int layerH = std::max(1, static_cast<int>(std::ceil(drawH)));
    FrameBuffer layer(layerW, layerH);
    layer.Clear(Color::FromRgba(0, 0, 0, 0));
    if (kind_ == Kind::Path && !commands_.empty()) {
        Svg svg;
        SvgStyle style;
        style.viewX = viewX_;
        style.viewY = viewY_;
        style.viewWidth = viewWidth_ > 0.0f ? viewWidth_ : width_;
        style.viewHeight = viewHeight_ > 0.0f ? viewHeight_ : height_;
        style.width = width_;
        style.height = height_;
        style.opacity = 1.0f;
        style.color = fill;
        svg.SetStyle(style);
        SvgShape shape;
        shape.commands = commands_;
        shape.hasFill = true;
        shape.currentFill = true;
        shape.hasStroke = false;
        shape.fillRule = SvgFillRule::EvenOdd;
        svg.AddShape(std::move(shape));
        SvgPainter::Paint(layer, svg, 0.0f, 0.0f, drawW, scale);
    } else if (fill.alpha > 0 || fillEnd.alpha > 0) {
        if (kind_ == Kind::Ellipse || radius_ > 0.0f) {
            const float radius = kind_ == Kind::Ellipse ? std::min(drawW, drawH) * 0.5f : radius_ * scale;
            if (fillEnd.alpha == 0) {
                layer.FillRoundedRect(0.0f, 0.0f, drawW, drawH, radius, fill, 1.0f);
            } else {
                FillGradient(layer, 0.0f, 0.0f, drawW, drawH, fill, fillEnd, horizontal_, 1.0f);
            }
        } else {
            FillGradient(layer, 0.0f, 0.0f, drawW, drawH, fill, fillEnd, horizontal_, 1.0f);
        }
    }
    for (const MotionShape& child : children_) {
        child.Paint(layer, 0.0f, 0.0f, drawW, drawH, scale);
        child.ShiftBounds(drawX, drawY);
    }
    if (clip_ && kind_ == Kind::Rect) {
        const int shade = std::max(1, static_cast<int>(std::round(7.0f * scale)));
        for (int row = 0; row < shade; ++row) {
            const float fade = 1.0f - static_cast<float>(row) / static_cast<float>(shade);
            layer.FillRect(0, row, layerW, 1, Color::FromRgba(0, 0, 0, 255), 0.34f * fade * fade);
        }
    }
    float mask = 0.0f;
    if (kind_ == Kind::Ellipse) {
        mask = std::min(drawW, drawH) * 0.5f;
    } else if (clip_ || radius_ > 0.0f) {
        mask = std::min(radius_ * scale, std::min(drawW, drawH) * 0.5f);
    }
    BlitMasked(buffer, layer, drawX, drawY, mask, visualScale, visualRotate, opacity);
}

}  // namespace Robo
