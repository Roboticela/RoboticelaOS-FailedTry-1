#pragma once

#include <stdint.h>

void RoboScheduleStart(void);
void RoboScheduleNotify(void);
void RoboSchedule(void);
void RoboThreadSleep(uint64_t wakeTick);
extern "C" uint64_t RoboSyscallDispatch(uint64_t number, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3);
void RoboThreadEnd(uint32_t reason);
void RoboKernelRspSet(uint64_t stackTop);
