#include "WebPlatform.hpp"
#include "Robo/FontEngine.hpp"
#include "Robo/RoboScene.hpp"

#include <emscripten.h>
#include <emscripten/bind.h>

EM_JS(void, JsSetPageBackground, (int red, int green, int blue), {
    const color = "rgb(" + red + "," + green + "," + blue + ")";
    const root = document.documentElement;
    if (root) {
        root.style.background = color;
    }
    if (document.body) {
        document.body.style.background = color;
    }
});

#include <algorithm>
#include <memory>
#include <string>

void BuildPage(Robo::RoboScene& scene);

namespace {

std::unique_ptr<Robo::WebPlatform> gPlatform;
std::unique_ptr<Robo::FontEngine> gFonts;
Robo::RoboScene gScene;
bool gReady = false;
bool gPresentQueued = false;
int gWidth = 0;
int gHeight = 0;
float gPixelScale = 1.0f;

void Render(int width, int height, float pixelScale);

void FlushPresent(void*) {
    gPresentQueued = false;
    if (gWidth > 0 && gHeight > 0) {
        Render(gWidth, gHeight, gPixelScale);
    }
}

void SchedulePresent() {
    if (gPresentQueued || gWidth <= 0 || gHeight <= 0 || !gScene.NeedsRender()) {
        return;
    }
    gPresentQueued = true;
    emscripten_async_call(FlushPresent, nullptr, 0);
}

bool EnsureApp() {
    if (!gPlatform) {
        gPlatform = std::make_unique<Robo::WebPlatform>();
    }
    if (!gFonts) {
        gFonts = std::make_unique<Robo::FontEngine>();
    }
    if (gReady) {
        return gFonts->IsReady();
    }

    const bool regular = gFonts->LoadFace(Robo::FontFaceId::Regular, gPlatform->LoadAsset("Assets/Fonts/Inter-Regular.ttf"));
    gFonts->LoadFace(Robo::FontFaceId::Medium, gPlatform->LoadAsset("Assets/Fonts/Inter-Medium.ttf"));
    gFonts->LoadFace(Robo::FontFaceId::SemiBold, gPlatform->LoadAsset("Assets/Fonts/Inter-SemiBold.ttf"));
    gFonts->LoadFace(Robo::FontFaceId::Italic, gPlatform->LoadAsset("Assets/Fonts/Inter-Italic.ttf"));
    if (!regular) {
        return false;
    }

    BuildPage(gScene);
    gReady = true;
    return true;
}

void Render(int width, int height, float pixelScale) {
    if (!EnsureApp()) {
        return;
    }
    gWidth = std::max(1, width);
    gHeight = std::max(1, height);
    gPixelScale = pixelScale > 0.0f ? pixelScale : 1.0f;
    gPlatform->SetSurfaceSize(gWidth, gHeight);
    gPlatform->SetPixelScale(gPixelScale);
    gScene.Render(*gPlatform, *gFonts, Robo::Color::FromHex("#FFFFFF"), 28.0f);
    const Robo::Color page = gScene.GetBackground();
    JsSetPageBackground(page.red, page.green, page.blue);
}

bool PointerMove(float x, float y) {
    if (!EnsureApp()) {
        return false;
    }
    const bool changed = gScene.PointerMove(x, y);
    SchedulePresent();
    return changed;
}

bool PointerDown(float x, float y, bool fromTouch, int clicks) {
    if (!EnsureApp()) {
        return false;
    }
    const bool changed = gScene.PointerDown(x, y, fromTouch, clicks);
    SchedulePresent();
    return changed;
}

bool LongPressSelect(float x, float y) {
    if (!EnsureApp()) {
        return false;
    }
    const bool changed = gScene.LongPressSelect(x, y);
    SchedulePresent();
    return changed;
}

bool HasCaret() {
    if (!EnsureApp()) {
        return false;
    }
    return gScene.HasCaret();
}

bool MoveSelection(int command, bool extend) {
    if (!EnsureApp()) {
        return false;
    }
    const bool changed = gScene.MoveSelection(command, extend);
    SchedulePresent();
    return changed;
}

bool PointerUp(float x, float y) {
    if (!EnsureApp()) {
        return false;
    }
    const bool changed = gScene.PointerUp(x, y);
    SchedulePresent();
    return changed;
}

bool ShowOrHideSidebar() {
    if (!EnsureApp()) {
        return false;
    }
    const bool changed = gScene.ShowOrHideSidebar();
    SchedulePresent();
    return changed;
}

void SetModifiers(int modifiers) {
    if (!EnsureApp()) {
        return;
    }
    gScene.SetModifiers(modifiers);
}

bool KeyDown(const std::string& key, bool shift) {
    if (!EnsureApp()) {
        return false;
    }
    const bool changed = gScene.KeyDown(key, shift);
    SchedulePresent();
    return changed;
}

bool KeyUp(const std::string& key) {
    if (!EnsureApp()) {
        return false;
    }
    const bool changed = gScene.KeyUp(key);
    SchedulePresent();
    return changed;
}

bool PointerLeave() {
    if (!EnsureApp()) {
        return false;
    }
    const bool changed = gScene.PointerLeave();
    SchedulePresent();
    return changed;
}

bool HitTest(float x, float y) {
    if (!EnsureApp()) {
        return false;
    }
    return gScene.HitTest(x, y);
}

bool Wheel(float x, float y, float deltaX, float deltaY) {
    if (!EnsureApp()) {
        return false;
    }
    const bool changed = gScene.Wheel(x, y, deltaX, deltaY);
    SchedulePresent();
    return changed;
}

bool ScrollBy(float deltaY) {
    if (!EnsureApp()) {
        return false;
    }
    const bool changed = gScene.ScrollBy(deltaY);
    SchedulePresent();
    return changed;
}

bool ScrollByX(float deltaX) {
    if (!EnsureApp()) {
        return false;
    }
    const bool changed = gScene.ScrollByX(deltaX);
    SchedulePresent();
    return changed;
}

bool ZoomBy(float factor, float anchorX, float anchorY) {
    if (!EnsureApp()) {
        return false;
    }
    const bool changed = gScene.ZoomBy(factor, anchorX, anchorY);
    SchedulePresent();
    return changed;
}

bool ZoomTo(float zoom, float anchorX, float anchorY) {
    if (!EnsureApp()) {
        return false;
    }
    const bool changed = gScene.ZoomTo(zoom, anchorX, anchorY);
    SchedulePresent();
    return changed;
}

float GetZoom() {
    if (!EnsureApp()) {
        return 1.0f;
    }
    return gScene.GetZoom();
}

bool KeyScrollLine(int direction) {
    if (!EnsureApp()) {
        return false;
    }
    const bool changed = gScene.KeyScrollLine(direction);
    SchedulePresent();
    return changed;
}

bool KeyScrollPage(int direction) {
    if (!EnsureApp()) {
        return false;
    }
    const bool changed = gScene.KeyScrollPage(direction);
    SchedulePresent();
    return changed;
}

bool KeyScrollToStart() {
    if (!EnsureApp()) {
        return false;
    }
    const bool changed = gScene.KeyScrollToStart();
    SchedulePresent();
    return changed;
}

bool KeyScrollToEnd() {
    if (!EnsureApp()) {
        return false;
    }
    const bool changed = gScene.KeyScrollToEnd();
    SchedulePresent();
    return changed;
}

bool Tick(float deltaMs) {
    if (!EnsureApp()) {
        return false;
    }
    gScene.Tick(static_cast<double>(deltaMs));
    SchedulePresent();
    return gScene.NeedsTick();
}

std::string HitTestKind(float x, float y) {
    if (!EnsureApp()) {
        return "none";
    }
    switch (gScene.HitTestKind(x, y)) {
        case Robo::HitKind::Button:
        case Robo::HitKind::Theme:
            return "button";
        case Robo::HitKind::Input:
            return "input";
        case Robo::HitKind::Scrollbar:
            return "scrollbar";
        default:
            return "none";
    }
}

bool KeyInsert(const std::string& utf8) {
    if (!EnsureApp()) {
        return false;
    }
    const bool changed = gScene.KeyInsert(utf8);
    SchedulePresent();
    return changed;
}

bool KeyBackspace() {
    if (!EnsureApp()) {
        return false;
    }
    const bool changed = gScene.KeyBackspace();
    SchedulePresent();
    return changed;
}

bool KeyDelete() {
    if (!EnsureApp()) {
        return false;
    }
    const bool changed = gScene.KeyDelete();
    SchedulePresent();
    return changed;
}

bool KeyLeft() {
    if (!EnsureApp()) {
        return false;
    }
    const bool changed = gScene.KeyLeft();
    SchedulePresent();
    return changed;
}

bool KeyRight() {
    if (!EnsureApp()) {
        return false;
    }
    const bool changed = gScene.KeyRight();
    SchedulePresent();
    return changed;
}

bool KeyEscape() {
    if (!EnsureApp()) {
        return false;
    }
    const bool changed = gScene.KeyEscape();
    SchedulePresent();
    return changed;
}

bool SelectableAt(float x, float y) {
    if (!EnsureApp()) {
        return false;
    }
    return gScene.SelectableAt(x, y);
}

bool CopySelection() {
    if (!EnsureApp()) {
        return false;
    }
    return gScene.CopySelection();
}

bool SelectAllText() {
    if (!EnsureApp()) {
        return false;
    }
    const bool changed = gScene.SelectAllText();
    SchedulePresent();
    return changed;
}

bool HasFocusedInput() {
    if (!EnsureApp()) {
        return false;
    }
    return gScene.HasFocusedInput();
}

int ActiveScrollbarAxis() {
    if (!EnsureApp()) {
        return 0;
    }
    return gScene.ActiveScrollbarAxis();
}

bool OwnsScrollGesture(float x, float y) {
    if (!EnsureApp()) {
        return false;
    }
    return gScene.OwnsScrollGesture(x, y);
}

bool SetMenuTransparency(float amount) {
    if (!EnsureApp()) {
        return false;
    }
    gScene.SetMenuTransparency(amount);
    SchedulePresent();
    return true;
}

bool SetBackground(const std::string& path) {
    if (!EnsureApp()) {
        return false;
    }
    gScene.SetDesktopBackgroundPath(path);
    SchedulePresent();
    return true;
}

}  // namespace

