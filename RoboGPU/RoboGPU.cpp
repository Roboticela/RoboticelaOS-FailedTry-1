#include "RoboGPU.hpp"

#include "RoboBootInfo.hpp"
#include "RoboGPUVendor.hpp"

uint32_t RoboGPUPack(const RoboGPU* gpu, uint32_t rgb) {
    uint32_t red = (rgb >> 16) & 0xFFu;
    uint32_t green = (rgb >> 8) & 0xFFu;
    uint32_t blue = rgb & 0xFFu;
    if (gpu != nullptr && gpu->pixelFormat == ROBO_PIXEL_RGBX) {
        return (red << 16) | (green << 8) | blue;
    }
    return (blue << 16) | (green << 8) | red;
}

uint32_t RoboGPUUnpack(const RoboGPU* gpu, uint32_t packed) {
    if (gpu != nullptr && gpu->pixelFormat == ROBO_PIXEL_RGBX) {
        uint32_t red = (packed >> 16) & 0xFFu;
        uint32_t green = (packed >> 8) & 0xFFu;
        uint32_t blue = packed & 0xFFu;
        return (red << 16) | (green << 8) | blue;
    }
    uint32_t blue = (packed >> 16) & 0xFFu;
    uint32_t green = (packed >> 8) & 0xFFu;
    uint32_t red = packed & 0xFFu;
    return (red << 16) | (green << 8) | blue;
}

bool RoboRectEmpty(RoboRect r) {
    return r.width <= 0 || r.height <= 0;
}

RoboRect RoboRectClipBounds(RoboRect r, RoboRect bounds) {
    if (RoboRectEmpty(r) || RoboRectEmpty(bounds)) {
        return {0, 0, 0, 0};
    }
    int64_t x0 = r.x > bounds.x ? r.x : bounds.x;
    int64_t y0 = r.y > bounds.y ? r.y : bounds.y;
    int64_t rx1 = static_cast<int64_t>(r.x) + r.width;
    int64_t ry1 = static_cast<int64_t>(r.y) + r.height;
    int64_t bx1 = static_cast<int64_t>(bounds.x) + bounds.width;
    int64_t by1 = static_cast<int64_t>(bounds.y) + bounds.height;
    int64_t x1 = rx1 < bx1 ? rx1 : bx1;
    int64_t y1 = ry1 < by1 ? ry1 : by1;
    if (x1 <= x0 || y1 <= y0) {
        return {0, 0, 0, 0};
    }
    return {static_cast<int32_t>(x0), static_cast<int32_t>(y0), static_cast<int32_t>(x1 - x0), static_cast<int32_t>(y1 - y0)};
}

RoboRect RoboRectClip(RoboRect rect, int32_t width, int32_t height) {
    RoboRect bounds{0, 0, width, height};
    return RoboRectClipBounds(rect, bounds);
}

bool RoboRectIntersects(RoboRect a, RoboRect b) {
    if (RoboRectEmpty(a) || RoboRectEmpty(b)) {
        return false;
    }
    return a.x < b.x + b.width && b.x < a.x + a.width && a.y < b.y + b.height && b.y < a.y + a.height;
}

RoboRect RoboRectUnion(RoboRect a, RoboRect b) {
    if (RoboRectEmpty(a)) return b;
    if (RoboRectEmpty(b)) return a;
    int64_t ax1 = static_cast<int64_t>(a.x) + a.width;
    int64_t ay1 = static_cast<int64_t>(a.y) + a.height;
    int64_t bx1 = static_cast<int64_t>(b.x) + b.width;
    int64_t by1 = static_cast<int64_t>(b.y) + b.height;
    int64_t x0 = a.x < b.x ? a.x : b.x;
    int64_t y0 = a.y < b.y ? a.y : b.y;
    int64_t x1 = ax1 > bx1 ? ax1 : bx1;
    int64_t y1 = ay1 > by1 ? ay1 : by1;
    return {static_cast<int32_t>(x0), static_cast<int32_t>(y0), static_cast<int32_t>(x1 - x0), static_cast<int32_t>(y1 - y0)};
}

