#pragma once

#include <stdint.h>

constexpr uint32_t ROBO_GPU_CAP_SOFTWARE = 0u;
constexpr uint32_t ROBO_GPU_CAP_WRITE_COMBINING = 1u << 0;
constexpr uint32_t ROBO_GPU_CAP_ACCEL_FILL = 1u << 1;
constexpr uint32_t ROBO_GPU_CAP_ACCEL_BLIT = 1u << 2;
constexpr uint32_t ROBO_GPU_CAP_ACCEL_PRESENT = 1u << 3;
constexpr uint32_t ROBO_GPU_CAP_VIRTIO = 1u << 4;
constexpr uint32_t ROBO_GPU_CAP_HARDWARE_CURSOR = 1u << 5;

constexpr uint32_t ROBO_FRAME_STATE_FREE = 0u;
constexpr uint32_t ROBO_FRAME_STATE_RENDERING = 1u << 0;
constexpr uint32_t ROBO_FRAME_STATE_READY = 1u << 1;
constexpr uint32_t ROBO_FRAME_STATE_PRESENTED = 1u << 2;

struct RoboRect {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
};

struct RoboPixel {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

// Premultiplied alpha composition:
// C_out = C_s + (C_d * (255 - A_s) + 127) / 255
// A_out = A_s + (A_d * (255 - A_s) + 127) / 255
inline RoboPixel RoboBlendOver(RoboPixel src, RoboPixel dst) {
    if (src.a == 255u) {
        return src;
    }
    if (src.a == 0u) {
        return dst;
    }
    uint32_t invA = 255u - src.a;
    RoboPixel out;
    out.r = static_cast<uint8_t>(src.r + ((static_cast<uint32_t>(dst.r) * invA + 127u) / 255u));
    out.g = static_cast<uint8_t>(src.g + ((static_cast<uint32_t>(dst.g) * invA + 127u) / 255u));
    out.b = static_cast<uint8_t>(src.b + ((static_cast<uint32_t>(dst.b) * invA + 127u) / 255u));
    out.a = static_cast<uint8_t>(src.a + ((static_cast<uint32_t>(dst.a) * invA + 127u) / 255u));
    return out;
}

bool RoboRectEmpty(RoboRect r);
RoboRect RoboRectClipBounds(RoboRect r, RoboRect bounds);
RoboRect RoboRectClip(RoboRect rect, int32_t width, int32_t height);
bool RoboRectIntersects(RoboRect a, RoboRect b);
RoboRect RoboRectUnion(RoboRect a, RoboRect b);

struct RoboSurface {
    uint8_t* pixels;
    int32_t width;
    int32_t height;
    int32_t stride;
    uint32_t pixelFormat;
    uint8_t hasAlpha;
};

struct RoboGPU {
    uint8_t* base;
    int32_t width;
    int32_t height;
    int32_t stride;
    uint32_t pixelFormat;
    uint32_t capabilities;
    volatile uint32_t frameState;
    uint8_t* backbuffer;
    int32_t backbufferStride;
};

struct RoboBootInfo;

void RoboGPUBind(RoboGPU* gpu, const RoboBootInfo* info);
uint32_t RoboGPUPack(const RoboGPU* gpu, uint32_t rgb);
uint32_t RoboGPUUnpack(const RoboGPU* gpu, uint32_t packed);
void RoboGPUFill(RoboGPU* gpu, uint32_t rgb);
void RoboGPUFillRect(RoboGPU* gpu, RoboRect rect, uint32_t rgb);
void RoboGPUCopyRect(RoboGPU* gpu, int32_t screenX, int32_t screenY, const uint32_t* pixels, int32_t pixelWidth, int32_t pixelHeight, int32_t srcX, int32_t srcY, int32_t width, int32_t height);
bool RoboGPUPresentDamage(RoboGPU* gpu, const uint8_t* pixels, int32_t pixelStride, RoboRect rect);
void RoboGPUFlush(RoboGPU* gpu);
void RoboGPUFlushCursor(RoboGPU* gpu, int32_t x, int32_t y);
