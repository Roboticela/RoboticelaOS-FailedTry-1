#pragma once

#include "Robo/IPlatform.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace Robo {

// Same values as the firmware pixel order in RoboBootInfo.
constexpr uint32_t ROBOTICELA_PIXEL_BGRX = 0u;
constexpr uint32_t ROBOTICELA_PIXEL_RGBX = 1u;

// The native platform. The surface is the framebuffer in physical pixels.
// Present packs only the damage rectangle into that scanout, 1:1.
class RoboticelaOSPlatform : public IPlatform {
public:
    void SetAssetRoot(const std::string& assetRoot);
    void SetScanout(std::uint8_t* pixels, int width, int height, int stride, std::uint32_t pixelFormat);
    void SetKeepPixel(bool (*keep)(void* cookie, int x, int y, const std::uint8_t* pixel), void* cookie);

    int GetSurfaceWidth() const override;
    int GetSurfaceHeight() const override;
    float GetPixelScale() const override;
    void SetSurfaceSize(int width, int height) override;
    void SetPixelScale(float scale) override;
    void Present(const FrameBuffer& buffer) override;
    void CopyText(const std::string& text) override;
    std::vector<std::uint8_t> LoadAsset(const std::string& relativePath) const override;

    int GetPresentX() const override;
    int GetPresentY() const override;
    int GetPresentWidth() const override;
    int GetPresentHeight() const override;

private:
    std::uint32_t Pack(std::uint8_t red, std::uint8_t green, std::uint8_t blue) const;

    bool (*keepPixel_)(void* cookie, int x, int y, const std::uint8_t* pixel) = nullptr;
    void* keepCookie_ = nullptr;
    std::string assetRoot_;
    std::string copiedText_;
    std::uint8_t* scanout_ = nullptr;
    int scanoutWidth_ = 0;
    int scanoutHeight_ = 0;
    int scanoutStride_ = 0;
    std::uint32_t pixelFormat_ = ROBOTICELA_PIXEL_BGRX;
    int width_ = 1;
    int height_ = 1;
    float pixelScale_ = 1.0f;
    int presentX_ = 0;
    int presentY_ = 0;
    int presentWidth_ = 0;
    int presentHeight_ = 0;
};

}  // namespace Robo
