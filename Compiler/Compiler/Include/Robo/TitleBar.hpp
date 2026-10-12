#pragma once

#include "BrandLogo.hpp"
#include "Button.hpp"
#include "Svg.hpp"
#include "ThemeIconToggle.hpp"

#include <string>

namespace Robo {

constexpr float TITLE_BAR_HEIGHT = 57.0f;

// TitleBar and Navbar are the same control.
// The mark sits on the left. The split theme control sits on the right.
class TitleBar {
public:
    TitleBar();

    void SetTitle(const std::string& title);
    const std::string& GetTitle() const;

    BrandLogo& Logo();
    const BrandLogo& Logo() const;
    Button& SidebarButton();
    const Button& SidebarButton() const;
    ThemeIconToggle& ThemeIcon();
    const ThemeIconToggle& ThemeIcon() const;

    void SetSidebarOpen(bool open);
    bool IsSidebarOpen() const;
    void SetSidebarAmount(float amount);
    float SidebarAmount() const;
    void SetSidebarIcons(Svg shown, Svg hidden);
    bool HasSidebarIcons() const;
    Svg& ShownIcon();
    Svg& HiddenIcon();

private:
    std::string title_;
    BrandLogo logo_;
    Button sidebarButton_;
    ThemeIconToggle themeIcon_;
    Svg shownIcon_;
    Svg hiddenIcon_;
    bool hasSidebarIcons_ = false;
    bool sidebarOpen_ = true;
    float sidebarAmount_ = 1.0f;
};

}  // namespace Robo
