#include "RoboticelaOSPlatform.hpp"

#include <algorithm>
#include <cmath>
#if defined(ROBO_USER)
#include "RoboCall.hpp"
#endif
#ifdef ROBO_KERNEL
#include "RoboDesktop.hpp"
#include "RoboFS.hpp"
#include "RoboGPUVendor.hpp"
#elif !defined(ROBO_USER)
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
    if (source == nullptr || damageWidth <= 0 || damageHeight <= 0 || width_ <= 0) {
        return;
    }
#if defined(ROBO_USER)
    RoboCallPresent request{};
    request.pixels = reinterpret_cast<std::uint64_t>(source);
    request.stride = width_ * 4;
    request.x = damageX;
    request.y = damageY;
    request.width = damageWidth;
    request.height = damageHeight;
    request.surfaceWidth = width_;
    request.surfaceHeight = height_;
    request.pixelFormat = pixelFormat_;
    (void)RoboUserCall(ROBO_CALL_PRESENT, reinterpret_cast<std::uint64_t>(&request), 0, 0, 0);
    return;
#endif
    if (scanout_ == nullptr) {
        return;
    }
    const bool scaled = scanoutWidth_ > 0 && scanoutHeight_ > 0 && (width_ > scanoutWidth_ || height_ > scanoutHeight_);
    if (scaled) {
        const float scaleX = static_cast<float>(width_) / static_cast<float>(scanoutWidth_);
        const float scaleY = static_cast<float>(height_) / static_cast<float>(scanoutHeight_);
        int screenX0 = static_cast<int>(std::floor(static_cast<float>(damageX) / scaleX));
        int screenY0 = static_cast<int>(std::floor(static_cast<float>(damageY) / scaleY));
        int screenX1 = static_cast<int>(std::ceil(static_cast<float>(damageX + damageWidth) / scaleX));
        int screenY1 = static_cast<int>(std::ceil(static_cast<float>(damageY + damageHeight) / scaleY));
        if (screenX0 < 0) {
            screenX0 = 0;
        }
        if (screenY0 < 0) {
            screenY0 = 0;
        }
        if (screenX1 > scanoutWidth_) {
            screenX1 = scanoutWidth_;
        }
        if (screenY1 > scanoutHeight_) {
            screenY1 = scanoutHeight_;
        }
        const int screenWidth = screenX1 - screenX0;
        const int screenHeight = screenY1 - screenY0;
        presentX_ = screenX0;
        presentY_ = screenY0;
        presentWidth_ = screenWidth;
        presentHeight_ = screenHeight;
        if (screenWidth <= 0 || screenHeight <= 0) {
            return;
        }
#ifdef ROBO_KERNEL
        int32_t cursorX = 0;
        int32_t cursorY = 0;
        int32_t cursorWidth = 0;
        int32_t cursorHeight = 0;
        const bool cursor = RoboDesktopCursorRect(&cursorX, &cursorY, &cursorWidth, &cursorHeight);
#endif
        // Fixed-point nearest-neighbor — avoid per-pixel float and keep cursor Pump cheap.
        const int stepX = std::max(1, static_cast<int>(scaleX * 256.0f));
        const int stepY = std::max(1, static_cast<int>(scaleY * 256.0f));
        int sourceYFixed = static_cast<int>((static_cast<float>(screenY0) + 0.5f) * scaleY * 256.0f);
        for (int row = 0; row < screenHeight; ++row) {
            const int screenY = screenY0 + row;
            const int sourceY = std::min(height_ - 1, std::max(0, sourceYFixed >> 8));
            sourceYFixed += stepY;
            std::uint8_t* to = scanout_ + static_cast<std::size_t>(screenY) * static_cast<std::size_t>(scanoutStride_) + static_cast<std::size_t>(screenX0) * 4u;
            auto* packed = reinterpret_cast<std::uint32_t*>(to);
#ifdef ROBO_KERNEL
            const bool cursorRow = cursor && screenY >= cursorY && screenY < cursorY + cursorHeight;
#endif
            int sourceXFixed = static_cast<int>((static_cast<float>(screenX0) + 0.5f) * scaleX * 256.0f);
            for (int column = 0; column < screenWidth; ++column) {
                const int screenX = screenX0 + column;
                const int sourceX = std::min(width_ - 1, std::max(0, sourceXFixed >> 8));
                sourceXFixed += stepX;
                const std::uint8_t* from = source + (static_cast<std::size_t>(sourceY) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(sourceX)) * 4u;
                const uint32_t scene = Pack(from[0], from[1], from[2]);
#ifdef ROBO_KERNEL
                uint32_t cursorRgb = 0;
                if (cursorRow && RoboDesktopCursorSample(screenX - cursorX, screenY - cursorY, &cursorRgb)) {
                    RoboDesktopRememberCursor(screenX - cursorX, screenY - cursorY, scene);
                    packed[column] = Pack(static_cast<std::uint8_t>(cursorRgb >> 16), static_cast<std::uint8_t>(cursorRgb >> 8),
                                          static_cast<std::uint8_t>(cursorRgb));
                } else {
                    packed[column] = scene;
                }
#else
                packed[column] = scene;
#endif
            }
        }
#ifdef ROBO_KERNEL
        const std::uint8_t* vendor = scanout_ + static_cast<std::size_t>(screenY0) * static_cast<std::size_t>(scanoutStride_) + static_cast<std::size_t>(screenX0) * 4u;
        if (RoboGPUVendorPresent(vendor, scanoutStride_, screenX0, screenY0, screenWidth, screenHeight, scanoutWidth_, scanoutHeight_, pixelFormat_)) {
            return;
        }
        __asm__ volatile("sfence" ::: "memory");
#endif
        return;
    }
    const int copyWidth = std::min(damageWidth, std::max(0, scanoutWidth_ - damageX));
    const int copyHeight = std::min(damageHeight, std::max(0, scanoutHeight_ - damageY));
    if (copyWidth <= 0 || copyHeight <= 0) {
        return;
    }
