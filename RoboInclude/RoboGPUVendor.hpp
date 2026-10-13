#pragma once

#include <stdint.h>

struct RoboGPU;
struct RoboBootInfo;

bool RoboGPUVendorBind(RoboGPU* gpu, const RoboBootInfo* info);
void RoboGPUVendorFallback(void);
bool RoboGPUVendorPresent(const uint8_t* pixels, int pixelStride, int x, int y, int width, int height, int surfaceWidth, int surfaceHeight, uint32_t pixelFormat);
void RoboGPUVendorFlush(void);
uint8_t* RoboGPUVendorCard(int* stride);
void RoboGPUVendorFlushCursor(int x, int y);
