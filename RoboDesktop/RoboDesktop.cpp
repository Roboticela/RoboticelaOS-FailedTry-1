#include "RoboDesktop.hpp"
#include "RoboBootInfo.hpp"
#include "RoboGPUVendor.hpp"

namespace {

constexpr int32_t ROBO_CURSOR_WIDTH = 12;
constexpr int32_t ROBO_CURSOR_HEIGHT = 17;
constexpr uint32_t ROBO_DESKTOP_COLOR = 0x202838u;
constexpr uint32_t ROBO_CURSOR_OUTLINE = 0x10141Cu;
constexpr uint32_t ROBO_CURSOR_FILL = 0xF4F7FBu;
constexpr uint8_t ROBO_CURSOR_EMPTY = 0;
constexpr uint8_t ROBO_CURSOR_EDGE = 1;
constexpr uint8_t ROBO_CURSOR_BODY = 2;

constexpr int32_t ROBO_MAX_DESKTOP_WIDTH = 1920;
constexpr int32_t ROBO_MAX_DESKTOP_HEIGHT = 1080;

alignas(4096) uint32_t gDesktopBackbufferStorage[ROBO_MAX_DESKTOP_WIDTH * ROBO_MAX_DESKTOP_HEIGHT];

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

RoboRect RoboCursorBounds(int32_t cursorX, int32_t cursorY) {
    return {cursorX, cursorY, ROBO_CURSOR_WIDTH, ROBO_CURSOR_HEIGHT};
}

int32_t RoboClamp(int32_t value, int32_t limit) {
    if (value < 0) return 0;
    if (limit <= 0) return 0;
    if (value >= limit) return limit - 1;
    return value;
}

void RoboStorePixel(RoboGPU* gpu, int32_t x, int32_t y, uint32_t rgb) {
    if (x < 0 || y < 0 || x >= gpu->width || y >= gpu->height || gpu->base == nullptr) {
        return;
    }
    uint32_t packed = RoboGPUPack(gpu, rgb);
    uint8_t* pixel = gpu->base + static_cast<int64_t>(y) * gpu->stride + static_cast<int64_t>(x) * 4;
    *reinterpret_cast<uint32_t*>(pixel) = packed;
}

void RoboDrawCursor(RoboDesktop* desktop) {
    RoboGPU* gpu = desktop->gpu;
    if (gpu == nullptr) return;
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
            uint32_t color = (sample == ROBO_CURSOR_EDGE) ? ROBO_CURSOR_OUTLINE : ROBO_CURSOR_FILL;
            RoboStorePixel(gpu, desktop->cursorX + x, screenY, color);
        }
    }
}

}  // namespace

void RoboDamageInit(RoboDamageTracker* tracker) {
    if (tracker == nullptr) return;
    tracker->count = 0;
    tracker->fullDamage = false;
    for (int32_t i = 0; i < ROBO_DAMAGE_MAX_RECTS; ++i) {
        tracker->rects[i] = {0, 0, 0, 0};
    }
}

void RoboDamageReset(RoboDamageTracker* tracker) {
    if (tracker == nullptr) return;
    tracker->count = 0;
    tracker->fullDamage = false;
}

void RoboDamageAdd(RoboDamageTracker* tracker, RoboRect rect, int32_t screenWidth, int32_t screenHeight) {
    if (tracker == nullptr) return;
    rect = RoboRectClip(rect, screenWidth, screenHeight);
    if (RoboRectEmpty(rect)) return;
    if (tracker->fullDamage) return;

    for (int32_t i = 0; i < tracker->count; ++i) {
        if (RoboRectIntersects(tracker->rects[i], rect)) {
            tracker->rects[i] = RoboRectUnion(tracker->rects[i], rect);
            return;
        }
    }

    if (tracker->count >= ROBO_DAMAGE_MAX_RECTS) {
        tracker->fullDamage = true;
        return;
    }

    tracker->rects[tracker->count] = rect;
    tracker->count++;

    int64_t totalArea = 0;
    for (int32_t i = 0; i < tracker->count; ++i) {
        totalArea += static_cast<int64_t>(tracker->rects[i].width) * tracker->rects[i].height;
    }
    int64_t screenArea = static_cast<int64_t>(screenWidth) * screenHeight;
    if (totalArea * 2 > screenArea) {
        tracker->fullDamage = true;
    }
}

