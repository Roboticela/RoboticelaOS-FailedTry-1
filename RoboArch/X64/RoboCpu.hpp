#pragma once

#include <stdint.h>

void RoboArchCpuStart(void);
void RoboArchEnterUser(uint64_t instructionPointer, uint64_t stackPointer);
extern "C" void RoboContextSwitch(uint64_t* saveRsp, uint64_t nextRsp, uint64_t nextCr3);
extern "C" void RoboSyscallEntry(void);
