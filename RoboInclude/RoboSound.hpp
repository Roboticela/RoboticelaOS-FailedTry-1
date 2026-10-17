#pragma once

#include <stdint.h>

bool RoboSoundStart(void);
bool RoboSoundWrite(const int16_t* samples, uint32_t count);
