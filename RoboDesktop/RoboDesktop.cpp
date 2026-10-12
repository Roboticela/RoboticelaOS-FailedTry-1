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

void RoboDesktopRestoreRect(RoboDesktop* desktop, RoboRect rect);

void RoboDesktopBoot(RoboDesktop* desktop, RoboGPU* gpu) {
    desktop->gpu = gpu;
    desktop->background = ROBO_DESKTOP_COLOR;
    desktop->cursorX = gpu->width / 2;
    desktop->cursorY = gpu->height / 2;
    desktop->picture = nullptr;
    desktop->pictureWidth = 0;
    desktop->pictureHeight = 0;
    desktop->cursorHeld = 0;
    RoboGPUFill(gpu, desktop->background);
    RoboDrawCursor(desktop);
}

void RoboDesktopRestoreRect(RoboDesktop* desktop, RoboRect rect) {
    RoboGPU* gpu = desktop->gpu;
    rect = RoboRectClip(rect, gpu->width, gpu->height);
    if (rect.width <= 0 || rect.height <= 0) {
        return;
    }
    if (desktop->picture == nullptr || desktop->pictureWidth <= 0 || desktop->pictureHeight <= 0) {
        RoboGPUFillRect(gpu, rect, desktop->background);
        return;
    }
    if (rect.x >= desktop->pictureWidth || rect.y >= desktop->pictureHeight) {
        RoboGPUFillRect(gpu, rect, desktop->background);
        return;
    }
    if (rect.x + rect.width > desktop->pictureWidth) {
        RoboRect outside = rect;
        outside.x = desktop->pictureWidth;
        outside.width = rect.x + rect.width - desktop->pictureWidth;
        RoboGPUFillRect(gpu, outside, desktop->background);
        rect.width = desktop->pictureWidth - rect.x;
    }
    if (rect.y + rect.height > desktop->pictureHeight) {
        RoboRect outside = rect;
        outside.y = desktop->pictureHeight;
        outside.height = rect.y + rect.height - desktop->pictureHeight;
        RoboGPUFillRect(gpu, outside, desktop->background);
        rect.height = desktop->pictureHeight - rect.y;
    }
    for (int32_t row = 0; row < rect.height; ++row) {
        const uint8_t* from = desktop->picture
            + (static_cast<int64_t>(rect.y + row) * desktop->pictureWidth + rect.x) * 4;
        uint8_t* to = gpu->base + static_cast<int64_t>(rect.y + row) * gpu->stride + static_cast<int64_t>(rect.x) * 4;
        for (int32_t column = 0; column < rect.width * 4; ++column) {
            to[column] = from[column];
        }
    }
}

void RoboCursorRead(RoboDesktop* desktop, int32_t originX, int32_t originY) {
    RoboGPU* gpu = desktop->gpu;
    for (int32_t y = 0; y < 17; ++y) {
        for (int32_t x = 0; x < 12; ++x) {
            uint8_t* saved = desktop->cursorSave + (static_cast<int64_t>(y) * 12 + x) * 4;
            int32_t screenX = originX + x;
            int32_t screenY = originY + y;
            if (screenX < 0 || screenY < 0 || screenX >= gpu->width || screenY >= gpu->height) {
                saved[0] = 0;
                saved[1] = 0;
                saved[2] = 0;
                saved[3] = 0;
                continue;
            }
            uint8_t* pixel = gpu->base + static_cast<int64_t>(screenY) * gpu->stride + static_cast<int64_t>(screenX) * 4;
            saved[0] = pixel[0];
            saved[1] = pixel[1];
            saved[2] = pixel[2];
            saved[3] = pixel[3];
        }
    }
}

