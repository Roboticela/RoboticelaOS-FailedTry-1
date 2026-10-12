#include "RoboArch.hpp"

#include "RoboTimer.hpp"

extern "C" char RoboVectorTable[];

void RoboArchCli(void) {
    __asm__ volatile("msr daifset, #2");
}

void RoboArchSti(void) {
    __asm__ volatile("msr daifset, #2");
}

void RoboArchIdle(void) {
    uint64_t start = 0;
    uint64_t now = 0;
    uint64_t frequency = 0;
    __asm__ volatile("mrs %0, cntpct_el0" : "=r"(start));
    __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(frequency));
    uint64_t step = frequency / 100ull;
    if (step == 0) {
        step = 1;
    }
    do {
        __asm__ volatile("mrs %0, cntpct_el0" : "=r"(now));
    } while (now - start < step);
    RoboTimerAdvance();
}

void RoboArchIrqClaim(uint8_t irq) {
    (void)irq;
}

void RoboArchInterruptsStart(void) {
    uint64_t table = reinterpret_cast<uint64_t>(RoboVectorTable);
    __asm__ volatile("msr vbar_el1, %0" : : "r"(table));
}

void RoboHalt(void) {
    for (;;) {
        __asm__ volatile("msr daifset, #2");
        __asm__ volatile("wfi");
    }
}
