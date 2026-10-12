#pragma once

#include <stdint.h>

bool RoboPS2MouseStart(void);
void RoboPS2MouseDrain(int32_t* dx, int32_t* dy);
