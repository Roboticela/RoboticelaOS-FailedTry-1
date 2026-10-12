#include "RoboArch.hpp"
#include "RoboBootInfo.hpp"
#include "RoboDesktop.hpp"
#include "RoboInput.hpp"
#include "RoboPS2Keyboard.hpp"
#include "RoboPS2Mouse.hpp"

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

    RoboGPU gpu;
    RoboGPUBind(&gpu, info);

    RoboDesktop desktop;
    RoboDesktopBoot(&desktop, &gpu);
    RoboPS2MouseStart();
    RoboArchInterruptsStart();
    RoboPS2MouseArm();
    RoboPS2KeyboardStart();

    for (;;) {
        RoboArchCli();
        RoboInputCollect();
        int32_t dx = 0;
        int32_t dy = 0;
        bool moved = false;
        RoboInputEvent event{};
        while (RoboInputPop(&event)) {
            if (event.kind == ROBO_INPUT_MOTION) {
                dx += event.dx;
                dy += event.dy;
                moved = true;
            }
        }
        if (!moved) {
            RoboArchIdle();
            continue;
        }
        RoboArchSti();
        RoboDesktopMoveCursor(&desktop, dx, dy);
    }
}
