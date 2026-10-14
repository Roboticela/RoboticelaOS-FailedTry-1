#include "RoboShell.hpp"

#include "RoboDesktop.hpp"
#include "RoboFS.hpp"
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
int gCtrl = 0;
int gAlt = 0;
int gMeta = 0;
bool gCursorMovedDuringPaint = false;
int gClickDuringPaint = 0;
uint32_t gNowMs = 0;
uint32_t gLastClickMs = 0;
int32_t gLastClickX = 0;
int32_t gLastClickY = 0;
int gLastClicks = 0;
bool gTouchHeld = false;
bool gLongPressSent = false;
uint32_t gHeldMs = 0;
int32_t gPressX = 0;
int32_t gPressY = 0;
float gSceneScale = 1.0f;

float RoboSceneCoord(int32_t value) {
    return static_cast<float>(value) * gSceneScale;
}

bool RoboShellIsCtrl(uint16_t code) {
    return code == 0x1Du || code == 0xE01Du;
}

bool RoboShellIsAlt(uint16_t code) {
    return code == 0x38u || code == 0xE038u;
}

bool RoboShellIsMeta(uint16_t code) {
    return code == 0xE05Bu || code == 0xE05Cu;
}

void RoboShellHold(int* count, bool pressed) {
    *count += pressed ? 1 : -1;
    if (*count < 0) {
        *count = 0;
    }
}

void RoboShellApplyModifiers(void) {
    if (gScene == nullptr) {
        return;
    }
    int modifiers = 0;
    if (gShift) {
        modifiers |= 1;
    }
    if (gCtrl > 0) {
        modifiers |= 2;
    }
    if (gMeta > 0) {
        modifiers |= 4;
    }
    if (gAlt > 0) {
        modifiers |= 8;
    }
    gScene->SetModifiers(modifiers);
}

int RoboShellClicks(int32_t x, int32_t y) {
    const int32_t dx = x - gLastClickX;
    const int32_t dy = y - gLastClickY;
    const uint32_t elapsed = gNowMs - gLastClickMs;
    int clicks = 1;
    if (gLastClicks == 1 && elapsed < 400u && dx > -8 && dx < 8 && dy > -8 && dy < 8) {
        clicks = 2;
    }
    gLastClickMs = gNowMs;
    gLastClickX = x;
    gLastClickY = y;
    gLastClicks = clicks;
    return clicks;
}

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
    uint8_t* file = static_cast<uint8_t*>(malloc(160u * 1024u));
    if (file == nullptr) {
        return false;
    }
    uint32_t size = 0;
    bool loaded = RoboFsReadFile(name, file, 160u * 1024u, &size) && size > 0;
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
    RoboLoadFace(Robo::FontFaceId::Bold, "INTERB.TTF");
    RoboLoadFace(Robo::FontFaceId::Italic, "INTERI.TTF");
    gPlatform->SetKeepPixel(RoboShellKeepCursor, desktop);
    gScene->SetPaintPump(RoboShellTrackCursor, nullptr);
    const int frameWidth = desktop->gpu->width;
    const int frameHeight = desktop->gpu->height;
    float pixelScale = 1.0f;
    int surfaceWidth = frameWidth;
    int surfaceHeight = frameHeight;
    if (frameHeight >= 1800) {
        pixelScale = 2.0f;
    } else if (frameHeight >= 1400) {
        pixelScale = 1.5f;
    } else if (frameWidth > 0 && frameHeight > 0) {
        pixelScale = 2.0f;
        surfaceWidth = frameWidth * 2;
        surfaceHeight = frameHeight * 2;
    }
    gSceneScale = frameWidth > 0 ? static_cast<float>(surfaceWidth) / static_cast<float>(frameWidth) : 1.0f;
    gPlatform->SetPixelScale(pixelScale);
    gFonts->WarmChrome(pixelScale);
    gPlatform->SetScanout(desktop->gpu->base, desktop->gpu->width, desktop->gpu->height, desktop->gpu->stride, desktop->gpu->pixelFormat);
    gPlatform->SetSurfaceSize(surfaceWidth, surfaceHeight);
    BuildPage(*gScene);
    RoboDesktopUseCursor(desktop);
    RoboDesktopShowCursor(desktop);
    gScene->Render(*gPlatform, *gFonts, Robo::Color::FromHex("#202838"), 24.0f);
    return true;
}

