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

bool RoboShellTick(uint32_t deltaMs) {
    (void)deltaMs;
    return false;
}

void RoboShellSetBackground(const char* path) {
    (void)path;
}

void RoboShellDamage(int32_t x, int32_t y, int32_t width, int32_t height) {
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}
