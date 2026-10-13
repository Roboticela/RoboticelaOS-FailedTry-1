#include "RoboTimer.hpp"

namespace {

constexpr uint32_t ROBO_TIMER_HZ = 100u;

volatile uint64_t gTimerTicks = 0;

uint64_t RoboTimerStep(void) {
    uint64_t frequency = 0;
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frequency));
    uint64_t step = frequency / ROBO_TIMER_HZ;
    if (step == 0) {
        step = 1;
    }
    return step;
}

}  // namespace

void RoboTimerStart(void) {
    uint64_t step = RoboTimerStep();
    __asm__ volatile("msr cntv_ctl_el0, %0" : : "r"(2ull));
    __asm__ volatile("msr cntp_tval_el0, %0" : : "r"(step));
    __asm__ volatile("msr cntp_ctl_el0, %0" : : "r"(1ull));
}

extern "C" void RoboTimerRearm(void) {
    uint64_t step = RoboTimerStep();
    __asm__ volatile("msr cntp_tval_el0, %0" : : "r"(step));
}

void RoboTimerAdvance(void) {
    gTimerTicks += 1;
}

uint64_t RoboTimerTicks(void) {
    return gTimerTicks;
}
