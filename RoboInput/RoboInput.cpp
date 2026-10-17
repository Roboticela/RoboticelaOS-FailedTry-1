#include "RoboInput.hpp"

#include "RoboPS2Keyboard.hpp"
#include "RoboPS2Mouse.hpp"
#include "RoboUsb.hpp"

namespace {

constexpr uint32_t ROBO_INPUT_CAPACITY = 256;

RoboInputEvent gEvents[ROBO_INPUT_CAPACITY];
uint32_t gEventHead = 0;
uint32_t gEventTail = 0;

}  // namespace

bool RoboInputPush(const RoboInputEvent* event) {
    uint32_t next = (gEventHead + 1u) & (ROBO_INPUT_CAPACITY - 1u);
    if (next == gEventTail || event == nullptr) {
        return false;
    }
    gEvents[gEventHead] = *event;
    gEventHead = next;
    return true;
}

bool RoboInputPop(RoboInputEvent* event) {
    if (event == nullptr || gEventHead == gEventTail) {
        return false;
    }
    *event = gEvents[gEventTail];
    gEventTail = (gEventTail + 1u) & (ROBO_INPUT_CAPACITY - 1u);
    return true;
}

void RoboInputCollect(void) {
    int32_t dx = 0;
    int32_t dy = 0;
    uint8_t leftPressed = 0;
    uint8_t leftChanged = 0;
    uint8_t rightPressed = 0;
    uint8_t rightChanged = 0;
    RoboPS2MouseDrain(&dx, &dy, &leftPressed, &leftChanged, &rightPressed, &rightChanged);
    if (dx != 0 || dy != 0 || leftChanged != 0) {
        RoboInputEvent motion{};
        motion.kind = ROBO_INPUT_MOTION;
        motion.dx = dx;
        motion.dy = dy;
        motion.pressed = leftPressed;
        motion.code = leftChanged;
        RoboInputPush(&motion);
    }
    if (rightChanged != 0) {
        RoboInputEvent motion{};
        motion.kind = ROBO_INPUT_MOTION;
        motion.button = 1;
        motion.pressed = rightPressed;
        motion.code = 1;
        RoboInputPush(&motion);
    }
    RoboPS2KeyboardCollect();
    RoboUsbCollect();
}
