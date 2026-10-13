#include "Robo/TopBarMenu.hpp"

namespace Robo {

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

void TopBarMenuItem::SetDisabled(bool disabled) {
    disabled_ = disabled;
}

bool TopBarMenuItem::IsDisabled() const {
    return disabled_;
}

void TopBarMenuItem::SetSeparator(bool separator) {
    separator_ = separator;
}

bool TopBarMenuItem::IsSeparator() const {
    return separator_;
}

void TopBarMenuItem::SetSubmenu(bool submenu) {
    submenu_ = submenu;
}

bool TopBarMenuItem::HasSubmenu() const {
    return submenu_;
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

void TopBarMenu::SetAppearanceDark(bool dark) {
    dark_ = dark;
    appearanceLocked_ = true;
}

void TopBarMenu::ApplyTheme(bool dark) {
    if (!appearanceLocked_) {
        dark_ = dark;
    }
}

bool TopBarMenu::IsDark() const {
    return dark_;
}

void TopBarMenu::SetTransparency(float amount) {
    if (amount < 0.0f) {
        amount = 0.0f;
    } else if (amount > 1.0f) {
        amount = 1.0f;
    }
    transparency_ = amount;
}

float TopBarMenu::GetTransparency() const {
    return transparency_;
}

void TopBarMenu::AddItem(TopBarMenuItem item) {
    if (item.IsSelected() && !item.IsDisabled() && !item.IsSeparator() && active_ < 0) {
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

const MenuBounds& TopBarMenu::AppleBounds() const {
    return apple_;
}

const MenuBounds& TopBarMenu::AppBounds() const {
    return appName_;
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

void TopBarMenu::SetPressed(int index) {
    pressed_ = index;
    if (index < 0) {
        pressCloses_ = false;
    }
}

void TopBarMenu::Press(int index) {
    if (!SlotEnabled(index)) {
        return;
    }
    pressCloses_ = active_ == index;
    pressed_ = index;
    if (!pressCloses_) {
        Select(index);
    }
}

bool TopBarMenu::ReleasePress(int hit) {
    const int pressed = pressed_;
    const bool closes = pressCloses_;
    pressed_ = -1;
    pressCloses_ = false;
    if (pressed < 0) {
        return false;
    }
    if (closes && hit == pressed) {
        return Select(pressed);
    }
    return true;
}

int TopBarMenu::Pressed() const {
    return pressed_;
}

int TopBarMenu::SlotCount() const {
    return 2 + static_cast<int>(items_.size());
}

bool TopBarMenu::SlotEnabled(int index) const {
    if (index < 0 || index >= SlotCount()) {
        return false;
    }
    if (index < 2) {
        return true;
    }
    const TopBarMenuItem& item = items_[static_cast<std::size_t>(index - 2)];
    return !item.IsDisabled() && !item.IsSeparator();
}

bool TopBarMenu::MoveFocus(int direction) {
    if (direction == 0 || SlotCount() == 0) {
        return false;
    }
    const int step = direction > 0 ? 1 : -1;
    int index = focus_;
    for (int tries = 0; tries < SlotCount(); ++tries) {
        if (index < 0) {
            index = step > 0 ? 0 : SlotCount() - 1;
        } else {
            index += step;
        }
        if (index < 0 || index >= SlotCount()) {
            return false;
        }
        if (!SlotEnabled(index)) {
            continue;
        }
        focus_ = index;
        keyboardFocus_ = true;
        return true;
    }
    return false;
}

int TopBarMenu::Focus() const {
    return focus_;
}

bool TopBarMenu::KeyboardFocus() const {
    return keyboardFocus_;
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
        const TopBarMenuItem& item = items_[static_cast<std::size_t>(index)];
        if (item.IsSeparator()) {
            continue;
        }
        if (item.GetBounds().Contains(x, y)) {
            return index + 2;
        }
    }
    return -1;
}

bool TopBarMenu::Select(int index) {
    if (!SlotEnabled(index)) {
        return false;
    }
    active_ = active_ == index ? -1 : index;
    keyboardFocus_ = false;
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

}  // namespace Robo
