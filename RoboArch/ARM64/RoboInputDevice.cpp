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

void RoboPS2MouseDrain(int32_t* dx, int32_t* dy, uint8_t* leftPressed, uint8_t* leftChanged, uint8_t* rightPressed, uint8_t* rightChanged) {
    if (dx != nullptr) {
        *dx = 0;
    }
    if (dy != nullptr) {
        *dy = 0;
    }
    if (leftPressed != nullptr) {
        *leftPressed = 0;
    }
    if (leftChanged != nullptr) {
        *leftChanged = 0;
    }
    if (rightPressed != nullptr) {
        *rightPressed = 0;
    }
    if (rightChanged != nullptr) {
        *rightChanged = 0;
    }
}

bool RoboPS2KeyboardStart(void) {
    return false;
}

void RoboPS2KeyboardInterrupt(void) {
}

void RoboPS2KeyboardCollect(void) {
}

bool RoboUsbStart(void) {
    return true;
}

void RoboUsbCollect(void) {
}
