#pragma once

#include "RoboGPU.hpp"

#include <stdint.h>

struct RoboDesktop {
    RoboGPU* gpu;
    int32_t cursorX;
    int32_t cursorY;
    uint32_t background;
};

struct RoboWindow {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    uint32_t* pixels;
};

void RoboDesktopBoot(RoboDesktop* desktop, RoboGPU* gpu);
void RoboDesktopMoveCursor(RoboDesktop* desktop, int32_t dx, int32_t dy);
void RoboDesktopPlaceCursor(RoboDesktop* desktop, int32_t x, int32_t y);
void RoboWindowFill(RoboWindow* window, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t rgb);
void RoboDesktopOpenWindow(RoboDesktop* desktop, RoboWindow* window, int32_t x, int32_t y, int32_t width, int32_t height);
void RoboDesktopPresentWindow(RoboDesktop* desktop, RoboWindow* window, int32_t localX, int32_t localY, int32_t width, int32_t height);
