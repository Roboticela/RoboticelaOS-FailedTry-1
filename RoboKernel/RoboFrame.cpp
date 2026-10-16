#include "RoboFrame.hpp"

#include "RoboDesktop.hpp"
#include "RoboGPUVendor.hpp"
#include "RoboInput.hpp"
#include "RoboSchedule.hpp"
#include "RoboShell.hpp"

namespace {

constexpr uint32_t ROBO_FRAME_MAX = 4u;
constexpr int32_t ROBO_FRAME_MAX_W = 320;
constexpr int32_t ROBO_FRAME_MAX_H = 220;
constexpr int32_t ROBO_FRAME_TITLE = 28;
constexpr int32_t ROBO_FRAME_MIN_W = 96;
constexpr int32_t ROBO_FRAME_MIN_H = 72;
constexpr uint32_t ROBO_FRAME_PAPER = 0xF3F0E8u;
constexpr uint32_t ROBO_FRAME_TITLE_ON = 0x3A5070u;
constexpr uint32_t ROBO_FRAME_TITLE_OFF = 0x243044u;
constexpr uint32_t ROBO_FRAME_EDGE = 0x1A2230u;
constexpr uint32_t ROBO_FRAME_CLOSE = 0xC45050u;
constexpr uint32_t ROBO_FRAME_GRIP = 0x8A8174u;
constexpr uint8_t ROBO_GESTURE_NONE = 0;
constexpr uint8_t ROBO_GESTURE_DRAG = 1;
constexpr uint8_t ROBO_GESTURE_RESIZE = 2;
constexpr uint8_t ROBO_GESTURE_CLOSE = 3;

struct RoboFrame {
    uint8_t used;
    uint8_t app;
    uint32_t thread;
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    int32_t z;
    uint32_t pixels[ROBO_FRAME_MAX_W * ROBO_FRAME_MAX_H];
};

RoboDesktop* gDesktop = nullptr;
RoboFrame gFrames[ROBO_FRAME_MAX]{};
uint8_t gOrder[ROBO_FRAME_MAX]{};
uint32_t gOrderCount = 0;
int32_t gNextZ = 1;
uint8_t gGesture = ROBO_GESTURE_NONE;
uint32_t gGestureFrame = 0;
int32_t gGrabX = 0;
int32_t gGrabY = 0;
uint8_t gRgba[ROBO_FRAME_MAX_W * ROBO_FRAME_MAX_H * 4u];

int32_t RoboFrameFind(uint32_t thread) {
    for (uint32_t index = 0; index < ROBO_FRAME_MAX; ++index) {
        if (gFrames[index].used && gFrames[index].thread == thread) {
            return static_cast<int32_t>(index);
        }
    }
    return -1;
}

int32_t RoboFrameTop(void) {
    if (gOrderCount == 0) {
        return -1;
    }
    return static_cast<int32_t>(gOrder[gOrderCount - 1u]);
}

void RoboFrameFill(RoboFrame* frame, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t rgb) {
    if (x < 0) {
        width += x;
        x = 0;
    }
    if (y < 0) {
        height += y;
        y = 0;
    }
    if (width <= 0 || height <= 0 || x >= frame->width || y >= frame->height) {
        return;
    }
    if (x + width > frame->width) {
        width = frame->width - x;
    }
    if (y + height > frame->height) {
        height = frame->height - y;
    }
    for (int32_t row = 0; row < height; ++row) {
        uint32_t* pixel = frame->pixels + static_cast<int64_t>(y + row) * ROBO_FRAME_MAX_W + x;
        for (int32_t column = 0; column < width; ++column) {
            pixel[column] = rgb;
        }
    }
}

void RoboFrameChrome(RoboFrame* frame, uint8_t focused, uint8_t clearClient) {
    if (clearClient) {
        RoboFrameFill(frame, 0, 0, frame->width, frame->height, ROBO_FRAME_PAPER);
    }
    RoboFrameFill(frame, 0, 0, frame->width, ROBO_FRAME_TITLE, focused ? ROBO_FRAME_TITLE_ON : ROBO_FRAME_TITLE_OFF);
    RoboFrameFill(frame, 0, 0, frame->width, 1, ROBO_FRAME_EDGE);
    RoboFrameFill(frame, 0, frame->height - 1, frame->width, 1, ROBO_FRAME_EDGE);
    RoboFrameFill(frame, 0, 0, 1, frame->height, ROBO_FRAME_EDGE);
    RoboFrameFill(frame, frame->width - 1, 0, 1, frame->height, ROBO_FRAME_EDGE);
    RoboFrameFill(frame, frame->width - 22, 6, 16, 16, ROBO_FRAME_CLOSE);
    RoboFrameFill(frame, frame->width - 16, frame->height - 16, 14, 14, ROBO_FRAME_GRIP);
}

void RoboFrameFence(void) {
#if defined(__aarch64__)
    __asm__ volatile("dsb sy" ::: "memory");
#else
    __asm__ volatile("sfence" ::: "memory");
#endif
}

bool RoboFrameCovers(uint32_t frameIndex, int32_t screenX, int32_t screenY) {
    int32_t top = -1;
    for (uint32_t order = 0; order < gOrderCount; ++order) {
        RoboFrame* frame = &gFrames[gOrder[order]];
        if (screenX < frame->x || screenY < frame->y || screenX >= frame->x + frame->width || screenY >= frame->y + frame->height) {
            continue;
        }
        top = static_cast<int32_t>(gOrder[order]);
    }
    return top == static_cast<int32_t>(frameIndex);
}

void RoboFramePresentSpan(RoboFrame* frame, int32_t localX, int32_t localY, int32_t width, int32_t height) {
    if (width <= 0 || height <= 0 || width > ROBO_FRAME_MAX_W || height > ROBO_FRAME_MAX_H) {
        return;
    }
    for (int32_t row = 0; row < height; ++row) {
        const uint32_t* source = frame->pixels + static_cast<int64_t>(localY + row) * ROBO_FRAME_MAX_W + localX;
        uint8_t* dest = gRgba + static_cast<int64_t>(row) * width * 4;
        for (int32_t column = 0; column < width; ++column) {
            uint32_t rgb = source[column];
            dest[column * 4] = static_cast<uint8_t>(rgb >> 16);
            dest[column * 4 + 1] = static_cast<uint8_t>(rgb >> 8);
            dest[column * 4 + 2] = static_cast<uint8_t>(rgb);
            dest[column * 4 + 3] = 255;
        }
    }
    int32_t screenX = frame->x + localX;
    int32_t screenY = frame->y + localY;
    if (!RoboGPUVendorPresent(gRgba, width * 4, screenX, screenY, width, height, gDesktop->gpu->width, gDesktop->gpu->height, gDesktop->gpu->pixelFormat)) {
        RoboGPUCopyRect(gDesktop->gpu, screenX, screenY, frame->pixels, ROBO_FRAME_MAX_W, ROBO_FRAME_MAX_H, localX, localY, width, height);
    }
}

void RoboFrameBlit(uint32_t frameIndex, int32_t localX, int32_t localY, int32_t width, int32_t height) {
    if (gDesktop == nullptr || gDesktop->gpu == nullptr || frameIndex >= ROBO_FRAME_MAX) {
        return;
    }
    RoboFrame* frame = &gFrames[frameIndex];
    if (!frame->used || width <= 0 || height <= 0) {
        return;
    }
    if (localX < 0) {
        width += localX;
        localX = 0;
    }
    if (localY < 0) {
        height += localY;
        localY = 0;
    }
    if (localX >= frame->width || localY >= frame->height) {
        return;
    }
    if (localX + width > frame->width) {
        width = frame->width - localX;
    }
    if (localY + height > frame->height) {
        height = frame->height - localY;
    }
    if (RoboFrameTop() == static_cast<int32_t>(frameIndex)) {
        RoboFramePresentSpan(frame, localX, localY, width, height);
        RoboFrameFence();
        return;
    }
    for (int32_t row = 0; row < height; ++row) {
        int32_t column = 0;
        int32_t screenY = frame->y + localY + row;
        while (column < width) {
            if (!RoboFrameCovers(frameIndex, frame->x + localX + column, screenY)) {
                column += 1;
                continue;
            }
            int32_t start = column;
            while (column < width && RoboFrameCovers(frameIndex, frame->x + localX + column, screenY)) {
                column += 1;
            }
            RoboFramePresentSpan(frame, localX + start, localY + row, column - start, 1);
        }
    }
    RoboFrameFence();
}

void RoboFrameRaise(uint32_t frameIndex) {
    uint32_t slot = 0;
    for (; slot < gOrderCount; ++slot) {
        if (gOrder[slot] == frameIndex) {
            break;
        }
    }
    if (slot >= gOrderCount) {
        return;
    }
    for (uint32_t index = slot + 1u; index < gOrderCount; ++index) {
        gOrder[index - 1u] = gOrder[index];
    }
    gOrder[gOrderCount - 1u] = static_cast<uint8_t>(frameIndex);
    gFrames[frameIndex].z = gNextZ;
    gNextZ += 1;
    for (uint32_t index = 0; index < gOrderCount; ++index) {
        RoboFrameChrome(&gFrames[gOrder[index]], index + 1u == gOrderCount ? 1u : 0u, 0);
        if (gOrder[index] != frameIndex) {
            RoboFrameBlit(gOrder[index], 0, 0, gFrames[gOrder[index]].width, ROBO_FRAME_TITLE);
        }
    }
    RoboFrameBlit(frameIndex, 0, 0, gFrames[frameIndex].width, gFrames[frameIndex].height);
}

void RoboFrameExpose(int32_t x, int32_t y, int32_t width, int32_t height) {
    if (width <= 0 || height <= 0) {
        return;
    }
    if (gDesktop != nullptr && gDesktop->gpu != nullptr) {
        RoboRect rect;
        rect.x = x;
        rect.y = y;
        rect.width = width;
        rect.height = height;
        rect = RoboRectClip(rect, gDesktop->gpu->width, gDesktop->gpu->height);
        if (rect.width > 0 && rect.height > 0) {
            RoboShellDamage(rect.x, rect.y, rect.width, rect.height);
        }
    }
}

void RoboFrameCloseAt(uint32_t frameIndex) {
    if (frameIndex >= ROBO_FRAME_MAX || !gFrames[frameIndex].used) {
        return;
    }
    RoboFrame* frame = &gFrames[frameIndex];
    int32_t x = frame->x;
    int32_t y = frame->y;
    int32_t width = frame->width;
    int32_t height = frame->height;
    uint32_t thread = frame->thread;
    frame->used = 0;
    uint32_t slot = 0;
    for (; slot < gOrderCount; ++slot) {
        if (gOrder[slot] == frameIndex) {
            break;
        }
    }
    if (slot < gOrderCount) {
        for (uint32_t index = slot + 1u; index < gOrderCount; ++index) {
            gOrder[index - 1u] = gOrder[index];
        }
        gOrderCount -= 1u;
    }
    RoboFrameExpose(x, y, width, height);
    if (gGestureFrame == frameIndex) {
        gGesture = ROBO_GESTURE_NONE;
    }
    if (thread != 0 && thread != RoboThreadCurrent()) {
        RoboThreadStop(thread);
    }
}

bool RoboFrameHit(uint32_t frameIndex, int32_t x, int32_t y, uint8_t* part, int32_t* localX, int32_t* localY) {
    RoboFrame* frame = &gFrames[frameIndex];
    if (x < frame->x || y < frame->y || x >= frame->x + frame->width || y >= frame->y + frame->height) {
        return false;
    }
    int32_t localRight = x - frame->x;
    int32_t localDown = y - frame->y;
    *localX = localRight;
    *localY = localDown;
    if (localDown < ROBO_FRAME_TITLE && localRight >= frame->width - 22 && localRight < frame->width - 6 && localDown >= 6 && localDown < 22) {
        *part = ROBO_GESTURE_CLOSE;
        return true;
    }
    if (localDown < ROBO_FRAME_TITLE) {
        *part = ROBO_GESTURE_DRAG;
        return true;
    }
    if (localRight >= frame->width - 16 && localDown >= frame->height - 16) {
        *part = ROBO_GESTURE_RESIZE;
        return true;
    }
    *part = ROBO_GESTURE_NONE;
    *localX = localRight - 1;
    *localY = localDown - ROBO_FRAME_TITLE;
    return true;
}

void RoboFrameMove(uint32_t frameIndex, int32_t x, int32_t y) {
    RoboFrame* frame = &gFrames[frameIndex];
    int32_t oldX = frame->x;
    int32_t oldY = frame->y;
    if (gDesktop != nullptr && gDesktop->gpu != nullptr) {
        int32_t limitX = gDesktop->gpu->width - 40;
        int32_t limitY = gDesktop->gpu->height - 20;
        if (x > limitX) {
            x = limitX;
        }
        if (y > limitY) {
            y = limitY;
        }
        if (x + frame->width < 40) {
            x = 40 - frame->width;
        }
        if (y < 0) {
            y = 0;
        }
    }
    if (x == oldX && y == oldY) {
        return;
    }
    frame->x = x;
    frame->y = y;
    RoboFrameExpose(oldX, oldY, frame->width, frame->height);
    RoboFrameBlit(frameIndex, 0, 0, frame->width, frame->height);
}

void RoboFrameResize(uint32_t frameIndex, int32_t width, int32_t height) {
    RoboFrame* frame = &gFrames[frameIndex];
    if (width < ROBO_FRAME_MIN_W) {
        width = ROBO_FRAME_MIN_W;
    }
    if (height < ROBO_FRAME_MIN_H) {
        height = ROBO_FRAME_MIN_H;
    }
    if (width > ROBO_FRAME_MAX_W) {
        width = ROBO_FRAME_MAX_W;
    }
    if (height > ROBO_FRAME_MAX_H) {
        height = ROBO_FRAME_MAX_H;
    }
    if (width == frame->width && height == frame->height) {
        return;
    }
    int32_t oldW = frame->width;
    int32_t oldH = frame->height;
    frame->width = width;
    frame->height = height;
    int32_t top = RoboFrameTop();
    RoboFrameChrome(frame, top == static_cast<int32_t>(frameIndex) ? 1u : 0u, 0);
    if (oldW > width || oldH > height) {
        RoboFrameExpose(frame->x, frame->y, oldW, oldH);
    }
    RoboFrameBlit(frameIndex, 0, 0, frame->width, frame->height);
}

}  // namespace