EMSCRIPTEN_BINDINGS(RoboModule) {
    emscripten::function("Render", &Render);
    emscripten::function("PointerMove", &PointerMove);
    emscripten::function("PointerDown", &PointerDown);
    emscripten::function("LongPressSelect", &LongPressSelect);
    emscripten::function("HasCaret", &HasCaret);
    emscripten::function("MoveSelection", &MoveSelection);
    emscripten::function("PointerUp", &PointerUp);
    emscripten::function("KeyDown", &KeyDown);
    emscripten::function("SetModifiers", &SetModifiers);
    emscripten::function("ShowOrHideSidebar", &ShowOrHideSidebar);
    emscripten::function("KeyUp", &KeyUp);
    emscripten::function("PointerLeave", &PointerLeave);
    emscripten::function("HitTest", &HitTest);
    emscripten::function("Wheel", &Wheel);
    emscripten::function("ScrollBy", &ScrollBy);
    emscripten::function("ScrollByX", &ScrollByX);
    emscripten::function("ZoomBy", &ZoomBy);
    emscripten::function("ZoomTo", &ZoomTo);
    emscripten::function("GetZoom", &GetZoom);
    emscripten::function("KeyScrollLine", &KeyScrollLine);
    emscripten::function("KeyScrollPage", &KeyScrollPage);
    emscripten::function("KeyScrollToStart", &KeyScrollToStart);
    emscripten::function("KeyScrollToEnd", &KeyScrollToEnd);
    emscripten::function("HitTestKind", &HitTestKind);
    emscripten::function("Tick", &Tick);
    emscripten::function("KeyInsert", &KeyInsert);
    emscripten::function("KeyBackspace", &KeyBackspace);
    emscripten::function("KeyDelete", &KeyDelete);
    emscripten::function("KeyLeft", &KeyLeft);
    emscripten::function("KeyRight", &KeyRight);
    emscripten::function("KeyEscape", &KeyEscape);
    emscripten::function("HasFocusedInput", &HasFocusedInput);
    emscripten::function("SelectableAt", &SelectableAt);
    emscripten::function("CopySelection", &CopySelection);
    emscripten::function("SelectAllText", &SelectAllText);
    emscripten::function("ActiveScrollbarAxis", &ActiveScrollbarAxis);
    emscripten::function("OwnsScrollGesture", &OwnsScrollGesture);
    emscripten::function("SetBackground", &SetBackground);
    emscripten::function("SetMenuTransparency", &SetMenuTransparency);
}
