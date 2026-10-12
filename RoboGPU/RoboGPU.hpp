#pragma once

#include <stdint.h>

struct RoboRect {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
};

struct RoboGPU {
    uint8_t* base;
    int32_t width;
    int32_t height;
    int32_t stride;
    uint32_t pixelFormat;
};

struct RoboBootInfo;

void RoboGPUBind(RoboGPU* gpu, const RoboBootInfo* info);
void RoboGPUFill(RoboGPU* gpu, uint32_t rgb);
void RoboGPUFillRect(RoboGPU* gpu, RoboRect rect, uint32_t rgb);
void RoboGPUCopyRect(RoboGPU* gpu, int32_t screenX, int32_t screenY, const uint32_t* pixels, int32_t pixelWidth, int32_t pixelHeight, int32_t srcX, int32_t srcY, int32_t width, int32_t height);
RoboRect RoboRectClip(RoboRect rect, int32_t width, int32_t height);
