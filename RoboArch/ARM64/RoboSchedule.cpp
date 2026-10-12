#include "RoboSchedule.hpp"

void RoboScheduleStart(void) {
}

void RoboScheduleNotify(void) {
}

void RoboSchedule(void) {
}

void RoboThreadSleep(uint64_t wakeTick) {
    (void)wakeTick;
}

extern "C" void RoboSyscallDispatch(uint64_t number) {
    (void)number;
}

void RoboKernelRspSet(uint64_t stackTop) {
    (void)stackTop;
}
