#pragma once

#include "Scrollbar.hpp"
#include "Svg.hpp"

#include <string>
#include <vector>

namespace Robo {

constexpr float SIDEBAR_WIDTH = 282.0f;
constexpr float SIDEBAR_RAIL = 48.0f;

// Options is the named sidebar of buttons: icon and label, and no inner panel.
// Activity is the named sidebar with an icon rail plus a wider panel.
enum class SidebarVariant {
    Normal,
    Activity
};

enum class SidebarItemInteraction {
    Normal,
    Hovered,
    Pressed
};

// The icon-rail scroll buttons. Up is the top of the rail, Down is the bottom.
enum class SidebarRailArrow {
    None,
    Up,
    Down
};

struct SidebarItemBounds {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    bool Contains(float pointX, float pointY) const;
};

class SidebarItem {
public:
    void SetLabel(const std::string& label);
    const std::string& GetLabel() const;

    void SetId(const std::string& id);
    const std::string& GetId() const;

    void SetSection(bool section);
    bool IsSection() const;

    void SetInteraction(SidebarItemInteraction interaction);
    SidebarItemInteraction GetInteraction() const;

    void SetDisabled(bool disabled);
    bool IsDisabled() const;
    void SetLoading(bool loading);
    bool IsLoading() const;
    bool CanActivate() const;

    void SetFocused(bool focused);
    bool IsFocused() const;
    void SetFocusVisible(bool visible);
    bool IsFocusVisible() const;

    void SetSelected(bool selected);
    bool IsSelected() const;

    void SetIcon(Svg icon);
    bool HasIcon() const;
    Svg& Icon();
    const Svg& Icon() const;

    void AddChild(SidebarItem child);
    std::vector<SidebarItem>& Children();
    const std::vector<SidebarItem>& Children() const;

    void SetBounds(const SidebarItemBounds& bounds);
    const SidebarItemBounds& GetBounds() const;

private:
    std::string label_;
    std::string id_;
    bool section_ = false;
    SidebarItemInteraction interaction_ = SidebarItemInteraction::Normal;
    bool disabled_ = false;
    bool loading_ = false;
    bool focused_ = false;
    bool focusVisible_ = false;
    bool selected_ = false;
    Svg icon_;
    bool hasIcon_ = false;
    std::vector<SidebarItem> children_;
    SidebarItemBounds bounds_;
};

class Sidebar {
public:
    void SetVariant(SidebarVariant variant);
    SidebarVariant GetVariant() const;

    void AddItem(SidebarItem item);
    std::vector<SidebarItem>& Items();
    const std::vector<SidebarItem>& Items() const;

    void Select(int index);
    int Selected() const;
    int IndexOf(const SidebarItem* item) const;
    bool SelectChild(SidebarItem* child);

    int HitIndex(float x, float y) const;
    SidebarItem* HitItem(float x, float y);
    float LayoutWidth(float openAmount) const;

    void SetPanel(float paneX, float paneY, float paneWidth, float paneHeight, float viewY, float viewHeight, float contentHeight);
    void SetRail(float railWidth, float contentHeight, float viewportHeight, float step);
    void SetRailArrows(const SidebarItemBounds& up, const SidebarItemBounds& down);
    const SidebarItemBounds& RailUpBounds() const;
    const SidebarItemBounds& RailDownBounds() const;
    SidebarRailArrow HitRailArrow(float x, float y) const;
    bool SetHotRailArrow(SidebarRailArrow arrow);
    bool SetPressedRailArrow(SidebarRailArrow arrow);
    SidebarRailArrow HotRailArrow() const;
    SidebarRailArrow PressedRailArrow() const;
    bool CanScrollRailUp() const;
    bool CanScrollRailDown() const;
    bool StepRail(SidebarRailArrow arrow);
    bool TickRail(double deltaMs);
    bool RailNeedsTick() const;
    bool Contains(float x, float y) const;
    bool ScrollBy(float delta, float x);
    void Reveal(float itemY, float itemHeight);
    void RevealRail(float itemY, float itemHeight);
    float Scroll() const;
    float RailScroll() const;

    // The wide panel's scrollbar. It is visible only while that panel overflows.
    Scrollbar& PanelScrollbar();
    const Scrollbar& PanelScrollbar() const;
    bool PanelOverflows() const;
    void PullPanelScroll();

private:
    SidebarVariant variant_ = SidebarVariant::Normal;
    std::vector<SidebarItem> items_;
    int selected_ = 0;
    float paneX_ = 0.0f;
    float paneY_ = 0.0f;
    float paneW_ = 0.0f;
    float paneH_ = 0.0f;
    float viewY_ = 0.0f;
    float viewH_ = 0.0f;
    float contentHeight_ = 0.0f;
    float scroll_ = 0.0f;
    float railWidth_ = 0.0f;
    float railContent_ = 0.0f;
    float railViewport_ = 0.0f;
    float railStep_ = 96.0f;
    float railScroll_ = 0.0f;
    float railTarget_ = 0.0f;
    SidebarItemBounds railUp_{};
    SidebarItemBounds railDown_{};
    SidebarRailArrow hotRail_ = SidebarRailArrow::None;
    SidebarRailArrow pressedRail_ = SidebarRailArrow::None;
    double railRepeatMs_ = 0.0;
    bool railRepeatArmed_ = false;

    float RailMaxScroll() const;
    bool MoveRailTarget(float delta);
    void EnsurePanelBar();
    bool PanelBarDriving() const;

    Scrollbar panelBar_;
    bool panelReady_ = false;
};

SidebarVariant ParseSidebarVariant(const std::string& value);

}  // namespace Robo