void RoboFrameStart(RoboDesktop* desktop) {
    gDesktop = desktop;
}

uint32_t RoboFrameOpen(uint32_t thread, int32_t x, int32_t y, int32_t width, int32_t height, uint8_t app) {
    if (thread == 0 || RoboFrameFind(thread) >= 0) {
        return 0;
    }
    uint32_t slot = 0;
    while (slot < ROBO_FRAME_MAX && gFrames[slot].used) {
        slot += 1u;
    }
    if (slot >= ROBO_FRAME_MAX || gOrderCount >= ROBO_FRAME_MAX) {
        return 0;
    }
    if (width < ROBO_FRAME_MIN_W) {
        width = ROBO_FRAME_MIN_W;
    }
    if (height < ROBO_FRAME_MIN_H) {
        height = ROBO_FRAME_MIN_H;
    }
    if (width > ROBO_FRAME_MAX_W) {
        width = ROBO_FRAME_MAX_W;
    }
    if (height > ROBO_FRAME_MAX_H) {
        height = ROBO_FRAME_MAX_H;
    }
    RoboFrame* frame = &gFrames[slot];
    frame->used = 1;
    frame->app = app;
    frame->thread = thread;
    frame->x = x;
    frame->y = y;
    frame->width = width;
    frame->height = height;
    frame->z = gNextZ;
    gNextZ += 1;
    gOrder[gOrderCount] = static_cast<uint8_t>(slot);
    gOrderCount += 1u;
    RoboFrameChrome(frame, 1u, 1u);
    for (uint32_t index = 0; index + 1u < gOrderCount; ++index) {
        RoboFrameChrome(&gFrames[gOrder[index]], 0, 0);
        RoboFrameBlit(gOrder[index], 0, 0, gFrames[gOrder[index]].width, ROBO_FRAME_TITLE);
    }
    RoboFrameBlit(slot, 0, 0, width, height);
    return slot + 1u;
}

