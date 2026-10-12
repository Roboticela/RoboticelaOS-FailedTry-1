#include "OsPlatform.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>

namespace Robo {

void OsPlatform::SetAssetRoot(const std::string& assetRoot) {
    assetRoot_ = assetRoot;
}

void OsPlatform::SetPresentTarget(std::uint8_t* rgba, int width, int height) {
    target_ = rgba;
    targetWidth_ = width > 0 ? width : 0;
    targetHeight_ = height > 0 ? height : 0;
}

int OsPlatform::GetSurfaceWidth() const {
    return width_;
}

int OsPlatform::GetSurfaceHeight() const {
    return height_;
}

float OsPlatform::GetPixelScale() const {
    return pixelScale_;
}

void OsPlatform::SetSurfaceSize(int width, int height) {
    width_ = width > 0 ? width : 1;
    height_ = height > 0 ? height : 1;
}

void OsPlatform::SetPixelScale(float scale) {
    pixelScale_ = std::max(0.5f, scale);
}

void OsPlatform::Present(const FrameBuffer& buffer) {
    width_ = buffer.GetWidth();
    height_ = buffer.GetHeight();
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
    if (target_ == nullptr || damageWidth <= 0 || damageHeight <= 0 || width_ <= 0) {
        return;
    }
    const std::uint8_t* source = buffer.GetPixels();
    if (source == nullptr) {
        return;
    }
    const int copyWidth = std::min(damageWidth, std::max(0, targetWidth_ - damageX));
    const int copyHeight = std::min(damageHeight, std::max(0, targetHeight_ - damageY));
    if (copyWidth <= 0 || copyHeight <= 0 || damageX >= targetWidth_ || damageY >= targetHeight_) {
        return;
    }
    const int sourceStride = width_ * 4;
    const int targetStride = targetWidth_ * 4;
    for (int row = 0; row < copyHeight; ++row) {
        const std::uint8_t* from = source + (damageY + row) * sourceStride + damageX * 4;
        std::uint8_t* to = target_ + (damageY + row) * targetStride + damageX * 4;
        std::memcpy(to, from, static_cast<std::size_t>(copyWidth) * 4u);
    }
}

void OsPlatform::CopyText(const std::string& text) {
    copiedText_ = text;
}

std::vector<std::uint8_t> OsPlatform::LoadAsset(const std::string& relativePath) const {
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
}

int OsPlatform::GetPresentX() const {
    return presentX_;
}

int OsPlatform::GetPresentY() const {
    return presentY_;
}

int OsPlatform::GetPresentWidth() const {
    return presentWidth_;
}

int OsPlatform::GetPresentHeight() const {
    return presentHeight_;
}

}  // namespace Robo
