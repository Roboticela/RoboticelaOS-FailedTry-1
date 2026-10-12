#include "RoboTimer.hpp"

#include "RoboArch.hpp"

namespace {

constexpr uint16_t ROBO_PIT_CHANNEL = 0x40;
constexpr uint16_t ROBO_PIT_COMMAND = 0x43;
constexpr uint32_t ROBO_PIT_FREQUENCY = 1193182u;
constexpr uint32_t ROBO_TIMER_HZ = 100u;
constexpr uint8_t ROBO_PIT_RATE_GENERATOR = 0x36;

volatile uint64_t gTimerTicks = 0;

}  // namespace

void RoboTimerStart(void) {
    uint32_t divisor = ROBO_PIT_FREQUENCY / ROBO_TIMER_HZ;
    RoboPortOut8(ROBO_PIT_COMMAND, ROBO_PIT_RATE_GENERATOR);
    RoboPortOut8(ROBO_PIT_CHANNEL, static_cast<uint8_t>(divisor & 0xFFu));
    RoboPortOut8(ROBO_PIT_CHANNEL, static_cast<uint8_t>((divisor >> 8) & 0xFFu));
}

void RoboTimerAdvance(void) {
    gTimerTicks += 1;
}

uint64_t RoboTimerTicks(void) {
    return gTimerTicks;
}
