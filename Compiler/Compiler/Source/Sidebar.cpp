#include "Robo/Sidebar.hpp"

#include "Robo/RoboIconPack.hpp"

#include <algorithm>
#include <cmath>

namespace Robo {

bool SidebarItemBounds::Contains(float pointX, float pointY) const {
    return pointX >= x && pointY >= y && pointX < x + width && pointY < y + height;
}

void SidebarItem::SetLabel(const std::string& label) {
    label_ = label;
}

const std::string& SidebarItem::GetLabel() const {
    return label_;
}

void SidebarItem::SetId(const std::string& id) {
    id_ = id;
}

const std::string& SidebarItem::GetId() const {
    return id_;
}

void SidebarItem::SetSection(bool section) {
    section_ = section;
}

bool SidebarItem::IsSection() const {
    return section_;
}

void SidebarItem::SetInteraction(SidebarItemInteraction interaction) {
    interaction_ = interaction;
}

SidebarItemInteraction SidebarItem::GetInteraction() const {
    return interaction_;
}

void SidebarItem::SetDisabled(bool disabled) {
    disabled_ = disabled;
}

bool SidebarItem::IsDisabled() const {
    return disabled_;
}

void SidebarItem::SetLoading(bool loading) {
    loading_ = loading;
}

bool SidebarItem::IsLoading() const {
    return loading_;
}

bool SidebarItem::CanActivate() const {
    return !section_ && !disabled_ && !loading_;
}

void SidebarItem::SetFocused(bool focused) {
    focused_ = focused;
    if (!focused_) {
        focusVisible_ = false;
    }
}

bool SidebarItem::IsFocused() const {
    return focused_;
}

void SidebarItem::SetFocusVisible(bool visible) {
    focusVisible_ = visible && focused_;
}

bool SidebarItem::IsFocusVisible() const {
    return focusVisible_;
}

void SidebarItem::SetSelected(bool selected) {
    selected_ = selected;
}

bool SidebarItem::IsSelected() const {
    return selected_;
}

void SidebarItem::SetIcon(Svg icon) {
    icon_ = std::move(icon);
    hasIcon_ = true;
    iconName_.clear();
}

void SidebarItem::SetIconName(const std::string& name) {
    iconName_ = name;
    if (!iconName_.empty()) {
        hasIcon_ = false;
        icon_ = Svg();
    }
}

bool SidebarItem::HasIcon() const {
    return hasIcon_ || !iconName_.empty();
}

Svg& SidebarItem::Icon() {
    if (!hasIcon_ && !iconName_.empty()) {
        icon_ = RoboBakeNamedIcon(iconName_.c_str(), 20.0f);
        hasIcon_ = !icon_.GetShapes().empty();
        if (hasIcon_) {
            iconName_.clear();
        }
    }
    return icon_;
}

const Svg& SidebarItem::Icon() const {
    return icon_;
}

void SidebarItem::AddChild(SidebarItem child) {
    children_.push_back(std::move(child));
}

std::vector<SidebarItem>& SidebarItem::Children() {
    return children_;
}

const std::vector<SidebarItem>& SidebarItem::Children() const {
    return children_;
}

void SidebarItem::SetBounds(const SidebarItemBounds& bounds) {
    bounds_ = bounds;
}

const SidebarItemBounds& SidebarItem::GetBounds() const {
    return bounds_;
}

void Sidebar::SetVariant(SidebarVariant variant) {
    variant_ = variant;
}

SidebarVariant Sidebar::GetVariant() const {
    return variant_;
}

void Sidebar::AddItem(SidebarItem item) {
    items_.push_back(std::move(item));
}

std::vector<SidebarItem>& Sidebar::Items() {
    return items_;
}

const std::vector<SidebarItem>& Sidebar::Items() const {
    return items_;
}

void Sidebar::Select(int index) {
    if (items_.empty() || index < 0 || index >= static_cast<int>(items_.size())) {
        return;
    }
    if (items_[static_cast<std::size_t>(index)].IsSection()) {
        return;
    }
    if (selected_ != index) {
        scroll_ = 0.0f;
        if (panelReady_) {
            panelBar_.SetValue(0.0);
        }
    }
    selected_ = index;
    for (int i = 0; i < static_cast<int>(items_.size()); ++i) {
        SidebarItem& item = items_[static_cast<std::size_t>(i)];
        item.SetSelected(false);
        if (i == index) {
            continue;
        }
        for (SidebarItem& child : item.Children()) {
            child.SetSelected(false);
        }
    }
}

int Sidebar::IndexOf(const SidebarItem* item) const {
    if (!item) {
        return -1;
    }
    for (int i = 0; i < static_cast<int>(items_.size()); ++i) {
        if (&items_[static_cast<std::size_t>(i)] == item) {
            return i;
        }
    }
    return -1;
}

bool Sidebar::SelectChild(SidebarItem* child) {
    if (!child || !child->CanActivate()) {
        return false;
    }
    bool found = false;
    for (SidebarItem& item : items_) {
        for (SidebarItem& entry : item.Children()) {
            const bool on = &entry == child;
            found = found || on;
            entry.SetSelected(on);
        }
    }
    return found;
}

int Sidebar::Selected() const {
    return selected_;
}

int Sidebar::HitIndex(float x, float y) const {
    if (HitRailArrow(x, y) != SidebarRailArrow::None) {
        return -1;
    }
    if (panelBar_.IsVisible() && panelBar_.GetBounds().Contains(x, y)) {
        return -1;
    }
    for (int i = 0; i < static_cast<int>(items_.size()); ++i) {
        const SidebarItem& item = items_[static_cast<std::size_t>(i)];
        if (!item.CanActivate()) {
            continue;
        }
        if (item.GetBounds().Contains(x, y) &&
            (paneH_ <= 1.0f || (y >= paneY_ && y < paneY_ + paneH_))) {
            return i;
        }
    }
    return -1;
}

SidebarItem* Sidebar::HitItem(float x, float y) {
    if (HitRailArrow(x, y) != SidebarRailArrow::None) {
        return nullptr;
    }
    if (panelBar_.IsVisible() && panelBar_.GetBounds().Contains(x, y)) {
        return nullptr;
    }
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        SidebarItem& item = items_[static_cast<std::size_t>(i)];
        for (int c = static_cast<int>(item.Children().size()) - 1; c >= 0; --c) {
            SidebarItem& child = item.Children()[static_cast<std::size_t>(c)];
            if (child.CanActivate() && child.GetBounds().height > 1.0f && child.GetBounds().Contains(x, y) &&
                (viewH_ <= 1.0f || (y >= viewY_ && y < viewY_ + viewH_))) {
                return &child;
            }
        }
        if (item.CanActivate() && item.GetBounds().Contains(x, y) &&
            (paneH_ <= 1.0f || (y >= paneY_ && y < paneY_ + paneH_))) {
            return &item;
        }
    }
    return nullptr;
}

