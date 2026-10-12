#include "RoboPS2Keyboard.hpp"
#include "RoboPS2Mouse.hpp"
#include "RoboUsb.hpp"

bool RoboPS2MouseStart(void) {
    return false;
}

void RoboPS2MouseArm(void) {
}

void RoboPS2MouseInterrupt(void) {
}

void RoboPS2MouseDrain(int32_t* dx, int32_t* dy) {
    if (dx != nullptr) {
        *dx = 0;
    }
    if (dy != nullptr) {
        *dy = 0;
    }
}

bool RoboPS2KeyboardStart(void) {
    return false;
}

void RoboPS2KeyboardInterrupt(void) {
}

void RoboPS2KeyboardCollect(void) {
}

void RoboUsbStart(void) {
}

void RoboUsbCollect(void) {
}
