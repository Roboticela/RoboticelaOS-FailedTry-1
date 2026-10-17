#pragma once

#include "FrameBuffer.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace Robo {

class IPlatform {
public:
    virtual ~IPlatform() = default;

    virtual int GetSurfaceWidth() const = 0;
    virtual int GetSurfaceHeight() const = 0;
    virtual float GetPixelScale() const = 0;
    virtual void SetSurfaceSize(int width, int height) = 0;
    virtual void SetPixelScale(float scale) = 0;
    virtual void Present(const FrameBuffer& buffer) = 0;
    virtual int GetPresentX() const { return 0; }
    virtual int GetPresentY() const { return 0; }
    virtual int GetPresentWidth() const { return 0; }
    virtual int GetPresentHeight() const { return 0; }
    virtual void CopyText(const std::string& text) = 0;
    virtual std::vector<std::uint8_t> LoadAsset(const std::string& relativePath) const = 0;
};

}  // namespace Robo