void RoboDamageMerge(RoboDamageTracker* tracker, int32_t screenWidth, int32_t screenHeight) {
    if (tracker == nullptr || tracker->count <= 1 || tracker->fullDamage) return;
    bool merged = true;
    while (merged) {
        merged = false;
        for (int32_t i = 0; i < tracker->count; ++i) {
            for (int32_t j = i + 1; j < tracker->count; ++j) {
                if (RoboRectIntersects(tracker->rects[i], tracker->rects[j])) {
                    tracker->rects[i] = RoboRectUnion(tracker->rects[i], tracker->rects[j]);
                    tracker->rects[j] = tracker->rects[tracker->count - 1];
                    tracker->count--;
                    merged = true;
                    break;
                }
            }
            if (merged) break;
        }
    }
    (void)screenWidth;
    (void)screenHeight;
}

void RoboWindowInvalidate(RoboWindow* window, RoboRect localRect) {
    if (window == nullptr) return;
    localRect = RoboRectClip(localRect, window->width, window->height);
    if (RoboRectEmpty(localRect)) return;
    if (window->needsRepaint) {
        window->damage = RoboRectUnion(window->damage, localRect);
    } else {
        window->damage = localRect;
        window->needsRepaint = true;
    }
}

void RoboWindowInvalidateAll(RoboWindow* window) {
    if (window == nullptr) return;
    window->damage = {0, 0, window->width, window->height};
    window->needsRepaint = true;
}

void RoboDesktopSnapshotScene(RoboDesktop* desktop, RoboSceneSnapshot* snapshot) {
    if (desktop == nullptr || snapshot == nullptr) return;
    snapshot->background = desktop->background;
    snapshot->picture = desktop->picture;
    snapshot->pictureWidth = desktop->pictureWidth;
    snapshot->pictureHeight = desktop->pictureHeight;
    snapshot->windowCount = desktop->windowCount;
    if (snapshot->windowCount > ROBO_DESKTOP_MAX_WINDOWS) {
        snapshot->windowCount = ROBO_DESKTOP_MAX_WINDOWS;
    }
    for (int32_t i = 0; i < snapshot->windowCount; ++i) {
        const RoboWindow* win = desktop->windows[i];
        if (win != nullptr) {
            snapshot->windows[i].bounds = {win->x, win->y, win->width, win->height};
            snapshot->windows[i].pixels = win->pixels;
            snapshot->windows[i].width = win->width;
            snapshot->windows[i].height = win->height;
            snapshot->windows[i].opacity = win->opacity;
            snapshot->windows[i].hasAlpha = win->hasAlpha;
        } else {
            snapshot->windows[i].bounds = {0, 0, 0, 0};
            snapshot->windows[i].pixels = nullptr;
            snapshot->windows[i].width = 0;
            snapshot->windows[i].height = 0;
            snapshot->windows[i].opacity = 255;
            snapshot->windows[i].hasAlpha = 0;
        }
    }
}

