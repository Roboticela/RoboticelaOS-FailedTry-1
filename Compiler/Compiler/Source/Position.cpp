#include "Robo/Position.hpp"
#include "Robo/Container.hpp"
#include "Robo/Flexbox.hpp"
#include "Robo/Grid.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <memory>
#include <utility>

namespace Robo {

namespace {

std::string ToLower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return str;
}

std::string Trim(const std::string& str) {
    const std::size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    const std::size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

}  // namespace

Position::Position() = default;
Position::~Position() = default;

Position::Position(const Position& other) : style_(other.style_), bounds_(other.bounds_) {
    items_.reserve(other.items_.size());
    for (const PositionItem& item : other.items_) {
        PositionItem copy = item;
        if (item.flexbox) {
            copy.flexbox = std::make_shared<Flexbox>(*item.flexbox);
        }
        if (item.grid) {
            copy.grid = std::make_shared<Grid>(*item.grid);
        }
        if (item.position) {
            copy.position = std::make_shared<Position>(*item.position);
        }
        items_.push_back(std::move(copy));
    }
}

Position& Position::operator=(const Position& other) {
    if (this == &other) {
        return *this;
    }
    style_ = other.style_;
    bounds_ = other.bounds_;
    items_.clear();
    items_.reserve(other.items_.size());
    for (const PositionItem& item : other.items_) {
        PositionItem copy = item;
        if (item.flexbox) {
            copy.flexbox = std::make_shared<Flexbox>(*item.flexbox);
        }
        if (item.grid) {
            copy.grid = std::make_shared<Grid>(*item.grid);
        }
        if (item.position) {
            copy.position = std::make_shared<Position>(*item.position);
        }
        items_.push_back(std::move(copy));
    }
    return *this;
}

Position::Position(Position&& other) noexcept = default;
Position& Position::operator=(Position&& other) noexcept = default;

void Position::SetStyle(const PositionStyle& style) {
    style_ = style;
    InvalidateLayoutCache();
}

const PositionStyle& Position::GetStyle() const {
    return style_;
}

PositionStyle& Position::GetStyle() {
    return style_;
}

void Position::AddText(Text text, const PositionItemProps& props) {
    PositionItem item;
    item.kind = PositionItemKind::Text;
    item.props = props;
    item.text = std::make_shared<Text>(std::move(text));
    items_.push_back(std::move(item));
}

void Position::AddSvg(Svg svg, const PositionItemProps& props) {
    PositionItem item;
    item.kind = PositionItemKind::Svg;
    item.props = props;
    item.svg = std::make_shared<Svg>(std::move(svg));
    items_.push_back(std::move(item));
}

void Position::AddBrandLogo(BrandLogo logo, const PositionItemProps& props) {
    PositionItem item;
    item.kind = PositionItemKind::BrandLogo;
    item.props = props;
    item.brandLogo = std::make_shared<BrandLogo>(std::move(logo));
    items_.push_back(std::move(item));
}

void Position::AddTitleBar(TitleBar bar, const PositionItemProps& props) {
    PositionItem item;
    item.kind = PositionItemKind::TitleBar;
    item.props = props;
    item.titleBar = std::make_shared<TitleBar>(std::move(bar));
    items_.push_back(std::move(item));
}

void Position::AddSidebar(Sidebar pane, const PositionItemProps& props) {
    PositionItem item;
    item.kind = PositionItemKind::Sidebar;
    item.props = props;
    item.sidebar = std::make_shared<Sidebar>(std::move(pane));
    items_.push_back(std::move(item));
}

void Position::AddSidebarItem(SidebarItem item, const PositionItemProps& props) {
    PositionItem entry;
    entry.kind = PositionItemKind::SidebarItem;
    entry.props = props;
    entry.sidebarItem = std::make_shared<SidebarItem>(std::move(item));
    items_.push_back(std::move(entry));
}

void Position::AddThemeToggle(ThemeToggle toggle, const PositionItemProps& props) {
    PositionItem item;
    item.kind = PositionItemKind::ThemeToggle;
    item.props = props;
    item.themeToggle = std::make_shared<ThemeToggle>(std::move(toggle));
    items_.push_back(std::move(item));
}

void Position::AddThemeIconToggle(ThemeIconToggle toggle, const PositionItemProps& props) {
    PositionItem item;
    item.kind = PositionItemKind::ThemeIconToggle;
    item.props = props;
    item.themeIconToggle = std::make_shared<ThemeIconToggle>(std::move(toggle));
    items_.push_back(std::move(item));
}

void Position::AddSeparator(Separator separator, const PositionItemProps& props) {
    PositionItem item;
    item.kind = PositionItemKind::Separator;
    item.props = props;
    item.separator = std::make_shared<Separator>(std::move(separator));
    items_.push_back(std::move(item));
}

void Position::AddMotionShape(MotionShape shape, const PositionItemProps& props) {
    PositionItem item;
    item.kind = PositionItemKind::MotionShape;
    item.props = props;
    item.motionShape = std::make_shared<MotionShape>(std::move(shape));
    items_.push_back(std::move(item));
}

void Position::AddButton(Button button, const PositionItemProps& props) {
    PositionItem item;
    item.kind = PositionItemKind::Button;
    item.props = props;
    item.button = std::make_shared<Button>(std::move(button));
    items_.push_back(std::move(item));
}

void Position::AddInput(Input input, const PositionItemProps& props) {
    PositionItem item;
    item.kind = PositionItemKind::Input;
    item.props = props;
    item.input = std::make_shared<Input>(std::move(input));
    items_.push_back(std::move(item));
}

void Position::AddScrollbar(Scrollbar scrollbar, const PositionItemProps& props) {
    PositionItem item;
    item.kind = PositionItemKind::Scrollbar;
    item.props = props;
    item.scrollbar = std::make_shared<Scrollbar>(std::move(scrollbar));
    items_.push_back(std::move(item));
}

void Position::AddContainer(Container container, const PositionItemProps& props) {
    PositionItem item;
    item.kind = PositionItemKind::Container;
    item.props = props;
    item.container = std::make_shared<Container>(std::move(container));
    items_.push_back(std::move(item));
}

void Position::AddFlexbox(Flexbox flexbox, const PositionItemProps& props) {
    PositionItem item;
    item.kind = PositionItemKind::Flexbox;
    item.props = props;
    item.flexbox = std::make_shared<Flexbox>(std::move(flexbox));
    items_.push_back(std::move(item));
}

void Position::AddGrid(Grid grid, const PositionItemProps& props) {
    PositionItem item;
    item.kind = PositionItemKind::Grid;
    item.props = props;
    item.grid = std::make_shared<Grid>(std::move(grid));
    items_.push_back(std::move(item));
}

void Position::AddPosition(Position position, const PositionItemProps& props) {
    PositionItem item;
    item.kind = PositionItemKind::Position;
    item.props = props;
    item.position = std::make_shared<Position>(std::move(position));
    items_.push_back(std::move(item));
}

const std::vector<PositionItem>& Position::GetItems() const {
    return items_;
}

std::vector<PositionItem>& Position::GetItems() {
    return items_;
}

void Position::SetBounds(const InputBounds& bounds) {
    bounds_ = bounds;
}

const InputBounds& Position::GetBounds() const {
    return bounds_;
}

void Position::Clear() {
    items_.clear();
    InvalidateLayoutCache();
}

bool Position::HasLayoutCache(float availableWidth, float pixelScale, float forcedWidth,
                              float forcedHeight) const {
    return layoutCache_.valid && !needsLayout_ &&
           std::fabs(layoutCache_.key.availableWidth - availableWidth) < 0.5f &&
           std::fabs(layoutCache_.key.pixelScale - pixelScale) < 0.001f &&
           std::fabs(layoutCache_.key.forcedWidth - forcedWidth) < 0.5f &&
           std::fabs(layoutCache_.key.forcedHeight - forcedHeight) < 0.5f;
}

const ComputedPositionLayout& Position::LayoutCache() const {
    return layoutCache_.layout;
}

void Position::StoreLayoutCache(float availableWidth, float pixelScale, float forcedWidth,
                                float forcedHeight, ComputedPositionLayout layout) const {
    layoutCache_.layout = std::move(layout);
    layoutCache_.key.availableWidth = availableWidth;
    layoutCache_.key.pixelScale = pixelScale;
    layoutCache_.key.forcedWidth = forcedWidth;
    layoutCache_.key.forcedHeight = forcedHeight;
    layoutCache_.valid = true;
    needsLayout_ = false;
}

void Position::InvalidateLayoutCache() const {
    layoutCache_.valid = false;
    layoutCache_.layout = ComputedPositionLayout{};
    needsLayout_ = true;
    needsPaint_ = true;
}

bool Position::NeedsLayout() const {
    return needsLayout_ || !layoutCache_.valid;
}

void Position::SetNeedsLayout(bool needs) const {
    needsLayout_ = needs;
    if (needs) {
        layoutCache_.valid = false;
        needsPaint_ = true;
    }
}

bool Position::NeedsPaint() const {
    return needsPaint_;
}

void Position::SetNeedsPaint(bool needs) const {
    needsPaint_ = needs;
}

PositionMode ParsePositionMode(const std::string& str) {
    const std::string s = ToLower(Trim(str));
    if (s == "relative") {
        return PositionMode::Relative;
    }
    if (s == "fixed") {
        return PositionMode::Fixed;
    }
    return PositionMode::Absolute;
}

PositionAnchor ParsePositionAnchor(const std::string& str) {
    const std::string s = ToLower(Trim(str));
    if (s == "topright" || s == "top-right" || s == "top_right") {
        return PositionAnchor::TopRight;
    }
    if (s == "bottomleft" || s == "bottom-left" || s == "bottom_left") {
        return PositionAnchor::BottomLeft;
    }
    if (s == "bottomright" || s == "bottom-right" || s == "bottom_right") {
        return PositionAnchor::BottomRight;
    }
    if (s == "center") {
        return PositionAnchor::Center;
    }
    if (s == "topcenter" || s == "top-center" || s == "top_center") {
        return PositionAnchor::TopCenter;
    }
    if (s == "bottomcenter" || s == "bottom-center" || s == "bottom_center") {
        return PositionAnchor::BottomCenter;
    }
    return PositionAnchor::TopLeft;
}

}  // namespace Robo
