#pragma once

#include <string>
#include <vector>

namespace Robo {

struct MenuBounds {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    bool Contains(float pointX, float pointY) const;
};

// One title in a top menu bar. A separator is a rule. A submenu shows a chevron.
class TopBarMenuItem {
public:
    void SetLabel(const std::string& label);
    const std::string& GetLabel() const;

    void SetSelected(bool selected);
    bool IsSelected() const;

    void SetDisabled(bool disabled);
    bool IsDisabled() const;

    void SetSeparator(bool separator);
    bool IsSeparator() const;

    void SetSubmenu(bool submenu);
    bool HasSubmenu() const;

    void SetBounds(const MenuBounds& bounds);
    const MenuBounds& GetBounds() const;

private:
    std::string label_;
    bool selected_ = false;
    bool disabled_ = false;
    bool separator_ = false;
    bool submenu_ = false;
    MenuBounds bounds_{};
};

// The menu bar across the top of a desktop. It can also stand alone.
class TopBarMenu {
public:
    void SetApp(const std::string& app);
    const std::string& GetApp() const;

    void SetClock(const std::string& clock);
    const std::string& GetClock() const;

    void SetAppearanceDark(bool dark);
    void ApplyTheme(bool dark);
    bool IsDark() const;

    void SetTransparency(float amount);
    float GetTransparency() const;

    void AddItem(TopBarMenuItem item);
    std::vector<TopBarMenuItem>& Items();
    const std::vector<TopBarMenuItem>& Items() const;

    void SetAppleBounds(const MenuBounds& bounds);
    void SetAppBounds(const MenuBounds& bounds);
    const MenuBounds& AppleBounds() const;
    const MenuBounds& AppBounds() const;

    void SetBar(float x, float y, float width, float height);
    float BarX() const;
    float BarY() const;
    float BarWidth() const;
    float BarHeight() const;

    void SetHover(int index);
    int Hover() const;

    void SetPressed(int index);
    void Press(int index);
    bool ReleasePress(int hit);
    int Pressed() const;

    bool MoveFocus(int direction);
    int Focus() const;
    bool KeyboardFocus() const;

    bool SlotEnabled(int index) const;
    int Hit(float x, float y) const;
    bool Select(int index);
    bool ClearSelection();
    int Active() const;

private:
    int SlotCount() const;

    std::string app_ = "Finder";
    std::string clock_;
    bool dark_ = false;
    bool appearanceLocked_ = false;
    float transparency_ = 0.0f;
    std::vector<TopBarMenuItem> items_;
    MenuBounds apple_{};
    MenuBounds appName_{};
    int active_ = -1;
    int hover_ = -1;
    int pressed_ = -1;
    bool pressCloses_ = false;
    int focus_ = -1;
    bool keyboardFocus_ = false;
    float barX_ = 0.0f;
    float barY_ = 0.0f;
    float barWidth_ = 0.0f;
    float barHeight_ = 0.0f;
};

}  // namespace Robo
