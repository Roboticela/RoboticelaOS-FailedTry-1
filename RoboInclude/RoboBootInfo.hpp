#pragma once

#include <stdint.h>

#define ROBO_PIXEL_BGRX 0u
#define ROBO_PIXEL_RGBX 1u

/* Keep in sync with RoboKernel/RoboLinker.ld */
#define ROBO_KERNEL_LOAD_ADDRESS 0x08000000ull

typedef struct RoboBootInfo {
    uint64_t frameBufferBase;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t pixelFormat;
} RoboBootInfo;
