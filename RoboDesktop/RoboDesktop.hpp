#pragma once

#include "RoboGPU.hpp"

#include <stdint.h>

constexpr int32_t ROBO_DAMAGE_MAX_RECTS = 32;
constexpr int32_t ROBO_DESKTOP_MAX_WINDOWS = 16;
constexpr int32_t ROBO_TILE_SIZE = 64;

struct RoboDamageTracker {
    RoboRect rects[ROBO_DAMAGE_MAX_RECTS];
    int32_t count;
    bool fullDamage;
};

struct RoboWindow {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    uint32_t* pixels;
    uint8_t opacity;
    uint8_t hasAlpha;
    RoboRect damage;
    bool needsRepaint;
};

struct RoboWindowSnapshot {
    RoboRect bounds;
    const uint32_t* pixels;
    int32_t width;
    int32_t height;
    uint8_t opacity;
    uint8_t hasAlpha;
};

struct RoboSceneSnapshot {
    uint32_t background;
    const uint8_t* picture;
    int32_t pictureWidth;
    int32_t pictureHeight;
    int32_t windowCount;
    RoboWindowSnapshot windows[ROBO_DESKTOP_MAX_WINDOWS];
};

struct RoboDesktop {
    RoboGPU* gpu;
    uint32_t* backbuffer;
    int32_t cursorX;
    int32_t cursorY;
    uint32_t background;
    const uint8_t* picture;
    int32_t pictureWidth;
    int32_t pictureHeight;
    uint8_t cursorSave[12 * 17 * 4];
    uint8_t cursorHeld;

    RoboDamageTracker damage;
    RoboWindow* windows[ROBO_DESKTOP_MAX_WINDOWS];
    int32_t windowCount;
};

// Damage Tracking APIs
void RoboDamageInit(RoboDamageTracker* tracker);
void RoboDamageReset(RoboDamageTracker* tracker);
void RoboDamageAdd(RoboDamageTracker* tracker, RoboRect rect, int32_t screenWidth, int32_t screenHeight);
void RoboDamageMerge(RoboDamageTracker* tracker, int32_t screenWidth, int32_t screenHeight);

// Window Invalidation and Geometry APIs
void RoboWindowInvalidate(RoboWindow* window, RoboRect localRect);
void RoboWindowInvalidateAll(RoboWindow* window);
void RoboWindowFill(RoboWindow* window, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t rgb);

// Desktop Lifecycle and Presentation APIs
void RoboDesktopBoot(RoboDesktop* desktop, RoboGPU* gpu);
void RoboDesktopShowPicture(RoboDesktop* desktop, const uint8_t* pixels, int32_t width, int32_t height);
void RoboDesktopHideCursor(RoboDesktop* desktop);
void RoboDesktopShowCursor(RoboDesktop* desktop);
void RoboDesktopRedrawCursor(RoboDesktop* desktop);
void RoboDesktopUseCursor(RoboDesktop* desktop);
bool RoboDesktopCursorRect(int32_t* x, int32_t* y, int32_t* width, int32_t* height);
bool RoboDesktopCursorSample(int32_t localX, int32_t localY, uint32_t* rgb);
void RoboDesktopRememberCursor(int32_t localX, int32_t localY, uint32_t packed);
bool RoboDesktopKeepCursor(RoboDesktop* desktop, int32_t x, int32_t y, const uint8_t* pixel);
void RoboDesktopMoveCursor(RoboDesktop* desktop, int32_t dx, int32_t dy);
void RoboDesktopPlaceCursor(RoboDesktop* desktop, int32_t x, int32_t y);

// Window Management with Damage Tracking
void RoboDesktopOpenWindow(RoboDesktop* desktop, RoboWindow* window, int32_t x, int32_t y, int32_t width, int32_t height);
void RoboDesktopPresentWindow(RoboDesktop* desktop, RoboWindow* window, int32_t localX, int32_t localY, int32_t width, int32_t height);
void RoboDesktopMoveWindow(RoboDesktop* desktop, RoboWindow* window, int32_t newX, int32_t newY);
void RoboDesktopResizeWindow(RoboDesktop* desktop, RoboWindow* window, int32_t newWidth, int32_t newHeight);
void RoboDesktopCloseWindow(RoboDesktop* desktop, RoboWindow* window);

// Compositing & Presentation
void RoboDesktopSnapshotScene(RoboDesktop* desktop, RoboSceneSnapshot* snapshot);
void RoboDesktopRenderDamage(RoboDesktop* desktop);
void RoboDesktopPresentDamage(RoboDesktop* desktop);
void RoboDesktopRestoreRect(RoboDesktop* desktop, RoboRect rect);
