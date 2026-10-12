#pragma once

#include "Color.hpp"
#include "Selectable.hpp"
#include "Flexbox.hpp"
#include "Grid.hpp"
#include "Position.hpp"
#include "Scrollbar.hpp"

#include <string>

namespace Robo {

// The axis the container scrolls along. Children are stacked along it by
// default, and the owned scrollbar is laid out parallel to it.
enum class ContainerOrientation {
    Vertical,
    Horizontal
};

// How children are arranged inside the clipped region. Stack is the block
// flow. Flex, Grid and Position reuse those layout engines and still scroll.
enum class ContainerLayout {
    Stack,
    Flex,
    Grid,
    Position
};

struct ContainerStyle {
    ContainerOrientation orientation = ContainerOrientation::Vertical;
    ContainerLayout layout = ContainerLayout::Stack;

    std::string width = "auto";
    std::string height = "auto";
    float minWidth = 0.0f;
    float maxWidth = 100000.0f;
    float minHeight = 0.0f;
    float maxHeight = 100000.0f;

    float paddingTop = 0.0f;
    float paddingRight = 0.0f;
    float paddingBottom = 0.0f;
    float paddingLeft = 0.0f;

    // Space between stacked children.
    float gap = 0.0f;

    Color backgroundColor = Color::FromRgba(0, 0, 0, 0);
    Color borderColor = Color::FromRgba(0, 0, 0, 0);
    float borderWidth = 0.0f;
    float cornerRadius = 0.0f;
    float opacity = 1.0f;
    SelectableMode selectable = SelectableMode::Inherit;

    // Clip the content to the body rect. Only scrolling regions need this off.
    bool clipToBounds = true;

    // The container scrolls exclusively through the scrollbar it owns.
    bool showScrollbar = true;
    double scrollbarGap = 0.0f;

    // Explicit extents; 0 means "derive from the stacked content / viewport".
    double contentExtent = 0.0;
    double viewportExtent = 0.0;

    ScrollbarStyle scrollbarStyle;
    std::map<std::string, std::string> tokens;
};

// A clipped, scrollable region. The container itself holds no scroll state: the
// scroll offset is always the value of the scrollbar it owns, so dragging the
// bar, wheeling, keyboard steps and touch scrolling all drive the same value.
class Container {
public:
    Container() = default;
    ~Container() = default;

    void SetStyle(const ContainerStyle& style);
    const ContainerStyle& GetStyle() const;

    void SetShownWhen(const std::string& paneId);
    const std::string& ShownWhen() const;
    ContainerStyle& GetStyle();

    // Stack and Position layouts keep children here. Flex and Grid keep them
    // on the matching engine; hit testing follows GetStyle().layout.
    Position& GetContent();
    const Position& GetContent() const;

    Flexbox& GetFlexbox();
    const Flexbox& GetFlexbox() const;
    void SetFlexbox(Flexbox flexbox);

    Grid& GetGrid();
    const Grid& GetGrid() const;
    void SetGrid(Grid grid);

    Scrollbar& GetScrollbar();
    const Scrollbar& GetScrollbar() const;

    void SetBounds(const InputBounds& bounds);
    const InputBounds& GetBounds() const;

    void SetPixelScale(float pixelScale);
    float GetPixelScale() const;

    // Extents along the scroll axis, in layout pixels.
    void SetContentExtent(double extent);
    double GetContentExtent() const;  // Explicit override, 0 when automatic.
    double GetResolvedContentExtent() const;
    double ResolveViewportExtent() const;

    // Scroll state; every one of these delegates to the owned scrollbar.
    void SetScrollValue(double value);
    double GetScrollValue() const;
    bool ScrollByLines(double lines);
    bool ScrollByPage(double pages);
    bool ScrollByPixels(double pixels);
    bool ScrollToStart();
    bool ScrollToEnd();

    double GetMaxScrollOffset() const;
    double GetScrollOffset() const;

    bool IsOverflowing() const;
    bool IsScrollbarVisible() const;

    // Pushes the resolved extents and track rect into the scrollbar and mirrors
    // its interaction state back, keeping the bar the single source of truth.
    void SyncScrollbar(double contentExtent, double viewportExtent, const ScrollbarBounds& track);

    void RefreshVariantDefaults();

private:
    ContainerStyle style_;
    Position content_;
    Flexbox flexbox_;
    Grid grid_;
    Scrollbar scrollbar_;
    InputBounds bounds_;
    std::string shownWhen_;
    double resolvedContentExtent_ = 0.0;
    float pixelScale_ = 1.0f;
};

ContainerOrientation ParseContainerOrientation(const std::string& value);
ContainerLayout ParseContainerLayout(const std::string& value);

}  // namespace Robo
