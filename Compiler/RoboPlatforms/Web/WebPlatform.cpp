#include "WebPlatform.hpp"

#include <algorithm>
#include <emscripten.h>
#include <fstream>
#include <iterator>
#include <vector>

namespace Robo {

namespace {

EM_JS(void, JsPresentRgba, (int width, int height, int damageX, int damageY, int damageW, int damageH, const uint8_t* pixels), {
    const canvas = document.getElementById("robo-canvas");
    if (!canvas || damageW <= 0 || damageH <= 0) {
        return;
    }
    const resized = canvas.width !== width || canvas.height !== height;
    if (resized) {
        canvas.width = width;
        canvas.height = height;
        damageX = 0;
        damageY = 0;
        damageW = width;
        damageH = height;
    }
    canvas.style.position = "fixed";
    canvas.style.left = "0px";
    canvas.style.top = "0px";
    canvas.style.width = "100%";
    canvas.style.height = "100%";
    const context = canvas.getContext("2d", { alpha: false, desynchronized: true });
    context.setTransform(1, 0, 0, 1, 0, 0);
    context.imageSmoothingEnabled = false;
    const source = HEAPU8.subarray(pixels, pixels + width * height * 4);
    if (damageX === 0 && damageY === 0 && damageW === width && damageH === height) {
        const imageData = context.createImageData(width, height);
        imageData.data.set(source);
        context.putImageData(imageData, 0, 0);
        return;
    }
    const imageData = context.createImageData(damageW, damageH);
    const destination = imageData.data;
    const rowBytes = damageW * 4;
    for (let row = 0; row < damageH; row++) {
        const offset = ((damageY + row) * width + damageX) * 4;
        destination.set(source.subarray(offset, offset + rowBytes), row * rowBytes);
    }
    context.putImageData(imageData, damageX, damageY);
});

EM_JS(void, JsCopyText, (const char* text), {
    const value = UTF8ToString(text);
    const area = document.createElement("textarea");
    area.value = value;
    area.setAttribute("readonly", "");
    area.style.position = "fixed";
    area.style.opacity = "0";
    document.body.appendChild(area);
    area.focus();
    area.select();
    document.execCommand("copy");
    area.remove();
});

}  // namespace

int WebPlatform::GetSurfaceWidth() const {
    return width_;
}

int WebPlatform::GetSurfaceHeight() const {
    return height_;
}

float WebPlatform::GetPixelScale() const {
    return pixelScale_;
}

void WebPlatform::SetSurfaceSize(int width, int height) {
    width_ = width > 0 ? width : 1;
    height_ = height > 0 ? height : 1;
}

void WebPlatform::SetPixelScale(float scale) {
    pixelScale_ = std::max(0.5f, scale);
}

void WebPlatform::Present(const FrameBuffer& buffer) {
    int damageX = buffer.GetPresentX();
    int damageY = buffer.GetPresentY();
    int damageWidth = buffer.GetPresentWidth();
    int damageHeight = buffer.GetPresentHeight();
    if (damageWidth <= 0 || damageHeight <= 0) {
        damageX = 0;
        damageY = 0;
        damageWidth = buffer.GetWidth();
        damageHeight = buffer.GetHeight();
    }
    JsPresentRgba(buffer.GetWidth(), buffer.GetHeight(), damageX, damageY, damageWidth, damageHeight, buffer.GetPixels());
}

void WebPlatform::CopyText(const std::string& text) {
    JsCopyText(text.c_str());
}

namespace {

std::vector<std::uint8_t> ReadPackedFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }
    return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

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

}  // namespace

std::vector<std::uint8_t> WebPlatform::LoadAsset(const std::string& relativePath) const {
    const std::string path = std::string("/") + relativePath;
    if (EndsWithMp4(path)) {
        const std::vector<std::uint8_t> reel = ReadPackedFile(path.substr(0, path.size() - 4) + ".rjp");
        if (!reel.empty()) {
            return reel;
        }
    }
    return ReadPackedFile(path);
}

}  // namespace Robo
