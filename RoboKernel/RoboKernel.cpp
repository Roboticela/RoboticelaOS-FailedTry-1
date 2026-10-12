#include "RoboArch.hpp"
#include "RoboBootInfo.hpp"
#include "RoboCpu.hpp"
#include "RoboDesktop.hpp"
#include "RoboInput.hpp"
#include "RoboMemory.hpp"
#include "RoboPS2Keyboard.hpp"
#include "RoboPS2Mouse.hpp"
#include "RoboSchedule.hpp"
#include "RoboTimer.hpp"
#include "RoboUsb.hpp"

static bool RoboBootInfoValid(const RoboBootInfo* info) {
    if (info == nullptr || info->frameBufferBase == 0) {
        return false;
    }
    if (info->width == 0 || info->height == 0 || info->width > 16384u || info->height > 16384u) {
        return false;
    }
    if (info->stride < info->width * 4u) {
        return false;
    }
    if (info->pixelFormat != ROBO_PIXEL_BGRX && info->pixelFormat != ROBO_PIXEL_RGBX) {
        return false;
    }
    return true;
}

constexpr int32_t ROBO_APP_WINDOW_X = 64;
constexpr int32_t ROBO_APP_WINDOW_Y = 48;
constexpr int32_t ROBO_APP_WINDOW_WIDTH = 280;
constexpr int32_t ROBO_APP_WINDOW_HEIGHT = 160;
constexpr int32_t ROBO_APP_WIDGET_X = 20;
constexpr int32_t ROBO_APP_WIDGET_Y = 48;
constexpr int32_t ROBO_APP_WIDGET_WIDTH = 88;
constexpr int32_t ROBO_APP_WIDGET_HEIGHT = 36;
constexpr uint32_t ROBO_APP_WIDGET_OFF = 0x3E6D8Eu;
constexpr uint32_t ROBO_APP_WIDGET_ON = 0xD1A15Cu;
constexpr uint64_t ROBO_APP_WIDGET_TICKS = 50;

uint32_t gAppPixels[ROBO_APP_WINDOW_WIDTH * ROBO_APP_WINDOW_HEIGHT];

void RoboAppPaintWidget(RoboWindow* window, bool on) {
    RoboWindowFill(window, ROBO_APP_WIDGET_X, ROBO_APP_WIDGET_Y, ROBO_APP_WIDGET_WIDTH, ROBO_APP_WIDGET_HEIGHT, on ? ROBO_APP_WIDGET_ON : ROBO_APP_WIDGET_OFF);
}

extern "C" void RoboKernelMain(RoboBootInfo* info) {
    RoboArchCli();
    if (!RoboBootInfoValid(info)) {
        RoboHalt();
    }

    RoboMemoryStart(info);
    RoboArchCpuStart();

    RoboGPU gpu;
    RoboGPUBind(&gpu, info);

    RoboDesktop desktop;
    RoboDesktopBoot(&desktop, &gpu);
    RoboWindow appWindow{};
    appWindow.pixels = gAppPixels;
    RoboDesktopOpenWindow(&desktop, &appWindow, ROBO_APP_WINDOW_X, ROBO_APP_WINDOW_Y, ROBO_APP_WINDOW_WIDTH, ROBO_APP_WINDOW_HEIGHT);
    RoboAppPaintWidget(&appWindow, false);
    RoboDesktopPresentWindow(&desktop, &appWindow, ROBO_APP_WIDGET_X, ROBO_APP_WIDGET_Y, ROBO_APP_WIDGET_WIDTH, ROBO_APP_WIDGET_HEIGHT);
    RoboPS2MouseStart();
    RoboArchInterruptsStart();
    RoboPS2MouseArm();
    RoboPS2KeyboardStart();
    RoboUsbStart();
    RoboScheduleStart();
    uint64_t nextWidgetTick = ROBO_APP_WIDGET_TICKS;
    bool widgetOn = false;

    for (;;) {
        RoboArchCli();
        RoboArenaReset(RoboArenaFrame());
        (void)RoboArenaAlloc(RoboArenaFrame(), 32, 8);
        RoboSchedule();
        RoboInputCollect();
        int32_t dx = 0;
        int32_t dy = 0;
        int32_t placeX = 0;
        int32_t placeY = 0;
        bool moved = false;
        bool placed = false;
        bool widgetDue = RoboTimerTicks() >= nextWidgetTick;
        RoboInputEvent event{};
        while (RoboInputPop(&event)) {
            if (event.kind == ROBO_INPUT_MOTION) {
                dx += event.dx;
                dy += event.dy;
                moved = true;
            } else if ((event.kind == ROBO_INPUT_TOUCHPAD || event.kind == ROBO_INPUT_TOUCH) && event.pressed != 0 && event.xMax > 0 && event.yMax > 0) {
                int32_t width = desktop.gpu->width;
                int32_t height = desktop.gpu->height;
                if (event.x < 0) {
                    event.x = 0;
                }
                if (event.y < 0) {
                    event.y = 0;
                }
                if (event.x > event.xMax) {
                    event.x = event.xMax;
                }
                if (event.y > event.yMax) {
                    event.y = event.yMax;
                }
                if (width > 1) {
                    placeX = static_cast<int32_t>((static_cast<int64_t>(event.x) * (width - 1)) / event.xMax);
                }
                if (height > 1) {
                    placeY = static_cast<int32_t>((static_cast<int64_t>(event.y) * (height - 1)) / event.yMax);
                }
                placed = true;
            }
        }
        if (!moved && !placed && !widgetDue) {
            RoboArchIdle();
            continue;
        }
        RoboArchSti();
        if (widgetDue) {
            widgetOn = !widgetOn;
            nextWidgetTick = RoboTimerTicks() + ROBO_APP_WIDGET_TICKS;
            RoboAppPaintWidget(&appWindow, widgetOn);
            RoboDesktopPresentWindow(&desktop, &appWindow, ROBO_APP_WIDGET_X, ROBO_APP_WIDGET_Y, ROBO_APP_WIDGET_WIDTH, ROBO_APP_WIDGET_HEIGHT);
        }
        if (moved) {
            RoboDesktopMoveCursor(&desktop, dx, dy);
        }
        if (placed) {
            RoboDesktopPlaceCursor(&desktop, placeX, placeY);
        }
    }
}
