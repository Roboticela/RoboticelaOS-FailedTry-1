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
RoboRect RoboRectClip(RoboRect rect, int32_t width, int32_t height);
