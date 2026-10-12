#pragma once

#include <stdint.h>

void RoboTimerStart(void);
void RoboTimerAdvance(void);
uint64_t RoboTimerTicks(void);