static void RoboDesktopRenderTile(RoboDesktop* desktop, const RoboSceneSnapshot* scene, RoboRect tile) {
    RoboGPU* gpu = desktop->gpu;
    if (gpu == nullptr || desktop->backbuffer == nullptr) return;
    tile = RoboRectClip(tile, gpu->width, gpu->height);
    if (RoboRectEmpty(tile)) return;

    // 1. Clear tile to background or picture
    for (int32_t row = 0; row < tile.height; ++row) {
        int32_t y = tile.y + row;
        uint32_t* dstRow = desktop->backbuffer + static_cast<int64_t>(y) * gpu->width + tile.x;
        if (scene->picture != nullptr && scene->pictureWidth > 0 && scene->pictureHeight > 0) {
            for (int32_t col = 0; col < tile.width; ++col) {
                int32_t x = tile.x + col;
                if (x < scene->pictureWidth && y < scene->pictureHeight) {
                    const uint8_t* picPixel = scene->picture + (static_cast<int64_t>(y) * scene->pictureWidth + x) * 4;
                    dstRow[col] = (static_cast<uint32_t>(picPixel[0]) << 16) | (static_cast<uint32_t>(picPixel[1]) << 8) | picPixel[2];
                } else {
                    dstRow[col] = scene->background;
                }
            }
        } else {
            for (int32_t col = 0; col < tile.width; ++col) {
                dstRow[col] = scene->background;
            }
        }
    }

    // 2. Composite intersecting windows in bottom-to-top z-order
    for (int32_t i = 0; i < scene->windowCount; ++i) {
        const RoboWindowSnapshot* win = &scene->windows[i];
        if (win->pixels == nullptr || !RoboRectIntersects(tile, win->bounds)) {
            continue;
        }
        RoboRect clipped = RoboRectClipBounds(tile, win->bounds);
        if (RoboRectEmpty(clipped)) continue;

        for (int32_t row = 0; row < clipped.height; ++row) {
            int32_t screenY = clipped.y + row;
            int32_t winY = screenY - win->bounds.y;
            uint32_t* dstRow = desktop->backbuffer + static_cast<int64_t>(screenY) * gpu->width + clipped.x;
            const uint32_t* srcRow = win->pixels + static_cast<int64_t>(winY) * win->width + (clipped.x - win->bounds.x);

            if (win->hasAlpha) {
                for (int32_t col = 0; col < clipped.width; ++col) {
                    uint32_t srcColor = srcRow[col];
                    uint32_t dstColor = dstRow[col];
                    RoboPixel srcPix{
                        static_cast<uint8_t>(srcColor >> 16),
                        static_cast<uint8_t>(srcColor >> 8),
                        static_cast<uint8_t>(srcColor),
                        static_cast<uint8_t>(srcColor >> 24)
                    };
                    RoboPixel dstPix{
                        static_cast<uint8_t>(dstColor >> 16),
                        static_cast<uint8_t>(dstColor >> 8),
                        static_cast<uint8_t>(dstColor),
                        255
                    };
                    RoboPixel blended = RoboBlendOver(srcPix, dstPix);
                    dstRow[col] = (static_cast<uint32_t>(blended.r) << 16) |
                                  (static_cast<uint32_t>(blended.g) << 8) |
                                  static_cast<uint32_t>(blended.b);
                }
            } else {
                for (int32_t col = 0; col < clipped.width; ++col) {
                    dstRow[col] = srcRow[col];
                }
            }
        }
    }
}

void RoboDesktopRenderDamage(RoboDesktop* desktop) {
    if (desktop == nullptr || desktop->gpu == nullptr || desktop->backbuffer == nullptr) return;
    RoboSceneSnapshot snapshot{};
    RoboDesktopSnapshotScene(desktop, &snapshot);

    if (desktop->damage.fullDamage) {
        int32_t tilesX = (desktop->gpu->width + ROBO_TILE_SIZE - 1) / ROBO_TILE_SIZE;
        int32_t tilesY = (desktop->gpu->height + ROBO_TILE_SIZE - 1) / ROBO_TILE_SIZE;
        for (int32_t ty = 0; ty < tilesY; ++ty) {
            for (int32_t tx = 0; tx < tilesX; ++tx) {
                RoboRect tile{tx * ROBO_TILE_SIZE, ty * ROBO_TILE_SIZE, ROBO_TILE_SIZE, ROBO_TILE_SIZE};
                RoboDesktopRenderTile(desktop, &snapshot, tile);
            }
        }
        return;
    }

    RoboDamageMerge(&desktop->damage, desktop->gpu->width, desktop->gpu->height);
    for (int32_t i = 0; i < desktop->damage.count; ++i) {
        RoboRect r = desktop->damage.rects[i];
        int32_t x0 = (r.x / ROBO_TILE_SIZE) * ROBO_TILE_SIZE;
        int32_t y0 = (r.y / ROBO_TILE_SIZE) * ROBO_TILE_SIZE;
        int32_t x1 = ((r.x + r.width + ROBO_TILE_SIZE - 1) / ROBO_TILE_SIZE) * ROBO_TILE_SIZE;
        int32_t y1 = ((r.y + r.height + ROBO_TILE_SIZE - 1) / ROBO_TILE_SIZE) * ROBO_TILE_SIZE;
        for (int32_t ty = y0; ty < y1; ty += ROBO_TILE_SIZE) {
            for (int32_t tx = x0; tx < x1; tx += ROBO_TILE_SIZE) {
                RoboRect tile{tx, ty, ROBO_TILE_SIZE, ROBO_TILE_SIZE};
                RoboDesktopRenderTile(desktop, &snapshot, tile);
            }
        }
    }
}

