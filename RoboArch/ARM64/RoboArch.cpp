#include "RoboArch.hpp"

#include "RoboSchedule.hpp"
#include "RoboTimer.hpp"

namespace {

constexpr uint64_t ROBO_GICD = 0x08000000ull;
constexpr uint64_t ROBO_GICC = 0x08010000ull;
constexpr uint32_t ROBO_TIMER_IRQ = 30u;

volatile uint32_t* RoboGic(uint64_t base) {
    return reinterpret_cast<volatile uint32_t*>(base);
}

}  // namespace

extern "C" void RoboTimerRearm(void);

uint8_t RoboPortIn8(uint16_t port) {
    (void)port;
    return 0;
}

void RoboPortOut8(uint16_t port, uint8_t value) {
    (void)port;
    (void)value;
}

uint16_t RoboPortIn16(uint16_t port) {
    (void)port;
    return 0;
}

void RoboPortOut16(uint16_t port, uint16_t value) {
    (void)port;
    (void)value;
}

uint32_t RoboPortIn32(uint16_t port) {
    (void)port;
    return 0;
}

void RoboPortOut32(uint16_t port, uint32_t value) {
    (void)port;
    (void)value;
}

void RoboArchCli(void) {
    __asm__ volatile("msr daifset, #3");
}

void RoboArchSti(void) {
    __asm__ volatile("msr daifclr, #3");
}

void RoboArchIdle(void) {
    __asm__ volatile("msr daifclr, #3; wfi" ::: "memory");
}

void RoboArchIrqClaim(uint8_t irq) {
    (void)irq;
}

extern "C" void RoboArmInterrupt(void) {
    volatile uint32_t* cpu = RoboGic(ROBO_GICC);
    uint32_t interrupt = cpu[0x00C / 4];
    uint32_t id = interrupt & 0x3FFu;
    if (id == ROBO_TIMER_IRQ) {
        RoboTimerAdvance();
        RoboScheduleNotify();
        RoboTimerRearm();
    }
    if (id < 1020u) {
        cpu[0x010 / 4] = interrupt;
    }
}

void RoboArchInterruptsStart(void) {
    volatile uint32_t* distributor = RoboGic(ROBO_GICD);
    volatile uint32_t* cpu = RoboGic(ROBO_GICC);
    distributor[0] |= 3u;
    volatile uint8_t* priority = reinterpret_cast<volatile uint8_t*>(ROBO_GICD + 0x400ull);
    distributor[0x080 / 4] |= 1u << ROBO_TIMER_IRQ;
    priority[ROBO_TIMER_IRQ] = 0x80u;
    distributor[0x100 / 4] |= 1u << ROBO_TIMER_IRQ;
    cpu[0x004 / 4] = 0xFFu;
    cpu[0] |= 7u;
    RoboTimerStart();
}

void RoboArchFpuStart(void) {
    uint64_t access = 0;
    __asm__ volatile("mrs %0, cpacr_el1" : "=r"(access));
    access |= 3ull << 20;
    __asm__ volatile("msr cpacr_el1, %0" : : "r"(access));
    __asm__ volatile("isb" ::: "memory");
}

void RoboHalt(void) {
    for (;;) {
        __asm__ volatile("msr daifset, #3");
        __asm__ volatile("wfi");
    }
}
