#pragma once

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
    bool HasBackground() const;
    const Color& GetBackground() const;
    const std::string& GetBackgroundToken() const;

    void SetAppShell(AppShell shell);
    bool HasAppShell() const;
    const AppShell& GetAppShell() const;

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
    bool hasBackground_ = false;
    Color background_ = Color::FromHex("#202838");
    std::string backgroundToken_;
    bool hasAppShell_ = false;
    AppShell appShell_;
    std::vector<DockItem> dock_;
    float dockBarX_ = 0.0f;
    float dockBarY_ = 0.0f;
    float dockBarWidth_ = 0.0f;
    float dockBarHeight_ = 0.0f;
};

}  // namespace Robo
