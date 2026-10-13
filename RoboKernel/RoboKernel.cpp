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
#include "RoboDisk.hpp"
#include "RoboFS.hpp"
#include "RoboDesktopImage.hpp"
#include "RoboShell.hpp"

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
uint32_t gWidgetOn = ROBO_APP_WIDGET_ON;
uint8_t gDesktopFile[16u + ROBO_DESKTOP_IMAGE_WIDTH * ROBO_DESKTOP_IMAGE_HEIGHT * 4u];
bool gShellLoaded = false;
bool gShellLive = false;
uint64_t gLastShellTick = 0;

uint32_t RoboLoad32(const uint8_t* bytes) {
    return static_cast<uint32_t>(bytes[0])
        | (static_cast<uint32_t>(bytes[1]) << 8)
        | (static_cast<uint32_t>(bytes[2]) << 16)
        | (static_cast<uint32_t>(bytes[3]) << 24);
}

bool RoboShellLoad(RoboDesktop* desktop) {
    uint32_t fileSize = 0;
    if (!RoboFsReadFile("DESKTOP.BIN", gDesktopFile, sizeof(gDesktopFile), &fileSize) || fileSize < 16u) {
        return false;
    }
    if (RoboLoad32(gDesktopFile) != ROBO_DESKTOP_IMAGE_MAGIC) {
        return false;
    }
    uint32_t width = RoboLoad32(gDesktopFile + 4);
    uint32_t height = RoboLoad32(gDesktopFile + 8);
    uint32_t format = RoboLoad32(gDesktopFile + 12);
    if (width == 0 || height == 0 || width > ROBO_DESKTOP_IMAGE_WIDTH || height > ROBO_DESKTOP_IMAGE_HEIGHT) {
        return false;
    }
    uint64_t need = 16ull + static_cast<uint64_t>(width) * static_cast<uint64_t>(height) * 4ull;
    if (fileSize < need) {
        return false;
    }
    uint8_t* pixels = gDesktopFile + 16;
    if (format != desktop->gpu->pixelFormat) {
        uint32_t count = width * height;
        for (uint32_t index = 0; index < count; ++index) {
            uint8_t* pixel = pixels + static_cast<uint64_t>(index) * 4u;
            uint8_t red = pixel[0];
            pixel[0] = pixel[2];
            pixel[2] = red;
        }
    }
    RoboDesktopShowPicture(desktop, pixels, static_cast<int32_t>(width), static_cast<int32_t>(height));
    return true;
}

void RoboAppLoad(void) {
    uint8_t file[16];
    uint32_t fileSize = 0;
    if (RoboDiskStart()) {
        RoboFsStart(RoboDiskReadSector);
    }
    if (!RoboFsReadFile("ROBOAPP.BIN", file, sizeof(file), &fileSize) || fileSize < 8u) {
        return;
    }
    if (file[0] != 'R' || file[1] != 'O' || file[2] != 'B' || file[3] != 'O') {
        return;
    }
    gWidgetOn = static_cast<uint32_t>(file[4])
        | (static_cast<uint32_t>(file[5]) << 8)
        | (static_cast<uint32_t>(file[6]) << 16)
        | (static_cast<uint32_t>(file[7]) << 24);
}

void RoboAppPaintWidget(RoboWindow* window, bool on) {
    RoboWindowFill(window, ROBO_APP_WIDGET_X, ROBO_APP_WIDGET_Y, ROBO_APP_WIDGET_WIDTH, ROBO_APP_WIDGET_HEIGHT, on ? gWidgetOn : ROBO_APP_WIDGET_OFF);
}

