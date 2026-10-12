#pragma once

#include <stdint.h>

#define ROBO_PIXEL_BGRX 0u
#define ROBO_PIXEL_RGBX 1u

/* Keep in sync with RoboKernel/RoboLinker.ld */
#define ROBO_KERNEL_LOAD_ADDRESS 0x08000000ull
#define ROBO_MEMORY_RANGE_MAX 48u

typedef struct RoboMemoryRange {
    uint64_t base;
    uint64_t pageCount;
    uint32_t type;
} RoboMemoryRange;

typedef struct RoboBootInfo {
    uint64_t frameBufferBase;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t pixelFormat;
    uint32_t memoryRangeCount;
    RoboMemoryRange memoryRanges[ROBO_MEMORY_RANGE_MAX];
} RoboBootInfo;
