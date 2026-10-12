#include "Robo/FrameBuffer.hpp"

#include <algorithm>
#include <cmath>

namespace Robo {

FrameBuffer::FrameBuffer(int width, int height) {
    Resize(width, height);
}

void FrameBuffer::Resize(int width, int height) {
    width_ = std::max(0, width);
    height_ = std::max(0, height);
    pixels_.assign(static_cast<std::size_t>(width_ * height_ * 4), 0);
}

void FrameBuffer::Clear(const Color& color) {
    for (int i = 0; i < width_ * height_; ++i) {
        const std::size_t index = static_cast<std::size_t>(i) * 4;
        pixels_[index + 0] = color.red;
        pixels_[index + 1] = color.green;
        pixels_[index + 2] = color.blue;
        pixels_[index + 3] = color.alpha;
    }
}

void FrameBuffer::ClearRect(int x, int y, int width, int height, const Color& color) {
    int x0 = std::max(0, x);
    int y0 = std::max(0, y);
    int x1 = std::min(width_, x + width);
    int y1 = std::min(height_, y + height);
    if (!clips_.empty()) {
        const ClipRect& clip = clips_.back();
        x0 = std::max(x0, clip.left);
        y0 = std::max(y0, clip.top);
        x1 = std::min(x1, clip.right);
        y1 = std::min(y1, clip.bottom);
    }
    if (x0 >= x1 || y0 >= y1) {
        return;
    }
    for (int py = y0; py < y1; ++py) {
        std::uint8_t* row = pixels_.data() + static_cast<std::size_t>(py * width_ + x0) * 4;
        for (int px = x0; px < x1; ++px) {
            row[0] = color.red;
            row[1] = color.green;
            row[2] = color.blue;
            row[3] = color.alpha;
            row += 4;
        }
    }
}

int FrameBuffer::GetWidth() const {
    return width_;
}

int FrameBuffer::GetHeight() const {
    return height_;
}

const std::uint8_t* FrameBuffer::GetPixels() const {
    return pixels_.data();
}

std::uint8_t* FrameBuffer::GetPixels() {
    return pixels_.data();
}

void FrameBuffer::SetPresentRect(int x, int y, int width, int height) {
    presentX_ = std::max(0, x);
    presentY_ = std::max(0, y);
    presentWidth_ = std::max(0, width);
    presentHeight_ = std::max(0, height);
}

int FrameBuffer::GetPresentX() const {
    return presentX_;
}

int FrameBuffer::GetPresentY() const {
    return presentY_;
}

int FrameBuffer::GetPresentWidth() const {
    return presentWidth_;
}

int FrameBuffer::GetPresentHeight() const {
    return presentHeight_;
}

bool FrameBuffer::MissesClip(int x, int y, int width, int height) const {
    if (clips_.empty() || width <= 0 || height <= 0) {
        return false;
    }
    const ClipRect& clip = clips_.back();
    return x + width <= clip.left || y + height <= clip.top || x >= clip.right || y >= clip.bottom;
}

namespace {

float LinearOf[256];
std::uint8_t SrgbOf[4096];
bool ColorTablesReady = false;

void EnsureColorTables() {
    if (ColorTablesReady) {
        return;
    }
    for (int i = 0; i < 256; ++i) {
        const float channel = static_cast<float>(i) / 255.0f;
        LinearOf[i] = channel <= 0.04045f ? channel / 12.92f : std::pow((channel + 0.055f) / 1.055f, 2.4f);
    }
    for (int i = 0; i < 4096; ++i) {
        const float channel = static_cast<float>(i) / 4095.0f;
        const float encoded = channel <= 0.0031308f ? channel * 12.92f : 1.055f * std::pow(channel, 1.0f / 2.4f) - 0.055f;
        SrgbOf[i] = static_cast<std::uint8_t>(std::min(255.0f, encoded * 255.0f + 0.5f));
    }
    ColorTablesReady = true;
}

std::uint8_t EncodeSrgb(float linear) {
    const int index = static_cast<int>(std::max(0.0f, std::min(1.0f, linear)) * 4095.0f + 0.5f);
    return SrgbOf[index];
}

}  // namespace

void FrameBuffer::BlendPixel(int x, int y, const Color& color, float opacity) {
    if (x < 0 || y < 0 || x >= width_ || y >= height_) {
        return;
    }
    if (!clips_.empty() && !InsideClip(x, y)) {
        return;
    }

    const float sourceAlpha = (static_cast<float>(color.alpha) / 255.0f) * opacity;
    if (sourceAlpha <= 0.0f) {
        return;
    }

    const std::size_t index = static_cast<std::size_t>((y * width_ + x) * 4);
    if (sourceAlpha >= 0.996f) {
        pixels_[index + 0] = color.red;
        pixels_[index + 1] = color.green;
        pixels_[index + 2] = color.blue;
        pixels_[index + 3] = 255;
        return;
    }

    EnsureColorTables();
    const float destR = LinearOf[pixels_[index + 0]];
    const float destG = LinearOf[pixels_[index + 1]];
    const float destB = LinearOf[pixels_[index + 2]];
    const float destA = pixels_[index + 3] / 255.0f;
    const float srcR = LinearOf[color.red];
    const float srcG = LinearOf[color.green];
    const float srcB = LinearOf[color.blue];
    const float outA = sourceAlpha + destA * (1.0f - sourceAlpha);
    if (outA <= 0.0f) {
        return;
    }
    const float inv = 1.0f / outA;
    const float keep = destA * (1.0f - sourceAlpha);
    pixels_[index + 0] = EncodeSrgb((srcR * sourceAlpha + destR * keep) * inv);
    pixels_[index + 1] = EncodeSrgb((srcG * sourceAlpha + destG * keep) * inv);
    pixels_[index + 2] = EncodeSrgb((srcB * sourceAlpha + destB * keep) * inv);
    pixels_[index + 3] = static_cast<std::uint8_t>(std::min(255.0f, outA * 255.0f + 0.5f));
}

void FrameBuffer::FillRoundedRect(float x, float y, float width, float height, float radius, const Color& color, float opacity) {
    if (width <= 0.0f || height <= 0.0f || opacity <= 0.0f || color.alpha == 0) {
        return;
    }

    const float cappedRadius = std::max(0.0f, std::min(radius, std::min(width, height) * 0.5f));
    const float centerX = x + width * 0.5f;
    const float centerY = y + height * 0.5f;
    const float halfW = width * 0.5f - cappedRadius;
    const float halfH = height * 0.5f - cappedRadius;
    int x0 = static_cast<int>(std::floor(x - 1.0f));
    int y0 = static_cast<int>(std::floor(y - 1.0f));
    int x1 = static_cast<int>(std::ceil(x + width + 1.0f));
    int y1 = static_cast<int>(std::ceil(y + height + 1.0f));
    if (x1 <= 0 || y1 <= 0 || x0 >= width_ || y0 >= height_) {
        return;
    }
    x0 = std::max(0, x0);
    y0 = std::max(0, y0);
    x1 = std::min(width_, x1);
    y1 = std::min(height_, y1);

    const int innerLeft = std::max(x0, static_cast<int>(std::ceil(x + 1.0f)));
    const int innerRight = std::min(x1, static_cast<int>(std::floor(x + width - 1.0f)));
    const int innerTop = std::max(y0, static_cast<int>(std::ceil(y + cappedRadius)));
    const int innerBottom = std::min(y1, static_cast<int>(std::floor(y + height - cappedRadius)));
    const bool solid = innerRight > innerLeft && innerBottom > innerTop;
    if (solid) {
        FillRect(innerLeft, innerTop, innerRight - innerLeft, innerBottom - innerTop, color, opacity);
    }

    auto Cover = [&](int px, int py) {
        const float fx = static_cast<float>(px) + 0.5f - centerX;
        const float fy = static_cast<float>(py) + 0.5f - centerY;
        const float qx = std::abs(fx) - halfW;
        const float qy = std::abs(fy) - halfH;
        const float outsideX = std::max(qx, 0.0f);
        const float outsideY = std::max(qy, 0.0f);
        const float distance = std::sqrt(outsideX * outsideX + outsideY * outsideY) + std::min(std::max(qx, qy), 0.0f) - cappedRadius;
        const float coverage = std::max(0.0f, std::min(1.0f, 0.5f - distance));
        if (coverage > 0.0f) {
            BlendPixel(px, py, color, opacity * coverage);
        }
    };

    if (!solid) {
        for (int py = y0; py < y1; ++py) {
            for (int px = x0; px < x1; ++px) {
                Cover(px, py);
            }
        }
        return;
    }

    for (int py = y0; py < innerTop; ++py) {
        for (int px = x0; px < x1; ++px) {
            Cover(px, py);
        }
    }
    for (int py = innerTop; py < innerBottom; ++py) {
        for (int px = x0; px < innerLeft; ++px) {
            Cover(px, py);
        }
        for (int px = innerRight; px < x1; ++px) {
            Cover(px, py);
        }
    }
    for (int py = innerBottom; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            Cover(px, py);
        }
    }
}