void Sidebar::SetPanel(float paneX, float paneY, float paneWidth, float paneHeight, float viewY, float viewHeight, float contentHeight) {
    paneX_ = paneX;
    paneY_ = paneY;
    paneW_ = std::max(0.0f, paneWidth);
    paneH_ = std::max(0.0f, paneHeight);
    viewY_ = viewY;
    viewH_ = std::max(0.0f, viewHeight);
    contentHeight_ = std::max(0.0f, contentHeight);
    EnsurePanelBar();
    panelBar_.SetContentSize(static_cast<double>(contentHeight_));
    panelBar_.SetViewportSize(static_cast<double>(viewH_));
    const float maxScroll = std::max(0.0f, contentHeight_ - viewH_);
    if (PanelBarDriving()) {
        scroll_ = maxScroll > 0.0f ? static_cast<float>(panelBar_.GetValue()) * maxScroll : 0.0f;
    } else {
        scroll_ = std::max(0.0f, std::min(scroll_, maxScroll));
        panelBar_.SetValue(maxScroll > 0.0f ? static_cast<double>(scroll_ / maxScroll) : 0.0);
    }
    if (!PanelOverflows()) {
        panelBar_.SetBounds({});
    }
}

void Sidebar::SetRail(float railWidth, float contentHeight, float viewportHeight, float step) {
    railWidth_ = std::max(0.0f, railWidth);
    railContent_ = std::max(0.0f, contentHeight);
    railViewport_ = std::max(0.0f, viewportHeight);
    railStep_ = std::max(1.0f, step);
    const float maxScroll = RailMaxScroll();
    railTarget_ = std::max(0.0f, std::min(railTarget_, maxScroll));
    railScroll_ = std::max(0.0f, std::min(railScroll_, maxScroll));
}

