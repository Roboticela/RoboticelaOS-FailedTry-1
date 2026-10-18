#pragma once

#include <stdint.h>

#define ROBO_INPUT_MOTION 1u
#define ROBO_INPUT_KEY 2u
#define ROBO_INPUT_TOUCHPAD 3u
#define ROBO_INPUT_TOUCH 4u
#define ROBO_INPUT_WHEEL 5u

struct RoboInputEvent {
    uint8_t kind;
    uint8_t pressed;
    uint8_t button;
    uint16_t code;
    int32_t dx;
    int32_t dy;
    int32_t x;
    int32_t y;
    int32_t xMax;
    int32_t yMax;
};

void RoboInputCollect(void);
bool RoboInputPop(RoboInputEvent* event);
bool RoboInputPush(const RoboInputEvent* event);
