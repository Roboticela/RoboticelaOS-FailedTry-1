#pragma once

#include <stdint.h>

#define ROBO_INPUT_MOTION 1u
#define ROBO_INPUT_KEY 2u

struct RoboInputEvent {
    uint8_t kind;
    uint8_t pressed;
    uint16_t code;
    int32_t dx;
    int32_t dy;
};

void RoboInputCollect(void);
bool RoboInputPop(RoboInputEvent* event);
bool RoboInputPush(const RoboInputEvent* event);
