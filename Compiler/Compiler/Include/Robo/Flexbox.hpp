#pragma once

#include "Button.hpp"
#include "Color.hpp"
#include "Selectable.hpp"
#include "FontStyle.hpp"
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
#include "TextAlign.hpp"

#include <memory>
#include <string>
#include <vector>

namespace Robo {

enum class FlexDirection {
    Row,
    RowReverse,
    Column,
    ColumnReverse
};

enum class FlexJustify {
    FlexStart,
    Center,
    FlexEnd,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly
};

enum class FlexAlign {
    Stretch,
    FlexStart,
    Center,
    FlexEnd,
    Auto
};

enum class FlexWrap {
    NoWrap,
    Wrap,
    WrapReverse
};

enum class FlexItemKind {
    Text,
    Button,
    Input,
    Scrollbar,
    Container,
    Flexbox,
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

struct FlexItemProps {
    float flexGrow = 0.0f;
    float flexShrink = 1.0f;
    float flexBasis = -1.0f;
    std::string flexBasisString;
    FlexAlign alignSelf = FlexAlign::Auto;
    std::string width;
    std::string height;
    std::string minWidth;
    std::string maxWidth;
    std::string minHeight;
    std::string maxHeight;
    float marginTop = 0.0f;
    float marginBottom = 0.0f;
    float marginLeft = 0.0f;
    float marginRight = 0.0f;
};

class Container;
class Flexbox;

struct FlexboxItem {
    FlexItemKind kind = FlexItemKind::Text;
    FlexItemProps props;
    Text text;
    Button button;
    Input input;
    Scrollbar scrollbar;
    std::shared_ptr<Container> container;
    std::shared_ptr<Flexbox> flexbox;
    Svg svg;
    BrandLogo brandLogo;
    Robo::TitleBar titleBar;
    Robo::Sidebar sidebar;
    Robo::SidebarItem sidebarItem;
    Robo::Separator separator;
    ThemeToggle themeToggle;
    ThemeIconToggle themeIconToggle;
    MotionShape motionShape;
};

struct FlexboxBounds {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    bool Contains(float pointX, float pointY) const;
};

struct FlexboxStyle {
    FlexDirection direction = FlexDirection::Row;
    FlexJustify justify = FlexJustify::FlexStart;
    FlexAlign align = FlexAlign::Stretch;
    FlexWrap wrap = FlexWrap::NoWrap;
    float gap = 0.0f;
    float rowGap = -1.0f;
    float columnGap = -1.0f;

    std::string width = "auto";
    std::string height = "auto";
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

class Flexbox {
public:
    Flexbox();

    void SetStyle(const FlexboxStyle& style);
    const FlexboxStyle& GetStyle() const;
    FlexboxStyle& GetStyle();

    void AddText(Text text, const FlexItemProps& props = {});
    void AddButton(Button button, const FlexItemProps& props = {});
    void AddInput(Input input, const FlexItemProps& props = {});
    void AddScrollbar(Scrollbar scrollbar, const FlexItemProps& props = {});
    void AddContainer(Container container, const FlexItemProps& props = {});
    void AddFlexbox(Flexbox flexbox, const FlexItemProps& props = {});
    void AddSvg(Svg svg, const FlexItemProps& props = {});
    void AddBrandLogo(BrandLogo logo, const FlexItemProps& props = {});
    void AddTitleBar(Robo::TitleBar bar, const FlexItemProps& props = {});
    void AddSidebar(Robo::Sidebar pane, const FlexItemProps& props = {});
    void AddSidebarItem(Robo::SidebarItem item, const FlexItemProps& props = {});
    void AddSeparator(Robo::Separator separator, const FlexItemProps& props = {});
    void AddThemeToggle(ThemeToggle toggle, const FlexItemProps& props = {});
    void AddThemeIconToggle(ThemeIconToggle toggle, const FlexItemProps& props = {});
    void AddMotionShape(MotionShape shape, const FlexItemProps& props = {});
    void AddItem(FlexboxItem item);

    const std::vector<FlexboxItem>& GetItems() const;
    std::vector<FlexboxItem>& GetItems();

    void SetBounds(const FlexboxBounds& bounds);
    const FlexboxBounds& GetBounds() const;

    float ResolveGapX(float pixelScale) const;
    float ResolveGapY(float pixelScale) const;

    bool HasMeasureCache(float containerWidth, float pixelScale, float forcedWidth, float forcedHeight) const;
    void ReadMeasureCache(float& width, float& height) const;
    void WriteMeasureCache(float containerWidth, float pixelScale, float forcedWidth, float forcedHeight,
                           float width, float height) const;
    void InvalidateMeasureCache() const;

private:
    FlexboxStyle style_;
    std::vector<FlexboxItem> items_;
    FlexboxBounds bounds_;
    mutable bool measureCacheValid_ = false;
    mutable float measureCacheContainer_ = -1.0f;
    mutable float measureCacheScale_ = -1.0f;
    mutable float measureCacheForcedW_ = -1.0f;
    mutable float measureCacheForcedH_ = -1.0f;
    mutable float measureCacheWidth_ = 0.0f;
    mutable float measureCacheHeight_ = 0.0f;
};

FlexDirection ParseFlexDirection(const std::string& value);
FlexJustify ParseFlexJustify(const std::string& value);
FlexAlign ParseFlexAlign(const std::string& value);
FlexWrap ParseFlexWrap(const std::string& value);

}  // namespace Robo

