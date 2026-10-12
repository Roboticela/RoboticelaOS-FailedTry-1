#include "WindowsPlatform.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>

namespace Robo {

namespace {

std::wstring Widen(const std::string& value) {
    if (value.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring wide(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), wide.data(), size);
    return wide;
}

}  // namespace

WindowsPlatform::WindowsPlatform(HWND previewHwnd) : previewHwnd_(previewHwnd) {}

void WindowsPlatform::SetPreviewHwnd(HWND previewHwnd) {
    previewHwnd_ = previewHwnd;
}

void WindowsPlatform::SetAssetRoot(const std::wstring& assetRoot) {
    assetRoot_ = assetRoot;
}

int WindowsPlatform::GetSurfaceWidth() const {
    return width_;
}

int WindowsPlatform::GetSurfaceHeight() const {
    return height_;
}

float WindowsPlatform::GetPixelScale() const {
    return pixelScale_;
}

void WindowsPlatform::SetSurfaceSize(int width, int height) {
    width_ = width > 0 ? width : 1;
    height_ = height > 0 ? height : 1;
}

void WindowsPlatform::SetPixelScale(float scale) {
    pixelScale_ = std::max(0.5f, scale);
}

void WindowsPlatform::Present(const FrameBuffer& buffer) {
    width_ = buffer.GetWidth();
    height_ = buffer.GetHeight();
    const std::size_t byteCount = static_cast<std::size_t>(width_ * height_ * 4);
    pixels_.resize(byteCount);
    if (byteCount > 0) {
        memcpy(pixels_.data(), buffer.GetPixels(), byteCount);
    }
    if (previewHwnd_) {
        InvalidateRect(previewHwnd_, nullptr, FALSE);
        UpdateWindow(previewHwnd_);
    }
}

void WindowsPlatform::CopyText(const std::string& text) {
    if (!OpenClipboard(nullptr)) {
        return;
    }
    EmptyClipboard();
    const std::wstring wide = Widen(text);
    const std::size_t bytes = (wide.size() + 1) * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (memory) {
        void* locked = GlobalLock(memory);
        if (locked) {
            std::memcpy(locked, wide.c_str(), bytes);
            GlobalUnlock(memory);
            if (!SetClipboardData(CF_UNICODETEXT, memory)) {
                GlobalFree(memory);
            }
        } else {
            GlobalFree(memory);
        }
    }
    CloseClipboard();
}

std::vector<std::uint8_t> WindowsPlatform::LoadAsset(const std::string& relativePath) const {
    std::wstring path = assetRoot_;
    if (!path.empty() && path.back() != L'\\' && path.back() != L'/') {
        path.push_back(L'\\');
    }
    path += Widen(relativePath);
    for (wchar_t& ch : path) {
        if (ch == L'/') {
            ch = L'\\';
        }
    }

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }
    return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

void WindowsPlatform::Paint(HDC deviceContext) const {
    RECT client{};
    GetClientRect(previewHwnd_, &client);
    const int clientWidth = std::max(1, static_cast<int>(client.right - client.left));
    const int clientHeight = std::max(1, static_cast<int>(client.bottom - client.top));

    if (pixels_.empty() || width_ <= 0 || height_ <= 0) {
        HBRUSH brush = CreateSolidBrush(RGB(247, 243, 235));
        FillRect(deviceContext, &client, brush);
        DeleteObject(brush);
        return;
    }

    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width_;
    info.bmiHeader.biHeight = -height_;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    std::vector<std::uint8_t> bgrx(static_cast<std::size_t>(width_ * height_ * 4));
    for (int i = 0; i < width_ * height_; ++i) {
        const std::size_t src = static_cast<std::size_t>(i) * 4;
        const std::size_t dst = static_cast<std::size_t>(i) * 4;
        bgrx[dst + 0] = pixels_[src + 2];
        bgrx[dst + 1] = pixels_[src + 1];
        bgrx[dst + 2] = pixels_[src + 0];
        bgrx[dst + 3] = 255;
    }

    if (width_ == clientWidth && height_ == clientHeight) {
        SetDIBitsToDevice(deviceContext,
                          0,
                          0,
                          clientWidth,
                          clientHeight,
                          0,
                          0,
                          0,
                          height_,
                          bgrx.data(),
                          &info,
                          DIB_RGB_COLORS);
        return;
    }

    SetStretchBltMode(deviceContext, HALFTONE);
    SetBrushOrgEx(deviceContext, 0, 0, nullptr);
    StretchDIBits(deviceContext,
                  0,
                  0,
                  clientWidth,
                  clientHeight,
                  0,
                  0,
                  width_,
                  height_,
                  bgrx.data(),
                  &info,
                  DIB_RGB_COLORS,
                  SRCCOPY);
}

}  // namespace Robo