void RoboFrameRelease(uint32_t thread) {
    int32_t slot = RoboFrameFind(thread);
    if (slot < 0) {
        return;
    }
    uint32_t saved = gFrames[slot].thread;
    gFrames[slot].thread = 0;
    RoboFrameCloseAt(static_cast<uint32_t>(slot));
    (void)saved;
}

void RoboFrameCompose(RoboDesktop* desktop) {
    if (desktop == nullptr || gOrderCount == 0) {
        return;
    }
    RoboDesktopHideCursor(desktop);
    for (uint32_t index = 0; index < gOrderCount; ++index) {
        RoboFrame* frame = &gFrames[gOrder[index]];
        RoboFrameBlit(gOrder[index], 0, 0, frame->width, frame->height);
    }
    RoboDesktopShowCursor(desktop);
}

void RoboFrameComposeDamage(RoboDesktop* desktop, int32_t x, int32_t y, int32_t width, int32_t height) {
    if (desktop == nullptr || gOrderCount == 0 || width <= 0 || height <= 0) {
        return;
    }
    const int32_t damageRight = x + width;
    const int32_t damageBottom = y + height;
    RoboDesktopHideCursor(desktop);
    for (uint32_t index = 0; index < gOrderCount; ++index) {
        RoboFrame* frame = &gFrames[gOrder[index]];
        if (!frame->used) {
            continue;
        }
        const int32_t frameRight = frame->x + frame->width;
        const int32_t frameBottom = frame->y + frame->height;
        if (frameRight <= x || frame->x >= damageRight || frameBottom <= y || frame->y >= damageBottom) {
            continue;
        }
        RoboFrameBlit(gOrder[index], 0, 0, frame->width, frame->height);
    }
    RoboDesktopShowCursor(desktop);
}

