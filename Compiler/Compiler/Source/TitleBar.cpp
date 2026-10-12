#include "Robo/TitleBar.hpp"

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
    hasSidebarIcons_ = true;
}

bool TitleBar::HasSidebarIcons() const {
    return hasSidebarIcons_;
}

Svg& TitleBar::ShownIcon() {
    return shownIcon_;
}

Svg& TitleBar::HiddenIcon() {
    return hiddenIcon_;
}

bool TitleBar::IsSidebarOpen() const {
    return sidebarOpen_;
}

}  // namespace Robo
