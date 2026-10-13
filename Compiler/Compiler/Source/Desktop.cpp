#include "Robo/Desktop.hpp"

namespace Robo {

bool DockItemBounds::Contains(float pointX, float pointY) const {
    return pointX >= x && pointY >= y && pointX < x + width && pointY < y + height;
}

bool MenuBounds::Contains(float pointX, float pointY) const {
    return pointX >= x && pointY >= y && pointX < x + width && pointY < y + height;
}

void TopBarMenuItem::SetLabel(const std::string& label) {
    label_ = label;
}

const std::string& TopBarMenuItem::GetLabel() const {
    return label_;
}

void TopBarMenuItem::SetSelected(bool selected) {
    selected_ = selected;
}

bool TopBarMenuItem::IsSelected() const {
    return selected_;
}

void TopBarMenuItem::SetBounds(const MenuBounds& bounds) {
    bounds_ = bounds;
}

const MenuBounds& TopBarMenuItem::GetBounds() const {
    return bounds_;
}

void TopBarMenu::SetApp(const std::string& app) {
    if (!app.empty()) {
        app_ = app;
    }
}

const std::string& TopBarMenu::GetApp() const {
    return app_;
}

void TopBarMenu::SetClock(const std::string& clock) {
    clock_ = clock;
}

const std::string& TopBarMenu::GetClock() const {
    return clock_;
}

void TopBarMenu::AddItem(TopBarMenuItem item) {
    if (item.IsSelected() && active_ < 0) {
        active_ = static_cast<int>(items_.size()) + 2;
    }
    items_.push_back(std::move(item));
}

std::vector<TopBarMenuItem>& TopBarMenu::Items() {
    return items_;
}

const std::vector<TopBarMenuItem>& TopBarMenu::Items() const {
    return items_;
}

void TopBarMenu::SetAppleBounds(const MenuBounds& bounds) {
    apple_ = bounds;
}

void TopBarMenu::SetAppBounds(const MenuBounds& bounds) {
    appName_ = bounds;
}

void TopBarMenu::SetBar(float x, float y, float width, float height) {
    barX_ = x;
    barY_ = y;
    barWidth_ = width;
    barHeight_ = height;
}

float TopBarMenu::BarX() const {
    return barX_;
}

float TopBarMenu::BarY() const {
    return barY_;
}

float TopBarMenu::BarWidth() const {
    return barWidth_;
}

float TopBarMenu::BarHeight() const {
    return barHeight_;
}

void TopBarMenu::SetHover(int index) {
    hover_ = index;
}

int TopBarMenu::Hover() const {
    return hover_;
}

int TopBarMenu::Hit(float x, float y) const {
    if (barHeight_ <= 0.0f || y < barY_ || y >= barY_ + barHeight_) {
        return -1;
    }
    if (apple_.Contains(x, y)) {
        return 0;
    }
    if (appName_.Contains(x, y)) {
        return 1;
    }
    for (int index = 0; index < static_cast<int>(items_.size()); ++index) {
        if (items_[static_cast<std::size_t>(index)].GetBounds().Contains(x, y)) {
            return index + 2;
        }
    }
    return -1;
}

bool TopBarMenu::Select(int index) {
    const int count = 2 + static_cast<int>(items_.size());
    if (index < 0 || index >= count) {
        return false;
    }
    active_ = active_ == index ? -1 : index;
    for (int item = 0; item < static_cast<int>(items_.size()); ++item) {
        items_[static_cast<std::size_t>(item)].SetSelected(active_ == item + 2);
    }
    return true;
}

bool TopBarMenu::ClearSelection() {
    if (active_ < 0) {
        return false;
    }
    active_ = -1;
    for (TopBarMenuItem& item : items_) {
        item.SetSelected(false);
    }
    return true;
}

int TopBarMenu::Active() const {
    return active_;
}

void DockItem::SetLabel(const std::string& label) {
    label_ = label;
}

const std::string& DockItem::GetLabel() const {
    return label_;
}

void DockItem::SetSelected(bool selected) {
    selected_ = selected;
}

bool DockItem::IsSelected() const {
    return selected_;
}

void DockItem::SetIcon(Svg icon) {
    icon_ = std::move(icon);
    hasIcon_ = true;
}

bool DockItem::HasIcon() const {
    return hasIcon_;
}

Svg& DockItem::Icon() {
    return icon_;
}

const Svg& DockItem::Icon() const {
    return icon_;
}

void DockItem::SetBounds(const DockItemBounds& bounds) {
    bounds_ = bounds;
}

const DockItemBounds& DockItem::GetBounds() const {
    return bounds_;
}

void AppShell::SetTitle(const std::string& title) {
    title_ = title;
}

const std::string& AppShell::GetTitle() const {
    return title_;
}

void Desktop::SetEnabled(bool enabled) {
    enabled_ = enabled;
}

bool Desktop::IsEnabled() const {
    return enabled_;
}

void Desktop::SetBackground(const Color& color, const std::string& token) {
    background_.SetColor(color, token);
}

void Desktop::SetBackgroundPath(const std::string& path) {
    background_.SetPath(path);
}

bool Desktop::HasBackground() const {
    return background_.HasSource();
}

const Color& Desktop::GetBackground() const {
    return background_.GetColor();
}

const std::string& Desktop::GetBackgroundToken() const {
    return background_.GetColorToken();
}

Background& Desktop::GetBackgroundSource() {
    return background_;
}

const Background& Desktop::GetBackgroundSource() const {
    return background_;
}

void Desktop::SetAppShell(AppShell shell) {
    appShell_ = std::move(shell);
    hasAppShell_ = true;
}

bool Desktop::HasAppShell() const {
    return hasAppShell_;
}

const AppShell& Desktop::GetAppShell() const {
    return appShell_;
}

void Desktop::SetTopBarMenu(TopBarMenu menu) {
    topBarMenu_ = std::move(menu);
    hasTopBarMenu_ = true;
}

bool Desktop::HasTopBarMenu() const {
    return hasTopBarMenu_;
}

TopBarMenu& Desktop::GetTopBarMenu() {
    return topBarMenu_;
}

const TopBarMenu& Desktop::GetTopBarMenu() const {
    return topBarMenu_;
}

void Desktop::AddDockItem(DockItem item) {
    dock_.push_back(std::move(item));
}

std::vector<DockItem>& Desktop::Dock() {
    return dock_;
}

const std::vector<DockItem>& Desktop::Dock() const {
    return dock_;
}

void Desktop::SetDockBar(float x, float y, float width, float height) {
    dockBarX_ = x;
    dockBarY_ = y;
    dockBarWidth_ = width;
    dockBarHeight_ = height;
}

float Desktop::DockBarX() const {
    return dockBarX_;
}

float Desktop::DockBarY() const {
    return dockBarY_;
}

float Desktop::DockBarWidth() const {
    return dockBarWidth_;
}

float Desktop::DockBarHeight() const {
    return dockBarHeight_;
}

int Desktop::HitDock(float x, float y) const {
    for (int index = 0; index < static_cast<int>(dock_.size()); ++index) {
        if (dock_[static_cast<std::size_t>(index)].GetBounds().Contains(x, y)) {
            return index;
        }
    }
    return -1;
}

bool Desktop::SelectDock(int index) {
    if (index < 0 || index >= static_cast<int>(dock_.size())) {
        return false;
    }
    bool changed = false;
    for (int item = 0; item < static_cast<int>(dock_.size()); ++item) {
        const bool selected = item == index;
        DockItem& dockItem = dock_[static_cast<std::size_t>(item)];
        if (dockItem.IsSelected() != selected) {
            dockItem.SetSelected(selected);
            changed = true;
        }
    }
    return changed;
}

}  // namespace Robo