static void RoboGPUStoreSpan(uint8_t* row, int32_t x, int32_t width, uint32_t packed) {
    uint8_t* pixel = row + static_cast<int64_t>(x) * 4;
    int32_t remaining = width;
    if ((reinterpret_cast<uintptr_t>(pixel) & 7u) != 0 && remaining > 0) {
        *reinterpret_cast<uint32_t*>(pixel) = packed;
        pixel += 4;
        remaining -= 1;
    }
    uint64_t pair = static_cast<uint64_t>(packed) | (static_cast<uint64_t>(packed) << 32);
    while (remaining >= 2) {
        *reinterpret_cast<uint64_t*>(pixel) = pair;
        pixel += 8;
        remaining -= 2;
    }
    if (remaining > 0) {
        *reinterpret_cast<uint32_t*>(pixel) = packed;
    }
}

static inline void RoboGPUMemoryBarrier(void) {
#if defined(__aarch64__)
    __asm__ volatile("dsb sy" ::: "memory");
#else
    __asm__ volatile("sfence" ::: "memory");
#endif
}

void RoboGPUBind(RoboGPU* gpu, const RoboBootInfo* info) {
    if (gpu == nullptr || info == nullptr) {
        return;
    }
    gpu->capabilities = ROBO_GPU_CAP_SOFTWARE;
    gpu->frameState = ROBO_FRAME_STATE_FREE;
    gpu->backbuffer = nullptr;
    gpu->backbufferStride = 0;

    if (RoboGPUVendorBind(gpu, info)) {
        gpu->capabilities = RoboGPUVendorCaps();
        return;
    }
    RoboGPUVendorFallback();
    gpu->base = reinterpret_cast<uint8_t*>(info->frameBufferBase);
    gpu->width = static_cast<int32_t>(info->width);
    gpu->height = static_cast<int32_t>(info->height);
    gpu->stride = static_cast<int32_t>(info->stride);
    gpu->pixelFormat = info->pixelFormat;
    gpu->capabilities = RoboGPUVendorCaps();
}

void RoboGPUFill(RoboGPU* gpu, uint32_t rgb) {
    if (gpu == nullptr || gpu->base == nullptr || gpu->width <= 0 || gpu->height <= 0) {
        return;
    }
    if ((gpu->capabilities & ROBO_GPU_CAP_ACCEL_FILL) != 0 &&
        RoboGPUVendorFillRect(0, 0, gpu->width, gpu->height, rgb)) {
        return;
    }
    uint32_t packed = RoboGPUPack(gpu, rgb);
    for (int32_t y = 0; y < gpu->height; ++y) {
        uint8_t* row = gpu->base + static_cast<int64_t>(y) * gpu->stride;
        RoboGPUStoreSpan(row, 0, gpu->width, packed);
    }
    RoboGPUMemoryBarrier();
}

void RoboGPUFillRect(RoboGPU* gpu, RoboRect rect, uint32_t rgb) {
    if (gpu == nullptr || gpu->base == nullptr) {
        return;
    }
    rect = RoboRectClip(rect, gpu->width, gpu->height);
    if (rect.width <= 0 || rect.height <= 0) {
        return;
    }
    if ((gpu->capabilities & ROBO_GPU_CAP_ACCEL_FILL) != 0 &&
        RoboGPUVendorFillRect(rect.x, rect.y, rect.width, rect.height, rgb)) {
        return;
    }
    uint32_t packed = RoboGPUPack(gpu, rgb);
    for (int32_t y = 0; y < rect.height; ++y) {
        uint8_t* row = gpu->base + static_cast<int64_t>(rect.y + y) * gpu->stride;
        RoboGPUStoreSpan(row, rect.x, rect.width, packed);
    }
    RoboGPUMemoryBarrier();
}

