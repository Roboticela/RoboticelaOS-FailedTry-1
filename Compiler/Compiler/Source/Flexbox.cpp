#include "Robo/Flexbox.hpp"
#include "Robo/Container.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

namespace Robo {

bool FlexboxBounds::Contains(float pointX, float pointY) const {
    return pointX >= x && pointX <= (x + width) && pointY >= y && pointY <= (y + height);
}

Flexbox::Flexbox() = default;

void Flexbox::SetStyle(const FlexboxStyle& style) {
    style_ = style;
    InvalidateMeasureCache();
}

const FlexboxStyle& Flexbox::GetStyle() const {
    return style_;
}

FlexboxStyle& Flexbox::GetStyle() {
    return style_;
}

void Flexbox::AddText(Text text, const FlexItemProps& props) {
    FlexboxItem item;
    item.kind = FlexItemKind::Text;
    item.text = std::make_shared<Text>(std::move(text));
    item.props = props;
    items_.push_back(std::move(item));
}

void Flexbox::AddSvg(Svg svg, const FlexItemProps& props) {
    FlexboxItem item;
    item.kind = FlexItemKind::Svg;
    item.svg = std::make_shared<Svg>(std::move(svg));
    item.props = props;
    items_.push_back(std::move(item));
}

void Flexbox::AddBrandLogo(BrandLogo logo, const FlexItemProps& props) {
    FlexboxItem item;
    item.kind = FlexItemKind::BrandLogo;
    item.brandLogo = std::make_shared<BrandLogo>(std::move(logo));
    item.props = props;
    items_.push_back(std::move(item));
}

void Flexbox::AddTitleBar(TitleBar bar, const FlexItemProps& props) {
    FlexboxItem item;
    item.kind = FlexItemKind::TitleBar;
    item.titleBar = std::make_shared<TitleBar>(std::move(bar));
    item.props = props;
    items_.push_back(std::move(item));
}

void Flexbox::AddSidebar(Sidebar pane, const FlexItemProps& props) {
    FlexboxItem item;
    item.kind = FlexItemKind::Sidebar;
    item.sidebar = std::make_shared<Sidebar>(std::move(pane));
    item.props = props;
    items_.push_back(std::move(item));
}

void Flexbox::AddSidebarItem(SidebarItem item, const FlexItemProps& props) {
    FlexboxItem entry;
    entry.kind = FlexItemKind::SidebarItem;
    entry.sidebarItem = std::make_shared<SidebarItem>(std::move(item));
    entry.props = props;
    items_.push_back(std::move(entry));
}

void Flexbox::AddThemeToggle(ThemeToggle toggle, const FlexItemProps& props) {
    FlexboxItem item;
    item.kind = FlexItemKind::ThemeToggle;
    item.themeToggle = std::make_shared<ThemeToggle>(std::move(toggle));
    item.props = props;
    items_.push_back(std::move(item));
}

void Flexbox::AddThemeIconToggle(ThemeIconToggle toggle, const FlexItemProps& props) {
    FlexboxItem item;
    item.kind = FlexItemKind::ThemeIconToggle;
    item.themeIconToggle = std::make_shared<ThemeIconToggle>(std::move(toggle));
    item.props = props;
    items_.push_back(std::move(item));
}

void Flexbox::AddSeparator(Separator separator, const FlexItemProps& props) {
    FlexboxItem item;
    item.kind = FlexItemKind::Separator;
    item.separator = std::make_shared<Separator>(std::move(separator));
    item.props = props;
    items_.push_back(std::move(item));
}

void Flexbox::AddMotionShape(MotionShape shape, const FlexItemProps& props) {
    FlexboxItem item;
    item.kind = FlexItemKind::MotionShape;
    item.motionShape = std::make_shared<MotionShape>(std::move(shape));
    item.props = props;
    items_.push_back(std::move(item));
}

void Flexbox::AddButton(Button button, const FlexItemProps& props) {
    FlexboxItem item;
    item.kind = FlexItemKind::Button;
    item.button = std::make_shared<Button>(std::move(button));
    item.props = props;
    items_.push_back(std::move(item));
}

void Flexbox::AddInput(Input input, const FlexItemProps& props) {
    FlexboxItem item;
    item.kind = FlexItemKind::Input;
    item.input = std::make_shared<Input>(std::move(input));
    item.props = props;
    items_.push_back(std::move(item));
}

void Flexbox::AddScrollbar(Scrollbar scrollbar, const FlexItemProps& props) {
    FlexboxItem item;
    item.kind = FlexItemKind::Scrollbar;
    item.scrollbar = std::make_shared<Scrollbar>(std::move(scrollbar));
    item.props = props;
    items_.push_back(std::move(item));
}

void Flexbox::AddFlexbox(Flexbox flexbox, const FlexItemProps& props) {
    FlexboxItem item;
    item.kind = FlexItemKind::Flexbox;
    item.flexbox = std::make_shared<Flexbox>(std::move(flexbox));
    item.props = props;
    items_.push_back(std::move(item));
}

void Flexbox::AddContainer(Container container, const FlexItemProps& props) {
    FlexboxItem item;
    item.kind = FlexItemKind::Container;
    item.props = props;
    item.container = std::make_shared<Container>(std::move(container));
    items_.push_back(std::move(item));
}

void Flexbox::AddItem(FlexboxItem item) {
    items_.push_back(std::move(item));
    InvalidateMeasureCache();
}

bool Flexbox::HasMeasureCache(float containerWidth, float pixelScale, float forcedWidth, float forcedHeight) const {
    return measureCacheValid_ && std::fabs(measureCacheContainer_ - containerWidth) < 0.5f &&
           std::fabs(measureCacheScale_ - pixelScale) < 0.001f &&
           std::fabs(measureCacheForcedW_ - forcedWidth) < 0.5f &&
           std::fabs(measureCacheForcedH_ - forcedHeight) < 0.5f;
}

void Flexbox::ReadMeasureCache(float& width, float& height) const {
    width = measureCacheWidth_;
    height = measureCacheHeight_;
}

void Flexbox::WriteMeasureCache(float containerWidth, float pixelScale, float forcedWidth, float forcedHeight,
                                float width, float height) const {
    measureCacheValid_ = true;
    measureCacheContainer_ = containerWidth;
    measureCacheScale_ = pixelScale;
    measureCacheForcedW_ = forcedWidth;
    measureCacheForcedH_ = forcedHeight;
    measureCacheWidth_ = width;
    measureCacheHeight_ = height;
}

void Flexbox::InvalidateMeasureCache() const {
    measureCacheValid_ = false;
    layoutCache_.valid = false;
    layoutCache_.layout = ComputedFlexLayout{};
    needsLayout_ = true;
    needsPaint_ = true;
}

bool Flexbox::HasLayoutCache(float containerWidth, float pixelScale, float forcedWidth,
                             float forcedHeight) const {
    return layoutCache_.valid && !needsLayout_ &&
           std::fabs(layoutCache_.key.containerWidth - containerWidth) < 0.5f &&
           std::fabs(layoutCache_.key.pixelScale - pixelScale) < 0.001f &&
           std::fabs(layoutCache_.key.forcedWidth - forcedWidth) < 0.5f &&
           std::fabs(layoutCache_.key.forcedHeight - forcedHeight) < 0.5f;
}

const ComputedFlexLayout& Flexbox::LayoutCache() const {
    return layoutCache_.layout;
}

void Flexbox::StoreLayoutCache(float containerWidth, float pixelScale, float forcedWidth,
                               float forcedHeight, ComputedFlexLayout layout) const {
    layoutCache_.layout = std::move(layout);
    layoutCache_.key.containerWidth = containerWidth;
    layoutCache_.key.pixelScale = pixelScale;
    layoutCache_.key.forcedWidth = forcedWidth;
    layoutCache_.key.forcedHeight = forcedHeight;
    layoutCache_.valid = true;
    needsLayout_ = false;
    measureCacheValid_ = true;
    measureCacheContainer_ = containerWidth;
    measureCacheScale_ = pixelScale;
    measureCacheForcedW_ = forcedWidth;
    measureCacheForcedH_ = forcedHeight;
    measureCacheWidth_ = layoutCache_.layout.containerWidth;
    measureCacheHeight_ = layoutCache_.layout.containerHeight;
}

bool Flexbox::NeedsLayout() const {
    return needsLayout_ || !layoutCache_.valid;
}

void Flexbox::SetNeedsLayout(bool needs) const {
    needsLayout_ = needs;
    if (needs) {
        layoutCache_.valid = false;
        measureCacheValid_ = false;
        needsPaint_ = true;
    }
}

bool Flexbox::NeedsPaint() const {
    return needsPaint_;
}

void Flexbox::SetNeedsPaint(bool needs) const {
    needsPaint_ = needs;
}

const std::vector<FlexboxItem>& Flexbox::GetItems() const {
    return items_;
}

std::vector<FlexboxItem>& Flexbox::GetItems() {
    return items_;
}

void Flexbox::SetBounds(const FlexboxBounds& bounds) {
    bounds_ = bounds;
}

const FlexboxBounds& Flexbox::GetBounds() const {
    return bounds_;
}

float Flexbox::ResolveGapX(float pixelScale) const {
    if (style_.columnGap >= 0.0f) {
        return style_.columnGap * pixelScale;
    }
    return style_.gap * pixelScale;
}

float Flexbox::ResolveGapY(float pixelScale) const {
    if (style_.rowGap >= 0.0f) {
        return style_.rowGap * pixelScale;
    }
    return style_.gap * pixelScale;
}

FlexDirection ParseFlexDirection(const std::string& value) {
    if (value == "column" || value == "col") {
        return FlexDirection::Column;
    }
    if (value == "column-reverse" || value == "col-reverse") {
        return FlexDirection::ColumnReverse;
    }
    if (value == "row-reverse") {
        return FlexDirection::RowReverse;
    }
    return FlexDirection::Row;
}

FlexJustify ParseFlexJustify(const std::string& value) {
    if (value == "center") {
        return FlexJustify::Center;
    }
    if (value == "flex-end" || value == "end" || value == "right") {
        return FlexJustify::FlexEnd;
    }
    if (value == "space-between" || value == "between") {
        return FlexJustify::SpaceBetween;
    }
    if (value == "space-around" || value == "around") {
        return FlexJustify::SpaceAround;
    }
    if (value == "space-evenly" || value == "evenly") {
        return FlexJustify::SpaceEvenly;
    }
    return FlexJustify::FlexStart;
}

FlexAlign ParseFlexAlign(const std::string& value) {
    if (value == "flex-start" || value == "start" || value == "top") {
        return FlexAlign::FlexStart;
    }
    if (value == "center" || value == "middle") {
        return FlexAlign::Center;
    }
    if (value == "flex-end" || value == "end" || value == "bottom") {
        return FlexAlign::FlexEnd;
    }
    if (value == "auto") {
        return FlexAlign::Auto;
    }
    return FlexAlign::Stretch;
}

FlexWrap ParseFlexWrap(const std::string& value) {
    if (value == "wrap") {
        return FlexWrap::Wrap;
    }
    if (value == "wrap-reverse") {
        return FlexWrap::WrapReverse;
    }
    return FlexWrap::NoWrap;
}

}  // namespace Robo
