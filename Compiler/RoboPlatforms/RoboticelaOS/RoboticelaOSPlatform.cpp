#include "RoboticelaOSPlatform.hpp"

#include <algorithm>
#ifndef ROBO_KERNEL
#include <fstream>
#include <iterator>
#endif

namespace Robo {

void RoboticelaOSPlatform::SetAssetRoot(const std::string& assetRoot) {
    assetRoot_ = assetRoot;
}

void RoboticelaOSPlatform::SetKeepPixel(bool (*keep)(void* cookie, int x, int y, const std::uint8_t* pixel), void* cookie) {
    keepPixel_ = keep;
    keepCookie_ = cookie;
}

void RoboticelaOSPlatform::SetScanout(std::uint8_t* pixels, int width, int height, int stride, std::uint32_t pixelFormat) {
    scanout_ = pixels;
    scanoutWidth_ = width > 0 ? width : 0;
    scanoutHeight_ = height > 0 ? height : 0;
    scanoutStride_ = stride > 0 ? stride : scanoutWidth_ * 4;
    pixelFormat_ = pixelFormat;
    if (scanoutWidth_ > 0 && scanoutHeight_ > 0) {
        width_ = scanoutWidth_;
        height_ = scanoutHeight_;
    }
}

int RoboticelaOSPlatform::GetSurfaceWidth() const {
    return width_;
}

int RoboticelaOSPlatform::GetSurfaceHeight() const {
    return height_;
}

float RoboticelaOSPlatform::GetPixelScale() const {
    return pixelScale_;
}

void RoboticelaOSPlatform::SetSurfaceSize(int width, int height) {
    width_ = width > 0 ? width : 1;
    height_ = height > 0 ? height : 1;
}

void RoboticelaOSPlatform::SetPixelScale(float scale) {
    pixelScale_ = std::max(0.5f, scale);
}

std::uint32_t RoboticelaOSPlatform::Pack(std::uint8_t red, std::uint8_t green, std::uint8_t blue) const {
    if (pixelFormat_ == ROBOTICELA_PIXEL_RGBX) {
        return (static_cast<std::uint32_t>(red) << 16) | (static_cast<std::uint32_t>(green) << 8) | blue;
    }
    return (static_cast<std::uint32_t>(blue) << 16) | (static_cast<std::uint32_t>(green) << 8) | red;
}

void RoboticelaOSPlatform::Present(const FrameBuffer& buffer) {
    width_ = buffer.GetWidth() > 0 ? buffer.GetWidth() : width_;
    height_ = buffer.GetHeight() > 0 ? buffer.GetHeight() : height_;
    int damageX = buffer.GetPresentX();
    int damageY = buffer.GetPresentY();
    int damageWidth = buffer.GetPresentWidth();
    int damageHeight = buffer.GetPresentHeight();
    if (damageWidth <= 0 || damageHeight <= 0) {
        damageX = 0;
        damageY = 0;
        damageWidth = width_;
        damageHeight = height_;
    }
    if (damageX < 0) {
        damageWidth += damageX;
        damageX = 0;
    }
    if (damageY < 0) {
        damageHeight += damageY;
        damageY = 0;
    }
    if (damageX + damageWidth > width_) {
        damageWidth = width_ - damageX;
    }
    if (damageY + damageHeight > height_) {
        damageHeight = height_ - damageY;
    }
    if (damageWidth < 0) {
        damageWidth = 0;
    }
    if (damageHeight < 0) {
        damageHeight = 0;
    }
    presentX_ = damageX;
    presentY_ = damageY;
    presentWidth_ = damageWidth;
    presentHeight_ = damageHeight;
    const std::uint8_t* source = buffer.GetPixels();
    if (scanout_ == nullptr || source == nullptr || damageWidth <= 0 || damageHeight <= 0 || width_ <= 0) {
        return;
    }
    const int copyWidth = std::min(damageWidth, std::max(0, scanoutWidth_ - damageX));
    const int copyHeight = std::min(damageHeight, std::max(0, scanoutHeight_ - damageY));
    if (copyWidth <= 0 || copyHeight <= 0) {
        return;
    }
    for (int row = 0; row < copyHeight; ++row) {
        const std::uint8_t* from = source + (static_cast<std::size_t>(damageY + row) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(damageX)) * 4u;
        std::uint8_t* to = scanout_ + static_cast<std::size_t>(damageY + row) * static_cast<std::size_t>(scanoutStride_) + static_cast<std::size_t>(damageX) * 4u;
        for (int column = 0; column < copyWidth; ++column) {
            const std::uint32_t packed = Pack(from[0], from[1], from[2]);
            const std::uint8_t pixel[4] = {
                static_cast<std::uint8_t>(packed),
                static_cast<std::uint8_t>(packed >> 8),
                static_cast<std::uint8_t>(packed >> 16),
                0,
            };
            if (keepPixel_ == nullptr || !keepPixel_(keepCookie_, damageX + column, damageY + row, pixel)) {
                to[0] = pixel[0];
                to[1] = pixel[1];
                to[2] = pixel[2];
                to[3] = pixel[3];
            }
            from += 4;
            to += 4;
        }
    }
}

void RoboticelaOSPlatform::CopyText(const std::string& text) {
    copiedText_ = text;
}

std::vector<std::uint8_t> RoboticelaOSPlatform::LoadAsset(const std::string& relativePath) const {
#ifdef ROBO_KERNEL
    (void)relativePath;
    return {};
#else
    std::string path = assetRoot_;
    if (!path.empty() && path.back() != '/' && path.back() != '\\') {
        path.push_back('/');
    }
    path += relativePath;
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }
    return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
#endif
}

int RoboticelaOSPlatform::GetPresentX() const {
    return presentX_;
}

int RoboticelaOSPlatform::GetPresentY() const {
    return presentY_;
}

int RoboticelaOSPlatform::GetPresentWidth() const {
    return presentWidth_;
}

int RoboticelaOSPlatform::GetPresentHeight() const {
    return presentHeight_;
}

}  // namespace Robo
