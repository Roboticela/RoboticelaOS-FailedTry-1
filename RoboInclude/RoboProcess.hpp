#pragma once

#include "RoboProgram.hpp"

#include <stdint.h>

bool RoboProcessLoad(RoboUserMap* map);
void RoboProcessNote(uint32_t reason);
void RoboLauncherScan(void);
bool RoboLauncherSelect(int index);
