#pragma once

#include <stdint.h>

struct RoboDesktop;
struct RoboInputEvent;

struct RoboFrameView {
    uint32_t* pixels;
    int32_t stride;
    int32_t originX;
    int32_t originY;
    int32_t width;
    int32_t height;
    int32_t screenX;
    int32_t screenY;
};

void RoboFrameStart(RoboDesktop* desktop);
uint32_t RoboFrameOpen(uint32_t thread, int32_t x, int32_t y, int32_t width, int32_t height, uint8_t app);
void RoboFrameRelease(uint32_t thread);
void RoboFrameCompose(RoboDesktop* desktop);
// Blit only frames that intersect the shell damage rect (desktop Present wipe).
void RoboFrameComposeDamage(RoboDesktop* desktop, int32_t x, int32_t y, int32_t width, int32_t height);
bool RoboFramePointer(const RoboInputEvent* event, int32_t x, int32_t y, uint8_t* appClient, int32_t* localX, int32_t* localY);
bool RoboFrameTakesKeys(void);
bool RoboFrameAppFocused(void);
bool RoboFrameAppView(RoboFrameView* view);
void RoboFrameShow(uint32_t thread, int32_t localX, int32_t localY, int32_t width, int32_t height);
