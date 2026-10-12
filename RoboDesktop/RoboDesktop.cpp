#include "RoboDesktop.hpp"
#include "RoboBootInfo.hpp"

namespace {

constexpr int32_t ROBO_CURSOR_WIDTH = 12;
constexpr int32_t ROBO_CURSOR_HEIGHT = 17;
constexpr uint32_t ROBO_DESKTOP_COLOR = 0x202838u;
constexpr uint32_t ROBO_CURSOR_OUTLINE = 0x10141Cu;
constexpr uint32_t ROBO_CURSOR_FILL = 0xF4F7FBu;
constexpr uint8_t ROBO_CURSOR_EMPTY = 0;
constexpr uint8_t ROBO_CURSOR_EDGE = 1;
constexpr uint8_t ROBO_CURSOR_BODY = 2;

/* 0 empty, 1 outline, 2 fill. Hotspot is the tip at 0,0. */
constexpr uint8_t ROBO_CURSOR_PIXELS[ROBO_CURSOR_HEIGHT * ROBO_CURSOR_WIDTH] = {
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0,
    1, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0,
    1, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0,
    1, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0,
    1, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0,
    1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0,
    1, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 0,
    1, 2, 2, 1, 2, 1, 0, 0, 0, 0, 0, 0,
    1, 2, 1, 0, 1, 2, 1, 0, 0, 0, 0, 0,
    1, 1, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0,
};

void RoboStorePixel(RoboGPU* gpu, int32_t x, int32_t y, uint32_t rgb) {
    if (x < 0 || y < 0 || x >= gpu->width || y >= gpu->height) {
        return;
    }
    uint32_t red = (rgb >> 16) & 0xFFu;
    uint32_t green = (rgb >> 8) & 0xFFu;
    uint32_t blue = rgb & 0xFFu;
    uint32_t packed = gpu->pixelFormat == ROBO_PIXEL_RGBX
        ? (red << 16) | (green << 8) | blue
        : (blue << 16) | (green << 8) | red;
    uint8_t* pixel = gpu->base + static_cast<int64_t>(y) * gpu->stride + static_cast<int64_t>(x) * 4;
    *reinterpret_cast<uint32_t*>(pixel) = packed;
}

RoboRect RoboCursorBounds(int32_t cursorX, int32_t cursorY) {
    RoboRect rect;
    rect.x = cursorX;
    rect.y = cursorY;
    rect.width = ROBO_CURSOR_WIDTH;
    rect.height = ROBO_CURSOR_HEIGHT;
    return rect;
}

bool RoboRectsOverlap(RoboRect a, RoboRect b) {
    if (a.width <= 0 || a.height <= 0 || b.width <= 0 || b.height <= 0) {
        return false;
    }
    return a.x < b.x + b.width && b.x < a.x + a.width && a.y < b.y + b.height && b.y < a.y + a.height;
}

RoboRect RoboRectUnion(RoboRect a, RoboRect b) {
    int32_t left = a.x < b.x ? a.x : b.x;
    int32_t top = a.y < b.y ? a.y : b.y;
    int32_t right = a.x + a.width > b.x + b.width ? a.x + a.width : b.x + b.width;
    int32_t bottom = a.y + a.height > b.y + b.height ? a.y + a.height : b.y + b.height;
    RoboRect rect;
    rect.x = left;
    rect.y = top;
    rect.width = right - left;
    rect.height = bottom - top;
    return rect;
}

void RoboDrawCursor(RoboDesktop* desktop) {
    RoboGPU* gpu = desktop->gpu;
    for (int32_t y = 0; y < ROBO_CURSOR_HEIGHT; ++y) {
        int32_t screenY = desktop->cursorY + y;
        if (screenY < 0 || screenY >= gpu->height) {
            continue;
        }
        for (int32_t x = 0; x < ROBO_CURSOR_WIDTH; ++x) {
            uint8_t sample = ROBO_CURSOR_PIXELS[y * ROBO_CURSOR_WIDTH + x];
            if (sample == ROBO_CURSOR_EMPTY) {
                continue;
            }
            if (sample == ROBO_CURSOR_EDGE) {
                RoboStorePixel(gpu, desktop->cursorX + x, screenY, ROBO_CURSOR_OUTLINE);
            } else if (sample == ROBO_CURSOR_BODY) {
                RoboStorePixel(gpu, desktop->cursorX + x, screenY, ROBO_CURSOR_FILL);
            }
        }
    }
}

int32_t RoboClamp(int32_t value, int32_t limit) {
    if (value < 0) {
        return 0;
    }
    if (limit <= 0) {
        return 0;
    }
    if (value >= limit) {
        return limit - 1;
    }
    return value;
}

}  // namespace

void RoboDesktopBoot(RoboDesktop* desktop, RoboGPU* gpu) {
    desktop->gpu = gpu;
    desktop->background = ROBO_DESKTOP_COLOR;
    desktop->cursorX = gpu->width / 2;
    desktop->cursorY = gpu->height / 2;
    RoboGPUFill(gpu, desktop->background);
    RoboDrawCursor(desktop);
}

void RoboDesktopMoveCursor(RoboDesktop* desktop, int32_t dx, int32_t dy) {
    if (dx == 0 && dy == 0) {
        return;
    }
    RoboGPU* gpu = desktop->gpu;
    int32_t previousX = desktop->cursorX;
    int32_t previousY = desktop->cursorY;
    int64_t nextX = static_cast<int64_t>(desktop->cursorX) + dx;
    int64_t nextY = static_cast<int64_t>(desktop->cursorY) + dy;
    if (nextX > 2147483647ll) {
        nextX = 2147483647ll;
    }
    if (nextY > 2147483647ll) {
        nextY = 2147483647ll;
    }
    if (nextX < -2147483647ll) {
        nextX = -2147483647ll;
    }
    if (nextY < -2147483647ll) {
        nextY = -2147483647ll;
    }
    desktop->cursorX = RoboClamp(static_cast<int32_t>(nextX), gpu->width);
    desktop->cursorY = RoboClamp(static_cast<int32_t>(nextY), gpu->height);
    if (desktop->cursorX == previousX && desktop->cursorY == previousY) {
        return;
    }
    RoboRect oldRect = RoboRectClip(RoboCursorBounds(previousX, previousY), gpu->width, gpu->height);
    RoboRect newRect = RoboRectClip(RoboCursorBounds(desktop->cursorX, desktop->cursorY), gpu->width, gpu->height);
    if (oldRect.width <= 0 && newRect.width <= 0) {
        return;
    }
    if (oldRect.width <= 0) {
        RoboDrawCursor(desktop);
        return;
    }
    if (newRect.width <= 0) {
        RoboGPUFillRect(gpu, oldRect, desktop->background);
        return;
    }
    if (RoboRectsOverlap(oldRect, newRect)) {
        RoboGPUFillRect(gpu, RoboRectUnion(oldRect, newRect), desktop->background);
    } else {
        RoboGPUFillRect(gpu, oldRect, desktop->background);
    }
    RoboDrawCursor(desktop);
}