void Sidebar::SetRailArrows(const SidebarItemBounds& up, const SidebarItemBounds& down) {
    railUp_ = up;
    railDown_ = down;
    if (hotRail_ == SidebarRailArrow::Up && railUp_.width <= 1.0f) {
        hotRail_ = SidebarRailArrow::None;
    }
    if (hotRail_ == SidebarRailArrow::Down && railDown_.width <= 1.0f) {
        hotRail_ = SidebarRailArrow::None;
    }
    if (pressedRail_ == SidebarRailArrow::Up && railUp_.width <= 1.0f) {
        pressedRail_ = SidebarRailArrow::None;
    }
    if (pressedRail_ == SidebarRailArrow::Down && railDown_.width <= 1.0f) {
        pressedRail_ = SidebarRailArrow::None;
    }
}

const SidebarItemBounds& Sidebar::RailUpBounds() const {
    return railUp_;
}

const SidebarItemBounds& Sidebar::RailDownBounds() const {
    return railDown_;
}

SidebarRailArrow Sidebar::HitRailArrow(float x, float y) const {
    if (railUp_.width > 1.0f && railUp_.Contains(x, y)) {
        return SidebarRailArrow::Up;
    }
    if (railDown_.width > 1.0f && railDown_.Contains(x, y)) {
        return SidebarRailArrow::Down;
    }
    return SidebarRailArrow::None;
}

bool Sidebar::SetHotRailArrow(SidebarRailArrow arrow) {
    if (hotRail_ == arrow) {
        return false;
    }
    hotRail_ = arrow;
    return true;
}

bool Sidebar::SetPressedRailArrow(SidebarRailArrow arrow) {
    if (pressedRail_ == arrow) {
        return false;
    }
    pressedRail_ = arrow;
    railRepeatMs_ = 0.0;
    railRepeatArmed_ = false;
    return true;
}

SidebarRailArrow Sidebar::HotRailArrow() const {
    return hotRail_;
}

SidebarRailArrow Sidebar::PressedRailArrow() const {
    return pressedRail_;
}

float Sidebar::RailMaxScroll() const {
    return std::max(0.0f, railContent_ - railViewport_);
}

bool Sidebar::CanScrollRailUp() const {
    return railScroll_ > 1.0f;
}

bool Sidebar::CanScrollRailDown() const {
    return railScroll_ < RailMaxScroll() - 1.0f;
}

bool Sidebar::MoveRailTarget(float delta) {
    if (std::fabs(delta) < 0.5f) {
        return false;
    }
    const float next = std::max(0.0f, std::min(RailMaxScroll(), railTarget_ + delta));
    if (std::fabs(next - railTarget_) < 0.5f) {
        return false;
    }
    railTarget_ = next;
    return true;
}

bool Sidebar::StepRail(SidebarRailArrow arrow) {
    if (arrow == SidebarRailArrow::Up) {
        return MoveRailTarget(-railStep_);
    }
    if (arrow == SidebarRailArrow::Down) {
        return MoveRailTarget(railStep_);
    }
    return false;
}

bool Sidebar::TickRail(double deltaMs) {
    const double delta = std::max(0.0, deltaMs);
    bool changed = false;
    if (pressedRail_ != SidebarRailArrow::None) {
        railRepeatMs_ += delta;
        const double interval = railRepeatArmed_ ? 70.0 : 280.0;
        if (railRepeatMs_ >= interval) {
            railRepeatMs_ = 0.0;
            railRepeatArmed_ = true;
            if (StepRail(pressedRail_)) {
                changed = true;
            }
        }
    } else {
        railRepeatMs_ = 0.0;
        railRepeatArmed_ = false;
    }

    const float gap = railTarget_ - railScroll_;
    if (std::fabs(gap) > 0.5f) {
        const float blend = static_cast<float>(1.0 - std::exp(-delta / 70.0));
        railScroll_ += gap * std::max(0.0f, blend);
        if (std::fabs(railTarget_ - railScroll_) < 0.5f) {
            railScroll_ = railTarget_;
        }
        changed = true;
    } else if (railScroll_ != railTarget_) {
        railScroll_ = railTarget_;
        changed = true;
    }
    return changed || pressedRail_ != SidebarRailArrow::None;
}

bool Sidebar::RailNeedsTick() const {
    return pressedRail_ != SidebarRailArrow::None || std::fabs(railTarget_ - railScroll_) > 0.5f;
}

bool Sidebar::Contains(float x, float y) const {
    return paneW_ > 1.0f && paneH_ > 1.0f && x >= paneX_ && y >= paneY_ && x < paneX_ + paneW_ && y < paneY_ + paneH_;
}