void RoboShellPointerMove(int32_t x, int32_t y) {
    if (gTouchHeld) {
        const int32_t dx = x - gPressX;
        const int32_t dy = y - gPressY;
        if (dx > 12 || dx < -12 || dy > 12 || dy < -12) {
            gLongPressSent = true;
        }
    }
    if (gScene != nullptr) {
        gScene->PointerMove(RoboSceneCoord(x), RoboSceneCoord(y));
    }
}

void RoboShellPointerContext(int32_t x, int32_t y) {
    if (gScene == nullptr) {
        return;
    }
    RoboShellApplyModifiers();
    gScene->PointerContext(RoboSceneCoord(x), RoboSceneCoord(y));
}

void RoboShellPointerDown(int32_t x, int32_t y, bool fromTouch) {
    if (gScene == nullptr) {
        return;
    }
    RoboShellApplyModifiers();
    gScene->PointerDown(RoboSceneCoord(x), RoboSceneCoord(y), fromTouch, RoboShellClicks(x, y));
    if (fromTouch) {
        gTouchHeld = true;
        gLongPressSent = false;
        gHeldMs = 0;
        gPressX = x;
        gPressY = y;
    }
}

void RoboShellPointerUp(int32_t x, int32_t y) {
    gTouchHeld = false;
    if (gScene != nullptr) {
        gScene->PointerUp(RoboSceneCoord(x), RoboSceneCoord(y));
    }
}

void RoboShellKey(uint16_t code, bool pressed) {
    if (gScene == nullptr) {
        return;
    }
    if (code == 0x2A || code == 0x36) {
        gShift = pressed;
        RoboShellApplyModifiers();
        return;
    }
    if (RoboShellIsCtrl(code)) {
        RoboShellHold(&gCtrl, pressed);
        RoboShellApplyModifiers();
        return;
    }
    if (RoboShellIsAlt(code)) {
        RoboShellHold(&gAlt, pressed);
        RoboShellApplyModifiers();
        return;
    }
    if (RoboShellIsMeta(code)) {
        RoboShellHold(&gMeta, pressed);
        RoboShellApplyModifiers();
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
    RoboShellApplyModifiers();
    char keyText[2] = {0, 0};
    const char* downName = name;
    if (downName == nullptr && printable != 0) {
        keyText[0] = printable;
        downName = keyText;
    }
    bool handled = false;
    if (downName != nullptr && strcmp(downName, "Backspace") == 0) {
        handled = gScene->KeyBackspace();
    } else if (downName != nullptr && strcmp(downName, "Delete") == 0) {
        handled = gScene->KeyDelete();
    } else if (downName != nullptr) {
        handled = gScene->KeyDown(downName, gShift);
        if (!handled && strcmp(downName, "Escape") == 0) {
            handled = gScene->KeyEscape();
        } else if (!handled && strcmp(downName, "ArrowLeft") == 0) {
            handled = gScene->KeyLeft();
        } else if (!handled && strcmp(downName, "ArrowRight") == 0) {
            handled = gScene->KeyRight();
        }
    }
    if (!handled && printable != 0 && gCtrl == 0 && gAlt == 0 && gMeta == 0) {
        char text[2] = {printable, 0};
        gScene->KeyInsert(text);
    }
}

bool RoboShellTick(uint32_t deltaMs) {
    if (gScene == nullptr || deltaMs == 0) {
        return false;
    }
    gNowMs += deltaMs;
    bool changed = false;
    if (gTouchHeld && !gLongPressSent) {
        gHeldMs += deltaMs;
        if (gHeldMs >= 500u) {
            gLongPressSent = true;
            changed = gScene->LongPressSelect(RoboSceneCoord(gPressX), RoboSceneCoord(gPressY));
        }
    }
    return gScene->Tick(static_cast<double>(deltaMs)) || changed;
}

void RoboShellSetBackground(const char* path) {
    if (gScene == nullptr || path == nullptr) {
        return;
    }
    gScene->SetDesktopBackgroundPath(path);
}

void RoboShellDamage(int32_t x, int32_t y, int32_t width, int32_t height) {
    if (gScene == nullptr || width <= 0 || height <= 0) {
        return;
    }
    gScene->DamageRect(RoboSceneCoord(x), RoboSceneCoord(y), RoboSceneCoord(width), RoboSceneCoord(height));
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
    return true;
}
