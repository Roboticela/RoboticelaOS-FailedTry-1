#pragma once

#include <stdint.h>

void RoboScheduleStart(void);
void RoboScheduleNotify(void);
void RoboSchedule(void);
void RoboThreadSleep(uint64_t wakeTick);
extern "C" void RoboSyscallDispatch(uint64_t number);
void RoboKernelRspSet(uint64_t stackTop);
