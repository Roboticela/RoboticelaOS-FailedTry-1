#include "RoboShell.hpp"

bool RoboShellStart(RoboDesktop* desktop) {
    (void)desktop;
    return false;
}

void RoboShellPointerMove(int32_t x, int32_t y) {
    (void)x;
    (void)y;
}

void RoboShellPointerDown(int32_t x, int32_t y, bool fromTouch) {
    (void)x;
    (void)y;
    (void)fromTouch;
}

void RoboShellPointerUp(int32_t x, int32_t y) {
    (void)x;
    (void)y;
}

void RoboShellKey(uint16_t code, bool pressed) {
    (void)code;
    (void)pressed;
}

bool RoboShellFlush(RoboDesktop* desktop) {
    (void)desktop;
    return false;
}
