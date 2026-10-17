#include "Robo/TitleBar.hpp"
#include "Robo/RoboIconPack.hpp"

#include <algorithm>

namespace Robo {

TitleBar::TitleBar() {
    logo_.SetSize(28.0f);
    logo_.SetVariant(BrandLogoVariant::Lockup);
    themeIcon_.SetVariant(ThemeIconToggleVariant::Split);

    ButtonStyle style;
    style.variant = ButtonVariant::Outline;
    sidebarButton_.SetStyle(style);
    sidebarButton_.SetIconOnly(true);
    sidebarButton_.SetLabel("Toggle Sidebar");
    sidebarButton_.ApplyVariantDefaults();
    style = sidebarButton_.GetStyle();
    style.cornerRadius = 8.0;
    style.tokens["cornerRadius"] = "radius.md";
    style.paddingX = 0.0;
    style.paddingY = 0.0;
    style.tokens["paddingX"] = "";
    style.tokens["paddingY"] = "";
    sidebarButton_.SetStyle(style);
}

void TitleBar::SetTitle(const std::string& title) {
    title_ = title;
}

const std::string& TitleBar::GetTitle() const {
    return title_;
}

BrandLogo& TitleBar::Logo() {
    return logo_;
}

const BrandLogo& TitleBar::Logo() const {
    return logo_;
}

Button& TitleBar::SidebarButton() {
    return sidebarButton_;
}

const Button& TitleBar::SidebarButton() const {
    return sidebarButton_;
}

ThemeIconToggle& TitleBar::ThemeIcon() {
    return themeIcon_;
}

const ThemeIconToggle& TitleBar::ThemeIcon() const {
    return themeIcon_;
}

void TitleBar::SetSidebarOpen(bool open) {
    sidebarOpen_ = open;
}

void TitleBar::SetSidebarAmount(float amount) {
    sidebarAmount_ = std::max(0.0f, std::min(1.0f, amount));
}

float TitleBar::SidebarAmount() const {
    return sidebarAmount_;
}

void TitleBar::SetSidebarIcons(Svg shown, Svg hidden) {
    shownIcon_ = std::move(shown);
    hiddenIcon_ = std::move(hidden);
    shownIconName_.clear();
    hiddenIconName_.clear();
    hasSidebarIcons_ = true;
}

void TitleBar::SetSidebarIconNames(const std::string& shown, const std::string& hidden) {
    shownIconName_ = shown;
    hiddenIconName_ = hidden;
    hasSidebarIcons_ = false;
    shownIcon_ = Svg();
    hiddenIcon_ = Svg();
}

void TitleBar::EnsureSidebarIcons() {
    if (hasSidebarIcons_ || shownIconName_.empty() || hiddenIconName_.empty()) {
        return;
    }
    shownIcon_ = RoboBakeNamedIcon(shownIconName_.c_str(), 20.0f);
    hiddenIcon_ = RoboBakeNamedIcon(hiddenIconName_.c_str(), 20.0f);
    hasSidebarIcons_ = !shownIcon_.GetShapes().empty() && !hiddenIcon_.GetShapes().empty();
    if (hasSidebarIcons_) {
        shownIconName_.clear();
        hiddenIconName_.clear();
    }
}

bool TitleBar::HasSidebarIcons() const {
    return hasSidebarIcons_ || (!shownIconName_.empty() && !hiddenIconName_.empty());
}

Svg& TitleBar::ShownIcon() {
    EnsureSidebarIcons();
    return shownIcon_;
}

Svg& TitleBar::HiddenIcon() {
    EnsureSidebarIcons();
    return hiddenIcon_;
}

bool TitleBar::IsSidebarOpen() const {
    return sidebarOpen_;
}

}  // namespace Robo
