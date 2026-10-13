#include "RoboGPUVendor.hpp"

bool RoboGPUVendorBind(RoboGPU* gpu, const RoboBootInfo* info) {
    (void)gpu;
    (void)info;
    return false;
}

void RoboGPUVendorFallback(void) {
}

bool RoboGPUVendorPresent(const uint8_t* pixels, int pixelStride, int x, int y, int width, int height, int surfaceWidth, int surfaceHeight, uint32_t pixelFormat) {
    (void)pixels;
    (void)pixelStride;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)surfaceWidth;
    (void)surfaceHeight;
    (void)pixelFormat;
    return false;
}

void RoboGPUVendorFlush(void) {
}

uint8_t* RoboGPUVendorCard(int* stride) {
    (void)stride;
    return nullptr;
}

void RoboGPUVendorFlushCursor(int x, int y) {
    (void)x;
    (void)y;
}
