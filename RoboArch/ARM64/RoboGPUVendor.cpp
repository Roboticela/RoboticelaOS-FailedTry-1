#include "RoboGPUVendor.hpp"

bool RoboGPUVendorBind(RoboGPU* gpu, const RoboBootInfo* info) {
    (void)gpu;
    (void)info;
    return false;
}

void RoboGPUVendorFallback(void) {
}

uint32_t RoboGPUVendorCaps(void) {
    return 0;
}

bool RoboGPUVendorFillRect(int32_t x, int32_t y, int32_t width, int32_t height, uint32_t rgb) {
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)rgb;
    return false;
}

bool RoboGPUVendorCopyRect(int32_t dstX, int32_t dstY, const uint32_t* srcPixels, int32_t srcStride, int32_t srcX, int32_t srcY, int32_t width, int32_t height) {
    (void)dstX;
    (void)dstY;
    (void)srcPixels;
    (void)srcStride;
    (void)srcX;
    (void)srcY;
    (void)width;
    (void)height;
    return false;
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
