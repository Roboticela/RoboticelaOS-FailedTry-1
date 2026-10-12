#include "RoboMemory.hpp"

namespace {

constexpr uint64_t ROBO_ARENA_BYTES = 4096ull;

alignas(16) uint8_t gFrameBytes[ROBO_ARENA_BYTES];
alignas(16) uint8_t gLongBytes[ROBO_ARENA_BYTES];
RoboArena gFrameArena{};
RoboArena gLongArena{};

}  // namespace

void RoboMemoryStart(const RoboBootInfo* info) {
    (void)info;
    gFrameArena.base = gFrameBytes;
    gFrameArena.used = 0;
    gFrameArena.capacity = ROBO_ARENA_BYTES;
    gLongArena.base = gLongBytes;
    gLongArena.used = 0;
    gLongArena.capacity = ROBO_ARENA_BYTES;
}

void RoboMemoryMarkUncached(uint64_t address, uint64_t bytes) {
    (void)address;
    (void)bytes;
}

uint64_t RoboMemoryKernelCr3(void) {
    return 0;
}

uint64_t RoboPageAlloc(void) {
    return 0;
}

void RoboArenaReset(RoboArena* arena) {
    if (arena != nullptr) {
        arena->used = 0;
    }
}

void* RoboArenaAlloc(RoboArena* arena, uint64_t size, uint64_t align) {
    if (arena == nullptr || arena->base == nullptr || align == 0 || (align & (align - 1ull)) != 0) {
        return nullptr;
    }
    uint64_t start = (arena->used + align - 1ull) & ~(align - 1ull);
    if (start + size > arena->capacity) {
        return nullptr;
    }
    arena->used = start + size;
    return arena->base + start;
}

RoboArena* RoboArenaFrame(void) {
    return &gFrameArena;
}

RoboArena* RoboArenaLong(void) {
    return &gLongArena;
}

uint64_t RoboMemoryCreateUserSpace(const void* code, uint64_t codeSize, uint64_t* userRip, uint64_t* userRsp) {
    (void)code;
    (void)codeSize;
    (void)userRip;
    (void)userRsp;
    return 0;
}