extern "C" void RoboKernelMain(RoboBootInfo* info) {
    RoboArchCli();
    if (!RoboBootInfoValid(info)) {
        RoboHalt();
    }

    RoboFsUseBootFiles(info);
    RoboMemoryStart(info);
    RoboArchCpuStart();
    RoboArchFpuStart();

    RoboGPU gpu;
    RoboGPUBind(&gpu, info);

    RoboDesktop desktop;
    RoboDesktopBoot(&desktop, &gpu);
    RoboAppLoad();
    RoboCpuJoin();
    RoboPS2MouseStart();
    RoboArchInterruptsStart();
    RoboPS2MouseArm();
    RoboPS2KeyboardStart();
    RoboUsbStart();
    RoboScheduleStart();
    gShellLive = RoboShellStart(&desktop);
    gShellLoaded = gShellLive || RoboShellLoad(&desktop);
    RoboWindow appWindow{};
    appWindow.pixels = gAppPixels;
    if (!gShellLoaded) {
        RoboDesktopOpenWindow(&desktop, &appWindow, ROBO_APP_WINDOW_X, ROBO_APP_WINDOW_Y, ROBO_APP_WINDOW_WIDTH, ROBO_APP_WINDOW_HEIGHT);
        RoboAppPaintWidget(&appWindow, false);
        RoboDesktopPresentWindow(&desktop, &appWindow, ROBO_APP_WIDGET_X, ROBO_APP_WIDGET_Y, ROBO_APP_WIDGET_WIDTH, ROBO_APP_WIDGET_HEIGHT);
    }
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
        bool widgetDue = !gShellLoaded && RoboTimerTicks() >= nextWidgetTick;
        bool shellInput = false;
        RoboInputEvent event{};
        while (RoboInputPop(&event)) {
            if (event.kind == ROBO_INPUT_MOTION) {
                if (gShellLive) {
                    if (event.dx != 0 || event.dy != 0) {
                        RoboDesktopMoveCursor(&desktop, event.dx, event.dy);
                        RoboShellPointerMove(desktop.cursorX, desktop.cursorY);
                    }
                    if (event.code != 0) {
                        RoboShellPointerMove(desktop.cursorX, desktop.cursorY);
                        if (event.pressed != 0) {
                            RoboShellPointerDown(desktop.cursorX, desktop.cursorY, false);
                        } else {
                            RoboShellPointerUp(desktop.cursorX, desktop.cursorY);
                        }
                    }
                    shellInput = true;
                } else {
                    dx += event.dx;
                    dy += event.dy;
                    moved = true;
                }
            } else if (event.kind == ROBO_INPUT_KEY && gShellLive) {
                RoboShellKey(event.code, event.pressed != 0);
                shellInput = true;
            } else if ((event.kind == ROBO_INPUT_TOUCHPAD || event.kind == ROBO_INPUT_TOUCH) && event.xMax > 0 && event.yMax > 0) {
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
                if (gShellLive) {
                    RoboDesktopPlaceCursor(&desktop, placeX, placeY);
                    RoboShellPointerMove(desktop.cursorX, desktop.cursorY);
                    if (event.code != 0) {
                        if (event.pressed != 0) {
                            RoboShellPointerDown(desktop.cursorX, desktop.cursorY, true);
                        } else {
                            RoboShellPointerUp(desktop.cursorX, desktop.cursorY);
                        }
                    }
                    shellInput = true;
                } else if (event.pressed != 0) {
                    placed = true;
                }
            }
        }
        if (gShellLive) {
            const uint64_t now = RoboTimerTicks();
            if (gLastShellTick == 0) {
                gLastShellTick = now;
            }
            const uint64_t deltaTicks = now - gLastShellTick;
            if (deltaTicks > 0) {
                gLastShellTick = now;
                uint32_t deltaMs = static_cast<uint32_t>(deltaTicks) * 10u;
                if (deltaMs > 100u) {
                    deltaMs = 100u;
                }
                if (RoboShellTick(deltaMs)) {
                    shellInput = true;
                }
            }
            RoboArchSti();
            if (RoboShellFlush(&desktop)) {
                shellInput = true;
            }
        }
        if (!moved && !placed && !widgetDue && !shellInput) {
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
