#pragma once

#include <stdint.h>

constexpr char ROBO_PROGRAM_MAGIC[8] = {'R', 'O', 'B', 'O', 'P', 'R', 'G', '1'};
constexpr uint32_t ROBO_PROGRAM_X64 = 1u;
constexpr uint32_t ROBO_PROGRAM_ARM64 = 2u;
constexpr uint32_t ROBO_PROGRAM_HEADER = 32u;
constexpr uint64_t ROBO_USER_CODE = 0x00007FFF00000000ull;
constexpr uint64_t ROBO_USER_STACK = 0x00007FFF00028000ull;
constexpr uint64_t ROBO_USER_HEAP = 0x00007FFF00030000ull;
constexpr uint32_t ROBO_PROGRAM_PAGE = 4096u;
constexpr uint32_t ROBO_PROGRAM_CODE_PAGES = 16u;
constexpr uint32_t ROBO_PROGRAM_HEAP_PAGES = 16u;

struct RoboUserMap {
    uint64_t tables;
    uint64_t rip;
    uint64_t rsp;
    uint64_t heap;
    uint64_t heapPhysical;
};
