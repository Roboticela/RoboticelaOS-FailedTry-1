#pragma once

#include "Robo/IPlatform.hpp"

namespace Robo {

class WebPlatform : public IPlatform {
public:
    int GetSurfaceWidth() const override;
    int GetSurfaceHeight() const override;
    float GetPixelScale() const override;
    void SetSurfaceSize(int width, int height) override;
    void SetPixelScale(float scale) override;
    void Present(const FrameBuffer& buffer) override;
    void CopyText(const std::string& text) override;
    std::vector<std::uint8_t> LoadAsset(const std::string& relativePath) const override;

private:
    int width_ = 800;
    int height_ = 600;
    float pixelScale_ = 1.0f;
};

}  // namespace Robo
