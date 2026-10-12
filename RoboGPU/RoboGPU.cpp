#include "RoboGPU.hpp"

#include "RoboBootInfo.hpp"

static uint32_t RoboGPUPack(const RoboGPU* gpu, uint32_t rgb) {
    uint32_t red = (rgb >> 16) & 0xFFu;
    uint32_t green = (rgb >> 8) & 0xFFu;
    uint32_t blue = rgb & 0xFFu;
    if (gpu->pixelFormat == ROBO_PIXEL_RGBX) {
        return (red << 16) | (green << 8) | blue;
    }
    return (blue << 16) | (green << 8) | red;
}

static void RoboGPUStoreSpan(uint8_t* row, int32_t x, int32_t width, uint32_t packed) {
    uint8_t* pixel = row + static_cast<int32_t>(x * 4);
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

void RoboGPUBind(RoboGPU* gpu, const RoboBootInfo* info) {
    gpu->base = reinterpret_cast<uint8_t*>(info->frameBufferBase);
    gpu->width = static_cast<int32_t>(info->width);
    gpu->height = static_cast<int32_t>(info->height);
    gpu->stride = static_cast<int32_t>(info->stride);
    gpu->pixelFormat = info->pixelFormat;
}

void RoboGPUFill(RoboGPU* gpu, uint32_t rgb) {
    uint32_t packed = RoboGPUPack(gpu, rgb);
    for (int32_t y = 0; y < gpu->height; ++y) {
        uint8_t* row = gpu->base + static_cast<int64_t>(y) * gpu->stride;
        RoboGPUStoreSpan(row, 0, gpu->width, packed);
    }
}

RoboRect RoboRectClip(RoboRect rect, int32_t width, int32_t height) {
    if (rect.width <= 0 || rect.height <= 0) {
        rect.width = 0;
        rect.height = 0;
        return rect;
    }
    if (rect.x < 0) {
        rect.width += rect.x;
        rect.x = 0;
    }
    if (rect.y < 0) {
        rect.height += rect.y;
        rect.y = 0;
    }
    if (rect.x >= width || rect.y >= height || rect.width <= 0 || rect.height <= 0) {
        rect.width = 0;
        rect.height = 0;
        return rect;
    }
    if (rect.x + rect.width > width) {
        rect.width = width - rect.x;
    }
    if (rect.y + rect.height > height) {
        rect.height = height - rect.y;
    }
    if (rect.width < 0) {
        rect.width = 0;
    }
    if (rect.height < 0) {
        rect.height = 0;
    }
    return rect;
}

void RoboGPUFillRect(RoboGPU* gpu, RoboRect rect, uint32_t rgb) {
    rect = RoboRectClip(rect, gpu->width, gpu->height);
    if (rect.width <= 0 || rect.height <= 0) {
        return;
    }
    uint32_t packed = RoboGPUPack(gpu, rgb);
    for (int32_t y = 0; y < rect.height; ++y) {
        uint8_t* row = gpu->base + static_cast<int64_t>(rect.y + y) * gpu->stride;
        RoboGPUStoreSpan(row, rect.x, rect.width, packed);
    }
}
