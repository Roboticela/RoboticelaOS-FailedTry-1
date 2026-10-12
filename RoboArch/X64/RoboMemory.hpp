#pragma once

#include <stdint.h>

struct RoboBootInfo;

struct RoboArena {
    uint8_t* base;
    uint64_t used;
    uint64_t capacity;
};

void RoboMemoryStart(const RoboBootInfo* info);
uint64_t RoboMemoryKernelCr3(void);
uint64_t RoboPageAlloc(void);
void RoboArenaReset(RoboArena* arena);
void* RoboArenaAlloc(RoboArena* arena, uint64_t size, uint64_t align);
RoboArena* RoboArenaFrame(void);
RoboArena* RoboArenaLong(void);
uint64_t RoboMemoryCreateUserSpace(const void* code, uint64_t codeSize, uint64_t* userRip, uint64_t* userRsp);