void RoboDesktopPresentDamage(RoboDesktop* desktop) {
    if (desktop == nullptr || desktop->gpu == nullptr || desktop->backbuffer == nullptr) return;
    RoboGPU* gpu = desktop->gpu;

    if (desktop->damage.fullDamage) {
        RoboRect screen{0, 0, gpu->width, gpu->height};
        RoboGPUPresentDamage(gpu, reinterpret_cast<const uint8_t*>(desktop->backbuffer), gpu->width * 4, screen);
        RoboDamageReset(&desktop->damage);
        RoboGPUFlush(gpu);
        return;
    }

    for (int32_t i = 0; i < desktop->damage.count; ++i) {
        RoboRect r = desktop->damage.rects[i];
        RoboGPUPresentDamage(gpu, reinterpret_cast<const uint8_t*>(desktop->backbuffer), gpu->width * 4, r);
    }
    RoboDamageReset(&desktop->damage);
    RoboGPUFlush(gpu);
}

void RoboDesktopBoot(RoboDesktop* desktop, RoboGPU* gpu) {
    desktop->gpu = gpu;
    desktop->backbuffer = gDesktopBackbufferStorage;
    gpu->backbuffer = reinterpret_cast<uint8_t*>(gDesktopBackbufferStorage);
    gpu->backbufferStride = gpu->width * 4;
    desktop->background = ROBO_DESKTOP_COLOR;
    desktop->cursorX = gpu->width / 2;
    desktop->cursorY = gpu->height / 2;
    desktop->picture = nullptr;
    desktop->pictureWidth = 0;
    desktop->pictureHeight = 0;
    desktop->cursorHeld = 0;
    desktop->windowCount = 0;
    RoboDamageInit(&desktop->damage);

    // Initial fill into RAM backbuffer
    int32_t totalPixels = gpu->width * gpu->height;
    for (int32_t i = 0; i < totalPixels; ++i) {
        desktop->backbuffer[i] = desktop->background;
    }

    // Physical presentation via GPU
    RoboGPUFill(gpu, desktop->background);
    RoboDrawCursor(desktop);
}

void RoboDesktopRestoreRect(RoboDesktop* desktop, RoboRect rect) {
    if (desktop == nullptr || desktop->gpu == nullptr) return;
    RoboDamageAdd(&desktop->damage, rect, desktop->gpu->width, desktop->gpu->height);
    RoboDesktopRenderDamage(desktop);
    RoboDesktopPresentDamage(desktop);
}

