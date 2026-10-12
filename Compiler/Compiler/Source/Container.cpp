#include "Robo/Container.hpp"

#include <algorithm>
#include <utility>

namespace Robo {

ContainerOrientation ParseContainerOrientation(const std::string& value) {
    if (value == "horizontal" || value == "h" || value == "row" || value == "x") {
        return ContainerOrientation::Horizontal;
    }
    return ContainerOrientation::Vertical;
}

ContainerLayout ParseContainerLayout(const std::string& value) {
    if (value == "flex" || value == "flexbox") {
        return ContainerLayout::Flex;
    }
    if (value == "grid") {
        return ContainerLayout::Grid;
    }
    if (value == "position" || value == "absolute") {
        return ContainerLayout::Position;
    }
    return ContainerLayout::Stack;
}

void Container::SetStyle(const ContainerStyle& style) {
    style_ = style;
    scrollbar_.SetStyle(style_.scrollbarStyle);
}

const ContainerStyle& Container::GetStyle() const {
    return style_;
}

void Container::SetShownWhen(const std::string& paneId) {
    shownWhen_ = paneId;
}

const std::string& Container::ShownWhen() const {
    return shownWhen_;
}

ContainerStyle& Container::GetStyle() {
    return style_;
}

Position& Container::GetContent() {
    return content_;
}

const Position& Container::GetContent() const {
    return content_;
}

Flexbox& Container::GetFlexbox() {
    return flexbox_;
}

const Flexbox& Container::GetFlexbox() const {
    return flexbox_;
}

void Container::SetFlexbox(Flexbox flexbox) {
    flexbox_ = std::move(flexbox);
}

Grid& Container::GetGrid() {
    return grid_;
}

const Grid& Container::GetGrid() const {
    return grid_;
}

void Container::SetGrid(Grid grid) {
    grid_ = std::move(grid);
}

Scrollbar& Container::GetScrollbar() {
    return scrollbar_;
}

const Scrollbar& Container::GetScrollbar() const {
    return scrollbar_;
}

void Container::SetBounds(const InputBounds& bounds) {
    bounds_ = bounds;
}

const InputBounds& Container::GetBounds() const {
    return bounds_;
}

void Container::SetPixelScale(float pixelScale) {
    pixelScale_ = std::max(0.5f, pixelScale);
}

float Container::GetPixelScale() const {
    return pixelScale_;
}

void Container::SetContentExtent(double extent) {
    style_.contentExtent = std::max(0.0, extent);
}

double Container::GetContentExtent() const {
    return style_.contentExtent;
}

// Both extents live on the scrollbar, so the bar stays the single source of
// truth for what is scrollable and how far the content can move.
double Container::GetResolvedContentExtent() const {
    if (scrollbar_.GetContentSize() > 0.0) {
        return scrollbar_.GetContentSize();
    }
    return resolvedContentExtent_;
}

double Container::ResolveViewportExtent() const {
    if (scrollbar_.GetViewportSize() > 0.0) {
        return scrollbar_.GetViewportSize();
    }
    const bool vertical = style_.orientation == ContainerOrientation::Vertical;
    const double length = vertical ? static_cast<double>(bounds_.height) : static_cast<double>(bounds_.width);
    const double padStart = vertical ? style_.paddingTop : style_.paddingLeft;
    const double padEnd = vertical ? style_.paddingBottom : style_.paddingRight;
    return std::max(0.0, length - padStart - padEnd);
}

void Container::SetScrollValue(double value) {
    scrollbar_.SetValue(value);
}

double Container::GetScrollValue() const {
    return scrollbar_.GetValue();
}

bool Container::ScrollByLines(double lines) {
    return scrollbar_.ScrollByLines(lines);
}

bool Container::ScrollByPage(double pages) {
    return scrollbar_.ScrollByPage(pages);
}

bool Container::ScrollByPixels(double pixels) {
    return scrollbar_.ScrollByPixels(pixels);
}

bool Container::ScrollToStart() {
    return scrollbar_.ScrollToStart();
}

bool Container::ScrollToEnd() {
    return scrollbar_.ScrollToEnd();
}

double Container::GetMaxScrollOffset() const {
    return std::max(0.0, GetResolvedContentExtent() - ResolveViewportExtent());
}

double Container::GetScrollOffset() const {
    return GetScrollValue() * GetMaxScrollOffset();
}

bool Container::IsOverflowing() const {
    return GetResolvedContentExtent() > ResolveViewportExtent() + 0.0001;
}

bool Container::IsScrollbarVisible() const {
    if (!style_.showScrollbar) {
        return false;
    }
    return scrollbar_.IsVisible();
}

void Container::SyncScrollbar(double contentExtent, double viewportExtent, const ScrollbarBounds& track) {
    resolvedContentExtent_ = std::max(0.0, contentExtent);
    scrollbar_.SetOrientation(style_.orientation == ContainerOrientation::Vertical
                                  ? ScrollbarOrientation::Vertical
                                  : ScrollbarOrientation::Horizontal);
    scrollbar_.SetContentSize(style_.contentExtent > 0.0 ? style_.contentExtent : resolvedContentExtent_);
    scrollbar_.SetViewportSize(style_.viewportExtent > 0.0 ? style_.viewportExtent : viewportExtent);
    scrollbar_.SetPixelScale(pixelScale_);
    scrollbar_.SetBounds(track);
}

void Container::RefreshVariantDefaults() {
    scrollbar_.SetStyle(style_.scrollbarStyle);
    scrollbar_.ApplyVariantDefaults();
    style_.scrollbarStyle = scrollbar_.GetStyle();
}

}  // namespace Robo
