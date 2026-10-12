#pragma once

#include "RoboGPU.hpp"

#include <stdint.h>

struct RoboDesktop {
    RoboGPU* gpu;
    int32_t cursorX;
    int32_t cursorY;
    uint32_t background;
    const uint8_t* picture;
    int32_t pictureWidth;
    int32_t pictureHeight;
    uint8_t cursorSave[12 * 17 * 4];
    uint8_t cursorHeld;
};

struct RoboWindow {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    uint32_t* pixels;
};

void RoboDesktopBoot(RoboDesktop* desktop, RoboGPU* gpu);
void RoboDesktopShowPicture(RoboDesktop* desktop, const uint8_t* pixels, int32_t width, int32_t height);
void RoboDesktopHideCursor(RoboDesktop* desktop);
void RoboDesktopShowCursor(RoboDesktop* desktop);
bool RoboDesktopKeepCursor(RoboDesktop* desktop, int32_t x, int32_t y, const uint8_t* pixel);
void RoboDesktopMoveCursor(RoboDesktop* desktop, int32_t dx, int32_t dy);
void RoboDesktopPlaceCursor(RoboDesktop* desktop, int32_t x, int32_t y);
void RoboWindowFill(RoboWindow* window, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t rgb);
void RoboDesktopOpenWindow(RoboDesktop* desktop, RoboWindow* window, int32_t x, int32_t y, int32_t width, int32_t height);
void RoboDesktopPresentWindow(RoboDesktop* desktop, RoboWindow* window, int32_t localX, int32_t localY, int32_t width, int32_t height);
