#pragma once

#include "Robo/IPlatform.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <string>
#include <vector>

namespace Robo {

class WindowsPlatform : public IPlatform {
public:
    explicit WindowsPlatform(HWND previewHwnd);

    void SetPreviewHwnd(HWND previewHwnd);
    void SetAssetRoot(const std::wstring& assetRoot);

    int GetSurfaceWidth() const override;
    int GetSurfaceHeight() const override;
    float GetPixelScale() const override;
    void SetSurfaceSize(int width, int height) override;
    void SetPixelScale(float scale) override;
    void Present(const FrameBuffer& buffer) override;
    void CopyText(const std::string& text) override;
    std::vector<std::uint8_t> LoadAsset(const std::string& relativePath) const override;

    void Paint(HDC deviceContext) const;

private:
    HWND previewHwnd_ = nullptr;
    std::wstring assetRoot_;
    int width_ = 800;
    int height_ = 600;
    float pixelScale_ = 1.0f;
    std::vector<std::uint8_t> pixels_;
};

}  // namespace Robo
