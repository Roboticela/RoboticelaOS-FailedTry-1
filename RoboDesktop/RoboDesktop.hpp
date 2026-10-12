#pragma once

#include "RoboGPU.hpp"

#include <stdint.h>

struct RoboDesktop {
    RoboGPU* gpu;
    int32_t cursorX;
    int32_t cursorY;
    uint32_t background;
};

void RoboDesktopBoot(RoboDesktop* desktop, RoboGPU* gpu);
void RoboDesktopMoveCursor(RoboDesktop* desktop, int32_t dx, int32_t dy);
void RoboDesktopPlaceCursor(RoboDesktop* desktop, int32_t x, int32_t y);
