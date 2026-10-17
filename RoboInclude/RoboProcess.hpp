#pragma once

#include "RoboProgram.hpp"

#include <stdint.h>

bool RoboProcessLoad(RoboUserMap* map);
void RoboProcessNote(uint32_t reason);
void RoboLauncherScan(void);
bool RoboLauncherSelect(int index);
int RoboLauncherCount(void);
const char* RoboLauncherName(int index);
extern "C" volatile uint8_t gRoboDiskReady;
extern "C" volatile uint8_t gRoboLaunchChosen;
