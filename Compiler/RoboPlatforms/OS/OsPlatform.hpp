#pragma once

#include "Robo/IPlatform.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace Robo {

// Hosts a Compiler app the same way Web and Windows do. Present copies only
// the frame's dirty rectangle into the target buffer.
class OsPlatform : public IPlatform {
public:
    void SetAssetRoot(const std::string& assetRoot);
    void SetPresentTarget(std::uint8_t* rgba, int width, int height);

    int GetSurfaceWidth() const override;
    int GetSurfaceHeight() const override;
    float GetPixelScale() const override;
    void SetSurfaceSize(int width, int height) override;
    void SetPixelScale(float scale) override;
    void Present(const FrameBuffer& buffer) override;
    void CopyText(const std::string& text) override;
    std::vector<std::uint8_t> LoadAsset(const std::string& relativePath) const override;

    int GetPresentX() const;
    int GetPresentY() const;
    int GetPresentWidth() const;
    int GetPresentHeight() const;

private:
    std::string assetRoot_;
    std::string copiedText_;
    std::uint8_t* target_ = nullptr;
    int targetWidth_ = 0;
    int targetHeight_ = 0;
    int width_ = 800;
    int height_ = 600;
    float pixelScale_ = 1.0f;
    int presentX_ = 0;
    int presentY_ = 0;
    int presentWidth_ = 0;
    int presentHeight_ = 0;
};

}  // namespace Robo
