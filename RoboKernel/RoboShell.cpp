#include "RoboShell.hpp"

#include "RoboDesktop.hpp"
#include "RoboFS.hpp"
#include "RoboGPUVendor.hpp"
#include "RoboPS2Mouse.hpp"
#include "RoboticelaOSPlatform.hpp"
#include "Robo/Color.hpp"
#include "Robo/FontEngine.hpp"
#include "Robo/RoboScene.hpp"

#include <stdint.h>
#include <string.h>

void BuildPage(Robo::RoboScene& scene);

namespace {

Robo::RoboticelaOSPlatform* gPlatform = nullptr;
Robo::FontEngine* gFonts = nullptr;
Robo::RoboScene* gScene = nullptr;
RoboDesktop* gDesktop = nullptr;
bool gShift = false;
bool gCursorMovedDuringPaint = false;
int gClickDuringPaint = 0;

void RoboShellTrackCursor(void* cookie) {
    (void)cookie;
    if (gDesktop == nullptr) {
        return;
    }
    for (int32_t count = 0; count < 32; ++count) {
        int32_t dx = 0;
        int32_t dy = 0;
        uint8_t pressed = 0;
        uint8_t changed = 0;
        if (!RoboPS2MouseReadPacket(&dx, &dy, &pressed, &changed)) {
            return;
        }
        if (dx != 0 || dy != 0) {
            RoboDesktopMoveCursor(gDesktop, dx, dy);
            gCursorMovedDuringPaint = true;
        }
        if (changed != 0) {
            gClickDuringPaint = pressed != 0 ? 1 : 2;
        }
    }
}

bool RoboLoadFace(Robo::FontFaceId face, const char* name) {
    uint8_t* file = static_cast<uint8_t*>(malloc(96u * 1024u));
    if (file == nullptr) {
        return false;
    }
    uint32_t size = 0;
    bool loaded = RoboFsReadFile(name, file, 96u * 1024u, &size) && size > 0;
    if (loaded) {
        std::vector<std::uint8_t> bytes(file, file + size);
        loaded = gFonts->LoadFace(face, bytes);
    }
    free(file);
    return loaded;
}

const char* RoboKeyName(uint16_t code, char* printable) {
    *printable = 0;
    if ((code & 0xE000u) != 0) {
        uint16_t extended = static_cast<uint16_t>(code & 0x7Fu);
        if (extended == 0x48) return "ArrowUp";
        if (extended == 0x50) return "ArrowDown";
        if (extended == 0x4B) return "ArrowLeft";
        if (extended == 0x4D) return "ArrowRight";
        if (extended == 0x47) return "Home";
        if (extended == 0x4F) return "End";
        if (extended == 0x53) return "Delete";
        return nullptr;
    }
    static const char kLower[] = "qwertyuiop";
    static const char kLower2[] = "asdfghjkl";
    static const char kLower3[] = "zxcvbnm";
    if (code >= 0x10 && code <= 0x19) {
        *printable = kLower[code - 0x10];
    } else if (code >= 0x1E && code <= 0x26) {
        *printable = kLower2[code - 0x1E];
    } else if (code >= 0x2C && code <= 0x32) {
        *printable = kLower3[code - 0x2C];
    } else if (code >= 0x02 && code <= 0x0B) {
        *printable = code == 0x0B ? '0' : static_cast<char>('1' + (code - 0x02));
    } else if (code == 0x39) {
        *printable = ' ';
        return " ";
    } else if (code == 0x1C) {
        return "Enter";
    } else if (code == 0x0F) {
        return "Tab";
    } else if (code == 0x01) {
        return "Escape";
    } else if (code == 0x0E) {
        return "Backspace";
    }
    if (*printable != 0 && gShift && *printable >= 'a' && *printable <= 'z') {
        *printable = static_cast<char>(*printable - 'a' + 'A');
    }
    return nullptr;
}

}  // namespace

bool RoboShellKeepCursor(void* cookie, int x, int y, const uint8_t* pixel) {
    return RoboDesktopKeepCursor(static_cast<RoboDesktop*>(cookie), x, y, pixel);
}