void FrameBuffer::FillRect(int x, int y, int width, int height, const Color& color, float opacity) {
    int x0 = std::max(0, x);
    int y0 = std::max(0, y);
    int x1 = std::min(width_, x + width);
    int y1 = std::min(height_, y + height);
    if (!clips_.empty()) {
        const ClipRect& clip = clips_.back();
        x0 = std::max(x0, clip.left);
        y0 = std::max(y0, clip.top);
        x1 = std::min(x1, clip.right);
        y1 = std::min(y1, clip.bottom);
    }
    if (x0 >= x1 || y0 >= y1) {
        return;
    }
    if (opacity >= 0.996f && color.alpha == 255) {
        for (int py = y0; py < y1; ++py) {
            std::uint8_t* row = pixels_.data() + static_cast<std::size_t>(py * width_ + x0) * 4;
            for (int px = x0; px < x1; ++px) {
                row[0] = color.red;
                row[1] = color.green;
                row[2] = color.blue;
                row[3] = 255;
                row += 4;
            }
        }
        return;
    }
    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            BlendPixel(px, py, color, opacity);
        }
    }
}

bool FrameBuffer::InsideClip(int x, int y) const {
    for (const ClipRect& clip : clips_) {
        if (x < clip.left || y < clip.top || x >= clip.right || y >= clip.bottom) {
            return false;
        }
    }
    return true;
}

void FrameBuffer::PushClipRect(float x, float y, float width, float height) {
    const int left = std::max(0, static_cast<int>(std::floor(x)));
    const int top = std::max(0, static_cast<int>(std::floor(y)));
    const int right = std::min(width_, static_cast<int>(std::ceil(x + std::max(0.0f, width))));
    const int bottom = std::min(height_, static_cast<int>(std::ceil(y + std::max(0.0f, height))));

    ClipRect rect{ left, top, std::max(left, right), std::max(top, bottom) };
    if (!clips_.empty()) {
        const ClipRect& parent = clips_.back();
        rect.left = std::max(rect.left, parent.left);
        rect.top = std::max(rect.top, parent.top);
        rect.right = std::min(rect.right, parent.right);
        rect.bottom = std::min(rect.bottom, parent.bottom);
    }
    clips_.push_back(rect);
}

void FrameBuffer::PopClip() {
    if (!clips_.empty()) {
        clips_.pop_back();
    }
}

bool FrameBuffer::HasClip() const {
    return !clips_.empty();
}

void FrameBuffer::ResetClip() {
    clips_.clear();
}

}  // namespace Robo