#ifdef ROBO_KERNEL
    const int pixelStride = width_ * 4;
    if (RoboGPUVendorPresent(source + (static_cast<std::size_t>(damageY) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(damageX)) * 4u,
                             pixelStride, damageX, damageY, copyWidth, copyHeight, width_, height_, pixelFormat_)) {
        return;
    }
    int32_t cursorX = 0;
    int32_t cursorY = 0;
    int32_t cursorWidth = 0;
    int32_t cursorHeight = 0;
    const bool cursor = RoboDesktopCursorRect(&cursorX, &cursorY, &cursorWidth, &cursorHeight);
#endif
    for (int row = 0; row < copyHeight; ++row) {
        const int screenY = damageY + row;
        const std::uint8_t* from = source + (static_cast<std::size_t>(screenY) * static_cast<std::size_t>(width_) + static_cast<std::size_t>(damageX)) * 4u;
        std::uint8_t* to = scanout_ + static_cast<std::size_t>(screenY) * static_cast<std::size_t>(scanoutStride_) + static_cast<std::size_t>(damageX) * 4u;
        auto* packed = reinterpret_cast<std::uint32_t*>(to);
#ifdef ROBO_KERNEL
        const bool cursorRow = cursor && screenY >= cursorY && screenY < cursorY + cursorHeight;
#endif
        for (int column = 0; column < copyWidth; ++column) {
            const int screenX = damageX + column;
            uint32_t scene = Pack(from[0], from[1], from[2]);
#ifdef ROBO_KERNEL
            uint32_t cursorRgb = 0;
            if (cursorRow && RoboDesktopCursorSample(screenX - cursorX, screenY - cursorY, &cursorRgb)) {
                RoboDesktopRememberCursor(screenX - cursorX, screenY - cursorY, scene);
                packed[column] = Pack(static_cast<std::uint8_t>(cursorRgb >> 16), static_cast<std::uint8_t>(cursorRgb >> 8), static_cast<std::uint8_t>(cursorRgb));
            } else {
                packed[column] = scene;
            }
#else
            (void)screenX;
            packed[column] = scene;
#endif
            from += 4;
        }
    }
#ifdef ROBO_KERNEL
    __asm__ volatile("sfence" ::: "memory");
#endif
}

void RoboticelaOSPlatform::CopyText(const std::string& text) {
    copiedText_ = text;
}

