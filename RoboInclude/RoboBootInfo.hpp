#pragma once

#include <stdint.h>

#define ROBO_PIXEL_BGRX 0u
#define ROBO_PIXEL_RGBX 1u

/* Keep in sync with RoboKernel/RoboLinker.ld and RoboKernel/RoboApp.ld */
#define ROBO_KERNEL_LOAD_ADDRESS 0x08000000ull
/* Covers .text/.data/.bss and vendor GPU card buffer. Heap is carved from free RAM. */
#define ROBO_KERNEL_BYTES (128ull * 1024ull * 1024ull)
/* Compiler apps loaded from Applications/ map here (one slot). */
#define ROBO_APP_LOAD_ADDRESS 0x10000000ull
#define ROBO_APP_BYTES (32ull * 1024ull * 1024ull)
/* Identity map scales with installed RAM: at least 1 GiB, up to 64 GiB. */
#define ROBO_RAM_MIN_BYTES (1ull * 1024ull * 1024ull * 1024ull)
#define ROBO_RAM_MAP_MAX_BYTES (64ull * 1024ull * 1024ull * 1024ull)
#define ROBO_MEMORY_RANGE_MAX 256u
#define ROBO_BOOT_FILE_MAX 24u
#define ROBO_BOOT_FILE_NAME 16u

typedef struct RoboMemoryRange {
    uint64_t base;
    uint64_t pageCount;
    uint32_t type;
} RoboMemoryRange;

typedef struct RoboBootFile {
    char name[ROBO_BOOT_FILE_NAME];
    uint32_t size;
    uint32_t reserved;
    uint64_t address;
} RoboBootFile;

typedef struct RoboBootInfo {
    uint64_t frameBufferBase;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t pixelFormat;
    uint32_t memoryRangeCount;
    RoboMemoryRange memoryRanges[ROBO_MEMORY_RANGE_MAX];
    uint32_t fileCount;
    RoboBootFile files[ROBO_BOOT_FILE_MAX];
} RoboBootInfo;