void RoboCursorWrite(RoboDesktop* desktop, int32_t originX, int32_t originY) {
    RoboGPU* gpu = desktop->gpu;
    for (int32_t y = 0; y < 17; ++y) {
        for (int32_t x = 0; x < 12; ++x) {
            int32_t screenX = originX + x;
            int32_t screenY = originY + y;
            if (screenX < 0 || screenY < 0 || screenX >= gpu->width || screenY >= gpu->height) {
                continue;
            }
            if (ROBO_CURSOR_PIXELS[y * 12 + x] == ROBO_CURSOR_EMPTY) {
                continue;
            }
            uint8_t* saved = desktop->cursorSave + (static_cast<int64_t>(y) * 12 + x) * 4;
            uint8_t* pixel = gpu->base + static_cast<int64_t>(screenY) * gpu->stride + static_cast<int64_t>(screenX) * 4;
            pixel[0] = saved[0];
            pixel[1] = saved[1];
            pixel[2] = saved[2];
            pixel[3] = saved[3];
        }
    }
}

void RoboDesktopHideCursor(RoboDesktop* desktop) {
    if (desktop == nullptr || desktop->cursorHeld == 0 || desktop->picture != nullptr) {
        return;
    }
    RoboCursorWrite(desktop, desktop->cursorX, desktop->cursorY);
    desktop->cursorHeld = 0;
}

void RoboDesktopShowCursor(RoboDesktop* desktop) {
    if (desktop == nullptr || desktop->gpu == nullptr || desktop->picture != nullptr || desktop->cursorHeld != 0) {
        return;
    }
    RoboCursorRead(desktop, desktop->cursorX, desktop->cursorY);
    desktop->cursorHeld = 1;
    RoboDrawCursor(desktop);
}

void RoboDesktopRedrawCursor(RoboDesktop* desktop) {
    if (desktop == nullptr || desktop->gpu == nullptr || desktop->cursorHeld == 0 || desktop->picture != nullptr) {
        return;
    }
    RoboDrawCursor(desktop);
}

bool RoboDesktopKeepCursor(RoboDesktop* desktop, int32_t x, int32_t y, const uint8_t* pixel) {
    if (desktop == nullptr || pixel == nullptr || desktop->cursorHeld == 0) {
        return false;
    }
    int32_t localX = x - desktop->cursorX;
    int32_t localY = y - desktop->cursorY;
    if (localX < 0 || localY < 0 || localX >= ROBO_CURSOR_WIDTH || localY >= ROBO_CURSOR_HEIGHT) {
        return false;
    }
    uint8_t* saved = desktop->cursorSave + (static_cast<int64_t>(localY) * ROBO_CURSOR_WIDTH + localX) * 4;
    saved[0] = pixel[0];
    saved[1] = pixel[1];
    saved[2] = pixel[2];
    saved[3] = pixel[3];
    uint8_t sample = ROBO_CURSOR_PIXELS[localY * ROBO_CURSOR_WIDTH + localX];
    return sample != ROBO_CURSOR_EMPTY;
}

void RoboDesktopShowPicture(RoboDesktop* desktop, const uint8_t* pixels, int32_t width, int32_t height) {
    if (desktop == nullptr || desktop->gpu == nullptr || pixels == nullptr || width <= 0 || height <= 0) {
        return;
    }
    desktop->picture = pixels;
    desktop->pictureWidth = width;
    desktop->pictureHeight = height;
    RoboRect screen;
    screen.x = 0;
    screen.y = 0;
    screen.width = desktop->gpu->width;
    screen.height = desktop->gpu->height;
    RoboDesktopRestoreRect(desktop, screen);
    RoboDrawCursor(desktop);
}

void RoboDesktopPaintCursor(RoboDesktop* desktop, int32_t previousX, int32_t previousY);

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
    RoboDesktopPaintCursor(desktop, previousX, previousY);
}

void RoboDesktopPaintCursor(RoboDesktop* desktop, int32_t previousX, int32_t previousY) {
    if (desktop->cursorX == previousX && desktop->cursorY == previousY) {
        return;
    }
    RoboGPU* gpu = desktop->gpu;
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
    if (desktop->picture == nullptr) {
        if (desktop->cursorHeld != 0) {
            RoboCursorWrite(desktop, previousX, previousY);
            desktop->cursorHeld = 0;
        }
        RoboCursorRead(desktop, desktop->cursorX, desktop->cursorY);
        desktop->cursorHeld = 1;
        RoboDrawCursor(desktop);
        return;
    }
    if (RoboRectsOverlap(oldRect, newRect)) {
        RoboDesktopRestoreRect(desktop, RoboRectUnion(oldRect, newRect));
    } else {
        RoboDesktopRestoreRect(desktop, oldRect);
    }
    RoboDrawCursor(desktop);
}

