#include "RoboArch.hpp"
#include "RoboBootInfo.hpp"
#include "RoboCpu.hpp"
#include "RoboDesktop.hpp"
#include "RoboInput.hpp"
#include "RoboMemory.hpp"
#include "RoboPS2Keyboard.hpp"
#include "RoboPS2Mouse.hpp"
#include "RoboSchedule.hpp"
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
    RoboPS2MouseStart();
    RoboArchInterruptsStart();
    RoboPS2MouseArm();
    RoboPS2KeyboardStart();
    RoboUsbStart();
    RoboScheduleStart();

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
        if (!moved && !placed) {
            RoboArchIdle();
            continue;
        }
        RoboArchSti();
        if (moved) {
            RoboDesktopMoveCursor(&desktop, dx, dy);
        }
        if (placed) {
            RoboDesktopPlaceCursor(&desktop, placeX, placeY);
        }
    }
}