void RoboCursorRead(RoboDesktop* desktop, int32_t originX, int32_t originY) {
    RoboGPU* gpu = desktop->gpu;
    if (gpu == nullptr || gpu->base == nullptr) return;
    for (int32_t y = 0; y < 17; ++y) {
        for (int32_t x = 0; x < 12; ++x) {
            uint8_t* saved = desktop->cursorSave + (static_cast<int64_t>(y) * 12 + x) * 4;
            int32_t screenX = originX + x;
            int32_t screenY = originY + y;
            if (screenX < 0 || screenY < 0 || screenX >= gpu->width || screenY >= gpu->height) {
                saved[0] = saved[1] = saved[2] = saved[3] = 0;
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
    if (gpu == nullptr || gpu->base == nullptr) return;
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

RoboDesktop* gCursorDesktop = nullptr;

void RoboDesktopUseCursor(RoboDesktop* desktop) {
    gCursorDesktop = desktop;
}

bool RoboDesktopCursorRect(int32_t* x, int32_t* y, int32_t* width, int32_t* height) {
    if (gCursorDesktop == nullptr || gCursorDesktop->cursorHeld == 0 || x == nullptr || y == nullptr || width == nullptr || height == nullptr) {
        return false;
    }
    *x = gCursorDesktop->cursorX;
    *y = gCursorDesktop->cursorY;
    *width = ROBO_CURSOR_WIDTH;
    *height = ROBO_CURSOR_HEIGHT;
    return true;
}

bool RoboDesktopCursorSample(int32_t localX, int32_t localY, uint32_t* rgb) {
    if (rgb == nullptr || localX < 0 || localY < 0 || localX >= ROBO_CURSOR_WIDTH || localY >= ROBO_CURSOR_HEIGHT) {
        return false;
    }
    uint8_t sample = ROBO_CURSOR_PIXELS[localY * ROBO_CURSOR_WIDTH + localX];
    if (sample == ROBO_CURSOR_EMPTY) {
        return false;
    }
    *rgb = sample == ROBO_CURSOR_EDGE ? ROBO_CURSOR_OUTLINE : ROBO_CURSOR_FILL;
    return true;
}

void RoboDesktopRememberCursor(int32_t localX, int32_t localY, uint32_t packed) {
    if (gCursorDesktop == nullptr || localX < 0 || localY < 0 || localX >= ROBO_CURSOR_WIDTH || localY >= ROBO_CURSOR_HEIGHT) {
        return;
    }
    uint8_t* saved = gCursorDesktop->cursorSave + (static_cast<int64_t>(localY) * ROBO_CURSOR_WIDTH + localX) * 4;
    saved[0] = static_cast<uint8_t>(packed);
    saved[1] = static_cast<uint8_t>(packed >> 8);
    saved[2] = static_cast<uint8_t>(packed >> 16);
    saved[3] = 0;
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
    return ROBO_CURSOR_PIXELS[localY * ROBO_CURSOR_WIDTH + localX] != ROBO_CURSOR_EMPTY;
}

void RoboDesktopShowPicture(RoboDesktop* desktop, const uint8_t* pixels, int32_t width, int32_t height) {
    if (desktop == nullptr || desktop->gpu == nullptr || pixels == nullptr || width <= 0 || height <= 0) {
        return;
    }
    desktop->picture = pixels;
    desktop->pictureWidth = width;
    desktop->pictureHeight = height;
    RoboRect screen{0, 0, desktop->gpu->width, desktop->gpu->height};
    RoboDamageAdd(&desktop->damage, screen, desktop->gpu->width, desktop->gpu->height);
    RoboDesktopRenderDamage(desktop);
    RoboDesktopPresentDamage(desktop);
    RoboDrawCursor(desktop);
}

void RoboDesktopPaintCursor(RoboDesktop* desktop, int32_t previousX, int32_t previousY) {
    if (desktop->cursorX == previousX && desktop->cursorY == previousY) {
        return;
    }
    RoboGPU* gpu = desktop->gpu;
    if (gpu == nullptr) return;

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
        RoboDesktopRestoreRect(desktop, oldRect);
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
        RoboGPUFlushCursor(gpu, desktop->cursorX, desktop->cursorY);
        return;
    }

    if (RoboRectIntersects(oldRect, newRect)) {
        RoboDesktopRestoreRect(desktop, RoboRectUnion(oldRect, newRect));
    } else {
        RoboDesktopRestoreRect(desktop, oldRect);
    }
    RoboDrawCursor(desktop);
    RoboGPUFlushCursor(gpu, desktop->cursorX, desktop->cursorY);
}

void RoboDesktopMoveCursor(RoboDesktop* desktop, int32_t dx, int32_t dy) {
    if (dx == 0 && dy == 0 || desktop == nullptr || desktop->gpu == nullptr) {
        return;
    }
    int32_t previousX = desktop->cursorX;
    int32_t previousY = desktop->cursorY;
    desktop->cursorX = RoboClamp(desktop->cursorX + dx, desktop->gpu->width);
    desktop->cursorY = RoboClamp(desktop->cursorY + dy, desktop->gpu->height);
    RoboDesktopPaintCursor(desktop, previousX, previousY);
}

void RoboDesktopPlaceCursor(RoboDesktop* desktop, int32_t x, int32_t y) {
    if (desktop == nullptr || desktop->gpu == nullptr) return;
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
    RoboRect r = RoboRectClip({x, y, width, height}, window->width, window->height);
    if (RoboRectEmpty(r)) return;

    for (int32_t row = 0; row < r.height; ++row) {
        uint32_t* pixel = window->pixels + static_cast<int64_t>(r.y + row) * window->width + r.x;
        for (int32_t col = 0; col < r.width; ++col) {
            pixel[col] = rgb;
        }
    }
    RoboWindowInvalidate(window, r);
}

void RoboDesktopMoveWindow(RoboDesktop* desktop, RoboWindow* window, int32_t newX, int32_t newY) {
    if (desktop == nullptr || window == nullptr || desktop->gpu == nullptr) return;
    if (window->x == newX && window->y == newY) return;

    // Rule from Section 5:
    // 1. Record old screen-space bounds
    RoboRect oldBounds{window->x, window->y, window->width, window->height};
    // 2. Update position
    window->x = newX;
    window->y = newY;
    // 3. Record new screen-space bounds
    RoboRect newBounds{window->x, window->y, window->width, window->height};
    // 4. Add BOTH rectangles to desktop damage
    RoboDamageAdd(&desktop->damage, oldBounds, desktop->gpu->width, desktop->gpu->height);
    RoboDamageAdd(&desktop->damage, newBounds, desktop->gpu->width, desktop->gpu->height);
    // 5. Recompose and present
    RoboDesktopRenderDamage(desktop);
    RoboDesktopPresentDamage(desktop);
}

void RoboDesktopResizeWindow(RoboDesktop* desktop, RoboWindow* window, int32_t newWidth, int32_t newHeight) {
    if (desktop == nullptr || window == nullptr || desktop->gpu == nullptr) return;
    if (newWidth <= 0 || newHeight <= 0) return;
    RoboRect oldBounds{window->x, window->y, window->width, window->height};
    window->width = newWidth;
    window->height = newHeight;
    RoboRect newBounds{window->x, window->y, window->width, window->height};
    RoboDamageAdd(&desktop->damage, oldBounds, desktop->gpu->width, desktop->gpu->height);
    RoboDamageAdd(&desktop->damage, newBounds, desktop->gpu->width, desktop->gpu->height);
    RoboWindowInvalidateAll(window);
    RoboDesktopRenderDamage(desktop);
    RoboDesktopPresentDamage(desktop);
}

void RoboDesktopCloseWindow(RoboDesktop* desktop, RoboWindow* window) {
    if (desktop == nullptr || window == nullptr || desktop->gpu == nullptr) return;
    RoboRect oldBounds{window->x, window->y, window->width, window->height};
    for (int32_t i = 0; i < desktop->windowCount; ++i) {
        if (desktop->windows[i] == window) {
            for (int32_t j = i; j + 1 < desktop->windowCount; ++j) {
                desktop->windows[j] = desktop->windows[j + 1];
            }
            desktop->windowCount--;
            break;
        }
    }
    RoboDamageAdd(&desktop->damage, oldBounds, desktop->gpu->width, desktop->gpu->height);
    RoboDesktopRenderDamage(desktop);
    RoboDesktopPresentDamage(desktop);
}

void RoboDesktopPresentWindow(RoboDesktop* desktop, RoboWindow* window, int32_t localX, int32_t localY, int32_t width, int32_t height) {
    if (desktop == nullptr || desktop->gpu == nullptr || window == nullptr || window->pixels == nullptr) {
        return;
    }
    if (width <= 0 || height <= 0) return;

    RoboRect localRect = RoboRectClip({localX, localY, width, height}, window->width, window->height);
    if (RoboRectEmpty(localRect)) return;

    // Convert local window damage to screen-space damage
    RoboRect screenDamage{window->x + localRect.x, window->y + localRect.y, localRect.width, localRect.height};
    RoboDamageAdd(&desktop->damage, screenDamage, desktop->gpu->width, desktop->gpu->height);
    RoboDesktopRenderDamage(desktop);
    RoboDesktopPresentDamage(desktop);

    RoboRect cursor = RoboRectClip(RoboCursorBounds(desktop->cursorX, desktop->cursorY), desktop->gpu->width, desktop->gpu->height);
    if (RoboRectIntersects(screenDamage, cursor)) {
        RoboDrawCursor(desktop);
    }
}

void RoboDesktopOpenWindow(RoboDesktop* desktop, RoboWindow* window, int32_t x, int32_t y, int32_t width, int32_t height) {
    if (desktop == nullptr || window == nullptr || window->pixels == nullptr || width <= 0 || height <= 0) {
        return;
    }
    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;
    window->opacity = 255;
    window->hasAlpha = 0;
    window->needsRepaint = true;
    window->damage = {0, 0, width, height};

    if (desktop->windowCount < ROBO_DESKTOP_MAX_WINDOWS) {
        desktop->windows[desktop->windowCount++] = window;
    }

    RoboWindowFill(window, 0, 0, width, height, ROBO_WINDOW_PAPER);
    RoboWindowFill(window, 0, 0, width, ROBO_WINDOW_TITLE_HEIGHT, ROBO_WINDOW_TITLE);
    RoboWindowFill(window, 0, 0, width, 1, ROBO_WINDOW_EDGE);
    RoboWindowFill(window, 0, height - 1, width, 1, ROBO_WINDOW_EDGE);
    RoboWindowFill(window, 0, 0, 1, height, ROBO_WINDOW_EDGE);
    RoboWindowFill(window, width - 1, 0, 1, height, ROBO_WINDOW_EDGE);

    RoboDesktopPresentWindow(desktop, window, 0, 0, width, height);
}