void RoboDesktopPlaceCursor(RoboDesktop* desktop, int32_t x, int32_t y) {
    int32_t previousX = desktop->cursorX;
    int32_t previousY = desktop->cursorY;
    desktop->cursorX = RoboClamp(x, desktop->gpu->width);
    desktop->cursorY = RoboClamp(y, desktop->gpu->height);
    RoboDesktopPaintCursor(desktop, previousX, previousY);
}

namespace {

constexpr uint32_t ROBO_WINDOW_PAPER = 0xF3F0E8u;
constexpr uint32_t ROBO_WINDOW_TITLE = 0x243044u;
constexpr uint32_t ROBO_WINDOW_EDGE = 0x1A2230u;
constexpr int32_t ROBO_WINDOW_TITLE_HEIGHT = 28;

}  // namespace

void RoboWindowFill(RoboWindow* window, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t rgb) {
    if (window == nullptr || window->pixels == nullptr || window->width <= 0 || window->height <= 0) {
        return;
    }
    if (x < 0) {
        width += x;
        x = 0;
    }
    if (y < 0) {
        height += y;
        y = 0;
    }
    if (width <= 0 || height <= 0 || x >= window->width || y >= window->height) {
        return;
    }
    if (x + width > window->width) {
        width = window->width - x;
    }
    if (y + height > window->height) {
        height = window->height - y;
    }
    for (int32_t row = 0; row < height; ++row) {
        uint32_t* pixel = window->pixels + static_cast<int64_t>(y + row) * window->width + x;
        for (int32_t column = 0; column < width; ++column) {
            pixel[column] = rgb;
        }
    }
}

void RoboDesktopPresentWindow(RoboDesktop* desktop, RoboWindow* window, int32_t localX, int32_t localY, int32_t width, int32_t height) {
    if (desktop == nullptr || desktop->gpu == nullptr || window == nullptr || window->pixels == nullptr) {
        return;
    }
    if (width <= 0 || height <= 0) {
        return;
    }
    RoboGPUCopyRect(desktop->gpu, window->x + localX, window->y + localY, window->pixels, window->width, window->height, localX, localY, width, height);
    RoboRect damage;
    damage.x = window->x + localX;
    damage.y = window->y + localY;
    damage.width = width;
    damage.height = height;
    damage = RoboRectClip(damage, desktop->gpu->width, desktop->gpu->height);
    RoboRect cursor = RoboRectClip(RoboCursorBounds(desktop->cursorX, desktop->cursorY), desktop->gpu->width, desktop->gpu->height);
    if (RoboRectsOverlap(damage, cursor)) {
        RoboDrawCursor(desktop);
    }
}

void RoboDesktopOpenWindow(RoboDesktop* desktop, RoboWindow* window, int32_t x, int32_t y, int32_t width, int32_t height) {
    if (window == nullptr || window->pixels == nullptr || width <= 0 || height <= 0) {
        return;
    }
    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
    RoboWindowFill(window, 0, 0, width, height, ROBO_WINDOW_PAPER);
    RoboWindowFill(window, 0, 0, width, ROBO_WINDOW_TITLE_HEIGHT, ROBO_WINDOW_TITLE);
    RoboWindowFill(window, 0, 0, width, 1, ROBO_WINDOW_EDGE);
    RoboWindowFill(window, 0, height - 1, width, 1, ROBO_WINDOW_EDGE);
    RoboWindowFill(window, 0, 0, 1, height, ROBO_WINDOW_EDGE);
    RoboWindowFill(window, width - 1, 0, 1, height, ROBO_WINDOW_EDGE);
    RoboDesktopPresentWindow(desktop, window, 0, 0, width, height);
}
