#pragma once

#include "Background.hpp"
#include "Color.hpp"
#include "Svg.hpp"

#include <string>
#include <vector>

namespace Robo {

struct DockItemBounds {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    bool Contains(float pointX, float pointY) const;
};

struct MenuBounds {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    bool Contains(float pointX, float pointY) const;
};

// One title in the top menu bar. The open title is the active menu.
class TopBarMenuItem {
public:
    void SetLabel(const std::string& label);
    const std::string& GetLabel() const;

    void SetSelected(bool selected);
    bool IsSelected() const;

    void SetBounds(const MenuBounds& bounds);
    const MenuBounds& GetBounds() const;

private:
    std::string label_;
    bool selected_ = false;
    MenuBounds bounds_{};
};

// The menu bar across the top of the desktop, above the wallpaper.
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
    void SetBar(float x, float y, float width, float height);
    float BarX() const;
    float BarY() const;
    float BarWidth() const;
    float BarHeight() const;

    void SetHover(int index);
    int Hover() const;

    int Hit(float x, float y) const;
    bool Select(int index);
    bool ClearSelection();
    int Active() const;

private:
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
    float barX_ = 0.0f;
    float barY_ = 0.0f;
    float barWidth_ = 0.0f;
    float barHeight_ = 0.0f;
};

// One launch tile on the desktop dock.
class DockItem {
public:
    void SetLabel(const std::string& label);
    const std::string& GetLabel() const;

    void SetSelected(bool selected);
    bool IsSelected() const;

    void SetIcon(Svg icon);
    bool HasIcon() const;
    Svg& Icon();
    const Svg& Icon() const;

    void SetBounds(const DockItemBounds& bounds);
    const DockItemBounds& GetBounds() const;

private:
    std::string label_;
    bool selected_ = false;
    bool hasIcon_ = false;
    Svg icon_;
    DockItemBounds bounds_{};
};

// The frame around the running app, above the dock.
class AppShell {
public:
    void SetTitle(const std::string& title);
    const std::string& GetTitle() const;

private:
    std::string title_;
};

// The OS shell: background, app frame, and dock. Content still flows in the scene.
class Desktop {
public:
    void SetEnabled(bool enabled);
    bool IsEnabled() const;

    void SetBackground(const Color& color, const std::string& token);
    void SetBackgroundPath(const std::string& path);
    bool HasBackground() const;
    const Color& GetBackground() const;
    const std::string& GetBackgroundToken() const;
    Background& GetBackgroundSource();
    const Background& GetBackgroundSource() const;

    void SetAppShell(AppShell shell);
    bool HasAppShell() const;
    const AppShell& GetAppShell() const;

    void SetTopBarMenu(TopBarMenu menu);
    bool HasTopBarMenu() const;
    TopBarMenu& GetTopBarMenu();
    const TopBarMenu& GetTopBarMenu() const;

    void AddDockItem(DockItem item);
    std::vector<DockItem>& Dock();
    const std::vector<DockItem>& Dock() const;

    void SetDockBar(float x, float y, float width, float height);
    float DockBarX() const;
    float DockBarY() const;
    float DockBarWidth() const;
    float DockBarHeight() const;

    int HitDock(float x, float y) const;
    bool SelectDock(int index);

private:
    bool enabled_ = false;
    Background background_;
    bool hasAppShell_ = false;
    AppShell appShell_;
    bool hasTopBarMenu_ = false;
    TopBarMenu topBarMenu_;
    std::vector<DockItem> dock_;
    float dockBarX_ = 0.0f;
    float dockBarY_ = 0.0f;
    float dockBarWidth_ = 0.0f;
    float dockBarHeight_ = 0.0f;
};

}  // namespace Robo
