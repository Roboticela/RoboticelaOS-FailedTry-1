#include "RoboShell.hpp"
#include "RoboBootInfo.hpp"

void RoboDevKitBind(const RoboBootInfo* info) {
    (void)info;
}

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

void RoboShellPointerContext(int32_t x, int32_t y) {
    (void)x;
    (void)y;
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

bool RoboShellNeedsTick() {
    return false;
}

void RoboShellLastPresentRect(int32_t* x, int32_t* y, int32_t* width, int32_t* height) {
    if (x != nullptr) {
        *x = 0;
    }
    if (y != nullptr) {
        *y = 0;
    }
    if (width != nullptr) {
        *width = 0;
    }
    if (height != nullptr) {
        *height = 0;
    }
}

extern "C" void RoboDebugLine(const char* text) {
    (void)text;
}
