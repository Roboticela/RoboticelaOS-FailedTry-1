#pragma once

#include <stdint.h>

typedef bool (*RoboFsSectorRead)(uint32_t lba, uint8_t* sector);

struct RoboBootInfo;

bool RoboFsStart(RoboFsSectorRead read);
void RoboFsUseBootFiles(const RoboBootInfo* info);
bool RoboFsReadFile(const char* name, uint8_t* dest, uint32_t capacity, uint32_t* size);