bool Sidebar::ScrollBy(float delta, float x) {
    const bool onRail = variant_ == SidebarVariant::Activity && railWidth_ > 1.0f && x < paneX_ + railWidth_;
    if (onRail) {
        return MoveRailTarget(delta);
    }
    if (!PanelOverflows()) {
        return false;
    }
    EnsurePanelBar();
    panelBar_.SetContentSize(static_cast<double>(contentHeight_));
    panelBar_.SetViewportSize(static_cast<double>(viewH_));
    if (!panelBar_.ScrollByPixels(static_cast<double>(delta))) {
        return false;
    }
    PullPanelScroll();
    return true;
}

void Sidebar::Reveal(float itemY, float itemHeight) {
    if (viewH_ <= 1.0f) {
        return;
    }
    if (itemY < viewY_) {
        scroll_ -= viewY_ - itemY;
    } else if (itemY + itemHeight > viewY_ + viewH_) {
        scroll_ += itemY + itemHeight - (viewY_ + viewH_);
    }
    const float maxScroll = std::max(0.0f, contentHeight_ - viewH_);
    scroll_ = std::max(0.0f, std::min(scroll_, maxScroll));
    EnsurePanelBar();
    panelBar_.SetContentSize(static_cast<double>(contentHeight_));
    panelBar_.SetViewportSize(static_cast<double>(viewH_));
    panelBar_.SetValue(maxScroll > 0.0f ? static_cast<double>(scroll_ / maxScroll) : 0.0);
    panelBar_.NotifyInteraction();
}

void Sidebar::RevealRail(float itemY, float itemHeight) {
    if (railViewport_ <= 1.0f) {
        return;
    }
    if (itemY < paneY_) {
        railScroll_ -= paneY_ - itemY;
    } else if (itemY + itemHeight > paneY_ + railViewport_) {
        railScroll_ += itemY + itemHeight - (paneY_ + railViewport_);
    }
    railScroll_ = std::max(0.0f, std::min(railScroll_, RailMaxScroll()));
    railTarget_ = railScroll_;
}

float Sidebar::Scroll() const {
    return scroll_;
}

float Sidebar::RailScroll() const {
    return railScroll_;
}

void Sidebar::EnsurePanelBar() {
    if (panelReady_) {
        return;
    }
    ScrollbarStyle style;
    style.orientation = ScrollbarOrientation::Vertical;
    style.thickness = 8.0;
    style.cornerRadius = 4.0;
    style.minThumbLength = 28.0;
    style.showArrows = true;
    style.arrowSize = 8.0;
    style.arrowGap = 3.0;
    style.autoHide = true;
    style.autoHideDelay = 700.0;
    style.fadeDuration = 180.0;
    style.visibility = ScrollbarVisibility::Auto;
    panelBar_.SetStyle(style);
    panelBar_.ApplyVariantDefaults();
    panelReady_ = true;
}

bool Sidebar::PanelBarDriving() const {
    return panelBar_.IsDragging() || panelBar_.NeedsAnimation() || panelBar_.GetPressedArrow() != ScrollbarArrow::None;
}

Scrollbar& Sidebar::PanelScrollbar() {
    EnsurePanelBar();
    return panelBar_;
}

const Scrollbar& Sidebar::PanelScrollbar() const {
    return panelBar_;
}

bool Sidebar::PanelOverflows() const {
    return contentHeight_ > viewH_ + 1.0f;
}

void Sidebar::PullPanelScroll() {
    const float maxScroll = std::max(0.0f, contentHeight_ - viewH_);
    scroll_ = maxScroll > 0.0f ? static_cast<float>(panelBar_.GetValue()) * maxScroll : 0.0f;
}

float Sidebar::LayoutWidth(float openAmount) const {
    const float open = std::max(0.0f, std::min(1.0f, openAmount));
    if (items_.empty() || variant_ != SidebarVariant::Activity) {
        return SIDEBAR_WIDTH * open;
    }
    return SIDEBAR_RAIL + (SIDEBAR_WIDTH - SIDEBAR_RAIL) * open;
}

SidebarVariant ParseSidebarVariant(const std::string& value) {
    if (value == "activity" || value == "dual" || value == "wide") {
        return SidebarVariant::Activity;
    }
    if (value == "options" || value == "normal" || value == "buttons") {
        return SidebarVariant::Normal;
    }
    return SidebarVariant::Normal;
}

}  // namespace Robo
