#pragma once

#include "Button.hpp"
#include "Color.hpp"
#include "Selectable.hpp"
#include "Input.hpp"
#include "Scrollbar.hpp"
#include "BrandLogo.hpp"
#include "TitleBar.hpp"
#include "Sidebar.hpp"
#include "Separator.hpp"
#include "ThemeIconToggle.hpp"
#include "MotionShape.hpp"
#include "ThemeToggle.hpp"
#include "Svg.hpp"
#include "Text.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace Robo {

class Container;
class Flexbox;
class Grid;
class Position;

enum class PositionMode {
    Absolute,
    Relative,
    Fixed
};

enum class PositionAnchor {
    TopLeft,
    TopRight,
    BottomLeft,
    BottomRight,
    Center,
    TopCenter,
    BottomCenter
};

enum class PositionItemKind {
    Text,
    Button,
    Input,
    Scrollbar,
    Container,
    Flexbox,
    Grid,
    Position,
    Svg,
    BrandLogo,
    TitleBar,
    Sidebar,
    SidebarItem,
    Separator,
    ThemeToggle,
    ThemeIconToggle,
    MotionShape
};

struct PositionItemProps {
    PositionMode mode = PositionMode::Absolute;
    PositionAnchor anchor = PositionAnchor::TopLeft;

    bool hasLeft = false;
    float left = 0.0f;

    bool hasTop = false;
    float top = 0.0f;

    bool hasRight = false;
    float right = 0.0f;

    bool hasBottom = false;
    float bottom = 0.0f;

    std::string width = "auto";
    std::string height = "auto";
    std::string minWidth;
    std::string maxWidth;
    std::string minHeight;
    std::string maxHeight;
    int zIndex = 0;
};

struct PositionStyle {
    PositionMode mode = PositionMode::Absolute;
    PositionAnchor anchor = PositionAnchor::TopLeft;

    bool hasLeft = false;
    float left = 0.0f;

    bool hasTop = false;
    float top = 0.0f;

    bool hasRight = false;
    float right = 0.0f;

    bool hasBottom = false;
    float bottom = 0.0f;

    std::string width = "auto";
    std::string height = "auto";
    Length widthLength = Length::Auto();
    Length heightLength = Length::Auto();
    bool hasWidthLength = false;
    bool hasHeightLength = false;
    float minWidth = 0.0f;
    float maxWidth = 100000.0f;
    float minHeight = 0.0f;
    float maxHeight = 100000.0f;

    int zIndex = 0;
    bool clipToBounds = false;

    float paddingTop = 0.0f;
    float paddingRight = 0.0f;
    float paddingBottom = 0.0f;
    float paddingLeft = 0.0f;

    Color backgroundColor = Color::FromRgba(0, 0, 0, 0);
    Color borderColor = Color::FromRgba(0, 0, 0, 0);
    float borderWidth = 0.0f;
    float cornerRadius = 0.0f;
    float opacity = 1.0f;
    SelectableMode selectable = SelectableMode::Inherit;
    std::map<std::string, std::string> tokens;
};

struct PositionItem {
    PositionItemKind kind = PositionItemKind::Text;
    PositionItemProps props;
    Text text;
    Button button;
    Input input;
    Scrollbar scrollbar;
    std::shared_ptr<Container> container;
    std::shared_ptr<Flexbox> flexbox;
    std::shared_ptr<Grid> grid;
    std::shared_ptr<Position> position;
    Svg svg;
    BrandLogo brandLogo;
    Robo::TitleBar titleBar;
    Robo::Sidebar sidebar;
    Robo::SidebarItem sidebarItem;
    Robo::Separator separator;
    ThemeToggle themeToggle;
    ThemeIconToggle themeIconToggle;
    MotionShape motionShape;
    InputBounds computedBounds;
};

class Position {
public:
    Position();
    ~Position();

    Position(const Position& other);
    Position& operator=(const Position& other);

    Position(Position&& other) noexcept;
    Position& operator=(Position&& other) noexcept;

    void SetStyle(const PositionStyle& style);
    const PositionStyle& GetStyle() const;
    PositionStyle& GetStyle();

    void AddText(Text text, const PositionItemProps& props = {});
    void AddButton(Button button, const PositionItemProps& props = {});
    void AddInput(Input input, const PositionItemProps& props = {});
    void AddScrollbar(Scrollbar scrollbar, const PositionItemProps& props = {});
    void AddContainer(Container container, const PositionItemProps& props = {});
    void AddFlexbox(Flexbox flexbox, const PositionItemProps& props = {});
    void AddGrid(Grid grid, const PositionItemProps& props = {});
    void AddPosition(Position position, const PositionItemProps& props = {});
    void AddSvg(Svg svg, const PositionItemProps& props = {});
    void AddBrandLogo(BrandLogo logo, const PositionItemProps& props = {});
    void AddTitleBar(Robo::TitleBar bar, const PositionItemProps& props = {});
    void AddSidebar(Robo::Sidebar pane, const PositionItemProps& props = {});
    void AddSidebarItem(Robo::SidebarItem item, const PositionItemProps& props = {});
    void AddSeparator(Robo::Separator separator, const PositionItemProps& props = {});
    void AddThemeToggle(ThemeToggle toggle, const PositionItemProps& props = {});
    void AddThemeIconToggle(ThemeIconToggle toggle, const PositionItemProps& props = {});
    void AddMotionShape(MotionShape shape, const PositionItemProps& props = {});

    const std::vector<PositionItem>& GetItems() const;
    std::vector<PositionItem>& GetItems();

    void SetBounds(const InputBounds& bounds);
    const InputBounds& GetBounds() const;

    void Clear();

private:
    PositionStyle style_;
    std::vector<PositionItem> items_;
    InputBounds bounds_;
};

PositionMode ParsePositionMode(const std::string& str);
PositionAnchor ParsePositionAnchor(const std::string& str);

}  // namespace Robo