bool RoboShellStart(RoboDesktop* desktop) {
    if (desktop == nullptr || desktop->gpu == nullptr || desktop->gpu->base == nullptr) {
        return false;
    }
    gDesktop = desktop;
    gPlatform = new Robo::RoboticelaOSPlatform();
    gFonts = new Robo::FontEngine();
    gScene = new Robo::RoboScene();
    RoboLoadFace(Robo::FontFaceId::Regular, "INTERR.TTF");
    RoboLoadFace(Robo::FontFaceId::Medium, "INTERM.TTF");
    RoboLoadFace(Robo::FontFaceId::SemiBold, "INTERS.TTF");
    RoboLoadFace(Robo::FontFaceId::Italic, "INTERI.TTF");
    gPlatform->SetKeepPixel(RoboShellKeepCursor, desktop);
    gScene->SetPaintPump(RoboShellTrackCursor, nullptr);
    gPlatform->SetSurfaceSize(desktop->gpu->width, desktop->gpu->height);
    gPlatform->SetPixelScale(1.0f);
    gPlatform->SetScanout(desktop->gpu->base, desktop->gpu->width, desktop->gpu->height, desktop->gpu->stride, desktop->gpu->pixelFormat);
    BuildPage(*gScene);
    gScene->Render(*gPlatform, *gFonts, Robo::Color::FromHex("#202838"), 24.0f);
    RoboDesktopShowCursor(desktop);
    return true;
}

void RoboShellCursorCard(void) {
    if (gDesktop == nullptr || gDesktop->gpu == nullptr) {
        return;
    }
    int stride = 0;
    uint8_t* card = RoboGPUVendorCard(&stride);
    if (card == nullptr) {
        return;
    }
    uint8_t* savedBase = gDesktop->gpu->base;
    int32_t savedStride = gDesktop->gpu->stride;
    gDesktop->gpu->base = card;
    gDesktop->gpu->stride = stride;
    RoboDesktopRedrawCursor(gDesktop);
    gDesktop->gpu->base = savedBase;
    gDesktop->gpu->stride = savedStride;
    RoboGPUVendorFlushCursor(gDesktop->cursorX, gDesktop->cursorY);
}

void RoboShellPointerMove(int32_t x, int32_t y) {
    if (gScene != nullptr) {
        gScene->PointerMove(static_cast<float>(x), static_cast<float>(y));
    }
    RoboShellCursorCard();
}

void RoboShellPointerDown(int32_t x, int32_t y, bool fromTouch) {
    if (gScene != nullptr) {
        gScene->PointerDown(static_cast<float>(x), static_cast<float>(y), fromTouch, 1);
    }
}

void RoboShellPointerUp(int32_t x, int32_t y) {
    if (gScene != nullptr) {
        gScene->PointerUp(static_cast<float>(x), static_cast<float>(y));
    }
}

void RoboShellKey(uint16_t code, bool pressed) {
    if (gScene == nullptr) {
        return;
    }
    if (code == 0x2A || code == 0x36) {
        gShift = pressed;
        return;
    }
    char printable = 0;
    const char* name = RoboKeyName(code, &printable);
    if (!pressed) {
        if (name != nullptr) {
            gScene->KeyUp(name);
        }
        return;
    }
    if (name != nullptr && strcmp(name, "Backspace") == 0) {
        gScene->KeyBackspace();
        return;
    }
    if (name != nullptr && strcmp(name, "Delete") == 0) {
        gScene->KeyDelete();
        return;
    }
    if (name != nullptr) {
        if (!gScene->KeyDown(name, gShift) && printable != 0) {
            char text[2] = {printable, 0};
            gScene->KeyInsert(text);
        }
        return;
    }
    if (printable != 0) {
        char text[2] = {printable, 0};
        gScene->KeyInsert(text);
    }
}

bool RoboShellTick(uint32_t deltaMs) {
    if (gScene == nullptr || deltaMs == 0) {
        return false;
    }
    return gScene->Tick(static_cast<double>(deltaMs));
}

void RoboShellSetBackground(const char* path) {
    if (gScene == nullptr || path == nullptr) {
        return;
    }
    gScene->SetDesktopBackgroundPath(path);
}

bool RoboShellFlush(RoboDesktop* desktop) {
    if (gScene == nullptr || gPlatform == nullptr || gFonts == nullptr || desktop == nullptr || !gScene->NeedsRender()) {
        return false;
    }
    for (int pass = 0; pass < 2; ++pass) {
        if (pass > 0 && !gScene->NeedsRender()) {
            break;
        }
        gCursorMovedDuringPaint = false;
        gClickDuringPaint = 0;
        gScene->Render(*gPlatform, *gFonts, Robo::Color::FromHex("#202838"), 24.0f);
        if (gCursorMovedDuringPaint) {
            RoboShellPointerMove(desktop->cursorX, desktop->cursorY);
        }
        if (gClickDuringPaint == 1) {
            RoboShellPointerDown(desktop->cursorX, desktop->cursorY, false);
        } else if (gClickDuringPaint == 2) {
            RoboShellPointerUp(desktop->cursorX, desktop->cursorY);
        }
    }
    RoboDesktopRedrawCursor(desktop);
    RoboShellCursorCard();
    return true;
}