void RoboGPUCopyRect(RoboGPU* gpu, int32_t screenX, int32_t screenY, const uint32_t* pixels, int32_t pixelWidth, int32_t pixelHeight, int32_t srcX, int32_t srcY, int32_t width, int32_t height) {
    if (gpu == nullptr || pixels == nullptr || pixelWidth <= 0 || pixelHeight <= 0 || width <= 0 || height <= 0) {
        return;
    }
    if (srcX < 0) {
        width += srcX;
        screenX -= srcX;
        srcX = 0;
    }
    if (srcY < 0) {
        height += srcY;
        screenY -= srcY;
        srcY = 0;
    }
    if (screenX < 0) {
        width += screenX;
        srcX -= screenX;
        screenX = 0;
    }
    if (screenY < 0) {
        height += screenY;
        srcY -= screenY;
        screenY = 0;
    }
    if (width <= 0 || height <= 0 || srcX >= pixelWidth || srcY >= pixelHeight) {
        return;
    }
    if (srcX + width > pixelWidth) {
        width = pixelWidth - srcX;
    }
    if (srcY + height > pixelHeight) {
        height = pixelHeight - srcY;
    }
    if (screenX >= gpu->width || screenY >= gpu->height || width <= 0 || height <= 0) {
        return;
    }
    if (screenX + width > gpu->width) {
        width = gpu->width - screenX;
    }
    if (screenY + height > gpu->height) {
        height = gpu->height - screenY;
    }

    if ((gpu->capabilities & ROBO_GPU_CAP_ACCEL_BLIT) != 0 &&
        RoboGPUVendorCopyRect(screenX, screenY, pixels, pixelWidth, srcX, srcY, width, height)) {
        return;
    }

    for (int32_t y = 0; y < height; ++y) {
        const uint32_t* source = pixels + static_cast<int64_t>(srcY + y) * pixelWidth + srcX;
        uint8_t* row = gpu->base + static_cast<int64_t>(screenY + y) * gpu->stride;
        uint8_t* pixel = row + static_cast<int64_t>(screenX) * 4;
        for (int32_t x = 0; x < width; ++x) {
            *reinterpret_cast<uint32_t*>(pixel) = RoboGPUPack(gpu, source[x]);
            pixel += 4;
        }
    }
    RoboGPUMemoryBarrier();
}

bool RoboGPUPresentDamage(RoboGPU* gpu, const uint8_t* pixels, int32_t pixelStride, RoboRect rect) {
    if (gpu == nullptr || pixels == nullptr) {
        return false;
    }
    rect = RoboRectClip(rect, gpu->width, gpu->height);
    if (rect.width <= 0 || rect.height <= 0) {
        return true;
    }

    gpu->frameState = ROBO_FRAME_STATE_RENDERING;

    const uint8_t* rectPixels = pixels + static_cast<int64_t>(rect.y) * pixelStride + static_cast<int64_t>(rect.x) * 4;
    if (RoboGPUVendorPresent(rectPixels, pixelStride, rect.x, rect.y, rect.width, rect.height, gpu->width, gpu->height, gpu->pixelFormat)) {
        gpu->frameState = ROBO_FRAME_STATE_PRESENTED;
        return true;
    }

    if (gpu->base == nullptr) {
        gpu->frameState = ROBO_FRAME_STATE_FREE;
        return false;
    }

    for (int32_t y = 0; y < rect.height; ++y) {
        const uint8_t* from = pixels + static_cast<int64_t>(rect.y + y) * pixelStride + static_cast<int64_t>(rect.x) * 4;
        uint8_t* to = gpu->base + static_cast<int64_t>(rect.y + y) * gpu->stride + static_cast<int64_t>(rect.x) * 4;
        for (int32_t x = 0; x < rect.width; ++x) {
            uint32_t color = *reinterpret_cast<const uint32_t*>(from + static_cast<int64_t>(x) * 4);
            *reinterpret_cast<uint32_t*>(to + static_cast<int64_t>(x) * 4) = RoboGPUPack(gpu, color);
        }
    }
    RoboGPUMemoryBarrier();
    gpu->frameState = ROBO_FRAME_STATE_PRESENTED;
    return true;
}

void RoboGPUFlush(RoboGPU* gpu) {
    (void)gpu;
    RoboGPUVendorFlush();
}

void RoboGPUFlushCursor(RoboGPU* gpu, int32_t x, int32_t y) {
    (void)gpu;
    RoboGPUVendorFlushCursor(x, y);
}