bool RoboFramePointer(const RoboInputEvent* event, int32_t x, int32_t y, uint8_t* appClient, int32_t* localX, int32_t* localY) {
    if (appClient != nullptr) {
        *appClient = 0;
    }
    if (event == nullptr) {
        return false;
    }
    bool down = event->code != 0 && event->pressed != 0;
    bool up = event->code != 0 && event->pressed == 0;
    if (gGesture != ROBO_GESTURE_NONE) {
        RoboFrame* frame = &gFrames[gGestureFrame];
        if (gGesture == ROBO_GESTURE_DRAG && frame->used) {
            RoboFrameMove(gGestureFrame, x - gGrabX, y - gGrabY);
        } else if (gGesture == ROBO_GESTURE_RESIZE && frame->used) {
            RoboFrameResize(gGestureFrame, x - frame->x + gGrabX, y - frame->y + gGrabY);
        } else if (gGesture == ROBO_GESTURE_CLOSE && up && frame->used) {
            uint8_t part = 0;
            int32_t hitX = 0;
            int32_t hitY = 0;
            if (RoboFrameHit(gGestureFrame, x, y, &part, &hitX, &hitY) && part == ROBO_GESTURE_CLOSE) {
                RoboFrameCloseAt(gGestureFrame);
            }
        }
        if (up) {
            gGesture = ROBO_GESTURE_NONE;
        }
        return true;
    }
    if (gOrderCount == 0) {
        return false;
    }
    for (uint32_t step = gOrderCount; step > 0; --step) {
        uint32_t frameIndex = gOrder[step - 1u];
        uint8_t part = 0;
        int32_t hitX = 0;
        int32_t hitY = 0;
        if (!RoboFrameHit(frameIndex, x, y, &part, &hitX, &hitY)) {
            continue;
        }
        if (RoboFrameTop() != static_cast<int32_t>(frameIndex) && down) {
            RoboFrameRaise(frameIndex);
        }
        if (down && part == ROBO_GESTURE_DRAG) {
            gGesture = ROBO_GESTURE_DRAG;
            gGestureFrame = frameIndex;
            gGrabX = x - gFrames[frameIndex].x;
            gGrabY = y - gFrames[frameIndex].y;
        } else if (down && part == ROBO_GESTURE_RESIZE) {
            gGesture = ROBO_GESTURE_RESIZE;
            gGestureFrame = frameIndex;
            gGrabX = gFrames[frameIndex].width - (x - gFrames[frameIndex].x);
            gGrabY = gFrames[frameIndex].height - (y - gFrames[frameIndex].y);
        } else if (down && part == ROBO_GESTURE_CLOSE) {
            gGesture = ROBO_GESTURE_CLOSE;
            gGestureFrame = frameIndex;
        } else if (part == ROBO_GESTURE_NONE && gFrames[frameIndex].app != 0 && appClient != nullptr) {
            *appClient = 1;
            if (localX != nullptr) {
                *localX = hitX;
            }
            if (localY != nullptr) {
                *localY = hitY;
            }
        }
        return true;
    }
    return false;
}

