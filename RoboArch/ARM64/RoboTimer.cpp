#include "RoboTimer.hpp"

namespace {

volatile uint64_t gTicks = 0;

}  // namespace

void RoboTimerStart(void) {
}

void RoboTimerAdvance(void) {
    gTicks += 1;
}

uint64_t RoboTimerTicks(void) {
    return gTicks;
}
