#pragma once

#include "Button.hpp"
#include "Color.hpp"
#include "GridLayout.hpp"
#include "Length.hpp"
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
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Robo {

class Container;
class Flexbox;
class Grid;

enum class GridTrackType {
    Fraction,
    Pixel,
    Percent,
    Auto
};

struct GridTrack {
    GridTrackType type = GridTrackType::Fraction;
    float value = 1.0f;
};

enum class GridAutoFlow {
    Row,
    Column
};

enum class GridAlignment {
    Stretch,
    Start,
    Center,
    End,
    Auto
};

enum class GridJustifyContent {
    Start,
    Center,
    End,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly
};

enum class GridItemKind {
    Text,
    Button,
    Input,
    Scrollbar,
    Container,
    Flexbox,
    Grid,
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

struct GridStyle {
    std::vector<GridTrack> templateColumns;
    std::vector<GridTrack> templateRows;
    int columnCount = 0;
    int rowCount = 0;
    std::string columnsString;
    std::string rowsString;

    float gap = 0.0f;
    float columnGap = 0.0f;
    float rowGap = 0.0f;

    GridAutoFlow autoFlow = GridAutoFlow::Row;
    GridAlignment justifyItems = GridAlignment::Stretch;
    GridAlignment alignItems = GridAlignment::Stretch;
    GridJustifyContent justifyContent = GridJustifyContent::Start;

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

    float paddingTop = 0.0f;
    float paddingBottom = 0.0f;
    float paddingLeft = 0.0f;
    float paddingRight = 0.0f;

    float marginTop = 0.0f;
    float marginBottom = 0.0f;
    float marginLeft = 0.0f;
    float marginRight = 0.0f;

    Color backgroundColor = Color::FromRgba(0, 0, 0, 0);
    Color borderColor = Color::FromRgba(0, 0, 0, 0);
    float borderWidth = 0.0f;
    float cornerRadius = 0.0f;
    float opacity = 1.0f;
    SelectableMode selectable = SelectableMode::Inherit;
    std::map<std::string, std::string> tokens;
};

struct GridItemProps {
    int column = 0;
    int row = 0;
    int columnSpan = 1;
    int rowSpan = 1;

    GridAlignment justifySelf = GridAlignment::Auto;
    GridAlignment alignSelf = GridAlignment::Auto;

    std::string width;
    std::string height;
    std::string minWidth;
    std::string maxWidth;
    std::string minHeight;
    std::string maxHeight;
    Length widthLength = Length::Auto();
    Length heightLength = Length::Auto();
    Length minWidthLength = Length::Auto();
    Length maxWidthLength = Length::Auto();
    Length minHeightLength = Length::Auto();
    Length maxHeightLength = Length::Auto();
    bool hasWidthLength = false;
    bool hasHeightLength = false;
    bool hasMinWidthLength = false;
    bool hasMaxWidthLength = false;
    bool hasMinHeightLength = false;
    bool hasMaxHeightLength = false;

    float marginTop = 0.0f;
    float marginBottom = 0.0f;
    float marginLeft = 0.0f;
    float marginRight = 0.0f;
};

struct GridBounds {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    bool Contains(float pointX, float pointY) const;
};

struct GridItem {
    GridItemKind kind = GridItemKind::Text;
    GridItemProps props;
    std::shared_ptr<Text> text;
    std::shared_ptr<Button> button;
    std::shared_ptr<Input> input;
    std::shared_ptr<Scrollbar> scrollbar;
    std::shared_ptr<Container> container;
    std::shared_ptr<Flexbox> flexbox;
    std::shared_ptr<Grid> grid;
    std::shared_ptr<Svg> svg;
    std::shared_ptr<BrandLogo> brandLogo;
    std::shared_ptr<Robo::TitleBar> titleBar;
    std::shared_ptr<Robo::Sidebar> sidebar;
    std::shared_ptr<Robo::SidebarItem> sidebarItem;
    std::shared_ptr<Robo::Separator> separator;
    std::shared_ptr<ThemeToggle> themeToggle;
    std::shared_ptr<ThemeIconToggle> themeIconToggle;
    std::shared_ptr<MotionShape> motionShape;
};

class Grid {
public:
    Grid();

    void SetStyle(const GridStyle& style);
    const GridStyle& GetStyle() const;
    GridStyle& GetStyle();

    void AddText(Text text, const GridItemProps& props = {});
    void AddButton(Button button, const GridItemProps& props = {});
    void AddInput(Input input, const GridItemProps& props = {});
    void AddScrollbar(Scrollbar scrollbar, const GridItemProps& props = {});
    void AddContainer(Container container, const GridItemProps& props = {});
    void AddFlexbox(Flexbox flexbox, const GridItemProps& props = {});
    void AddGrid(Grid grid, const GridItemProps& props = {});
    void AddSvg(Svg svg, const GridItemProps& props = {});
    void AddBrandLogo(BrandLogo logo, const GridItemProps& props = {});
    void AddTitleBar(Robo::TitleBar bar, const GridItemProps& props = {});
    void AddSidebar(Robo::Sidebar pane, const GridItemProps& props = {});
    void AddSidebarItem(Robo::SidebarItem item, const GridItemProps& props = {});
    void AddSeparator(Robo::Separator separator, const GridItemProps& props = {});
    void AddThemeToggle(ThemeToggle toggle, const GridItemProps& props = {});
    void AddThemeIconToggle(ThemeIconToggle toggle, const GridItemProps& props = {});
    void AddMotionShape(MotionShape shape, const GridItemProps& props = {});
    void AddItem(GridItem item);

    const std::vector<GridItem>& GetItems() const;
    std::vector<GridItem>& GetItems();

    void SetBounds(const GridBounds& bounds);
    const GridBounds& GetBounds() const;

    float ResolveGapX(float pixelScale) const;
    float ResolveGapY(float pixelScale) const;

    bool HasLayoutCache(float containerWidth, float pixelScale, float forcedWidth, float forcedHeight) const;
    const ComputedGridLayout& LayoutCache() const;
    void StoreLayoutCache(float containerWidth, float pixelScale, float forcedWidth, float forcedHeight,
                          ComputedGridLayout layout) const;
    void InvalidateLayoutCache() const;
    bool NeedsLayout() const;
    void SetNeedsLayout(bool needs) const;
    bool NeedsPaint() const;
    void SetNeedsPaint(bool needs) const;

private:
    GridStyle style_;
    std::vector<GridItem> items_;
    GridBounds bounds_;
    mutable GridLayoutCache layoutCache_{};
    mutable bool needsLayout_ = true;
    mutable bool needsPaint_ = true;
};

std::vector<GridTrack> ParseGridTracks(const std::string& value);
GridAutoFlow ParseGridAutoFlow(const std::string& value);
GridAlignment ParseGridAlignment(const std::string& value);
GridJustifyContent ParseGridJustifyContent(const std::string& value);

}  // namespace Robo