bool RoboFrameTakesKeys(void) {
    return RoboFrameTop() >= 0;
}

bool RoboFrameAppFocused(void) {
    int32_t top = RoboFrameTop();
    return top >= 0 && gFrames[top].app != 0;
}

bool RoboFrameAppView(RoboFrameView* view) {
    if (view == nullptr) {
        return false;
    }
    for (uint32_t index = 0; index < ROBO_FRAME_MAX; ++index) {
        RoboFrame* frame = &gFrames[index];
        if (!frame->used || frame->app == 0) {
            continue;
        }
        view->pixels = frame->pixels;
        view->stride = ROBO_FRAME_MAX_W;
        view->originX = 1;
        view->originY = ROBO_FRAME_TITLE;
        view->width = frame->width - 2;
        view->height = frame->height - ROBO_FRAME_TITLE - 1;
        view->screenX = frame->x + 1;
        view->screenY = frame->y + ROBO_FRAME_TITLE;
        return view->width > 0 && view->height > 0;
    }
    return false;
}

void RoboFrameShow(uint32_t thread, int32_t localX, int32_t localY, int32_t width, int32_t height) {
    int32_t slot = RoboFrameFind(thread);
    if (slot < 0) {
        return;
    }
    RoboFrame* frame = &gFrames[slot];
    RoboFrameBlit(static_cast<uint32_t>(slot), localX + 1, localY + ROBO_FRAME_TITLE, width, height);
    (void)frame;
}