namespace {

bool EndsWithMp4(const std::string& name) {
    if (name.size() < 4) {
        return false;
    }
    const char a = name[name.size() - 3];
    const char b = name[name.size() - 2];
    const char c = name[name.size() - 1];
    const char dot = name[name.size() - 4];
    const char la = a >= 'A' && a <= 'Z' ? static_cast<char>(a - 'A' + 'a') : a;
    const char lb = b >= 'A' && b <= 'Z' ? static_cast<char>(b - 'A' + 'a') : b;
    const char lc = c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
    return dot == '.' && la == 'm' && lb == 'p' && lc == '4';
}

std::string ReelName(const std::string& name) {
    return name.substr(0, name.size() - 4) + ".rjp";
}

bool SameAssetName(const std::string& left, const char* right) {
    std::size_t index = 0;
    for (; index < left.size() && right[index] != 0; ++index) {
        char a = left[index];
        char b = right[index];
        if (a >= 'A' && a <= 'Z') {
            a = static_cast<char>(a - 'A' + 'a');
        }
        if (b >= 'A' && b <= 'Z') {
            b = static_cast<char>(b - 'A' + 'a');
        }
        if (a != b) {
            return false;
        }
    }
    return index == left.size() && right[index] == 0;
}

const char* BootAssetAlias(const std::string& name) {
    if (SameAssetName(name, "RoboticelaLogo.svg") || SameAssetName(name, "ROBOLOGO.SVG")) {
        return "ROBOLOGO.SVG";
    }
    if (SameAssetName(name, "EmptyFolder.svg") || SameAssetName(name, "EMPTYFOL.SVG")) {
        return "EMPTYFOL.SVG";
    }
    if (SameAssetName(name, "Background.rjp") || SameAssetName(name, "BACKGROU.RJP")) {
        return "BACKGROU.RJP";
    }
    if (SameAssetName(name, "Background.mp4") || SameAssetName(name, "BACKGROU.MP4")) {
        return "BACKGROU.MP4";
    }
    if (SameAssetName(name, "ROBOICON.PAK") || SameAssetName(name, "RoboIcons.pak")) {
        return "ROBOICON.PAK";
    }
    if (SameAssetName(name, "ROBOCTRY.PAK") || SameAssetName(name, "RoboCountry.pak")) {
        return "ROBOCTRY.PAK";
    }
    return nullptr;
}

}  // namespace

std::vector<std::uint8_t> RoboticelaOSPlatform::LoadAsset(const std::string& relativePath) const {
#ifdef ROBO_KERNEL
    std::string name = relativePath;
    std::size_t slash = std::string::npos;
    for (std::size_t index = 0; index < name.size(); ++index) {
        if (name[index] == '/' || name[index] == '\\') {
            slash = index;
        }
    }
    if (slash != std::string::npos) {
        name = name.substr(slash + 1);
    }
    auto readFile = [](const std::string& fileName) {
        std::uint8_t probe = 0;
        std::uint32_t size = 0;
        if (!RoboFsReadFile(fileName.c_str(), &probe, 1, &size) || size == 0 || size > 24u * 1024u * 1024u) {
            return std::vector<std::uint8_t>();
        }
        std::vector<std::uint8_t> bytes(size);
        std::uint32_t got = 0;
        if (!RoboFsReadFile(fileName.c_str(), bytes.data(), size, &got) || got == 0) {
            return std::vector<std::uint8_t>();
        }
        if (got < size) {
            bytes.resize(got);
        }
        return bytes;
    };
    auto readNamed = [&](const std::string& fileName) {
        std::vector<std::uint8_t> bytes = readFile(fileName);
        if (!bytes.empty()) {
            return bytes;
        }
        const char* alias = BootAssetAlias(fileName);
        if (alias != nullptr && !SameAssetName(fileName, alias)) {
            return readFile(alias);
        }
        return bytes;
    };
    if (EndsWithMp4(name)) {
        const std::vector<std::uint8_t> reel = readNamed(ReelName(name));
        if (!reel.empty()) {
            return reel;
        }
        const std::vector<std::uint8_t> shortReel = readNamed("BACKGROU.RJP");
        if (!shortReel.empty()) {
            return shortReel;
        }
        const std::vector<std::uint8_t> volumeReel = readFile("Desktop/Background.rjp");
        if (!volumeReel.empty()) {
            return volumeReel;
        }
    }
    return readNamed(name);
#elif defined(ROBO_USER)
    auto readFile = [](const std::string& fileName) {
        std::vector<std::uint8_t> bytes(ROBO_CALL_FILE_MAX);
        std::uint32_t copied = 0;
        const std::uint64_t status = RoboUserCall(ROBO_CALL_READ, reinterpret_cast<std::uint64_t>(fileName.c_str()),
            reinterpret_cast<std::uint64_t>(bytes.data()), ROBO_CALL_FILE_MAX, reinterpret_cast<std::uint64_t>(&copied));
        if (status != 0 || copied == 0) {
            return std::vector<std::uint8_t>();
        }
        bytes.resize(copied);
        return bytes;
    };
    if (EndsWithMp4(relativePath)) {
        const std::vector<std::uint8_t> reel = readFile(ReelName(relativePath));
        if (!reel.empty()) {
            return reel;
        }
    }
    return readFile(relativePath);
#else
    std::string path = assetRoot_;
    if (!path.empty() && path.back() != '/' && path.back() != '\\') {
        path.push_back('/');
    }
    std::string relative = relativePath;
    if (EndsWithMp4(relative)) {
        std::string reelPath = path + ReelName(relative);
        std::ifstream reel(reelPath, std::ios::binary);
        if (reel) {
            return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(reel)), std::istreambuf_iterator<char>());
        }
    }
    path += relative;
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
