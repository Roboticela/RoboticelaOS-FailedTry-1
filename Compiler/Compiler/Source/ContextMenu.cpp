#include "Robo/ContextMenu.hpp"

#include "Robo/FontEngine.hpp"
#include "Robo/RoboIconPack.hpp"

#include <algorithm>
#include <cmath>

namespace Robo {

namespace {

constexpr float MENU_WIDTH = 252.0f;
constexpr float MENU_MIN_WIDTH = 230.0f;
constexpr float MENU_MAX_WIDTH = 340.0f;
constexpr float MENU_PAD = 6.0f;
constexpr float MENU_ROW = 29.0f;
constexpr float MENU_FONT = 13.0f;
constexpr float MENU_ROW_PAD = 10.0f;
constexpr float MENU_SEPARATOR = 11.0f;
constexpr float MENU_CHEVRON = 16.0f;
constexpr float MENU_EDGE = 8.0f;
constexpr float MENU_OPEN_MS = 150.0f;
constexpr float MENU_CLOSE_MS = 120.0f;
constexpr float MENU_SUBMENU_MS = 180.0f;

float RowHeight(const ContextMenuItem& item, float scale) {
    if (item.IsSeparator()) {
        return MENU_SEPARATOR * scale;
    }
    return MENU_ROW * scale;
}

float MeasurePanelWidth(const FontEngine& fonts, const std::vector<ContextMenuItem>& items, float scale) {
    float width = MENU_WIDTH * scale;
    const float fontSize = MENU_FONT * scale;
    const float pad = MENU_ROW_PAD * scale;
    const float glyph = 14.0f * scale;
    bool anyIcon = false;
    for (const ContextMenuItem& item : items) {
        if (item.HasIcon()) {
            anyIcon = true;
        }
    }
    const float lead = anyIcon ? glyph + 8.0f * scale : 0.0f;
    for (const ContextMenuItem& item : items) {
        if (item.IsSeparator()) {
            continue;
        }
        float text = fonts.MeasureWidth(item.GetLabel(), fontSize, FontFaceId::Medium, 0.0f);
        float extra = lead;
        if (item.HasChevron() || item.HasSubmenu()) {
            extra += glyph + 8.0f * scale;
        }
        extra += item.ShortcutWidth(fonts, fontSize, glyph);
        if (item.Shortcut().empty() && extra > lead) {
            extra += 8.0f * scale;
        }
        width = std::max(width, text + pad * 2.0f + extra);
    }
    const float minimum = MENU_MIN_WIDTH * scale;
    const float maximum = MENU_MAX_WIDTH * scale;
    return std::max(minimum, std::min(maximum, width));
}

float MeasurePanelHeight(const std::vector<ContextMenuItem>& items, float scale) {
    float height = MENU_PAD * 2.0f * scale;
    for (const ContextMenuItem& item : items) {
        height += RowHeight(item, scale);
    }
    return height;
}

void PlaceRows(std::vector<ContextMenuItem>& items, float x, float y, float width, float scale) {
    const float pad = MENU_PAD * scale;
    float cursor = y + pad;
    for (ContextMenuItem& item : items) {
        const float height = RowHeight(item, scale);
        ContextMenuBounds bounds;
        bounds.x = x + pad;
        bounds.y = cursor;
        bounds.width = std::max(0.0f, width - pad * 2.0f);
        bounds.height = height;
        item.SetBounds(bounds);
        cursor += height;
    }
}

ContextMenuBounds ClampPanel(float x, float y, float width, float height, float viewWidth, float viewHeight, float top, float bottom) {
    const float edge = MENU_EDGE;
    const float minX = edge;
    const float minY = top + edge;
    const float maxX = std::max(minX, viewWidth - width - edge);
    const float maxY = std::max(minY, viewHeight - bottom - height - edge);
    ContextMenuBounds bounds;
    bounds.width = width;
    bounds.height = height;
    bounds.x = std::max(minX, std::min(x, maxX));
    bounds.y = std::max(minY, std::min(y, maxY));
    return bounds;
}

}  // namespace

bool ContextMenuBounds::Contains(float pointX, float pointY) const {
    return pointX >= x && pointY >= y && pointX < x + width && pointY < y + height;
}

void ContextMenuItem::SetLabel(const std::string& label) {
    label_ = label;
}

const std::string& ContextMenuItem::GetLabel() const {
    return label_;
}

void ContextMenuItem::SetDisabled(bool disabled) {
    disabled_ = disabled;
}

bool ContextMenuItem::IsDisabled() const {
    return disabled_;
}

void ContextMenuItem::SetSeparator(bool separator) {
    separator_ = separator;
}

bool ContextMenuItem::IsSeparator() const {
    return separator_;
}

void ContextMenuItem::SetChecked(bool checked) {
    checked_ = checked;
}

bool ContextMenuItem::IsChecked() const {
    return checked_;
}

void ContextMenuItem::SetCheckable(bool checkable) {
    checkable_ = checkable;
}

bool ContextMenuItem::IsCheckable() const {
    return checkable_;
}

void ContextMenuItem::SetIcon(Svg icon) {
    icon_ = std::move(icon);
    hasIcon_ = true;
    iconName_.clear();
}

void ContextMenuItem::SetIconName(const std::string& name) {
    iconName_ = name;
    if (!iconName_.empty()) {
        hasIcon_ = false;
        icon_ = Svg();
    }
}

bool ContextMenuItem::HasIcon() const {
    return hasIcon_ || !iconName_.empty();
}

Svg& ContextMenuItem::Icon() {
    return icon_;
}

const Svg& ContextMenuItem::Icon() const {
    return icon_;
}

void ContextMenuItem::SetChevron(Svg graphic) {
    chevron_ = std::move(graphic);
    hasChevron_ = true;
    chevronName_.clear();
}

void ContextMenuItem::SetChevronName(const std::string& name) {
    chevronName_ = name;
    if (!chevronName_.empty()) {
        hasChevron_ = false;
        chevron_ = Svg();
    }
}

bool ContextMenuItem::HasChevron() const {
    return hasChevron_ || !chevronName_.empty();
}

Svg& ContextMenuItem::Chevron() {
    return chevron_;
}

void ContextMenuItem::SetCheckGraphic(Svg graphic) {
    check_ = std::move(graphic);
    hasCheck_ = true;
    checkName_.clear();
}

void ContextMenuItem::SetCheckName(const std::string& name) {
    checkName_ = name;
    if (!checkName_.empty()) {
        hasCheck_ = false;
        check_ = Svg();
    }
}

bool ContextMenuItem::HasCheckGraphic() const {
    return hasCheck_ || !checkName_.empty();
}

Svg& ContextMenuItem::CheckGraphic() {
    return check_;
}

void ContextMenuItem::AddShortcutGlyph(Svg graphic) {
    ContextMenuGlyph part;
    part.graphic = std::move(graphic);
    shortcut_.push_back(std::move(part));
}

void ContextMenuItem::AddShortcutText(const std::string& text) {
    if (text.empty()) {
        return;
    }
    ContextMenuGlyph part;
    part.text = true;
    part.label = text;
    shortcut_.push_back(std::move(part));
}

void ContextMenuItem::SetShortcutSpec(const std::string& spec) {
    shortcutSpec_ = spec;
}

void ContextMenuItem::EnsureGraphics() {
    if (!hasIcon_ && !iconName_.empty()) {
        icon_ = RoboBakeNamedIcon(iconName_.c_str(), 16.0f);
        hasIcon_ = !icon_.GetShapes().empty();
        if (hasIcon_) {
            iconName_.clear();
        }
    }
    if (!hasChevron_ && !chevronName_.empty()) {
        chevron_ = RoboBakeNamedIcon(chevronName_.c_str(), 16.0f);
        hasChevron_ = !chevron_.GetShapes().empty();
        if (hasChevron_) {
            chevronName_.clear();
        }
    }
    if (!hasCheck_ && !checkName_.empty()) {
        check_ = RoboBakeNamedIcon(checkName_.c_str(), 16.0f);
        hasCheck_ = !check_.GetShapes().empty();
        if (hasCheck_) {
            checkName_.clear();
        }
    }
    if (!shortcutSpec_.empty() && shortcut_.empty()) {
        std::string shortcut = shortcutSpec_;
        while (!shortcut.empty()) {
            if (shortcut[0] == ' ') {
                shortcut.erase(0, 1);
                continue;
            }
            std::string iconName;
            std::size_t used = 0;
            if (shortcut.compare(0, 5, "Shift") == 0) {
                iconName = "Shift";
                used = 5;
            } else if (shortcut.compare(0, 7, "Command") == 0) {
                iconName = "Command";
                used = 7;
            } else if (shortcut.compare(0, 3, "Cmd") == 0) {
                iconName = "Command";
                used = 3;
            } else if (shortcut.compare(0, 3, "\xE2\x87\xA7") == 0) {
                iconName = "Shift";
                used = 3;
            } else if (shortcut.compare(0, 3, "\xE2\x8C\x98") == 0) {
                iconName = "Command";
                used = 3;
            }
            if (!iconName.empty()) {
                ContextMenuGlyph part;
                part.graphic = RoboBakeNamedIcon(iconName.c_str(), 13.0f);
                if (!part.graphic.GetShapes().empty()) {
                    shortcut_.push_back(std::move(part));
                }
                shortcut.erase(0, used);
                continue;
            }
            const std::size_t end = shortcut.find(' ');
            const std::string piece = shortcut.substr(0, end);
            AddShortcutText(piece);
            shortcut.erase(0, piece.size());
        }
        shortcutSpec_.clear();
    }
    for (ContextMenuItem& child : items_) {
        child.EnsureGraphics();
    }
}

const std::vector<ContextMenuGlyph>& ContextMenuItem::Shortcut() const {
    return shortcut_;
}

float ContextMenuItem::ShortcutWidth(const FontEngine& fonts, float fontSize, float glyphSize) const {
    float width = 0.0f;
    for (const ContextMenuGlyph& part : shortcut_) {
        if (width > 0.0f) {
            width += glyphSize * 0.15f;
        }
        if (part.text) {
            width += fonts.MeasureWidth(part.label, fontSize, FontFaceId::Medium, 0.0f);
        } else {
            width += glyphSize;
        }
    }
    if (!shortcut_.empty()) {
        width += glyphSize * 0.7f;
    }
    return width;
}

void ContextMenuItem::AddItem(ContextMenuItem item) {
    items_.push_back(std::move(item));
}

std::vector<ContextMenuItem>& ContextMenuItem::Items() {
    return items_;
}

const std::vector<ContextMenuItem>& ContextMenuItem::Items() const {
    return items_;
}

bool ContextMenuItem::HasSubmenu() const {
    return !items_.empty();
}

void ContextMenuItem::SetBounds(const ContextMenuBounds& bounds) {
    bounds_ = bounds;
}

const ContextMenuBounds& ContextMenuItem::GetBounds() const {
    return bounds_;
}

void ContextMenu::SetAppearanceDark(bool dark) {
    dark_ = dark;
    appearanceLocked_ = true;
}

void ContextMenu::ApplyTheme(bool dark) {
    if (!appearanceLocked_) {
        dark_ = dark;
    }
}

bool ContextMenu::IsDark() const {
    return dark_;
}

void ContextMenu::SetTransparency(float amount) {
    transparency_ = std::max(0.0f, std::min(1.0f, amount));
}

float ContextMenu::GetTransparency() const {
    return transparency_;
}

void ContextMenu::AddItem(ContextMenuItem item) {
    items_.push_back(std::move(item));
}

std::vector<ContextMenuItem>& ContextMenu::Items() {
    return items_;
}

const std::vector<ContextMenuItem>& ContextMenu::Items() const {
    return items_;
}

bool ContextMenu::Open(float x, float y) {
    if (visible_ && !closing_) {
        const float dx = x - anchorX_;
        const float dy = y - anchorY_;
        if (dx * dx + dy * dy < 4.0f) {
            return false;
        }
    }
    for (ContextMenuItem& item : items_) {
        item.EnsureGraphics();
    }
    visible_ = true;
    opening_ = false;
    closing_ = false;
    amount_ = 1.0f;
    anchorX_ = x;
    anchorY_ = y;
    submenu_ = -1;
    hover_ = -1;
    hoverChild_ = -1;
    focus_ = -1;
    focusChild_ = -1;
    pressed_ = -1;
    pressedChild_ = -1;
    hoverMs_ = 0.0f;
    action_.clear();
    submenuPanel_ = {};
    return true;
}

void ContextMenu::Close() {
    if (!visible_ || closing_) {
        return;
    }
    closing_ = true;
    opening_ = false;
    pressed_ = -1;
    pressedChild_ = -1;
}

bool ContextMenu::IsVisible() const {
    return visible_;
}

float ContextMenu::OpenAmount() const {
    return amount_;
}

void ContextMenu::Layout(const FontEngine& fonts, float pixelScale, float viewWidth, float viewHeight, float topInset, float bottomInset) {
    if (!visible_) {
        return;
    }
    const float scale = std::max(0.5f, pixelScale);
    const float width = MeasurePanelWidth(fonts, items_, scale);
    const float height = MeasurePanelHeight(items_, scale);
    panel_ = ClampPanel(anchorX_, anchorY_, width, height, viewWidth, viewHeight, topInset, bottomInset);
    PlaceRows(items_, panel_.x, panel_.y, panel_.width, scale);
    submenuPanel_ = {};
    if (submenu_ < 0 || submenu_ >= static_cast<int>(items_.size())) {
        return;
    }
    ContextMenuItem& parent = items_[static_cast<std::size_t>(submenu_)];
    if (!parent.HasSubmenu()) {
        submenu_ = -1;
        return;
    }
    const float childWidth = MeasurePanelWidth(fonts, parent.Items(), scale);
    const float childHeight = MeasurePanelHeight(parent.Items(), scale);
    const ContextMenuBounds& row = parent.GetBounds();
    float childX = panel_.x + panel_.width - 8.0f * scale;
    if (childX + childWidth > viewWidth - MENU_EDGE) {
        childX = panel_.x - childWidth + 8.0f * scale;
    }
    const float childY = row.y - MENU_PAD * scale;
    submenuPanel_ = ClampPanel(childX, childY, childWidth, childHeight, viewWidth, viewHeight, topInset, bottomInset);
    PlaceRows(parent.Items(), submenuPanel_.x, submenuPanel_.y, submenuPanel_.width, scale);
}

bool ContextMenu::Contains(float x, float y) const {
    if (!visible_) {
        return false;
    }
    if (panel_.Contains(x, y)) {
        return true;
    }
    return submenu_ >= 0 && submenuPanel_.Contains(x, y);
}

int ContextMenu::Hit(float x, float y, int& child) const {
    child = -1;
    if (!visible_) {
        return -1;
    }
    if (submenu_ >= 0 && submenu_ < static_cast<int>(items_.size()) && submenuPanel_.Contains(x, y)) {
        const std::vector<ContextMenuItem>& rows = items_[static_cast<std::size_t>(submenu_)].Items();
        for (int index = 0; index < static_cast<int>(rows.size()); ++index) {
            if (rows[static_cast<std::size_t>(index)].GetBounds().Contains(x, y)) {
                child = index;
                return submenu_;
            }
        }
        return submenu_;
    }
    if (!panel_.Contains(x, y)) {
        return -1;
    }
    for (int index = 0; index < static_cast<int>(items_.size()); ++index) {
        if (items_[static_cast<std::size_t>(index)].GetBounds().Contains(x, y)) {
            return index;
        }
    }
    return -1;
}

bool ContextMenu::PointerMove(float x, float y) {
    if (!visible_ || closing_) {
        return false;
    }
    int child = -1;
    const int hit = Hit(x, y, child);
    const int nextHover = child >= 0 ? submenu_ : hit;
    const int nextChild = child;
    if (nextHover == hover_ && nextChild == hoverChild_) {
        return false;
    }
    hover_ = nextHover;
    hoverChild_ = nextChild;
    hoverMs_ = 0.0f;
    if (hover_ >= 0 && hoverChild_ < 0) {
        focus_ = hover_;
        focusChild_ = -1;
        const ContextMenuItem& item = items_[static_cast<std::size_t>(hover_)];
        if (!item.HasSubmenu() || item.IsDisabled() || item.IsSeparator()) {
            submenu_ = -1;
        }
    } else if (hoverChild_ >= 0) {
        focus_ = hover_;
        focusChild_ = hoverChild_;
    }
    return true;
}

bool ContextMenu::PointerDown(float x, float y) {
    if (!visible_ || closing_) {
        return false;
    }
    int child = -1;
    const int hit = Hit(x, y, child);
    if (hit < 0) {
        return false;
    }
    const ContextMenuItem& item = items_[static_cast<std::size_t>(hit)];
    if (child < 0 && (item.IsSeparator() || item.IsDisabled())) {
        pressed_ = -1;
        pressedChild_ = -1;
        return true;
    }
    if (child >= 0) {
        const ContextMenuItem& row = item.Items()[static_cast<std::size_t>(child)];
        if (row.IsSeparator() || row.IsDisabled()) {
            pressed_ = -1;
            pressedChild_ = -1;
            return true;
        }
    }
    pressed_ = hit;
    pressedChild_ = child;
    if (child < 0 && item.HasSubmenu()) {
        OpenSubmenu(hit);
    }
    return true;
}

bool ContextMenu::PointerUp(float x, float y) {
    if (!visible_ || pressed_ < 0) {
        return false;
    }
    int child = -1;
    const int hit = Hit(x, y, child);
    const int pressed = pressed_;
    const int pressedChild = pressedChild_;
    pressed_ = -1;
    pressedChild_ = -1;
    if (hit != pressed || child != pressedChild) {
        return true;
    }
    const ContextMenuItem& item = items_[static_cast<std::size_t>(hit)];
    if (pressedChild < 0 && item.HasSubmenu()) {
        return true;
    }
    Activate(hit, pressedChild);
    return true;
}

int ContextMenu::NextRow(const std::vector<ContextMenuItem>& rows, int index, int direction) const {
    if (rows.empty() || direction == 0) {
        return -1;
    }
    int cursor = index;
    for (int step = 0; step < static_cast<int>(rows.size()); ++step) {
        if (cursor < 0) {
            cursor = direction > 0 ? 0 : static_cast<int>(rows.size()) - 1;
        } else {
            cursor += direction;
        }
        if (cursor < 0 || cursor >= static_cast<int>(rows.size())) {
            return -1;
        }
        const ContextMenuItem& item = rows[static_cast<std::size_t>(cursor)];
        if (!item.IsSeparator() && !item.IsDisabled()) {
            return cursor;
        }
    }
    return -1;
}

void ContextMenu::OpenSubmenu(int index) {
    if (index < 0 || index >= static_cast<int>(items_.size())) {
        submenu_ = -1;
        return;
    }
    if (!items_[static_cast<std::size_t>(index)].HasSubmenu()) {
        submenu_ = -1;
        return;
    }
    submenu_ = index;
    hoverMs_ = 0.0f;
}

void ContextMenu::Activate(int item, int child) {
    if (item < 0 || item >= static_cast<int>(items_.size())) {
        return;
    }
    ContextMenuItem& row = items_[static_cast<std::size_t>(item)];
    ContextMenuItem* target = &row;
    if (child >= 0) {
        if (child >= static_cast<int>(row.Items().size())) {
            return;
        }
        target = &row.Items()[static_cast<std::size_t>(child)];
    }
    if (target->IsSeparator() || target->IsDisabled() || target->HasSubmenu()) {
        return;
    }
    if (target->IsCheckable()) {
        target->SetChecked(!target->IsChecked());
    }
    action_ = target->GetLabel();
    Close();
}

bool ContextMenu::KeyDown(const std::string& key) {
    if (!visible_) {
        return false;
    }
    if (key == "Escape") {
        if (submenu_ >= 0 && focusChild_ >= 0) {
            focusChild_ = -1;
            submenu_ = -1;
            return true;
        }
        Close();
        return true;
    }
    if (key == "ArrowDown" || key == "ArrowUp") {
        const int direction = key == "ArrowDown" ? 1 : -1;
        if (submenu_ >= 0 && focusChild_ >= 0 && submenu_ < static_cast<int>(items_.size())) {
            focusChild_ = NextRow(items_[static_cast<std::size_t>(submenu_)].Items(), focusChild_, direction);
            hover_ = submenu_;
            hoverChild_ = focusChild_;
            return true;
        }
        focus_ = NextRow(items_, focus_, direction);
        focusChild_ = -1;
        hover_ = focus_;
        hoverChild_ = -1;
        if (focus_ >= 0 && !items_[static_cast<std::size_t>(focus_)].HasSubmenu()) {
            submenu_ = -1;
        }
        return true;
    }
    if (key == "ArrowRight") {
        if (focus_ >= 0 && focus_ < static_cast<int>(items_.size()) && items_[static_cast<std::size_t>(focus_)].HasSubmenu()) {
            OpenSubmenu(focus_);
            focusChild_ = NextRow(items_[static_cast<std::size_t>(focus_)].Items(), -1, 1);
            hover_ = focus_;
            hoverChild_ = focusChild_;
        }
        return true;
    }
    if (key == "ArrowLeft") {
        if (submenu_ >= 0) {
            submenu_ = -1;
            focusChild_ = -1;
            hoverChild_ = -1;
        }
        return true;
    }
    if (key == "Enter" || key == " ") {
        if (focus_ < 0) {
            return true;
        }
        if (focusChild_ >= 0) {
            Activate(focus_, focusChild_);
            return true;
        }
        if (items_[static_cast<std::size_t>(focus_)].HasSubmenu()) {
            OpenSubmenu(focus_);
            focusChild_ = NextRow(items_[static_cast<std::size_t>(focus_)].Items(), -1, 1);
            return true;
        }
        Activate(focus_, -1);
        return true;
    }
    return false;
}

bool ContextMenu::Tick(float deltaMs) {
    if (!visible_ || deltaMs <= 0.0f) {
        return false;
    }
    bool changed = false;
    if (opening_) {
        amount_ = std::min(1.0f, amount_ + deltaMs / MENU_OPEN_MS);
        opening_ = amount_ < 1.0f;
        changed = true;
    }
    if (closing_) {
        amount_ = std::max(0.0f, amount_ - deltaMs / MENU_CLOSE_MS);
        changed = true;
        if (amount_ <= 0.0f) {
            closing_ = false;
            visible_ = false;
            submenu_ = -1;
            hover_ = -1;
            hoverChild_ = -1;
            focus_ = -1;
            focusChild_ = -1;
        }
    }
    if (!closing_ && hover_ >= 0 && hoverChild_ < 0 && hover_ < static_cast<int>(items_.size())) {
        const ContextMenuItem& item = items_[static_cast<std::size_t>(hover_)];
        if (item.HasSubmenu() && !item.IsDisabled() && submenu_ != hover_) {
            hoverMs_ += deltaMs;
            if (hoverMs_ >= MENU_SUBMENU_MS) {
                OpenSubmenu(hover_);
                changed = true;
            }
        }
    }
    return changed;
}

const ContextMenuBounds& ContextMenu::Panel() const {
    return panel_;
}

const ContextMenuBounds& ContextMenu::SubmenuPanel() const {
    return submenuPanel_;
}

int ContextMenu::Submenu() const {
    return submenu_;
}

int ContextMenu::Hover() const {
    return hover_;
}

int ContextMenu::HoverChild() const {
    return hoverChild_;
}

int ContextMenu::Focus() const {
    return focus_;
}

int ContextMenu::FocusChild() const {
    return focusChild_;
}

int ContextMenu::Pressed() const {
    return pressed_;
}

int ContextMenu::PressedChild() const {
    return pressedChild_;
}

ContextMenuBounds ContextMenu::DamageBounds() const {
    ContextMenuBounds bounds = panel_;
    if (submenu_ >= 0 && submenuPanel_.width > 0.0f) {
        const float left = std::min(bounds.x, submenuPanel_.x);
        const float top = std::min(bounds.y, submenuPanel_.y);
        const float right = std::max(bounds.x + bounds.width, submenuPanel_.x + submenuPanel_.width);
        const float bottom = std::max(bounds.y + bounds.height, submenuPanel_.y + submenuPanel_.height);
        bounds.x = left;
        bounds.y = top;
        bounds.width = right - left;
        bounds.height = bottom - top;
    }
    return bounds;
}

std::string ContextMenu::ConsumeAction() {
    std::string action = std::move(action_);
    action_.clear();
    return action;
}

}  // namespace Robo
