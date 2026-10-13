#include "Robo/RoboScene.hpp"
#include "Robo/Motion.hpp"
#include "Robo/TextSelection.hpp"
#include "Robo/BrandLogoPainter.hpp"
#include "Robo/BackgroundPainter.hpp"
#include "Robo/DesktopPainter.hpp"
#include "Robo/Foundation.hpp"
#include "Robo/TitleBarPainter.hpp"
#include "Robo/SidebarPainter.hpp"
#include "Robo/SeparatorPainter.hpp"
#include "Robo/ThemeIconTogglePainter.hpp"
#include "Robo/ThemeTogglePainter.hpp"
#include "Robo/ButtonPainter.hpp"
#include "Robo/ContainerPainter.hpp"
#include "Robo/FlexboxPainter.hpp"
#include "Robo/GridPainter.hpp"
#include "Robo/InputPainter.hpp"
#include "Robo/PositionPainter.hpp"
#include "Robo/ScrollbarPainter.hpp"
#include "Robo/SvgPainter.hpp"
#include "Robo/TextPainter.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>

namespace Robo {

namespace {

void ScaleBounds(ButtonBounds& bounds, float inv) {
    bounds.x *= inv;
    bounds.y *= inv;
    bounds.width *= inv;
    bounds.height *= inv;
}

void ScaleBounds(InputBounds& bounds, float inv) {
    bounds.x *= inv;
    bounds.y *= inv;
    bounds.width *= inv;
    bounds.height *= inv;
}

void ScaleBounds(ScrollbarBounds& bounds, float inv) {
    bounds.x *= inv;
    bounds.y *= inv;
    bounds.width *= inv;
    bounds.height *= inv;
}

void ScaleScrollbar(Scrollbar& scrollbar, float inv) {
    ScrollbarBounds bounds = scrollbar.GetBounds();
    ScaleBounds(bounds, inv);
    scrollbar.SetBounds(bounds);
    scrollbar.SetPixelScale(scrollbar.GetPixelScale() * inv);
}

void ScaleBounds(FlexboxBounds& bounds, float inv) {
    bounds.x *= inv;
    bounds.y *= inv;
    bounds.width *= inv;
    bounds.height *= inv;
}

void ScaleFlexboxBounds(Flexbox& flexbox, float inv);
void ScaleGridBounds(Grid& grid, float inv);
void ScalePositionBounds(Position& position, float inv);
void ScaleContainerBounds(Container& container, float inv);

void ScaleBounds(GridBounds& bounds, float inv) {
    bounds.x *= inv;
    bounds.y *= inv;
    bounds.width *= inv;
    bounds.height *= inv;
}

void ScaleFlexboxBounds(Flexbox& flexbox, float inv) {
    FlexboxBounds fb = flexbox.GetBounds();
    ScaleBounds(fb, inv);
    flexbox.SetBounds(fb);

    for (FlexboxItem& item : flexbox.GetItems()) {
        if (item.kind == FlexItemKind::Button) {
            ButtonBounds bb = item.button.GetBounds();
            ScaleBounds(bb, inv);
            item.button.SetBounds(bb);
        } else if (item.kind == FlexItemKind::ThemeToggle) {
            item.themeToggle.ScaleBounds(inv);
        } else if (item.kind == FlexItemKind::ThemeIconToggle) {
            item.themeIconToggle.ScaleBounds(inv);
        } else if (item.kind == FlexItemKind::TitleBar) {
            item.titleBar.ThemeIcon().ScaleBounds(inv);
            ButtonBounds sidebarBounds = item.titleBar.SidebarButton().GetBounds();
            ScaleBounds(sidebarBounds, inv);
            item.titleBar.SidebarButton().SetBounds(sidebarBounds);
        } else if (item.kind == FlexItemKind::MotionShape) {
            item.motionShape.ScaleBounds(inv);
        } else if (item.kind == FlexItemKind::Input) {
            InputBounds ib = item.input.GetBounds();
            ScaleBounds(ib, inv);
            item.input.SetBounds(ib);
        } else if (item.kind == FlexItemKind::Scrollbar) {
            ScaleScrollbar(item.scrollbar, inv);
        } else if (item.kind == FlexItemKind::Container && item.container) {
            ScaleContainerBounds(*item.container, inv);
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            ScaleFlexboxBounds(*item.flexbox, inv);
        }
    }
}

void ScaleContainerBounds(Container& container, float inv) {
    InputBounds cb = container.GetBounds();
    ScaleBounds(cb, inv);
    container.SetBounds(cb);
    container.SetPixelScale(container.GetPixelScale() * inv);
    ScaleScrollbar(container.GetScrollbar(), inv);
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        ScaleFlexboxBounds(container.GetFlexbox(), inv);
    } else if (container.GetStyle().layout == ContainerLayout::Grid) {
        ScaleGridBounds(container.GetGrid(), inv);
    } else {
        ScalePositionBounds(container.GetContent(), inv);
    }
}

void ScaleGridBounds(Grid& grid, float inv) {
    GridBounds gb = grid.GetBounds();
    ScaleBounds(gb, inv);
    grid.SetBounds(gb);

    for (GridItem& item : grid.GetItems()) {
        if (item.kind == GridItemKind::Button) {
            ButtonBounds bb = item.button.GetBounds();
            ScaleBounds(bb, inv);
            item.button.SetBounds(bb);
        } else if (item.kind == GridItemKind::ThemeToggle) {
            item.themeToggle.ScaleBounds(inv);
        } else if (item.kind == GridItemKind::ThemeIconToggle) {
            item.themeIconToggle.ScaleBounds(inv);
        } else if (item.kind == GridItemKind::TitleBar) {
            item.titleBar.ThemeIcon().ScaleBounds(inv);
            ButtonBounds sidebarBounds = item.titleBar.SidebarButton().GetBounds();
            ScaleBounds(sidebarBounds, inv);
            item.titleBar.SidebarButton().SetBounds(sidebarBounds);
        } else if (item.kind == GridItemKind::MotionShape) {
            item.motionShape.ScaleBounds(inv);
        } else if (item.kind == GridItemKind::Input) {
            InputBounds ib = item.input.GetBounds();
            ScaleBounds(ib, inv);
            item.input.SetBounds(ib);
        } else if (item.kind == GridItemKind::Scrollbar) {
            ScaleScrollbar(item.scrollbar, inv);
        } else if (item.kind == GridItemKind::Container && item.container) {
            ScaleContainerBounds(*item.container, inv);
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            ScaleFlexboxBounds(*item.flexbox, inv);
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            ScaleGridBounds(*item.grid, inv);
        }
    }
}

template <typename Visit>
bool VisitScrollbars(Flexbox& flexbox, Visit& visit);
template <typename Visit>
bool VisitScrollbars(Grid& grid, Visit& visit);
template <typename Visit>
bool VisitScrollbars(Position& position, Visit& visit);
template <typename Visit>
bool VisitScrollbars(Container& container, Visit& visit);

// Walks every scrollbar in a nested container tree, including any living inside
// Flexbox / Grid / Position children.
template <typename Visit>
bool VisitScrollbars(Flexbox& flexbox, Visit& visit) {
    bool visited = false;
    for (FlexboxItem& item : flexbox.GetItems()) {
        if (item.kind == FlexItemKind::Scrollbar) {
            visit(item.scrollbar);
            visited = true;
        } else if (item.kind == FlexItemKind::Container && item.container) {
            visited = VisitScrollbars(*item.container, visit) || visited;
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            visited = VisitScrollbars(*item.flexbox, visit) || visited;
        }
    }
    return visited;
}

template <typename Visit>
bool VisitScrollbars(Grid& grid, Visit& visit) {
    bool visited = false;
    for (GridItem& item : grid.GetItems()) {
        if (item.kind == GridItemKind::Scrollbar) {
            visit(item.scrollbar);
            visited = true;
        } else if (item.kind == GridItemKind::Container && item.container) {
            visited = VisitScrollbars(*item.container, visit) || visited;
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            visited = VisitScrollbars(*item.flexbox, visit) || visited;
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            visited = VisitScrollbars(*item.grid, visit) || visited;
        }
    }
    return visited;
}

template <typename Visit>
bool VisitScrollbars(Position& position, Visit& visit) {
    bool visited = false;
    for (PositionItem& item : position.GetItems()) {
        if (item.kind == PositionItemKind::Scrollbar) {
            visit(item.scrollbar);
            visited = true;
        } else if (item.kind == PositionItemKind::Container && item.container) {
            visited = VisitScrollbars(*item.container, visit) || visited;
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            visited = VisitScrollbars(*item.flexbox, visit) || visited;
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            visited = VisitScrollbars(*item.grid, visit) || visited;
        } else if (item.kind == PositionItemKind::Position && item.position) {
            visited = VisitScrollbars(*item.position, visit) || visited;
        }
    }
    return visited;
}

// Visits every scrollbar in the scene tree, at any nesting depth.
template <typename Visit>
void VisitSceneScrollbars(std::vector<SceneItem>& items, Visit& visit) {
    for (SceneItem& item : items) {
        if (item.kind == SceneItemKind::Scrollbar) {
            visit(item.scrollbar);
        } else if (item.kind == SceneItemKind::Container) {
            VisitScrollbars(*item.container, visit);
        } else if (item.kind == SceneItemKind::Flexbox) {
            VisitScrollbars(item.flexbox, visit);
        } else if (item.kind == SceneItemKind::Grid) {
            VisitScrollbars(item.grid, visit);
        } else if (item.kind == SceneItemKind::Position) {
            VisitScrollbars(item.position, visit);
        } else if (item.kind == SceneItemKind::Sidebar) {
            visit(item.sidebar.PanelScrollbar());
        }
    }
}

// A container owns a scrollbar, and its clipped content may hold more of them.
template <typename Visit>
bool VisitScrollbars(Container& container, Visit& visit) {
    visit(container.GetScrollbar());
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        VisitScrollbars(container.GetFlexbox(), visit);
    } else if (container.GetStyle().layout == ContainerLayout::Grid) {
        VisitScrollbars(container.GetGrid(), visit);
    } else {
        VisitScrollbars(container.GetContent(), visit);
    }
    return true;
}

// Containers can be nested at any depth, so the finders below are mutually
// recursive and need forward declarations.
Button* FindButtonInContainer(Container& container, float x, float y);
Input* FindInputInContainer(Container& container, float x, float y);
Scrollbar* FindScrollbarInContainer(Container& container, float x, float y);
Container* FindContainerInContainer(Container& container, float x, float y);
const Button* FindButtonInContainerConst(const Container& container, float x, float y);
const Input* FindInputInContainerConst(const Container& container, float x, float y);
const Scrollbar* FindScrollbarInContainerConst(const Container& container, float x, float y);
Container* FindContainerInFlexbox(Flexbox& flexbox, float x, float y);
Container* FindContainerInGrid(Grid& grid, float x, float y);
Container* FindContainerInPosition(Position& position, float x, float y);
void CollectContainersIn(Container& container, float x, float y, std::vector<Container*>& out);
void CollectContainersIn(Flexbox& flexbox, float x, float y, std::vector<Container*>& out);
void CollectContainersIn(Grid& grid, float x, float y, std::vector<Container*>& out);
void CollectContainersIn(Position& position, float x, float y, std::vector<Container*>& out);

Button* FindButtonInFlexbox(Flexbox& flexbox, float x, float y) {
    std::vector<FlexboxItem>& items = flexbox.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        FlexboxItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == FlexItemKind::Button) {
            if (item.button.GetBounds().Contains(x, y)) {
                return &item.button;
            }
        } else if (item.kind == FlexItemKind::TitleBar) {
            if (item.titleBar.SidebarButton().GetBounds().Contains(x, y)) {
                return &item.titleBar.SidebarButton();
            }
        } else if (item.kind == FlexItemKind::Container && item.container) {
            Button* found = FindButtonInContainer(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            Button* found = FindButtonInFlexbox(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

Input* FindInputInFlexbox(Flexbox& flexbox, float x, float y) {
    std::vector<FlexboxItem>& items = flexbox.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        FlexboxItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == FlexItemKind::Input) {
            if (item.input.GetBounds().Contains(x, y)) {
                return &item.input;
            }
        } else if (item.kind == FlexItemKind::Container && item.container) {
            Input* found = FindInputInContainer(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            Input* found = FindInputInFlexbox(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

Scrollbar* FindScrollbarInFlexbox(Flexbox& flexbox, float x, float y) {
    std::vector<FlexboxItem>& items = flexbox.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        FlexboxItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == FlexItemKind::Scrollbar) {
            if (item.scrollbar.IsVisible() && item.scrollbar.GetBounds().Contains(x, y)) {
                return &item.scrollbar;
            }
        } else if (item.kind == FlexItemKind::Container && item.container) {
            Scrollbar* found = FindScrollbarInContainer(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            Scrollbar* found = FindScrollbarInFlexbox(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

const Button* FindButtonInFlexboxConst(const Flexbox& flexbox, float x, float y) {
    const std::vector<FlexboxItem>& items = flexbox.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        const FlexboxItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == FlexItemKind::Button) {
            if (item.button.GetBounds().Contains(x, y)) {
                return &item.button;
            }
        } else if (item.kind == FlexItemKind::TitleBar) {
            if (item.titleBar.SidebarButton().GetBounds().Contains(x, y)) {
                return &item.titleBar.SidebarButton();
            }
        } else if (item.kind == FlexItemKind::Container && item.container) {
            const Button* found = FindButtonInContainerConst(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            const Button* found = FindButtonInFlexboxConst(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

const Input* FindInputInFlexboxConst(const Flexbox& flexbox, float x, float y) {
    const std::vector<FlexboxItem>& items = flexbox.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        const FlexboxItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == FlexItemKind::Input) {
            if (item.input.GetBounds().Contains(x, y)) {
                return &item.input;
            }
        } else if (item.kind == FlexItemKind::Container && item.container) {
            const Input* found = FindInputInContainerConst(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            const Input* found = FindInputInFlexboxConst(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

const Scrollbar* FindScrollbarInFlexboxConst(const Flexbox& flexbox, float x, float y) {
    const std::vector<FlexboxItem>& items = flexbox.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        const FlexboxItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == FlexItemKind::Scrollbar) {
            if (item.scrollbar.IsVisible() && item.scrollbar.GetBounds().Contains(x, y)) {
                return &item.scrollbar;
            }
        } else if (item.kind == FlexItemKind::Container && item.container) {
            const Scrollbar* found = FindScrollbarInContainerConst(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            const Scrollbar* found = FindScrollbarInFlexboxConst(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

Button* FindButtonInGrid(Grid& grid, float x, float y);
Input* FindInputInGrid(Grid& grid, float x, float y);
Scrollbar* FindScrollbarInGrid(Grid& grid, float x, float y);
const Button* FindButtonInGridConst(const Grid& grid, float x, float y);
const Input* FindInputInGridConst(const Grid& grid, float x, float y);
const Scrollbar* FindScrollbarInGridConst(const Grid& grid, float x, float y);

Button* FindButtonInGrid(Grid& grid, float x, float y) {
    std::vector<GridItem>& items = grid.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        GridItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == GridItemKind::Button) {
            if (item.button.GetBounds().Contains(x, y)) {
                return &item.button;
            }
        } else if (item.kind == GridItemKind::TitleBar) {
            if (item.titleBar.SidebarButton().GetBounds().Contains(x, y)) {
                return &item.titleBar.SidebarButton();
            }
        } else if (item.kind == GridItemKind::Container && item.container) {
            Button* found = FindButtonInContainer(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            Button* found = FindButtonInFlexbox(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            Button* found = FindButtonInGrid(*item.grid, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

Input* FindInputInGrid(Grid& grid, float x, float y) {
    std::vector<GridItem>& items = grid.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        GridItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == GridItemKind::Input) {
            if (item.input.GetBounds().Contains(x, y)) {
                return &item.input;
            }
        } else if (item.kind == GridItemKind::Container && item.container) {
            Input* found = FindInputInContainer(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            Input* found = FindInputInFlexbox(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            Input* found = FindInputInGrid(*item.grid, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

Scrollbar* FindScrollbarInGrid(Grid& grid, float x, float y) {
    std::vector<GridItem>& items = grid.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        GridItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == GridItemKind::Scrollbar) {
            if (item.scrollbar.IsVisible() && item.scrollbar.GetBounds().Contains(x, y)) {
                return &item.scrollbar;
            }
        } else if (item.kind == GridItemKind::Container && item.container) {
            Scrollbar* found = FindScrollbarInContainer(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            Scrollbar* found = FindScrollbarInFlexbox(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            Scrollbar* found = FindScrollbarInGrid(*item.grid, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

const Button* FindButtonInGridConst(const Grid& grid, float x, float y) {
    const std::vector<GridItem>& items = grid.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        const GridItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == GridItemKind::Button) {
            if (item.button.GetBounds().Contains(x, y)) {
                return &item.button;
            }
        } else if (item.kind == GridItemKind::TitleBar) {
            if (item.titleBar.SidebarButton().GetBounds().Contains(x, y)) {
                return &item.titleBar.SidebarButton();
            }
        } else if (item.kind == GridItemKind::Container && item.container) {
            const Button* found = FindButtonInContainerConst(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            const Button* found = FindButtonInFlexboxConst(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            const Button* found = FindButtonInGridConst(*item.grid, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

const Input* FindInputInGridConst(const Grid& grid, float x, float y) {
    const std::vector<GridItem>& items = grid.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        const GridItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == GridItemKind::Input) {
            if (item.input.GetBounds().Contains(x, y)) {
                return &item.input;
            }
        } else if (item.kind == GridItemKind::Container && item.container) {
            const Input* found = FindInputInContainerConst(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            const Input* found = FindInputInFlexboxConst(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            const Input* found = FindInputInGridConst(*item.grid, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

const Scrollbar* FindScrollbarInGridConst(const Grid& grid, float x, float y) {
    const std::vector<GridItem>& items = grid.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        const GridItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == GridItemKind::Scrollbar) {
            if (item.scrollbar.IsVisible() && item.scrollbar.GetBounds().Contains(x, y)) {
                return &item.scrollbar;
            }
        } else if (item.kind == GridItemKind::Container && item.container) {
            const Scrollbar* found = FindScrollbarInContainerConst(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            const Scrollbar* found = FindScrollbarInFlexboxConst(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            const Scrollbar* found = FindScrollbarInGridConst(*item.grid, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

void ScalePositionBounds(Position& position, float inv) {
    InputBounds pb = position.GetBounds();
    ScaleBounds(pb, inv);
    position.SetBounds(pb);

    for (PositionItem& item : position.GetItems()) {
        ScaleBounds(item.computedBounds, inv);
        if (item.kind == PositionItemKind::Button) {
            ButtonBounds bb = item.button.GetBounds();
            ScaleBounds(bb, inv);
            item.button.SetBounds(bb);
        } else if (item.kind == PositionItemKind::ThemeToggle) {
            item.themeToggle.ScaleBounds(inv);
        } else if (item.kind == PositionItemKind::ThemeIconToggle) {
            item.themeIconToggle.ScaleBounds(inv);
        } else if (item.kind == PositionItemKind::TitleBar) {
            item.titleBar.ThemeIcon().ScaleBounds(inv);
            ButtonBounds sidebarBounds = item.titleBar.SidebarButton().GetBounds();
            ScaleBounds(sidebarBounds, inv);
            item.titleBar.SidebarButton().SetBounds(sidebarBounds);
        } else if (item.kind == PositionItemKind::MotionShape) {
            item.motionShape.ScaleBounds(inv);
        } else if (item.kind == PositionItemKind::Input) {
            InputBounds ib = item.input.GetBounds();
            ScaleBounds(ib, inv);
            item.input.SetBounds(ib);
        } else if (item.kind == PositionItemKind::Scrollbar) {
            ScaleScrollbar(item.scrollbar, inv);
        } else if (item.kind == PositionItemKind::Container && item.container) {
            ScaleContainerBounds(*item.container, inv);
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            ScaleFlexboxBounds(*item.flexbox, inv);
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            ScaleGridBounds(*item.grid, inv);
        } else if (item.kind == PositionItemKind::Position && item.position) {
            ScalePositionBounds(*item.position, inv);
        }
    }
}

Button* FindButtonInPosition(Position& position, float x, float y) {
    std::vector<PositionItem>& items = position.GetItems();
    std::vector<std::size_t> indices(items.size());
    for (std::size_t i = 0; i < items.size(); ++i) {
        indices[i] = i;
    }
    std::stable_sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
        return items[a].props.zIndex > items[b].props.zIndex;
    });
    for (std::size_t idx : indices) {
        PositionItem& item = items[idx];
        if (item.kind == PositionItemKind::Button) {
            if (item.button.GetBounds().Contains(x, y)) {
                return &item.button;
            }
        } else if (item.kind == PositionItemKind::TitleBar) {
            if (item.titleBar.SidebarButton().GetBounds().Contains(x, y)) {
                return &item.titleBar.SidebarButton();
            }
        } else if (item.kind == PositionItemKind::Container && item.container) {
            Button* found = FindButtonInContainer(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            Button* found = FindButtonInFlexbox(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            Button* found = FindButtonInGrid(*item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Position && item.position) {
            Button* found = FindButtonInPosition(*item.position, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

Input* FindInputInPosition(Position& position, float x, float y) {
    std::vector<PositionItem>& items = position.GetItems();
    std::vector<std::size_t> indices(items.size());
    for (std::size_t i = 0; i < items.size(); ++i) {
        indices[i] = i;
    }
    std::stable_sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
        return items[a].props.zIndex > items[b].props.zIndex;
    });
    for (std::size_t idx : indices) {
        PositionItem& item = items[idx];
        if (item.kind == PositionItemKind::Input) {
            if (item.input.GetBounds().Contains(x, y)) {
                return &item.input;
            }
        } else if (item.kind == PositionItemKind::Container && item.container) {
            Input* found = FindInputInContainer(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            Input* found = FindInputInFlexbox(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            Input* found = FindInputInGrid(*item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Position && item.position) {
            Input* found = FindInputInPosition(*item.position, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

Scrollbar* FindScrollbarInPosition(Position& position, float x, float y) {
    std::vector<PositionItem>& items = position.GetItems();
    std::vector<std::size_t> indices(items.size());
    for (std::size_t i = 0; i < items.size(); ++i) {
        indices[i] = i;
    }
    std::stable_sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
        return items[a].props.zIndex > items[b].props.zIndex;
    });
    for (std::size_t idx : indices) {
        PositionItem& item = items[idx];
        if (item.kind == PositionItemKind::Scrollbar) {
            if (item.scrollbar.IsVisible() && item.scrollbar.GetBounds().Contains(x, y)) {
                return &item.scrollbar;
            }
        } else if (item.kind == PositionItemKind::Container && item.container) {
            Scrollbar* found = FindScrollbarInContainer(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            Scrollbar* found = FindScrollbarInFlexbox(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            Scrollbar* found = FindScrollbarInGrid(*item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Position && item.position) {
            Scrollbar* found = FindScrollbarInPosition(*item.position, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

const Button* FindButtonInPositionConst(const Position& position, float x, float y) {
    const std::vector<PositionItem>& items = position.GetItems();
    std::vector<std::size_t> indices(items.size());
    for (std::size_t i = 0; i < items.size(); ++i) {
        indices[i] = i;
    }
    std::stable_sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
        return items[a].props.zIndex > items[b].props.zIndex;
    });
    for (std::size_t idx : indices) {
        const PositionItem& item = items[idx];
        if (item.kind == PositionItemKind::Button) {
            if (item.button.GetBounds().Contains(x, y)) {
                return &item.button;
            }
        } else if (item.kind == PositionItemKind::TitleBar) {
            if (item.titleBar.SidebarButton().GetBounds().Contains(x, y)) {
                return &item.titleBar.SidebarButton();
            }
        } else if (item.kind == PositionItemKind::Container && item.container) {
            const Button* found = FindButtonInContainerConst(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            const Button* found = FindButtonInFlexboxConst(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            const Button* found = FindButtonInGridConst(*item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Position && item.position) {
            const Button* found = FindButtonInPositionConst(*item.position, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

const Input* FindInputInPositionConst(const Position& position, float x, float y) {
    const std::vector<PositionItem>& items = position.GetItems();
    std::vector<std::size_t> indices(items.size());
    for (std::size_t i = 0; i < items.size(); ++i) {
        indices[i] = i;
    }
    std::stable_sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
        return items[a].props.zIndex > items[b].props.zIndex;
    });
    for (std::size_t idx : indices) {
        const PositionItem& item = items[idx];
        if (item.kind == PositionItemKind::Input) {
            if (item.input.GetBounds().Contains(x, y)) {
                return &item.input;
            }
        } else if (item.kind == PositionItemKind::Container && item.container) {
            const Input* found = FindInputInContainerConst(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            const Input* found = FindInputInFlexboxConst(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            const Input* found = FindInputInGridConst(*item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Position && item.position) {
            const Input* found = FindInputInPositionConst(*item.position, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

const Scrollbar* FindScrollbarInPositionConst(const Position& position, float x, float y) {
    const std::vector<PositionItem>& items = position.GetItems();
    std::vector<std::size_t> indices(items.size());
    for (std::size_t i = 0; i < items.size(); ++i) {
        indices[i] = i;
    }
    std::stable_sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
        return items[a].props.zIndex > items[b].props.zIndex;
    });
    for (std::size_t idx : indices) {
        const PositionItem& item = items[idx];
        if (item.kind == PositionItemKind::Scrollbar) {
            if (item.scrollbar.IsVisible() && item.scrollbar.GetBounds().Contains(x, y)) {
                return &item.scrollbar;
            }
        } else if (item.kind == PositionItemKind::Container && item.container) {
            const Scrollbar* found = FindScrollbarInContainerConst(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            const Scrollbar* found = FindScrollbarInFlexboxConst(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            const Scrollbar* found = FindScrollbarInGridConst(*item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Position && item.position) {
            const Scrollbar* found = FindScrollbarInPositionConst(*item.position, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

// --- Container lookups -------------------------------------------------------
// A container clips its content, so a point outside its body can neither hit a
// child control nor start a scroll there.

Button* FindButtonInContainer(Container& container, float x, float y) {
    if (!container.GetBounds().Contains(x, y)) {
        return nullptr;
    }
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        return FindButtonInFlexbox(container.GetFlexbox(), x, y);
    }
    if (container.GetStyle().layout == ContainerLayout::Grid) {
        return FindButtonInGrid(container.GetGrid(), x, y);
    }
    return FindButtonInPosition(container.GetContent(), x, y);
}

Input* FindInputInContainer(Container& container, float x, float y) {
    if (!container.GetBounds().Contains(x, y)) {
        return nullptr;
    }
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        return FindInputInFlexbox(container.GetFlexbox(), x, y);
    }
    if (container.GetStyle().layout == ContainerLayout::Grid) {
        return FindInputInGrid(container.GetGrid(), x, y);
    }
    return FindInputInPosition(container.GetContent(), x, y);
}

Scrollbar* FindScrollbarInContainer(Container& container, float x, float y) {
    if (!container.GetBounds().Contains(x, y)) {
        return nullptr;
    }
    Scrollbar& bar = container.GetScrollbar();
    if (container.GetStyle().showScrollbar && bar.IsVisible() && bar.GetBounds().Contains(x, y)) {
        return &bar;
    }
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        return FindScrollbarInFlexbox(container.GetFlexbox(), x, y);
    }
    if (container.GetStyle().layout == ContainerLayout::Grid) {
        return FindScrollbarInGrid(container.GetGrid(), x, y);
    }
    return FindScrollbarInPosition(container.GetContent(), x, y);
}

const Button* FindButtonInContainerConst(const Container& container, float x, float y) {
    if (!container.GetBounds().Contains(x, y)) {
        return nullptr;
    }
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        return FindButtonInFlexboxConst(container.GetFlexbox(), x, y);
    }
    if (container.GetStyle().layout == ContainerLayout::Grid) {
        return FindButtonInGridConst(container.GetGrid(), x, y);
    }
    return FindButtonInPositionConst(container.GetContent(), x, y);
}

const Input* FindInputInContainerConst(const Container& container, float x, float y) {
    if (!container.GetBounds().Contains(x, y)) {
        return nullptr;
    }
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        return FindInputInFlexboxConst(container.GetFlexbox(), x, y);
    }
    if (container.GetStyle().layout == ContainerLayout::Grid) {
        return FindInputInGridConst(container.GetGrid(), x, y);
    }
    return FindInputInPositionConst(container.GetContent(), x, y);
}

const Scrollbar* FindScrollbarInContainerConst(const Container& container, float x, float y) {
    if (!container.GetBounds().Contains(x, y)) {
        return nullptr;
    }
    const Scrollbar& bar = container.GetScrollbar();
    if (container.GetStyle().showScrollbar && bar.IsVisible() && bar.GetBounds().Contains(x, y)) {
        return &bar;
    }
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        return FindScrollbarInFlexboxConst(container.GetFlexbox(), x, y);
    }
    if (container.GetStyle().layout == ContainerLayout::Grid) {
        return FindScrollbarInGridConst(container.GetGrid(), x, y);
    }
    return FindScrollbarInPositionConst(container.GetContent(), x, y);
}

Container* FindContainerInFlexbox(Flexbox& flexbox, float x, float y) {
    for (FlexboxItem& item : flexbox.GetItems()) {
        if (item.kind == FlexItemKind::Container && item.container) {
            Container* found = FindContainerInContainer(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            Container* found = FindContainerInFlexbox(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

Container* FindContainerInGrid(Grid& grid, float x, float y) {
    for (GridItem& item : grid.GetItems()) {
        if (item.kind == GridItemKind::Container && item.container) {
            Container* found = FindContainerInContainer(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            Container* found = FindContainerInFlexbox(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            Container* found = FindContainerInGrid(*item.grid, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

Container* FindContainerInPosition(Position& position, float x, float y) {
    for (PositionItem& item : position.GetItems()) {
        if (item.kind == PositionItemKind::Container && item.container) {
            Container* found = FindContainerInContainer(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            Container* found = FindContainerInFlexbox(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            Container* found = FindContainerInGrid(*item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Position && item.position) {
            Container* found = FindContainerInPosition(*item.position, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

// The deepest container under the point, which is the one a wheel event scrolls.
Container* FindContainerInContainer(Container& container, float x, float y) {
    if (!container.GetBounds().Contains(x, y)) {
        return nullptr;
    }
    Container* nested = nullptr;
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        nested = FindContainerInFlexbox(container.GetFlexbox(), x, y);
    } else if (container.GetStyle().layout == ContainerLayout::Grid) {
        nested = FindContainerInGrid(container.GetGrid(), x, y);
    } else {
        nested = FindContainerInPosition(container.GetContent(), x, y);
    }
    return nested ? nested : &container;
}

// Containers under a point, innermost first, so wheel scrolling can bubble out
// of a region that has nothing left to scroll.
void CollectContainersIn(Container& container, float x, float y, std::vector<Container*>& out) {
    if (!container.GetBounds().Contains(x, y)) {
        return;
    }
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        CollectContainersIn(container.GetFlexbox(), x, y, out);
    } else if (container.GetStyle().layout == ContainerLayout::Grid) {
        CollectContainersIn(container.GetGrid(), x, y, out);
    } else {
        CollectContainersIn(container.GetContent(), x, y, out);
    }
    out.push_back(&container);
}

void CollectContainersIn(Flexbox& flexbox, float x, float y, std::vector<Container*>& out) {
    for (FlexboxItem& item : flexbox.GetItems()) {
        if (item.kind == FlexItemKind::Container && item.container) {
            CollectContainersIn(*item.container, x, y, out);
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            CollectContainersIn(*item.flexbox, x, y, out);
        }
    }
}

void CollectContainersIn(Grid& grid, float x, float y, std::vector<Container*>& out) {
    for (GridItem& item : grid.GetItems()) {
        if (item.kind == GridItemKind::Container && item.container) {
            CollectContainersIn(*item.container, x, y, out);
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            CollectContainersIn(*item.flexbox, x, y, out);
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            CollectContainersIn(*item.grid, x, y, out);
        }
    }
}

void CollectContainersIn(Position& position, float x, float y, std::vector<Container*>& out) {
    for (PositionItem& item : position.GetItems()) {
        if (item.kind == PositionItemKind::Container && item.container) {
            CollectContainersIn(*item.container, x, y, out);
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            CollectContainersIn(*item.flexbox, x, y, out);
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            CollectContainersIn(*item.grid, x, y, out);
        } else if (item.kind == PositionItemKind::Position && item.position) {
            CollectContainersIn(*item.position, x, y, out);
        }
    }
}

void VisitMotionFlex(Flexbox& flexbox, const std::function<void(MotionShape&)>& visit);
void VisitMotionGrid(Grid& grid, const std::function<void(MotionShape&)>& visit);
void VisitMotionPosition(Position& position, const std::function<void(MotionShape&)>& visit);

void VisitMotionContainer(Container& container, const std::function<void(MotionShape&)>& visit) {
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        VisitMotionFlex(container.GetFlexbox(), visit);
    } else if (container.GetStyle().layout == ContainerLayout::Grid) {
        VisitMotionGrid(container.GetGrid(), visit);
    } else {
        VisitMotionPosition(container.GetContent(), visit);
    }
}

void VisitMotionFlex(Flexbox& flexbox, const std::function<void(MotionShape&)>& visit) {
    for (FlexboxItem& item : flexbox.GetItems()) {
        if (item.kind == FlexItemKind::MotionShape) {
            visit(item.motionShape);
        } else if (item.kind == FlexItemKind::Container && item.container) {
            VisitMotionContainer(*item.container, visit);
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            VisitMotionFlex(*item.flexbox, visit);
        }
    }
}

void VisitMotionGrid(Grid& grid, const std::function<void(MotionShape&)>& visit) {
    for (GridItem& item : grid.GetItems()) {
        if (item.kind == GridItemKind::MotionShape) {
            visit(item.motionShape);
        } else if (item.kind == GridItemKind::Container && item.container) {
            VisitMotionContainer(*item.container, visit);
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            VisitMotionFlex(*item.flexbox, visit);
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            VisitMotionGrid(*item.grid, visit);
        }
    }
}

void VisitMotionPosition(Position& position, const std::function<void(MotionShape&)>& visit) {
    for (PositionItem& item : position.GetItems()) {
        if (item.kind == PositionItemKind::MotionShape) {
            visit(item.motionShape);
        } else if (item.kind == PositionItemKind::Container && item.container) {
            VisitMotionContainer(*item.container, visit);
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            VisitMotionFlex(*item.flexbox, visit);
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            VisitMotionGrid(*item.grid, visit);
        } else if (item.kind == PositionItemKind::Position && item.position) {
            VisitMotionPosition(*item.position, visit);
        }
    }
}

template <typename Visit>
void VisitThemeFlex(Flexbox& flexbox, Visit& visit);
template <typename Visit>
void VisitThemeGrid(Grid& grid, Visit& visit);
template <typename Visit>
void VisitThemePosition(Position& position, Visit& visit);

template <typename Visit>
void VisitThemeContainer(Container& container, Visit& visit) {
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        VisitThemeFlex(container.GetFlexbox(), visit);
    } else if (container.GetStyle().layout == ContainerLayout::Grid) {
        VisitThemeGrid(container.GetGrid(), visit);
    } else {
        VisitThemePosition(container.GetContent(), visit);
    }
}

template <typename Visit>
void VisitThemeFlex(Flexbox& flexbox, Visit& visit) {
    for (FlexboxItem& item : flexbox.GetItems()) {
        if (item.kind == FlexItemKind::ThemeToggle) {
            visit(item.themeToggle);
        } else if (item.kind == FlexItemKind::ThemeIconToggle) {
            visit(item.themeIconToggle);
        } else if (item.kind == FlexItemKind::TitleBar) {
            visit(item.titleBar.ThemeIcon());
        } else if (item.kind == FlexItemKind::Container && item.container) {
            VisitThemeContainer(*item.container, visit);
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            VisitThemeFlex(*item.flexbox, visit);
        }
    }
}

template <typename Visit>
void VisitThemeGrid(Grid& grid, Visit& visit) {
    for (GridItem& item : grid.GetItems()) {
        if (item.kind == GridItemKind::ThemeToggle) {
            visit(item.themeToggle);
        } else if (item.kind == GridItemKind::ThemeIconToggle) {
            visit(item.themeIconToggle);
        } else if (item.kind == GridItemKind::TitleBar) {
            visit(item.titleBar.ThemeIcon());
        } else if (item.kind == GridItemKind::Container && item.container) {
            VisitThemeContainer(*item.container, visit);
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            VisitThemeFlex(*item.flexbox, visit);
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            VisitThemeGrid(*item.grid, visit);
        }
    }
}

template <typename Visit>
void VisitThemePosition(Position& position, Visit& visit) {
    for (PositionItem& item : position.GetItems()) {
        if (item.kind == PositionItemKind::ThemeToggle) {
            visit(item.themeToggle);
        } else if (item.kind == PositionItemKind::ThemeIconToggle) {
            visit(item.themeIconToggle);
        } else if (item.kind == PositionItemKind::TitleBar) {
            visit(item.titleBar.ThemeIcon());
        } else if (item.kind == PositionItemKind::Container && item.container) {
            VisitThemeContainer(*item.container, visit);
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            VisitThemeFlex(*item.flexbox, visit);
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            VisitThemeGrid(*item.grid, visit);
        } else if (item.kind == PositionItemKind::Position && item.position) {
            VisitThemePosition(*item.position, visit);
        }
    }
}

template <typename Visit>
void VisitSceneThemes(std::vector<SceneItem>& items, Visit& visit) {
    for (SceneItem& item : items) {
        if (item.kind == SceneItemKind::ThemeToggle) {
            visit(item.themeToggle);
        } else if (item.kind == SceneItemKind::ThemeIconToggle) {
            visit(item.themeIconToggle);
        } else if (item.kind == SceneItemKind::TitleBar) {
            visit(item.titleBar.ThemeIcon());
        } else if (item.kind == SceneItemKind::Container && item.container) {
            VisitThemeContainer(*item.container, visit);
        } else if (item.kind == SceneItemKind::Flexbox) {
            VisitThemeFlex(item.flexbox, visit);
        } else if (item.kind == SceneItemKind::Grid) {
            VisitThemeGrid(item.grid, visit);
        } else if (item.kind == SceneItemKind::Position) {
            VisitThemePosition(item.position, visit);
        }
    }
}

void VisitSceneMotion(std::vector<SceneItem>& items, const std::function<void(MotionShape&)>& visit) {
    for (SceneItem& item : items) {
        if (item.kind == SceneItemKind::MotionShape) {
            visit(item.motionShape);
        } else if (item.kind == SceneItemKind::Container && item.container) {
            VisitMotionContainer(*item.container, visit);
        } else if (item.kind == SceneItemKind::Flexbox) {
            VisitMotionFlex(item.flexbox, visit);
        } else if (item.kind == SceneItemKind::Grid) {
            VisitMotionGrid(item.grid, visit);
        } else if (item.kind == SceneItemKind::Position) {
            VisitMotionPosition(item.position, visit);
        }
    }
}

MotionShape* FindThemeMotionFlex(Flexbox& flexbox, float x, float y);
MotionShape* FindThemeMotionGrid(Grid& grid, float x, float y);
MotionShape* FindThemeMotionPosition(Position& position, float x, float y);

MotionShape* FindThemeMotionContainer(Container& container, float x, float y) {
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        return FindThemeMotionFlex(container.GetFlexbox(), x, y);
    }
    if (container.GetStyle().layout == ContainerLayout::Grid) {
        return FindThemeMotionGrid(container.GetGrid(), x, y);
    }
    return FindThemeMotionPosition(container.GetContent(), x, y);
}

MotionShape* FindThemeMotionFlex(Flexbox& flexbox, float x, float y) {
    std::vector<FlexboxItem>& items = flexbox.GetItems();
    for (int index = static_cast<int>(items.size()) - 1; index >= 0; --index) {
        FlexboxItem& item = items[static_cast<std::size_t>(index)];
        if (item.kind == FlexItemKind::Container && item.container) {
            if (MotionShape* found = FindThemeMotionContainer(*item.container, x, y)) {
                return found;
            }
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            if (MotionShape* found = FindThemeMotionFlex(*item.flexbox, x, y)) {
                return found;
            }
        } else if (item.kind == FlexItemKind::MotionShape && item.motionShape.TogglesTheme() &&
                   item.motionShape.Bounds().Contains(x, y)) {
            return &item.motionShape;
        }
    }
    return nullptr;
}

MotionShape* FindThemeMotionGrid(Grid& grid, float x, float y) {
    std::vector<GridItem>& items = grid.GetItems();
    for (int index = static_cast<int>(items.size()) - 1; index >= 0; --index) {
        GridItem& item = items[static_cast<std::size_t>(index)];
        if (item.kind == GridItemKind::Container && item.container) {
            if (MotionShape* found = FindThemeMotionContainer(*item.container, x, y)) {
                return found;
            }
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            if (MotionShape* found = FindThemeMotionFlex(*item.flexbox, x, y)) {
                return found;
            }
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            if (MotionShape* found = FindThemeMotionGrid(*item.grid, x, y)) {
                return found;
            }
        } else if (item.kind == GridItemKind::MotionShape && item.motionShape.TogglesTheme() &&
                   item.motionShape.Bounds().Contains(x, y)) {
            return &item.motionShape;
        }
    }
    return nullptr;
}

MotionShape* FindThemeMotionPosition(Position& position, float x, float y) {
    std::vector<PositionItem>& items = position.GetItems();
    for (int index = static_cast<int>(items.size()) - 1; index >= 0; --index) {
        PositionItem& item = items[static_cast<std::size_t>(index)];
        if (item.kind == PositionItemKind::Container && item.container) {
            if (MotionShape* found = FindThemeMotionContainer(*item.container, x, y)) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            if (MotionShape* found = FindThemeMotionFlex(*item.flexbox, x, y)) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            if (MotionShape* found = FindThemeMotionGrid(*item.grid, x, y)) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Position && item.position) {
            if (MotionShape* found = FindThemeMotionPosition(*item.position, x, y)) {
                return found;
            }
        } else if (item.kind == PositionItemKind::MotionShape && item.motionShape.TogglesTheme() &&
                   item.motionShape.Bounds().Contains(x, y)) {
            return &item.motionShape;
        }
    }
    return nullptr;
}

MotionShape* FindThemeMotionScene(std::vector<SceneItem>& items, float x, float y) {
    for (int index = static_cast<int>(items.size()) - 1; index >= 0; --index) {
        SceneItem& item = items[static_cast<std::size_t>(index)];
        if (item.kind == SceneItemKind::Container && item.container) {
            if (MotionShape* found = FindThemeMotionContainer(*item.container, x, y)) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Flexbox) {
            if (MotionShape* found = FindThemeMotionFlex(item.flexbox, x, y)) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Grid) {
            if (MotionShape* found = FindThemeMotionGrid(item.grid, x, y)) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Position) {
            if (MotionShape* found = FindThemeMotionPosition(item.position, x, y)) {
                return found;
            }
        } else if (item.kind == SceneItemKind::MotionShape && item.motionShape.TogglesTheme() &&
                   item.motionShape.Bounds().Contains(x, y)) {
            return &item.motionShape;
        }
    }
    return nullptr;
}

void CollectSceneContainers(std::vector<SceneItem>& items, float x, float y, std::vector<Container*>& out) {
    for (SceneItem& item : items) {
        if (item.kind == SceneItemKind::Container) {
            CollectContainersIn(*item.container, x, y, out);
        } else if (item.kind == SceneItemKind::Flexbox) {
            CollectContainersIn(item.flexbox, x, y, out);
        } else if (item.kind == SceneItemKind::Grid) {
            CollectContainersIn(item.grid, x, y, out);
        } else if (item.kind == SceneItemKind::Position) {
            CollectContainersIn(item.position, x, y, out);
        }
    }
}

void CollectButtonsInFlexbox(Flexbox& flexbox, std::vector<Button*>& out);
void CollectButtonsInGrid(Grid& grid, std::vector<Button*>& out);
void CollectButtonsInPosition(Position& position, std::vector<Button*>& out);
void CollectButtonsInContainer(Container& container, std::vector<Button*>& out);

void CollectButtonsInFlexbox(Flexbox& flexbox, std::vector<Button*>& out) {
    for (FlexboxItem& item : flexbox.GetItems()) {
        if (item.kind == FlexItemKind::Button) {
            out.push_back(&item.button);
        } else if (item.kind == FlexItemKind::Container && item.container) {
            CollectButtonsInContainer(*item.container, out);
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            CollectButtonsInFlexbox(*item.flexbox, out);
        }
    }
}

void CollectButtonsInGrid(Grid& grid, std::vector<Button*>& out) {
    for (GridItem& item : grid.GetItems()) {
        if (item.kind == GridItemKind::Button) {
            out.push_back(&item.button);
        } else if (item.kind == GridItemKind::Container && item.container) {
            CollectButtonsInContainer(*item.container, out);
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            CollectButtonsInFlexbox(*item.flexbox, out);
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            CollectButtonsInGrid(*item.grid, out);
        }
    }
}

void CollectButtonsInPosition(Position& position, std::vector<Button*>& out) {
    for (PositionItem& item : position.GetItems()) {
        if (item.kind == PositionItemKind::Button) {
            out.push_back(&item.button);
        } else if (item.kind == PositionItemKind::Container && item.container) {
            CollectButtonsInContainer(*item.container, out);
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            CollectButtonsInFlexbox(*item.flexbox, out);
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            CollectButtonsInGrid(*item.grid, out);
        } else if (item.kind == PositionItemKind::Position && item.position) {
            CollectButtonsInPosition(*item.position, out);
        }
    }
}

SidebarItem* FindPaneInFlexbox(Flexbox& flexbox, float x, float y);
SidebarItem* FindPaneInGrid(Grid& grid, float x, float y);
SidebarItem* FindPaneInPosition(Position& position, float x, float y);
SidebarItem* FindPaneInContainer(Container& container, float x, float y);

SidebarItem* HitPane(SidebarItem& item, float x, float y) {
    if (item.CanActivate() && item.GetBounds().Contains(x, y)) {
        return &item;
    }
    return nullptr;
}

SidebarItem* FindPaneInFlexbox(Flexbox& flexbox, float x, float y) {
    std::vector<FlexboxItem>& items = flexbox.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        FlexboxItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == FlexItemKind::SidebarItem) {
            if (SidebarItem* found = HitPane(item.sidebarItem, x, y)) {
                return found;
            }
        } else if (item.kind == FlexItemKind::Sidebar) {
            if (SidebarItem* found = item.sidebar.HitItem(x, y)) {
                return found;
            }
        } else if (item.kind == FlexItemKind::Container && item.container) {
            if (SidebarItem* found = FindPaneInContainer(*item.container, x, y)) {
                return found;
            }
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            if (SidebarItem* found = FindPaneInFlexbox(*item.flexbox, x, y)) {
                return found;
            }
        }
    }
    return nullptr;
}

SidebarItem* FindPaneInGrid(Grid& grid, float x, float y) {
    std::vector<GridItem>& items = grid.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        GridItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == GridItemKind::SidebarItem) {
            if (SidebarItem* found = HitPane(item.sidebarItem, x, y)) {
                return found;
            }
        } else if (item.kind == GridItemKind::Sidebar) {
            if (SidebarItem* found = item.sidebar.HitItem(x, y)) {
                return found;
            }
        } else if (item.kind == GridItemKind::Container && item.container) {
            if (SidebarItem* found = FindPaneInContainer(*item.container, x, y)) {
                return found;
            }
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            if (SidebarItem* found = FindPaneInFlexbox(*item.flexbox, x, y)) {
                return found;
            }
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            if (SidebarItem* found = FindPaneInGrid(*item.grid, x, y)) {
                return found;
            }
        }
    }
    return nullptr;
}

SidebarItem* FindPaneInPosition(Position& position, float x, float y) {
    std::vector<PositionItem>& items = position.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        PositionItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == PositionItemKind::SidebarItem) {
            if (SidebarItem* found = HitPane(item.sidebarItem, x, y)) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Sidebar) {
            if (SidebarItem* found = item.sidebar.HitItem(x, y)) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Container && item.container) {
            if (SidebarItem* found = FindPaneInContainer(*item.container, x, y)) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            if (SidebarItem* found = FindPaneInFlexbox(*item.flexbox, x, y)) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            if (SidebarItem* found = FindPaneInGrid(*item.grid, x, y)) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Position && item.position) {
            if (SidebarItem* found = FindPaneInPosition(*item.position, x, y)) {
                return found;
            }
        }
    }
    return nullptr;
}

SidebarItem* FindPaneInContainer(Container& container, float x, float y) {
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        return FindPaneInFlexbox(container.GetFlexbox(), x, y);
    }
    if (container.GetStyle().layout == ContainerLayout::Grid) {
        return FindPaneInGrid(container.GetGrid(), x, y);
    }
    return FindPaneInPosition(container.GetContent(), x, y);
}

struct FocusSlot {
    Button* button = nullptr;
    SidebarItem* item = nullptr;
};

void AppendSidebarSlots(Sidebar& sidebar, std::vector<FocusSlot>& out) {
    for (SidebarItem& item : sidebar.Items()) {
        if (item.CanActivate()) {
            FocusSlot slot;
            slot.item = &item;
            out.push_back(slot);
        }
        for (SidebarItem& child : item.Children()) {
            if (child.CanActivate() && child.GetBounds().height > 1.0f) {
                FocusSlot slot;
                slot.item = &child;
                out.push_back(slot);
            }
        }
    }
}

void AppendPaneSlotsFlex(Flexbox& flexbox, std::vector<FocusSlot>& out);
void AppendPaneSlotsGrid(Grid& grid, std::vector<FocusSlot>& out);
void AppendPaneSlotsPosition(Position& position, std::vector<FocusSlot>& out);
void AppendPaneSlotsContainer(Container& container, std::vector<FocusSlot>& out);

void AppendPaneSlotsFlex(Flexbox& flexbox, std::vector<FocusSlot>& out) {
    for (FlexboxItem& item : flexbox.GetItems()) {
        if (item.kind == FlexItemKind::SidebarItem && item.sidebarItem.CanActivate()) {
            FocusSlot slot;
            slot.item = &item.sidebarItem;
            out.push_back(slot);
        } else if (item.kind == FlexItemKind::Sidebar) {
            AppendSidebarSlots(item.sidebar, out);
        } else if (item.kind == FlexItemKind::Container && item.container) {
            AppendPaneSlotsContainer(*item.container, out);
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            AppendPaneSlotsFlex(*item.flexbox, out);
        }
    }
}

void AppendPaneSlotsGrid(Grid& grid, std::vector<FocusSlot>& out) {
    for (GridItem& item : grid.GetItems()) {
        if (item.kind == GridItemKind::SidebarItem && item.sidebarItem.CanActivate()) {
            FocusSlot slot;
            slot.item = &item.sidebarItem;
            out.push_back(slot);
        } else if (item.kind == GridItemKind::Sidebar) {
            AppendSidebarSlots(item.sidebar, out);
        } else if (item.kind == GridItemKind::Container && item.container) {
            AppendPaneSlotsContainer(*item.container, out);
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            AppendPaneSlotsFlex(*item.flexbox, out);
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            AppendPaneSlotsGrid(*item.grid, out);
        }
    }
}

void AppendPaneSlotsPosition(Position& position, std::vector<FocusSlot>& out) {
    for (PositionItem& item : position.GetItems()) {
        if (item.kind == PositionItemKind::SidebarItem && item.sidebarItem.CanActivate()) {
            FocusSlot slot;
            slot.item = &item.sidebarItem;
            out.push_back(slot);
        } else if (item.kind == PositionItemKind::Sidebar) {
            AppendSidebarSlots(item.sidebar, out);
        } else if (item.kind == PositionItemKind::Container && item.container) {
            AppendPaneSlotsContainer(*item.container, out);
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            AppendPaneSlotsFlex(*item.flexbox, out);
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            AppendPaneSlotsGrid(*item.grid, out);
        } else if (item.kind == PositionItemKind::Position && item.position) {
            AppendPaneSlotsPosition(*item.position, out);
        }
    }
}

void AppendPaneSlotsContainer(Container& container, std::vector<FocusSlot>& out) {
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        AppendPaneSlotsFlex(container.GetFlexbox(), out);
    } else if (container.GetStyle().layout == ContainerLayout::Grid) {
        AppendPaneSlotsGrid(container.GetGrid(), out);
    } else {
        AppendPaneSlotsPosition(container.GetContent(), out);
    }
}

void CollectButtonsInContainer(Container& container, std::vector<Button*>& out) {
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        CollectButtonsInFlexbox(container.GetFlexbox(), out);
    } else if (container.GetStyle().layout == ContainerLayout::Grid) {
        CollectButtonsInGrid(container.GetGrid(), out);
    } else {
        CollectButtonsInPosition(container.GetContent(), out);
    }
}

}  // namespace

void RoboScene::Clear() {
    items_.clear();
    desktop_ = Desktop();
    hoveredDock_ = -1;
    hoveredButton_ = nullptr;
    hoveredInput_ = nullptr;
    hoveredScrollbar_ = nullptr;
    pressedButton_ = nullptr;
    focusedButton_ = nullptr;
    keyboardPressed_ = false;
    hoveredSidebarItem_ = nullptr;
    pressedSidebarItem_ = nullptr;
    focusedSidebarItem_ = nullptr;
    sidebarKeyboardPressed_ = false;
    focusedInput_ = nullptr;
    activePaneId_.clear();
    draggingScrollbar_ = nullptr;
    focusedScrollbar_ = nullptr;
    panningContainer_ = nullptr;
}

void RoboScene::AddText(Text text) {
    SceneItem item;
    item.kind = SceneItemKind::Text;
    item.text = std::move(text);
    items_.push_back(std::move(item));
}

void RoboScene::AddButton(Button button) {
    SceneItem item;
    item.kind = SceneItemKind::Button;
    item.button = std::move(button);
    items_.push_back(std::move(item));
}

void RoboScene::AddInput(Input input) {
    SceneItem item;
    item.kind = SceneItemKind::Input;
    item.input = std::move(input);
    items_.push_back(std::move(item));
}

void RoboScene::AddScrollbar(Scrollbar scrollbar) {
    SceneItem item;
    item.kind = SceneItemKind::Scrollbar;
    item.scrollbar = std::move(scrollbar);
    items_.push_back(std::move(item));
}

void RoboScene::AddContainer(Container container) {
    SceneItem item;
    item.kind = SceneItemKind::Container;
    item.container = std::make_shared<Container>(std::move(container));
    items_.push_back(std::move(item));
}

void RoboScene::AddFlexbox(Flexbox flexbox) {
    SceneItem item;
    item.kind = SceneItemKind::Flexbox;
    item.flexbox = std::move(flexbox);
    items_.push_back(std::move(item));
}

void RoboScene::AddGrid(Grid grid) {
    SceneItem item;
    item.kind = SceneItemKind::Grid;
    item.grid = std::move(grid);
    items_.push_back(std::move(item));
}

void RoboScene::AddPosition(Position position) {
    SceneItem item;
    item.kind = SceneItemKind::Position;
    item.position = std::move(position);
    items_.push_back(std::move(item));
}

void RoboScene::AddSvg(Svg svg) {
    SceneItem item;
    item.kind = SceneItemKind::Svg;
    item.svg = std::move(svg);
    items_.push_back(std::move(item));
}

void RoboScene::AddBrandLogo(BrandLogo logo) {
    SceneItem item;
    item.kind = SceneItemKind::BrandLogo;
    item.brandLogo = std::move(logo);
    items_.push_back(std::move(item));
}

void RoboScene::AddTitleBar(TitleBar bar) {
    SceneItem item;
    item.kind = SceneItemKind::TitleBar;
    item.titleBar = std::move(bar);
    items_.push_back(std::move(item));
}

void RoboScene::AddSidebar(Sidebar pane) {
    SceneItem item;
    item.kind = SceneItemKind::Sidebar;
    item.sidebar = std::move(pane);
    items_.push_back(std::move(item));
}

void RoboScene::AddSidebarItem(SidebarItem entry) {
    SceneItem item;
    item.kind = SceneItemKind::SidebarItem;
    item.sidebarItem = std::move(entry);
    items_.push_back(std::move(item));
}

void RoboScene::AddThemeToggle(ThemeToggle toggle) {
    SceneItem item;
    item.kind = SceneItemKind::ThemeToggle;
    item.themeToggle = std::move(toggle);
    items_.push_back(std::move(item));
}

void RoboScene::AddThemeIconToggle(ThemeIconToggle toggle) {
    SceneItem item;
    item.kind = SceneItemKind::ThemeIconToggle;
    item.themeIconToggle = std::move(toggle);
    items_.push_back(std::move(item));
}

void RoboScene::AddSeparator(Separator separator) {
    SceneItem item;
    item.kind = SceneItemKind::Separator;
    item.separator = std::move(separator);
    items_.push_back(std::move(item));
}

void RoboScene::EnableDesktop() {
    desktop_.SetEnabled(true);
}

void RoboScene::SetDesktopBackground(const Color& color, const std::string& token) {
    desktop_.SetEnabled(true);
    desktop_.SetBackground(color, token);
}

void RoboScene::SetDesktopBackgroundPath(const std::string& path) {
    desktop_.SetEnabled(true);
    desktop_.SetBackgroundPath(path);
    if (platform_ != nullptr) {
        desktop_.GetBackgroundSource().EnsureLoaded(*platform_);
    }
    MarkFullDirty();
}

void RoboScene::SetAppShell(AppShell shell) {
    desktop_.SetEnabled(true);
    desktop_.SetAppShell(std::move(shell));
}

void RoboScene::SetAppShellTitle(const std::string& title) {
    AppShell shell;
    shell.SetTitle(title);
    SetAppShell(std::move(shell));
}

void RoboScene::AddDockItem(DockItem item) {
    desktop_.SetEnabled(true);
    desktop_.AddDockItem(std::move(item));
}

void RoboScene::AddMotionShape(MotionShape shape) {
    SceneItem item;
    item.kind = SceneItemKind::MotionShape;
    item.motionShape = std::move(shape);
    items_.push_back(std::move(item));
}

bool RoboScene::RerenderIfPossible() {
    if (!hasRenderContext_ || !platform_ || !fonts_) {
        return false;
    }
    MarkFullDirty();
    return true;
}

void RoboScene::MarkFullDirty() {
    needsRender_ = true;
    fullDirty_ = true;
    damageOpen_ = false;
}

void RoboScene::MarkSurfaceDirty() {
    needsRender_ = true;
    if (!frameReady_ || paintedWidth_ <= 0 || paintedHeight_ <= 0) {
        MarkFullDirty();
        return;
    }
    fullDirty_ = false;
    damageOpen_ = true;
    dirtyLeft_ = 0.0f;
    dirtyTop_ = 0.0f;
    dirtyRight_ = static_cast<float>(paintedWidth_);
    dirtyBottom_ = static_cast<float>(paintedHeight_);
}

void RoboScene::MarkDirty(float x, float y, float width, float height) {
    needsRender_ = true;
    if (fullDirty_ || !frameReady_ || width <= 0.0f || height <= 0.0f) {
        fullDirty_ = true;
        damageOpen_ = false;
        return;
    }
    const float pad = 32.0f;
    const float left = x - pad;
    const float top = y - pad;
    const float right = x + width + pad;
    const float bottom = y + height + pad;
    if (!damageOpen_) {
        dirtyLeft_ = left;
        dirtyTop_ = top;
        dirtyRight_ = right;
        dirtyBottom_ = bottom;
        damageOpen_ = true;
    } else {
        dirtyLeft_ = std::min(dirtyLeft_, left);
        dirtyTop_ = std::min(dirtyTop_, top);
        dirtyRight_ = std::max(dirtyRight_, right);
        dirtyBottom_ = std::max(dirtyBottom_, bottom);
    }
    const float area = std::max(0.0f, dirtyRight_ - dirtyLeft_) * std::max(0.0f, dirtyBottom_ - dirtyTop_);
    const float surface = static_cast<float>(std::max(1, paintedWidth_)) * static_cast<float>(std::max(1, paintedHeight_));
    if (area > surface * 0.5f) {
        fullDirty_ = true;
        damageOpen_ = false;
    }
}

void RoboScene::DirtyButton(const Button* button) {
    if (!button) {
        return;
    }
    const ButtonBounds& bounds = button->GetBounds();
    if (bounds.width < 1.0f || bounds.height < 1.0f) {
        MarkFullDirty();
        return;
    }
    MarkDirty(bounds.x - 8.0f, bounds.y - 8.0f, bounds.width + 16.0f, bounds.height + 16.0f);
}

void RoboScene::DirtyInput(const Input* input) {
    if (!input) {
        return;
    }
    const InputBounds& bounds = input->GetBounds();
    MarkDirty(bounds.x, bounds.y, bounds.width, bounds.height);
}

void RoboScene::DirtyScrollbar(const Scrollbar* scrollbar) {
    if (!scrollbar) {
        return;
    }
    const ScrollbarBounds& bounds = scrollbar->GetBounds();
    MarkDirty(bounds.x, bounds.y, bounds.width, bounds.height);
}

void RoboScene::DirtySelectionVisual() {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    if (TextSelection::VisualBounds(x, y, width, height)) {
        MarkDirty(x, y, width, height);
    }
}

bool RoboScene::CanPaintPartial(float paintScale, int width, int height) const {
    if (fullDirty_ || !frameReady_ || !damageOpen_) {
        return false;
    }
    if (width != paintedWidth_ || height != paintedHeight_) {
        return false;
    }
    if (std::fabs(paintScale - paintedScale_) > 0.001f) {
        return false;
    }
    if (std::fabs(scrollX_ - paintedScrollX_) > 0.5f || std::fabs(scrollY_ - paintedScrollY_) > 0.5f) {
        return false;
    }
    return dirtyRight_ > dirtyLeft_ && dirtyBottom_ > dirtyTop_;
}

bool RoboScene::NeedsRender() const {
    return needsRender_;
}

void RoboScene::ConfigurePageBars() {
    if (pageBarsReady_) {
        return;
    }
    auto makeStyle = [](ScrollbarOrientation orientation) {
        ScrollbarStyle style;
        style.orientation = orientation;
        style.thickness = 8.0;
        style.cornerRadius = 4.0;
        style.minThumbLength = 28.0;
        style.showArrows = true;
        style.arrowSize = 8.0;
        style.arrowGap = 3.0;
        style.autoHide = true;
        style.autoHideDelay = 700.0;
        style.fadeDuration = 180.0;
        style.visibility = ScrollbarVisibility::Auto;
        return style;
    };
    pageVertical_.SetStyle(makeStyle(ScrollbarOrientation::Vertical));
    pageVertical_.ApplyVariantDefaults();
    pageHorizontal_.SetStyle(makeStyle(ScrollbarOrientation::Horizontal));
    pageHorizontal_.ApplyVariantDefaults();
    pageBarsReady_ = true;
}

void RoboScene::PullPageScroll() {
    scrollY_ = maxScrollY_ > 0.0f ? static_cast<float>(pageVertical_.GetValue()) * maxScrollY_ : 0.0f;
    scrollX_ = maxScrollX_ > 0.0f ? static_cast<float>(pageHorizontal_.GetValue()) * maxScrollX_ : 0.0f;
}

void RoboScene::PullSidebarScroll(const Scrollbar* bar) {
    if (!bar) {
        return;
    }
    for (SceneItem& item : items_) {
        if (item.kind == SceneItemKind::Sidebar && &item.sidebar.PanelScrollbar() == bar) {
            item.sidebar.PullPanelScroll();
            return;
        }
    }
}

bool RoboScene::PageBarDriving(const Scrollbar& bar) const {
    return bar.IsDragging() || bar.NeedsAnimation() || bar.GetPressedArrow() != ScrollbarArrow::None;
}

void RoboScene::SetFoundation(const std::string& name) {
    foundation_ = name;
    MarkFullDirty();
}

void RoboScene::SetFoundationForTheme(Theme theme, const std::string& name) {
    if (theme == Theme::Dark) {
        darkFoundation_ = name;
    } else {
        lightFoundation_ = name;
    }
    MarkFullDirty();
}

void RoboScene::SetPagePadding(float padding) {
    padding_ = std::max(0.0f, padding);
    pagePaddingSet_ = true;
}

void RoboScene::SetTheme(Theme theme) {
    if (theme_ == theme) {
        return;
    }
    theme_ = theme;
    MarkFullDirty();
}

const Foundation& RoboScene::ActiveFoundation() const {
    const std::string* name = &foundation_;
    if (theme_ == Theme::Dark && !darkFoundation_.empty()) {
        name = &darkFoundation_;
    } else if (theme_ == Theme::Light && !lightFoundation_.empty()) {
        name = &lightFoundation_;
    }
    const Foundation* found = Foundation::Find(*name);
    if (found == nullptr) {
        return Foundation::RoboFoundation();
    }
    return *found;
}

Color RoboScene::GetBackground() const {
    return background_;
}

namespace {

Color LerpPageColor(const Color& light, const Color& dark, float mix) {
    mix = std::max(0.0f, std::min(1.0f, mix));
    auto channel = [mix](std::uint8_t from, std::uint8_t to) {
        return static_cast<std::uint8_t>(
            std::lround(static_cast<float>(from) + (static_cast<float>(to) - static_cast<float>(from)) * mix));
    };
    return Color::FromRgba(channel(light.red, dark.red), channel(light.green, dark.green), channel(light.blue, dark.blue),
                           channel(light.alpha, dark.alpha));
}

}  // namespace

void RoboScene::SetPaintPump(void (*pump)(void*), void* cookie) {
    paintPump_ = pump;
    paintPumpCookie_ = cookie;
}

bool RoboScene::PresentVideoFrame(IPlatform& platform) {
    if (!chromeReady_ || platform_ == nullptr || paintedWidth_ <= 0 || paintedHeight_ <= 0) {
        return false;
    }
    if (frame_.GetWidth() != paintedWidth_ || frame_.GetHeight() != paintedHeight_) {
        return false;
    }
    BackgroundPainter::Paint(frame_, desktop_.GetBackgroundSource(), paintedWidth_, paintedHeight_, 0, 0, paintedWidth_, paintedHeight_);
    ApplyChrome(frame_);
    frame_.SetPresentRect(0, 0, paintedWidth_, paintedHeight_);
    platform.Present(frame_);
    needsRender_ = false;
    fullDirty_ = false;
    damageOpen_ = false;
    frameReady_ = true;
    return true;
}

#ifdef ROBO_KERNEL
extern "C" int RoboHeapHas(size_t bytes);
#endif

bool RoboScene::ChromeFits(std::size_t bytes, std::size_t maskBytes) const {
#ifdef ROBO_KERNEL
    std::size_t need = 64u;
    if (chromeBase_.size() != bytes) {
        need += bytes + 64u;
    }
    if (chromePixels_.size() != bytes) {
        need += bytes + 64u;
    }
    if (chromeMask_.size() != maskBytes) {
        need += maskBytes + 64u;
    }
    return need <= 64u || RoboHeapHas(need) != 0;
#else
    (void)bytes;
    (void)maskBytes;
    return true;
#endif
}

void RoboScene::CaptureChrome() {
    const std::uint8_t* frame = frame_.GetPixels();
    const int width = frame_.GetWidth();
    const int height = frame_.GetHeight();
    const std::size_t count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    if (frame == nullptr || count == 0 || chromeBase_.size() != count * 4u) {
        chromeReady_ = false;
        return;
    }
    if (!ChromeFits(count * 4u, count)) {
        chromeReady_ = false;
        return;
    }
    if (chromePixels_.size() != count * 4u) {
        chromePixels_.assign(count * 4u, 0);
    }
    if (chromeMask_.size() != count) {
        chromeMask_.assign(count, 0);
    }
    for (std::size_t index = 0; index < count; ++index) {
        const std::uint8_t* painted = frame + index * 4u;
        const std::uint8_t* base = chromeBase_.data() + index * 4u;
        const bool widget = painted[0] != base[0] || painted[1] != base[1] || painted[2] != base[2];
        chromeMask_[index] = widget ? 1u : 0u;
        if (widget) {
            chromePixels_[index * 4u] = painted[0];
            chromePixels_[index * 4u + 1u] = painted[1];
            chromePixels_[index * 4u + 2u] = painted[2];
            chromePixels_[index * 4u + 3u] = painted[3];
        }
    }
    chromeReady_ = true;
}

void RoboScene::ApplyChrome(FrameBuffer& frame) {
    std::uint8_t* pixels = frame.GetPixels();
    const std::size_t count = chromeMask_.size();
    if (pixels == nullptr || chromePixels_.size() != count * 4u) {
        return;
    }
    for (std::size_t index = 0; index < count; ++index) {
        if (chromeMask_[index] == 0) {
            continue;
        }
        pixels[index * 4u] = chromePixels_[index * 4u];
        pixels[index * 4u + 1u] = chromePixels_[index * 4u + 1u];
        pixels[index * 4u + 2u] = chromePixels_[index * 4u + 2u];
        pixels[index * 4u + 3u] = chromePixels_[index * 4u + 3u];
    }
}

void RoboScene::Render(IPlatform& platform, FontEngine& fonts, const Color& background, float padding) {
    if (videoOnly_ && !fullDirty_ && PresentVideoFrame(platform)) {
        videoOnly_ = false;
        return;
    }
    videoOnly_ = false;
    const FoundationScope foundationScope(ActiveFoundation(), theme_, themeMix_);
    platform_ = &platform;
    fonts_ = &fonts;
    Color lightPage = background;
    Color darkPage = Color::FromHex("#000000");
    ActiveFoundation().LookupColor("bg.primary", Theme::Light, lightPage);
    ActiveFoundation().LookupColor("bg.primary", Theme::Dark, darkPage);
    background_ = LerpPageColor(lightPage, darkPage, themeMix_);
    if (desktop_.IsEnabled() && desktop_.HasBackground()) {
        desktop_.GetBackgroundSource().EnsureLoaded(platform);
        background_ = ResolveColor(desktop_.GetBackground(), desktop_.GetBackgroundToken());
    }
    if (!pagePaddingSet_) {
        padding_ = padding;
    }
    hasRenderContext_ = true;
    needsRender_ = false;
    ConfigurePageBars();

    const int width = std::max(1, platform.GetSurfaceWidth());
    const int height = std::max(1, platform.GetSurfaceHeight());
    surfaceWidth_ = width;
    surfaceHeight_ = height;
    // The surface is already device pixels and matches the screen. Page overflow
    // scrolls inside this canvas; the canvas itself does not grow.
    const float paintScale = std::max(0.5f, platform.GetPixelScale() * appZoom_);
    if (lastPaintScale_ > 0.0f) {
        const float ratio = paintScale / lastPaintScale_;
        if (std::fabs(ratio - 1.0f) > 0.001f) {
            scrollX_ *= ratio;
            scrollY_ *= ratio;
            maxScrollX_ *= ratio;
            maxScrollY_ *= ratio;
        }
    }
    lastPaintScale_ = paintScale;
    const float pad = padding_ * paintScale;
    const float barThickness = static_cast<float>(pageVertical_.GetStyle().thickness) * paintScale;
    const float edge = 3.0f * paintScale;

    if (frame_.GetWidth() != width || frame_.GetHeight() != height) {
        frame_.Resize(width, height);
    }
    FrameBuffer& frame = frame_;
    frame.SetPump(paintPump_, paintPumpCookie_);
    const bool partial = CanPaintPartial(paintScale, width, height);
    int damageX = 0;
    int damageY = 0;
    int damageWidth = width;
    int damageHeight = height;
    if (partial) {
        damageX = std::max(0, static_cast<int>(std::floor(dirtyLeft_)));
        damageY = std::max(0, static_cast<int>(std::floor(dirtyTop_)));
        const int damageRight = std::min(width, static_cast<int>(std::ceil(dirtyRight_)));
        const int damageBottom = std::min(height, static_cast<int>(std::ceil(dirtyBottom_)));
        damageWidth = std::max(0, damageRight - damageX);
        damageHeight = std::max(0, damageBottom - damageY);
        frame.PushClipRect(static_cast<float>(damageX), static_cast<float>(damageY),
                           static_cast<float>(damageWidth), static_cast<float>(damageHeight));
        if (desktop_.IsEnabled() && desktop_.GetBackgroundSource().PaintsMedia()) {
            BackgroundPainter::Paint(frame, desktop_.GetBackgroundSource(), width, height, damageX, damageY, damageWidth, damageHeight);
        } else {
            frame.ClearRect(damageX, damageY, damageWidth, damageHeight, background_);
        }
    }

    float verticalReserve = partial ? paintedVerticalReserve_ : 0.0f;
    float horizontalReserve = partial ? paintedHorizontalReserve_ : 0.0f;
    float titleBarBottom = 0.0f;
    for (int pass = 0; pass < 3; ++pass) {
        titleBarBottom = 0.0f;
        scrollX_ = std::max(0.0f, std::min(scrollX_, maxScrollX_));
        scrollY_ = std::max(0.0f, std::min(scrollY_, maxScrollY_));

        if (!partial) {
            if (desktop_.IsEnabled() && desktop_.GetBackgroundSource().PaintsMedia()) {
                BackgroundPainter::Paint(frame, desktop_.GetBackgroundSource(), width, height, 0, 0, width, height);
            } else {
                frame.Clear(background_);
            }
        }
        if (frame.GetPixels() != nullptr) {
            const std::size_t bytes = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u;
            if (ChromeFits(bytes, static_cast<std::size_t>(width) * static_cast<std::size_t>(height))) {
                if (chromeBase_.size() != bytes) {
                    chromeBase_.assign(bytes, 0);
                }
                memcpy(chromeBase_.data(), frame.GetPixels(), bytes);
            } else {
                chromeReady_ = false;
            }
        }
        TextSelection::BeginFrame();

        const float contentWidth = std::max(0.0f, static_cast<float>(width) - pad * 2.0f - verticalReserve);
        const float originX = pad - scrollX_;
        float columnX = originX;
        float columnW = contentWidth;
        float cursorY = pad - scrollY_;
        float flowEdge = cursorY;
        float contentRight = originX + scrollX_;
        bool titleClip = false;
        DesktopChrome desktopChrome{};
        bool desktopClip = false;
        if (desktop_.IsEnabled()) {
            desktopChrome = DesktopPainter::Layout(static_cast<float>(width), static_cast<float>(height), paintScale,
                                                   desktop_.HasAppShell(), !desktop_.Dock().empty());
            frame.PushClipRect(0.0f, desktopChrome.contentY, static_cast<float>(width), desktopChrome.contentHeight);
            desktopClip = true;
            cursorY = desktopChrome.contentY + pad - scrollY_;
            flowEdge = cursorY;
        }

        for (int i = 0; i < static_cast<int>(items_.size()); ++i) {
            frame.Pump();
            SceneItem& item = items_[static_cast<std::size_t>(i)];
            const float yBefore = cursorY;
            bool edgeSet = false;
            if (item.kind == SceneItemKind::Text) {
                cursorY += TextPainter::Paint(frame, fonts, item.text, columnX, cursorY, columnW, paintScale);
                contentRight = std::max(contentRight, columnX + columnW + scrollX_);
            } else if (item.kind == SceneItemKind::Button) {
                const ButtonPaintResult painted =
                    ButtonPainter::Paint(frame, fonts, item.button, columnX, cursorY, columnW, paintScale);
                cursorY += painted.advanceY;
                const ButtonBounds& bounds = item.button.GetBounds();
                contentRight = std::max(contentRight, bounds.x + bounds.width + scrollX_);
            } else if (item.kind == SceneItemKind::Input) {
                if (&item.input == focusedInput_) {
                    item.input.SetInteraction(InputInteraction::Focused);
                } else if (&item.input == hoveredInput_ && item.input.GetInteraction() != InputInteraction::Focused) {
                    item.input.SetInteraction(InputInteraction::Hovered);
                } else if (&item.input != focusedInput_) {
                    item.input.SetInteraction(InputInteraction::Normal);
                }

                const InputPaintResult painted =
                    InputPainter::Paint(frame, fonts, item.input, columnX, cursorY, columnW, paintScale);
                cursorY += painted.advanceY;
                const InputBounds& bounds = item.input.GetBounds();
                contentRight = std::max(contentRight, bounds.x + bounds.width + scrollX_);
            } else if (item.kind == SceneItemKind::Scrollbar) {
                if (&item.scrollbar == draggingScrollbar_) {
                    item.scrollbar.SetInteraction(ScrollbarInteraction::Dragging);
                } else if (&item.scrollbar == hoveredScrollbar_) {
                    item.scrollbar.SetInteraction(ScrollbarInteraction::Hovered);
                } else {
                    item.scrollbar.SetInteraction(ScrollbarInteraction::Normal);
                }

                const ScrollbarPaintResult painted =
                    ScrollbarPainter::Paint(frame, item.scrollbar, columnX, cursorY, columnW, paintScale);
                cursorY += painted.advanceY;
                const ScrollbarBounds& bounds = item.scrollbar.GetBounds();
                contentRight = std::max(contentRight, bounds.x + bounds.width + scrollX_);
            } else if (item.kind == SceneItemKind::Container) {
                if (!item.container->ShownWhen().empty() && item.container->ShownWhen() != ActivePaneId()) {
                    item.container->SetBounds(InputBounds{});
                    continue;
                }
                const ContainerPaintResult painted =
                    ContainerPainter::Paint(frame, fonts, *item.container, columnX, cursorY, columnW, paintScale);
                cursorY += painted.advanceY;
                const InputBounds& bounds = item.container->GetBounds();
                contentRight = std::max(contentRight, bounds.x + bounds.width + scrollX_);
            } else if (item.kind == SceneItemKind::Flexbox) {
                const FlexboxPaintResult painted =
                    FlexboxPainter::Paint(frame, fonts, item.flexbox, columnX, cursorY, columnW, paintScale);
                cursorY += painted.advanceY;
                const FlexboxBounds& bounds = item.flexbox.GetBounds();
                contentRight = std::max(contentRight, bounds.x + bounds.width + scrollX_);
            } else if (item.kind == SceneItemKind::Grid) {
                const GridPaintResult painted =
                    GridPainter::Paint(frame, fonts, item.grid, columnX, cursorY, columnW, paintScale);
                cursorY += painted.advanceY;
                const GridBounds& bounds = item.grid.GetBounds();
                contentRight = std::max(contentRight, bounds.x + bounds.width + scrollX_);
            } else if (item.kind == SceneItemKind::Svg) {
                const SvgPaintResult painted =
                    SvgPainter::Paint(frame, item.svg, columnX, cursorY, columnW, paintScale);
                cursorY += painted.advanceY;
                const SvgBounds& bounds = item.svg.GetBounds();
                contentRight = std::max(contentRight, bounds.x + bounds.width + scrollX_);
            } else if (item.kind == SceneItemKind::BrandLogo) {
                const BrandLogoPaintResult painted =
                    BrandLogoPainter::Paint(frame, fonts, item.brandLogo, columnX, cursorY, paintScale);
                cursorY += painted.advanceY;
                const BrandLogoMeasureResult measured = BrandLogoPainter::Measure(fonts, item.brandLogo, paintScale);
                contentRight = std::max(contentRight, columnX + measured.width + scrollX_);
            } else if (item.kind == SceneItemKind::TitleBar) {
                item.titleBar.SetSidebarOpen(sidebarVisible_);
                item.titleBar.SetSidebarAmount(sidebarAmount_);
                const float barX = 0.0f;
                const float barY = cursorY + scrollY_;
                const float barWidth = static_cast<float>(width);
                const TitleBarPaintResult painted =
                    TitleBarPainter::Paint(frame, fonts, item.titleBar, barX, barY, barWidth, paintScale);
                cursorY += painted.advanceY;
                flowEdge = yBefore + painted.height;
                edgeSet = true;
                titleBarBottom = barY + painted.height;
                contentRight = std::max(contentRight, pad + contentWidth);
                if (!titleClip) {
                    const float clipTop = barY + painted.height;
                    frame.PushClipRect(0.0f, clipTop, static_cast<float>(width), std::max(0.0f, static_cast<float>(height) - clipTop));
                    titleClip = true;
                }
            } else if (item.kind == SceneItemKind::Sidebar) {
                const float surfaceTop = flowEdge + scrollY_;
                const float sidebarHeight = std::max(0.0f, static_cast<float>(height) - horizontalReserve - surfaceTop);
                const float paneWidth = item.sidebar.LayoutWidth(sidebarAmount_) * paintScale;
                if (paneWidth > 0.5f) {
                    frame.PushClipRect(columnX, surfaceTop, paneWidth, sidebarHeight);
                    SidebarPainter::Paint(frame, fonts, item.sidebar, columnX, surfaceTop, sidebarHeight, paintScale, sidebarAmount_);
                    frame.PopClip();
                    columnX += paneWidth;
                    columnW = std::max(0.0f, columnW - paneWidth);
                }
                cursorY = flowEdge;
                edgeSet = true;
                contentRight = std::max(contentRight, columnX + scrollX_);
            } else if (item.kind == SceneItemKind::SidebarItem) {
                const SidebarItemMeasureResult measured = SidebarPainter::MeasureItem(fonts, item.sidebarItem, paintScale);
                SidebarPainter::PaintItem(frame, fonts, item.sidebarItem, columnX, cursorY, measured.width, measured.height, paintScale, false);
                cursorY += measured.height + 8.0f * paintScale;
                contentRight = std::max(contentRight, columnX + measured.width + scrollX_);
            } else if (item.kind == SceneItemKind::Separator) {
                const bool vertical = item.separator.GetOrientation() == SeparatorOrientation::Vertical;
                const SeparatorMeasureResult measured =
                    SeparatorPainter::Measure(fonts, item.separator, columnW, 0.0f, paintScale);
                const float boxWidth = vertical ? measured.width : columnW;
                const float boxHeight = measured.height;
                const SeparatorPaintResult painted =
                    SeparatorPainter::Paint(frame, fonts, item.separator, columnX, cursorY, boxWidth, boxHeight, paintScale);
                cursorY += painted.advanceY;
                contentRight = std::max(contentRight, columnX + painted.width + scrollX_);
            } else if (item.kind == SceneItemKind::ThemeToggle) {
                const ThemeTogglePaintResult painted =
                    ThemeTogglePainter::Paint(frame, fonts, item.themeToggle, columnX, cursorY, columnW, paintScale);
                cursorY += painted.advanceY;
                contentRight = std::max(contentRight, columnX + item.themeToggle.GetBounds().width + scrollX_);
            } else if (item.kind == SceneItemKind::ThemeIconToggle) {
                const ThemeIconTogglePaintResult painted =
                    ThemeIconTogglePainter::Paint(frame, item.themeIconToggle, columnX, cursorY, paintScale);
                cursorY += painted.advanceY;
                contentRight = std::max(contentRight, columnX + item.themeIconToggle.GetBounds().width + scrollX_);
            } else if (item.kind == SceneItemKind::MotionShape) {
                item.motionShape.Paint(frame, columnX, cursorY, columnW, item.motionShape.Height() * paintScale, paintScale);
                const float advance = item.motionShape.Height() * paintScale + 12.0f * paintScale;
                cursorY += advance;
                contentRight = std::max(contentRight, columnX + item.motionShape.Width() * paintScale + scrollX_);
            } else if (item.kind == SceneItemKind::Position) {
                const PositionPaintResult painted =
                    PositionPainter::Paint(frame, fonts, item.position, columnX, cursorY, columnW, paintScale);
                cursorY += painted.advanceY;
                const InputBounds& bounds = item.position.GetBounds();
                contentRight = std::max(contentRight, bounds.x + bounds.width + scrollX_);
            }
            if (!edgeSet) {
                flowEdge = cursorY;
            }
        }
        if (desktopClip) {
            frame.PopClip();
            frame.Pump();
            DesktopPainter::Paint(frame, fonts, desktop_, desktopChrome, paintScale, hoveredDock_);
        }
        if (titleClip) {
            frame.PopClip();
        }

        float contentHeight = cursorY + scrollY_ + pad;
        float viewportHeight = std::max(1.0f, static_cast<float>(height) - horizontalReserve);
        if (desktop_.IsEnabled()) {
            contentHeight = (cursorY + scrollY_) - desktopChrome.contentY + pad;
            viewportHeight = std::max(1.0f, desktopChrome.contentHeight);
        }
        const float viewportWidth = std::max(1.0f, static_cast<float>(width) - verticalReserve);
        const float nextMaxY = std::max(0.0f, contentHeight - viewportHeight);
        const float nextMaxX = std::max(0.0f, contentRight + pad - viewportWidth);
        const float nextVertical = nextMaxY > 1.0f ? barThickness : 0.0f;
        const float nextHorizontal = nextMaxX > 1.0f ? barThickness : 0.0f;
        const bool scrollClamped = scrollY_ > nextMaxY + 0.5f || scrollX_ > nextMaxX + 0.5f;
        maxScrollY_ = nextMaxY;
        maxScrollX_ = nextMaxX;
        scrollY_ = std::max(0.0f, std::min(scrollY_, maxScrollY_));
        scrollX_ = std::max(0.0f, std::min(scrollX_, maxScrollX_));
        if (!partial && pass < 2 && (nextVertical != verticalReserve || nextHorizontal != horizontalReserve || scrollClamped)) {
            verticalReserve = nextVertical;
            horizontalReserve = nextHorizontal;
            continue;
        }
        verticalReserve = nextVertical;
        horizontalReserve = nextHorizontal;
        break;
    }

    const auto syncBar = [](Scrollbar& bar, float scroll, float maxScroll, float viewport) {
        bar.SetContentSize(static_cast<double>(maxScroll + viewport));
        bar.SetViewportSize(static_cast<double>(viewport));
        if (!bar.IsDragging() && !bar.NeedsAnimation() && bar.GetPressedArrow() == ScrollbarArrow::None) {
            bar.SetValue(maxScroll > 0.0f ? static_cast<double>(scroll / maxScroll) : 0.0);
        }
    };
    syncBar(pageVertical_, scrollY_, maxScrollY_, static_cast<float>(height));
    syncBar(pageHorizontal_, scrollX_, maxScrollX_, static_cast<float>(width));

    const auto paintPageBar = [&](Scrollbar& bar, float x, float y, float barWidth, float barHeight) {
        if (bar.IsDragging() || draggingScrollbar_ == &bar) {
            bar.SetInteraction(ScrollbarInteraction::Dragging);
        } else if (hoveredScrollbar_ == &bar) {
            bar.SetInteraction(ScrollbarInteraction::Hovered);
        } else {
            bar.SetInteraction(ScrollbarInteraction::Normal);
        }
        ScrollbarPainter::Paint(frame, bar, x, y, barWidth, paintScale, barWidth, barHeight);
    };

    if (maxScrollY_ > 1.0f) {
        const float barTop = titleBarBottom > 0.5f ? titleBarBottom : edge;
        paintPageBar(pageVertical_,
                     static_cast<float>(width) - barThickness,
                     barTop,
                     barThickness,
                     std::max(0.0f, static_cast<float>(height) - barTop - horizontalReserve));
    } else {
        pageVertical_.SetBounds({});
    }
    if (maxScrollX_ > 1.0f) {
        paintPageBar(pageHorizontal_,
                     edge,
                     static_cast<float>(height) - barThickness,
                     std::max(0.0f, static_cast<float>(width) - edge * 2.0f - verticalReserve),
                     barThickness);
    } else {
        pageHorizontal_.SetBounds({});
    }

    TextSelection::EndFrame();
    if (TextSelection::ShowsHandles()) {
        float startX = 0.0f;
        float startY = 0.0f;
        float endX = 0.0f;
        float endY = 0.0f;
        float lineHeight = 0.0f;
        if (TextSelection::HandleGeometry(startX, startY, endX, endY, lineHeight)) {
            const float radius = std::max(5.0f, lineHeight * 0.22f);
            const Color handle = Color::FromHex("#007AFF");
            frame.FillRoundedRect(startX - radius, startY + lineHeight - radius * 0.15f, radius * 2.0f, radius * 2.0f,
                                  radius, handle, 1.0f);
            frame.FillRoundedRect(endX - radius, endY + lineHeight - radius * 0.15f, radius * 2.0f, radius * 2.0f,
                                  radius, handle, 1.0f);
        }
    }
    if (partial) {
        frame.PopClip();
    }
    if (!partial && desktop_.IsEnabled() && desktop_.GetBackgroundSource().PaintsMedia()) {
        CaptureChrome();
    }
    if (!partial || damageWidth <= 0 || damageHeight <= 0) {
        damageX = 0;
        damageY = 0;
        damageWidth = width;
        damageHeight = height;
    }
    frame.SetPump(nullptr, nullptr);
    frame.SetPresentRect(damageX, damageY, damageWidth, damageHeight);
    fullDirty_ = false;
    damageOpen_ = false;
    frameReady_ = true;
    paintedScrollX_ = scrollX_;
    paintedScrollY_ = scrollY_;
    paintedScale_ = paintScale;
    paintedWidth_ = width;
    paintedHeight_ = height;
    paintedVerticalReserve_ = verticalReserve;
    paintedHorizontalReserve_ = horizontalReserve;
    platform.Present(frame);
}

bool RoboScene::ScrollBy(float deltaY) {
    if (!hasRenderContext_ || deltaY == 0.0f || maxScrollY_ <= 0.0f) {
        return false;
    }
    if (!pageVertical_.ScrollByPixels(static_cast<double>(deltaY))) {
        return false;
    }
    RerenderIfPossible();
    return true;
}

bool RoboScene::ZoomTo(float zoom, float anchorX, float anchorY) {
    constexpr float MIN_ZOOM = 0.5f;
    constexpr float MAX_ZOOM = 3.0f;
    if (!std::isfinite(zoom) || !std::isfinite(anchorX) || !std::isfinite(anchorY)) {
        return false;
    }
    const float next = std::max(MIN_ZOOM, std::min(MAX_ZOOM, zoom));
    if (std::fabs(next - appZoom_) < 0.0005f) {
        return false;
    }
    const float ratio = next / std::max(MIN_ZOOM, appZoom_);
    scrollX_ = (scrollX_ + anchorX) * ratio - anchorX;
    scrollY_ = (scrollY_ + anchorY) * ratio - anchorY;
    if (scrollX_ < 0.0f) {
        scrollX_ = 0.0f;
    }
    if (scrollY_ < 0.0f) {
        scrollY_ = 0.0f;
    }
    maxScrollX_ *= ratio;
    maxScrollY_ *= ratio;
    appZoom_ = next;
    RerenderIfPossible();
    return true;
}

bool RoboScene::ZoomBy(float factor, float anchorX, float anchorY) {
    if (!(factor > 0.0f)) {
        return false;
    }
    return ZoomTo(appZoom_ * factor, anchorX, anchorY);
}

float RoboScene::GetZoom() const {
    return appZoom_;
}

bool RoboScene::ScrollByX(float deltaX) {
    if (!hasRenderContext_ || deltaX == 0.0f || maxScrollX_ <= 0.0f) {
        return false;
    }
    if (!pageHorizontal_.ScrollByPixels(static_cast<double>(deltaX))) {
        return false;
    }
    RerenderIfPossible();
    return true;
}

ThemeToggle* FindThemeToggleInFlexbox(Flexbox& flexbox, float x, float y);
ThemeToggle* FindThemeToggleInGrid(Grid& grid, float x, float y);
ThemeToggle* FindThemeToggleInPosition(Position& position, float x, float y);
ThemeToggle* FindThemeToggleInContainer(Container& container, float x, float y);
ThemeIconToggle* FindThemeIconInFlexbox(Flexbox& flexbox, float x, float y);
ThemeIconToggle* FindThemeIconInGrid(Grid& grid, float x, float y);
ThemeIconToggle* FindThemeIconInPosition(Position& position, float x, float y);
ThemeIconToggle* FindThemeIconInContainer(Container& container, float x, float y);

ThemeToggle* FindThemeToggleInFlexbox(Flexbox& flexbox, float x, float y) {
    std::vector<FlexboxItem>& items = flexbox.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        FlexboxItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == FlexItemKind::ThemeToggle && item.themeToggle.GetBounds().Contains(x, y)) {
            return &item.themeToggle;
        }
        if (item.kind == FlexItemKind::Container && item.container) {
            if (ThemeToggle* found = FindThemeToggleInContainer(*item.container, x, y)) {
                return found;
            }
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            if (ThemeToggle* found = FindThemeToggleInFlexbox(*item.flexbox, x, y)) {
                return found;
            }
        }
    }
    return nullptr;
}

ThemeIconToggle* FindThemeIconInFlexbox(Flexbox& flexbox, float x, float y) {
    std::vector<FlexboxItem>& items = flexbox.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        FlexboxItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == FlexItemKind::ThemeIconToggle && item.themeIconToggle.GetBounds().Contains(x, y)) {
            return &item.themeIconToggle;
        }
        if (item.kind == FlexItemKind::TitleBar && item.titleBar.ThemeIcon().GetBounds().Contains(x, y)) {
            return &item.titleBar.ThemeIcon();
        }
        if (item.kind == FlexItemKind::Container && item.container) {
            if (ThemeIconToggle* found = FindThemeIconInContainer(*item.container, x, y)) {
                return found;
            }
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            if (ThemeIconToggle* found = FindThemeIconInFlexbox(*item.flexbox, x, y)) {
                return found;
            }
        }
    }
    return nullptr;
}

ThemeToggle* FindThemeToggleInGrid(Grid& grid, float x, float y) {
    std::vector<GridItem>& items = grid.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        GridItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == GridItemKind::ThemeToggle && item.themeToggle.GetBounds().Contains(x, y)) {
            return &item.themeToggle;
        }
        if (item.kind == GridItemKind::Container && item.container) {
            if (ThemeToggle* found = FindThemeToggleInContainer(*item.container, x, y)) {
                return found;
            }
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            if (ThemeToggle* found = FindThemeToggleInFlexbox(*item.flexbox, x, y)) {
                return found;
            }
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            if (ThemeToggle* found = FindThemeToggleInGrid(*item.grid, x, y)) {
                return found;
            }
        }
    }
    return nullptr;
}

ThemeIconToggle* FindThemeIconInGrid(Grid& grid, float x, float y) {
    std::vector<GridItem>& items = grid.GetItems();
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        GridItem& item = items[static_cast<std::size_t>(i)];
        if (item.kind == GridItemKind::ThemeIconToggle && item.themeIconToggle.GetBounds().Contains(x, y)) {
            return &item.themeIconToggle;
        }
        if (item.kind == GridItemKind::TitleBar && item.titleBar.ThemeIcon().GetBounds().Contains(x, y)) {
            return &item.titleBar.ThemeIcon();
        }
        if (item.kind == GridItemKind::Container && item.container) {
            if (ThemeIconToggle* found = FindThemeIconInContainer(*item.container, x, y)) {
                return found;
            }
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            if (ThemeIconToggle* found = FindThemeIconInFlexbox(*item.flexbox, x, y)) {
                return found;
            }
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            if (ThemeIconToggle* found = FindThemeIconInGrid(*item.grid, x, y)) {
                return found;
            }
        }
    }
    return nullptr;
}

ThemeToggle* FindThemeToggleInPosition(Position& position, float x, float y) {
    for (PositionItem& item : position.GetItems()) {
        if (item.kind == PositionItemKind::ThemeToggle && item.themeToggle.GetBounds().Contains(x, y)) {
            return &item.themeToggle;
        }
        if (item.kind == PositionItemKind::Container && item.container) {
            if (ThemeToggle* found = FindThemeToggleInContainer(*item.container, x, y)) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            if (ThemeToggle* found = FindThemeToggleInFlexbox(*item.flexbox, x, y)) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            if (ThemeToggle* found = FindThemeToggleInGrid(*item.grid, x, y)) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Position && item.position) {
            if (ThemeToggle* found = FindThemeToggleInPosition(*item.position, x, y)) {
                return found;
            }
        }
    }
    return nullptr;
}

ThemeIconToggle* FindThemeIconInPosition(Position& position, float x, float y) {
    for (PositionItem& item : position.GetItems()) {
        if (item.kind == PositionItemKind::ThemeIconToggle && item.themeIconToggle.GetBounds().Contains(x, y)) {
            return &item.themeIconToggle;
        }
        if (item.kind == PositionItemKind::TitleBar && item.titleBar.ThemeIcon().GetBounds().Contains(x, y)) {
            return &item.titleBar.ThemeIcon();
        }
        if (item.kind == PositionItemKind::Container && item.container) {
            if (ThemeIconToggle* found = FindThemeIconInContainer(*item.container, x, y)) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            if (ThemeIconToggle* found = FindThemeIconInFlexbox(*item.flexbox, x, y)) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            if (ThemeIconToggle* found = FindThemeIconInGrid(*item.grid, x, y)) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Position && item.position) {
            if (ThemeIconToggle* found = FindThemeIconInPosition(*item.position, x, y)) {
                return found;
            }
        }
    }
    return nullptr;
}

ThemeToggle* FindThemeToggleInContainer(Container& container, float x, float y) {
    if (!container.GetBounds().Contains(x, y)) {
        return nullptr;
    }
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        return FindThemeToggleInFlexbox(container.GetFlexbox(), x, y);
    }
    if (container.GetStyle().layout == ContainerLayout::Grid) {
        return FindThemeToggleInGrid(container.GetGrid(), x, y);
    }
    return FindThemeToggleInPosition(container.GetContent(), x, y);
}

ThemeIconToggle* FindThemeIconInContainer(Container& container, float x, float y) {
    if (!container.GetBounds().Contains(x, y)) {
        return nullptr;
    }
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        return FindThemeIconInFlexbox(container.GetFlexbox(), x, y);
    }
    if (container.GetStyle().layout == ContainerLayout::Grid) {
        return FindThemeIconInGrid(container.GetGrid(), x, y);
    }
    return FindThemeIconInPosition(container.GetContent(), x, y);
}

ThemeIconHot HotFor(const ThemeIconToggle& toggle, float x, float y) {
    if (toggle.IsDisabled() || !toggle.GetBounds().Contains(x, y)) {
        return ThemeIconHot::None;
    }
    const ThemeTogglePart part = toggle.HitPart(x, y);
    if (part == ThemeTogglePart::Light) {
        return ThemeIconHot::Light;
    }
    if (part == ThemeTogglePart::Dark) {
        return ThemeIconHot::Dark;
    }
    if (part == ThemeTogglePart::Switch) {
        return ThemeIconHot::Button;
    }
    return ThemeIconHot::None;
}

bool SyncHotInGrid(Grid& grid, float x, float y);
bool SyncHotInPosition(Position& position, float x, float y);
bool SyncHotInContainer(Container& container, float x, float y);

bool SyncHotInFlexbox(Flexbox& flexbox, float x, float y) {
    bool changed = false;
    for (FlexboxItem& item : flexbox.GetItems()) {
        if (item.kind == FlexItemKind::ThemeIconToggle) {
            const ThemeIconHot next = HotFor(item.themeIconToggle, x, y);
            if (item.themeIconToggle.GetHot() != next) {
                item.themeIconToggle.SetHot(next);
                changed = true;
            }
        } else if (item.kind == FlexItemKind::TitleBar) {
            const ThemeIconHot next = HotFor(item.titleBar.ThemeIcon(), x, y);
            if (item.titleBar.ThemeIcon().GetHot() != next) {
                item.titleBar.ThemeIcon().SetHot(next);
                changed = true;
            }
        } else if (item.kind == FlexItemKind::Container && item.container) {
            changed = SyncHotInContainer(*item.container, x, y) || changed;
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            changed = SyncHotInFlexbox(*item.flexbox, x, y) || changed;
        }
    }
    return changed;
}

bool SyncHotInGrid(Grid& grid, float x, float y) {
    bool changed = false;
    for (GridItem& item : grid.GetItems()) {
        if (item.kind == GridItemKind::ThemeIconToggle) {
            const ThemeIconHot next = HotFor(item.themeIconToggle, x, y);
            if (item.themeIconToggle.GetHot() != next) {
                item.themeIconToggle.SetHot(next);
                changed = true;
            }
        } else if (item.kind == GridItemKind::TitleBar) {
            const ThemeIconHot next = HotFor(item.titleBar.ThemeIcon(), x, y);
            if (item.titleBar.ThemeIcon().GetHot() != next) {
                item.titleBar.ThemeIcon().SetHot(next);
                changed = true;
            }
        } else if (item.kind == GridItemKind::Container && item.container) {
            changed = SyncHotInContainer(*item.container, x, y) || changed;
        } else if (item.kind == GridItemKind::Flexbox && item.flexbox) {
            changed = SyncHotInFlexbox(*item.flexbox, x, y) || changed;
        } else if (item.kind == GridItemKind::Grid && item.grid) {
            changed = SyncHotInGrid(*item.grid, x, y) || changed;
        }
    }
    return changed;
}

bool SyncHotInPosition(Position& position, float x, float y) {
    bool changed = false;
    for (PositionItem& item : position.GetItems()) {
        if (item.kind == PositionItemKind::ThemeIconToggle) {
            const ThemeIconHot next = HotFor(item.themeIconToggle, x, y);
            if (item.themeIconToggle.GetHot() != next) {
                item.themeIconToggle.SetHot(next);
                changed = true;
            }
        } else if (item.kind == PositionItemKind::TitleBar) {
            const ThemeIconHot next = HotFor(item.titleBar.ThemeIcon(), x, y);
            if (item.titleBar.ThemeIcon().GetHot() != next) {
                item.titleBar.ThemeIcon().SetHot(next);
                changed = true;
            }
        } else if (item.kind == PositionItemKind::Container && item.container) {
            changed = SyncHotInContainer(*item.container, x, y) || changed;
        } else if (item.kind == PositionItemKind::Flexbox && item.flexbox) {
            changed = SyncHotInFlexbox(*item.flexbox, x, y) || changed;
        } else if (item.kind == PositionItemKind::Grid && item.grid) {
            changed = SyncHotInGrid(*item.grid, x, y) || changed;
        } else if (item.kind == PositionItemKind::Position && item.position) {
            changed = SyncHotInPosition(*item.position, x, y) || changed;
        }
    }
    return changed;
}

bool SyncHotInContainer(Container& container, float x, float y) {
    if (container.GetStyle().layout == ContainerLayout::Flex) {
        return SyncHotInFlexbox(container.GetFlexbox(), x, y);
    }
    if (container.GetStyle().layout == ContainerLayout::Grid) {
        return SyncHotInGrid(container.GetGrid(), x, y);
    }
    return SyncHotInPosition(container.GetContent(), x, y);
}

ThemeToggle* RoboScene::FindThemeToggleAt(float x, float y) {
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        SceneItem& item = items_[static_cast<std::size_t>(i)];
        if (item.kind == SceneItemKind::ThemeToggle && item.themeToggle.GetBounds().Contains(x, y)) {
            return &item.themeToggle;
        }
        if (item.kind == SceneItemKind::Container) {
            if (ThemeToggle* found = FindThemeToggleInContainer(*item.container, x, y)) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Flexbox) {
            if (ThemeToggle* found = FindThemeToggleInFlexbox(item.flexbox, x, y)) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Grid) {
            if (ThemeToggle* found = FindThemeToggleInGrid(item.grid, x, y)) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Position) {
            if (ThemeToggle* found = FindThemeToggleInPosition(item.position, x, y)) {
                return found;
            }
        }
    }
    return nullptr;
}

ThemeIconToggle* RoboScene::FindThemeIconToggleAt(float x, float y) {
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        SceneItem& item = items_[static_cast<std::size_t>(i)];
        if (item.kind == SceneItemKind::ThemeIconToggle && item.themeIconToggle.GetBounds().Contains(x, y)) {
            return &item.themeIconToggle;
        }
        if (item.kind == SceneItemKind::TitleBar && item.titleBar.ThemeIcon().GetBounds().Contains(x, y)) {
            return &item.titleBar.ThemeIcon();
        }
        if (item.kind == SceneItemKind::Container) {
            if (ThemeIconToggle* found = FindThemeIconInContainer(*item.container, x, y)) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Flexbox) {
            if (ThemeIconToggle* found = FindThemeIconInFlexbox(item.flexbox, x, y)) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Grid) {
            if (ThemeIconToggle* found = FindThemeIconInGrid(item.grid, x, y)) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Position) {
            if (ThemeIconToggle* found = FindThemeIconInPosition(item.position, x, y)) {
                return found;
            }
        }
    }
    return nullptr;
}

bool SyncThemeHot(RoboScene& scene, std::vector<SceneItem>& items, float x, float y) {
    (void)scene;
    bool changed = false;
    for (SceneItem& item : items) {
        if (item.kind == SceneItemKind::ThemeIconToggle) {
            const ThemeIconHot next = HotFor(item.themeIconToggle, x, y);
            if (item.themeIconToggle.GetHot() != next) {
                item.themeIconToggle.SetHot(next);
                changed = true;
            }
        } else if (item.kind == SceneItemKind::TitleBar) {
            const ThemeIconHot next = HotFor(item.titleBar.ThemeIcon(), x, y);
            if (item.titleBar.ThemeIcon().GetHot() != next) {
                item.titleBar.ThemeIcon().SetHot(next);
                changed = true;
            }
        } else if (item.kind == SceneItemKind::Container && item.container) {
            changed = SyncHotInContainer(*item.container, x, y) || changed;
        } else if (item.kind == SceneItemKind::Flexbox) {
            changed = SyncHotInFlexbox(item.flexbox, x, y) || changed;
        } else if (item.kind == SceneItemKind::Grid) {
            changed = SyncHotInGrid(item.grid, x, y) || changed;
        } else if (item.kind == SceneItemKind::Position) {
            changed = SyncHotInPosition(item.position, x, y) || changed;
        }
    }
    return changed;
}

Button* RoboScene::FindButtonAt(float x, float y) {
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        SceneItem& item = items_[static_cast<std::size_t>(i)];
        if (item.kind == SceneItemKind::Button) {
            if (item.button.GetBounds().Contains(x, y)) {
                return &item.button;
            }
        } else if (item.kind == SceneItemKind::TitleBar) {
            if (item.titleBar.SidebarButton().GetBounds().Contains(x, y)) {
                return &item.titleBar.SidebarButton();
            }
        } else if (item.kind == SceneItemKind::Container) {
            Button* found = FindButtonInContainer(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Flexbox) {
            Button* found = FindButtonInFlexbox(item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Grid) {
            Button* found = FindButtonInGrid(item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Position) {
            Button* found = FindButtonInPosition(item.position, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

Input* RoboScene::FindInputAt(float x, float y) {
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        SceneItem& item = items_[static_cast<std::size_t>(i)];
        if (item.kind == SceneItemKind::Input) {
            if (item.input.GetBounds().Contains(x, y)) {
                return &item.input;
            }
        } else if (item.kind == SceneItemKind::Container) {
            Input* found = FindInputInContainer(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Flexbox) {
            Input* found = FindInputInFlexbox(item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Grid) {
            Input* found = FindInputInGrid(item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Position) {
            Input* found = FindInputInPosition(item.position, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

Scrollbar* RoboScene::PageScrollbarAt(float x, float y) {
    if (pageVertical_.IsVisible() && pageVertical_.GetBounds().Contains(x, y)) {
        return &pageVertical_;
    }
    if (pageHorizontal_.IsVisible() && pageHorizontal_.GetBounds().Contains(x, y)) {
        return &pageHorizontal_;
    }
    return nullptr;
}

const Scrollbar* RoboScene::PageScrollbarAt(float x, float y) const {
    if (pageVertical_.IsVisible() && pageVertical_.GetBounds().Contains(x, y)) {
        return &pageVertical_;
    }
    if (pageHorizontal_.IsVisible() && pageHorizontal_.GetBounds().Contains(x, y)) {
        return &pageHorizontal_;
    }
    return nullptr;
}

Scrollbar* RoboScene::FindScrollbarAt(float x, float y) {
    if (Scrollbar* pageBar = PageScrollbarAt(x, y)) {
        return pageBar;
    }
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        SceneItem& item = items_[static_cast<std::size_t>(i)];
        if (item.kind == SceneItemKind::Sidebar) {
            Scrollbar& bar = item.sidebar.PanelScrollbar();
            if (bar.IsVisible() && bar.GetBounds().Contains(x, y)) {
                return &bar;
            }
        } else if (item.kind == SceneItemKind::Scrollbar) {
            if (item.scrollbar.IsVisible() && item.scrollbar.GetBounds().Contains(x, y)) {
                return &item.scrollbar;
            }
        } else if (item.kind == SceneItemKind::Container) {
            Scrollbar* found = FindScrollbarInContainer(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Flexbox) {
            Scrollbar* found = FindScrollbarInFlexbox(item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Grid) {
            Scrollbar* found = FindScrollbarInGrid(item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Position) {
            Scrollbar* found = FindScrollbarInPosition(item.position, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

const Button* RoboScene::FindButtonAt(float x, float y) const {
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        const SceneItem& item = items_[static_cast<std::size_t>(i)];
        if (item.kind == SceneItemKind::Button) {
            if (item.button.GetBounds().Contains(x, y)) {
                return &item.button;
            }
        } else if (item.kind == SceneItemKind::TitleBar) {
            if (item.titleBar.SidebarButton().GetBounds().Contains(x, y)) {
                return &item.titleBar.SidebarButton();
            }
        } else if (item.kind == SceneItemKind::Container) {
            const Button* found = FindButtonInContainerConst(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Flexbox) {
            const Button* found = FindButtonInFlexboxConst(item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Grid) {
            const Button* found = FindButtonInGridConst(item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Position) {
            const Button* found = FindButtonInPositionConst(item.position, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

const Input* RoboScene::FindInputAt(float x, float y) const {
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        const SceneItem& item = items_[static_cast<std::size_t>(i)];
        if (item.kind == SceneItemKind::Input) {
            if (item.input.GetBounds().Contains(x, y)) {
                return &item.input;
            }
        } else if (item.kind == SceneItemKind::Container) {
            const Input* found = FindInputInContainerConst(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Flexbox) {
            const Input* found = FindInputInFlexboxConst(item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Grid) {
            const Input* found = FindInputInGridConst(item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Position) {
            const Input* found = FindInputInPositionConst(item.position, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

const Scrollbar* RoboScene::FindScrollbarAt(float x, float y) const {
    if (const Scrollbar* pageBar = PageScrollbarAt(x, y)) {
        return pageBar;
    }
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        const SceneItem& item = items_[static_cast<std::size_t>(i)];
        if (item.kind == SceneItemKind::Sidebar) {
            const Scrollbar& bar = item.sidebar.PanelScrollbar();
            if (bar.IsVisible() && bar.GetBounds().Contains(x, y)) {
                return &bar;
            }
        } else if (item.kind == SceneItemKind::Scrollbar) {
            if (item.scrollbar.IsVisible() && item.scrollbar.GetBounds().Contains(x, y)) {
                return &item.scrollbar;
            }
        } else if (item.kind == SceneItemKind::Container) {
            const Scrollbar* found = FindScrollbarInContainerConst(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Flexbox) {
            const Scrollbar* found = FindScrollbarInFlexboxConst(item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Grid) {
            const Scrollbar* found = FindScrollbarInGridConst(item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Position) {
            const Scrollbar* found = FindScrollbarInPositionConst(item.position, x, y);
            if (found) {
                return found;
            }
        }
    }
    return nullptr;
}

bool RoboScene::SetFocusedScrollbar(Scrollbar* scrollbar) {
    if (focusedScrollbar_ == scrollbar) {
        return false;
    }
    DirtyScrollbar(focusedScrollbar_);
    if (focusedScrollbar_) {
        focusedScrollbar_->SetFocused(false);
        if (focusedScrollbar_ != hoveredScrollbar_ && focusedScrollbar_ != draggingScrollbar_) {
            focusedScrollbar_->SetInteraction(ScrollbarInteraction::Normal);
        }
    }
    focusedScrollbar_ = scrollbar;
    if (focusedScrollbar_) {
        focusedScrollbar_->SetFocused(true);
        if (focusedScrollbar_ != draggingScrollbar_) {
            focusedScrollbar_->SetInteraction(ScrollbarInteraction::Hovered);
        }
    }
    DirtyScrollbar(focusedScrollbar_);
    return true;
}

Scrollbar* RoboScene::ActiveScrollbar() {
    // A control the user explicitly interacted with wins over a passing hover,
    // so keyboard scrolling keeps going to the last one that was used.
    if (focusedScrollbar_) {
        return focusedScrollbar_;
    }
    return hoveredScrollbar_;
}

Container* RoboScene::FindContainerAt(float x, float y) {
    // Scene items are painted in order, so later ones sit on top.
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        SceneItem& item = items_[static_cast<std::size_t>(i)];
        Container* found = nullptr;
        if (item.kind == SceneItemKind::Container) {
            found = FindContainerInContainer(*item.container, x, y);
        } else if (item.kind == SceneItemKind::Flexbox) {
            found = FindContainerInFlexbox(item.flexbox, x, y);
        } else if (item.kind == SceneItemKind::Grid) {
            found = FindContainerInGrid(item.grid, x, y);
        } else if (item.kind == SceneItemKind::Position) {
            found = FindContainerInPosition(item.position, x, y);
        }
        if (found) {
            return found;
        }
    }
    return nullptr;
}

Scrollbar* RoboScene::ScrollTargetAt(float x, float y) {
    // Wheel scrolling goes to the innermost scroll region under the cursor and
    // bubbles outwards while that region has nothing left to scroll.
    std::vector<Container*> containers;
    CollectSceneContainers(items_, x, y, containers);
    for (Container* container : containers) {
        if (container->GetStyle().showScrollbar && container->IsOverflowing()) {
            return &container->GetScrollbar();
        }
    }
    if (Scrollbar* bar = FindScrollbarAt(x, y)) {
        return bar;
    }
    return FindScrollbarNear(x, y, 28.0f);
}

Scrollbar* RoboScene::FindScrollbarNear(float x, float y, float tolerance) {
    Scrollbar* best = nullptr;
    auto visit = [&](Scrollbar& scrollbar) {
        if (!scrollbar.IsVisible()) {
            return;
        }
        ScrollbarBounds bounds = scrollbar.GetBounds();
        if (scrollbar.GetOrientation() == ScrollbarOrientation::Vertical) {
            bounds.x -= tolerance;
            bounds.width += tolerance * 2.0f;
        } else {
            bounds.y -= tolerance;
            bounds.height += tolerance * 2.0f;
        }
        if (bounds.Contains(x, y)) {
            best = &scrollbar;  // Later controls are drawn on top, so they win.
        }
    };
    VisitSceneScrollbars(items_, visit);
    visit(pageVertical_);
    visit(pageHorizontal_);
    return best;
}

bool RoboScene::Wheel(float x, float y, float deltaX, float deltaY) {
    if (std::fabs(deltaY) >= std::fabs(deltaX)) {
        for (SceneItem& item : items_) {
            if (item.kind == SceneItemKind::Sidebar && item.sidebar.Contains(x, y) && item.sidebar.ScrollBy(deltaY, x)) {
                MarkFullDirty();
                return true;
            }
        }
    }
    // Prefer the bar whose axis matches the gesture, then any scroller under
    // the cursor. A mouse wheel over a horizontal bar still moves it.
    const bool wantHorizontal = std::fabs(deltaX) > std::fabs(deltaY);
    Scrollbar* target = nullptr;
    std::vector<Container*> containers;
    CollectSceneContainers(items_, x, y, containers);
    for (Container* container : containers) {
        if (!container->GetStyle().showScrollbar || !container->IsOverflowing()) {
            continue;
        }
        const bool horizontal = container->GetScrollbar().GetOrientation() == ScrollbarOrientation::Horizontal;
        if (horizontal == wantHorizontal) {
            target = &container->GetScrollbar();
            break;
        }
    }
    if (!target) {
        target = ScrollTargetAt(x, y);
    }
    if (!target) {
        bool moved = false;
        if (maxScrollY_ > 0.0f && deltaY != 0.0f) {
            moved = pageVertical_.ScrollByPixels(static_cast<double>(deltaY)) || moved;
        }
        if (maxScrollX_ > 0.0f && deltaX != 0.0f) {
            moved = pageHorizontal_.ScrollByPixels(static_cast<double>(deltaX)) || moved;
        }
        if (moved) {
            RerenderIfPossible();
        }
        return moved;
    }

    SetHoveredScrollbar(target);
    SetFocusedScrollbar(target);
    const bool horizontal = target->GetOrientation() == ScrollbarOrientation::Horizontal;
    double pixels = 0.0;
    if (horizontal) {
        pixels = std::fabs(deltaX) >= std::fabs(deltaY) ? static_cast<double>(deltaX) : static_cast<double>(deltaY);
    } else {
        pixels = std::fabs(deltaY) >= std::fabs(deltaX) ? static_cast<double>(deltaY) : static_cast<double>(deltaX);
    }
    if (pixels == 0.0) {
        return false;
    }
    const bool moved = target->ScrollByPixels(pixels);
    target->SetHotArrow(ScrollbarArrow::None);
    if (moved) {
        RerenderIfPossible();
    }
    return moved;
}

int RoboScene::ActiveScrollbarAxis() const {
    const Scrollbar* target = focusedScrollbar_ ? focusedScrollbar_ : hoveredScrollbar_;
    if (!target || !target->IsVisible()) {
        return 0;
    }
    return target->GetOrientation() == ScrollbarOrientation::Vertical ? 1 : 2;
}

bool RoboScene::OwnsScrollGesture(float x, float y) {
    if (FindScrollbarAt(x, y)) {
        return true;
    }
    std::vector<Container*> containers;
    CollectSceneContainers(items_, x, y, containers);
    for (Container* container : containers) {
        if (container->GetStyle().showScrollbar && container->IsOverflowing()) {
            return true;
        }
    }
    return false;
}

bool RoboScene::KeyScrollLine(int direction) {
    Scrollbar* target = ActiveScrollbar();
    if (!target || direction == 0) {
        return false;
    }
    const bool moved = target->ScrollByLines(static_cast<double>(direction));
    if (moved) {
        RerenderIfPossible();
    }
    return moved;
}

bool RoboScene::KeyScrollPage(int direction) {
    Scrollbar* target = ActiveScrollbar();
    if (!target || direction == 0) {
        return false;
    }
    const bool moved = target->ScrollByPage(static_cast<double>(direction));
    if (moved) {
        RerenderIfPossible();
    }
    return moved;
}

bool RoboScene::KeyScrollToStart() {
    Scrollbar* target = ActiveScrollbar();
    if (!target) {
        return false;
    }
    const bool moved = target->ScrollToStart();
    if (moved) {
        RerenderIfPossible();
    }
    return moved;
}

bool RoboScene::KeyScrollToEnd() {
    Scrollbar* target = ActiveScrollbar();
    if (!target) {
        return false;
    }
    const bool moved = target->ScrollToEnd();
    if (moved) {
        RerenderIfPossible();
    }
    return moved;
}

HitKind RoboScene::HitTestKind(float x, float y) const {
    if (FindScrollbarAt(x, y)) {
        return HitKind::Scrollbar;
    }
    if (const_cast<RoboScene*>(this)->FindThemeToggleAt(x, y) || const_cast<RoboScene*>(this)->FindThemeIconToggleAt(x, y) ||
        FindThemeMotionScene(const_cast<RoboScene*>(this)->items_, x, y)) {
        return HitKind::Theme;
    }
    for (const SceneItem& item : items_) {
        if (item.kind == SceneItemKind::Sidebar && item.sidebar.HitRailArrow(x, y) != SidebarRailArrow::None) {
            return HitKind::Button;
        }
    }
    if (const_cast<RoboScene*>(this)->FindSidebarItemAt(x, y)) {
        return HitKind::Button;
    }
    if (const Button* button = FindButtonAt(x, y)) {
        if (button->CanActivate()) {
            return HitKind::Button;
        }
    }
    if (FindInputAt(x, y)) {
        return HitKind::Input;
    }
    return HitKind::None;
}

bool RoboScene::HitTest(float x, float y) const {
    return HitTestKind(x, y) != HitKind::None;
}

bool RoboScene::SetHoveredButton(Button* button) {
    if (button && !button->CanActivate()) {
        button = nullptr;
    }
    if (hoveredButton_ == button) {
        return false;
    }
    DirtyButton(hoveredButton_);
    if (hoveredButton_ && hoveredButton_ != pressedButton_) {
        hoveredButton_->SetInteraction(ButtonInteraction::Normal);
    }
    hoveredButton_ = button;
    if (hoveredButton_ && hoveredButton_ != pressedButton_) {
        hoveredButton_->SetInteraction(ButtonInteraction::Hovered);
    }
    DirtyButton(hoveredButton_);
    return true;
}

bool RoboScene::SetHoveredInput(Input* input) {
    if (hoveredInput_ == input) {
        return false;
    }
    DirtyInput(hoveredInput_);
    if (hoveredInput_ && hoveredInput_ != focusedInput_) {
        hoveredInput_->SetInteraction(InputInteraction::Normal);
    }
    hoveredInput_ = input;
    if (hoveredInput_ && hoveredInput_ != focusedInput_) {
        hoveredInput_->SetInteraction(InputInteraction::Hovered);
    }
    DirtyInput(hoveredInput_);
    return true;
}

bool RoboScene::SetHoveredScrollbar(Scrollbar* scrollbar) {
    if (hoveredScrollbar_ == scrollbar) {
        if (scrollbar) {
            // Pointer stayed on the same control: keep it revealed.
            scrollbar->NotifyInteraction();
        }
        return false;
    }
    DirtyScrollbar(hoveredScrollbar_);
    if (hoveredScrollbar_) {
        hoveredScrollbar_->SetPointerInside(false);
        hoveredScrollbar_->SetHotArrow(ScrollbarArrow::None);
        if (hoveredScrollbar_ != draggingScrollbar_) {
            hoveredScrollbar_->SetInteraction(hoveredScrollbar_->IsFocused() ? ScrollbarInteraction::Hovered
                                                                             : ScrollbarInteraction::Normal);
        }
    }
    hoveredScrollbar_ = scrollbar;
    if (hoveredScrollbar_) {
        hoveredScrollbar_->SetPointerInside(true);
        if (hoveredScrollbar_ != draggingScrollbar_) {
            hoveredScrollbar_->SetInteraction(ScrollbarInteraction::Hovered);
        }
    }
    DirtyScrollbar(hoveredScrollbar_);
    return true;
}

bool RoboScene::SetPressedButton(Button* button) {
    if (button && !button->CanActivate()) {
        button = nullptr;
    }
    if (pressedButton_ == button) {
        return false;
    }
    DirtyButton(pressedButton_);
    if (pressedButton_) {
        const ButtonInteraction next =
            (pressedButton_ == hoveredButton_) ? ButtonInteraction::Hovered : ButtonInteraction::Normal;
        pressedButton_->SetInteraction(next);
    }
    pressedButton_ = button;
    if (pressedButton_) {
        pressedButton_->SetInteraction(ButtonInteraction::Pressed);
    }
    DirtyButton(pressedButton_);
    return true;
}

bool RoboScene::SetFocusedButton(Button* button, bool showRing) {
    if (button && !button->CanActivate()) {
        button = nullptr;
        showRing = false;
    }
    if (focusedButton_ == button) {
        if (focusedButton_ && focusedButton_->IsFocusVisible() != showRing) {
            focusedButton_->SetFocusVisible(showRing);
            DirtyButton(focusedButton_);
            return true;
        }
        return false;
    }
    DirtyButton(focusedButton_);
    if (focusedButton_) {
        focusedButton_->SetFocused(false);
    }
    focusedButton_ = button;
    if (focusedButton_) {
        focusedButton_->SetFocused(true);
        focusedButton_->SetFocusVisible(showRing);
    }
    DirtyButton(focusedButton_);
    return true;
}

SidebarItem* RoboScene::FindSidebarItemAt(float x, float y) {
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        SceneItem& item = items_[static_cast<std::size_t>(i)];
        if (item.kind == SceneItemKind::Sidebar) {
            if (SidebarItem* found = item.sidebar.HitItem(x, y)) {
                return found;
            }
        } else if (item.kind == SceneItemKind::SidebarItem) {
            if (item.sidebarItem.CanActivate() && item.sidebarItem.GetBounds().Contains(x, y)) {
                return &item.sidebarItem;
            }
        } else if (item.kind == SceneItemKind::Container && item.container) {
            if (SidebarItem* found = FindPaneInContainer(*item.container, x, y)) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Flexbox) {
            if (SidebarItem* found = FindPaneInFlexbox(item.flexbox, x, y)) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Grid) {
            if (SidebarItem* found = FindPaneInGrid(item.grid, x, y)) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Position) {
            if (SidebarItem* found = FindPaneInPosition(item.position, x, y)) {
                return found;
            }
        }
    }
    return nullptr;
}

bool RoboScene::SetHoveredSidebarItem(SidebarItem* item) {
    if (item && !item->CanActivate()) {
        item = nullptr;
    }
    if (hoveredSidebarItem_ == item) {
        return false;
    }
    if (hoveredSidebarItem_ && hoveredSidebarItem_ != pressedSidebarItem_) {
        hoveredSidebarItem_->SetInteraction(SidebarItemInteraction::Normal);
    }
    hoveredSidebarItem_ = item;
    if (hoveredSidebarItem_ && hoveredSidebarItem_ != pressedSidebarItem_) {
        hoveredSidebarItem_->SetInteraction(SidebarItemInteraction::Hovered);
    }
    MarkFullDirty();
    return true;
}

bool RoboScene::SetPressedSidebarItem(SidebarItem* item) {
    if (item && !item->CanActivate()) {
        item = nullptr;
    }
    if (pressedSidebarItem_ == item) {
        return false;
    }
    if (pressedSidebarItem_) {
        const SidebarItemInteraction next =
            (pressedSidebarItem_ == hoveredSidebarItem_) ? SidebarItemInteraction::Hovered : SidebarItemInteraction::Normal;
        pressedSidebarItem_->SetInteraction(next);
    }
    pressedSidebarItem_ = item;
    if (pressedSidebarItem_) {
        pressedSidebarItem_->SetInteraction(SidebarItemInteraction::Pressed);
    }
    MarkFullDirty();
    return true;
}

bool RoboScene::SetFocusedSidebarItem(SidebarItem* item, bool showRing) {
    if (item && !item->CanActivate()) {
        item = nullptr;
        showRing = false;
    }
    if (focusedSidebarItem_ == item) {
        if (focusedSidebarItem_ && focusedSidebarItem_->IsFocusVisible() != showRing) {
            focusedSidebarItem_->SetFocusVisible(showRing);
            MarkFullDirty();
            return true;
        }
        return false;
    }
    if (focusedSidebarItem_) {
        focusedSidebarItem_->SetFocused(false);
    }
    focusedSidebarItem_ = item;
    if (focusedSidebarItem_) {
        focusedSidebarItem_->SetFocused(true);
        focusedSidebarItem_->SetFocusVisible(showRing);
    }
    MarkFullDirty();
    return true;
}

bool RoboScene::UpdateRailArrows(float x, float y) {
    bool changed = false;
    for (SceneItem& item : items_) {
        if (item.kind != SceneItemKind::Sidebar) {
            continue;
        }
        const SidebarRailArrow hit = item.sidebar.HitRailArrow(x, y);
        if (item.sidebar.SetHotRailArrow(hit)) {
            changed = true;
        }
    }
    return changed;
}

bool RoboScene::PressRailArrow(float x, float y) {
    bool pressed = false;
    for (SceneItem& item : items_) {
        if (item.kind != SceneItemKind::Sidebar) {
            continue;
        }
        const SidebarRailArrow hit = item.sidebar.HitRailArrow(x, y);
        if (hit == SidebarRailArrow::None) {
            continue;
        }
        item.sidebar.SetPressedRailArrow(hit);
        item.sidebar.SetHotRailArrow(hit);
        item.sidebar.StepRail(hit);
        pressed = true;
    }
    if (!pressed) {
        return false;
    }
    SetPressedSidebarItem(nullptr);
    SetPressedButton(nullptr);
    SetFocusedButton(nullptr);
    SetFocusedInput(nullptr);
    MarkFullDirty();
    return true;
}

bool RoboScene::ReleaseRailArrows() {
    bool changed = false;
    for (SceneItem& item : items_) {
        if (item.kind != SceneItemKind::Sidebar) {
            continue;
        }
        if (item.sidebar.SetPressedRailArrow(SidebarRailArrow::None)) {
            changed = true;
        }
    }
    return changed;
}

Sidebar* RoboScene::SidebarOwning(SidebarItem* item) {
    if (!item) {
        return nullptr;
    }
    for (SceneItem& scene : items_) {
        if (scene.kind != SceneItemKind::Sidebar) {
            continue;
        }
        if (scene.sidebar.IndexOf(item) >= 0) {
            return &scene.sidebar;
        }
        for (SidebarItem& row : scene.sidebar.Items()) {
            for (SidebarItem& child : row.Children()) {
                if (&child == item) {
                    return &scene.sidebar;
                }
            }
        }
    }
    return nullptr;
}

void RoboScene::ActivateSidebarItem(SidebarItem* item) {
    if (!item || !item->CanActivate()) {
        return;
    }
    Sidebar* owner = SidebarOwning(item);
    if (!owner) {
        item->SetSelected(true);
        MarkFullDirty();
        return;
    }
    const int index = owner->IndexOf(item);
    if (index >= 0) {
        const bool activity = owner->GetVariant() == SidebarVariant::Activity;
        const bool same = index == owner->Selected();
        owner->Select(index);
        if (activity && same && sidebarVisible_) {
            ToggleSidebar();
        } else if (activity && !sidebarVisible_) {
            ToggleSidebar();
        }
        OpenSelectedChild(owner, index);
        RememberPane(item);
        MarkFullDirty();
        return;
    }
    owner->SelectChild(item);
    RememberPane(item);
    MarkFullDirty();
}

const std::string& RoboScene::ActivePaneId() {
    if (!activePaneId_.empty()) {
        return activePaneId_;
    }
    for (SceneItem& scene : items_) {
        if (scene.kind != SceneItemKind::Sidebar) {
            continue;
        }
        for (const SidebarItem& row : scene.sidebar.Items()) {
            if (!row.GetId().empty() && row.IsSelected()) {
                activePaneId_ = row.GetId();
                return activePaneId_;
            }
            for (const SidebarItem& child : row.Children()) {
                if (!child.GetId().empty() && child.IsSelected()) {
                    activePaneId_ = child.GetId();
                    return activePaneId_;
                }
            }
        }
    }
    return activePaneId_;
}

void RoboScene::OpenSelectedChild(Sidebar* owner, int index) {
    if (!owner || index < 0 || index >= static_cast<int>(owner->Items().size())) {
        return;
    }
    SidebarItem& row = owner->Items()[static_cast<std::size_t>(index)];
    for (SidebarItem& child : row.Children()) {
        if (child.IsSelected() && child.CanActivate()) {
            RememberPane(&child);
            return;
        }
    }
    for (SidebarItem& child : row.Children()) {
        if (!child.CanActivate()) {
            continue;
        }
        owner->SelectChild(&child);
        RememberPane(&child);
        return;
    }
}

void RoboScene::RememberPane(const SidebarItem* item) {
    if (!item || item->GetId().empty() || item->GetId() == activePaneId_) {
        return;
    }
    activePaneId_ = item->GetId();
    pageVertical_.SetValue(0);
    scrollY_ = 0.0f;
    SetFocusedButton(nullptr);
    SetFocusedInput(nullptr);
}

void RoboScene::CollectButtons(std::vector<Button*>& out) {
    for (SceneItem& item : items_) {
        if (item.kind == SceneItemKind::Button) {
            out.push_back(&item.button);
        } else if (item.kind == SceneItemKind::TitleBar) {
            out.push_back(&item.titleBar.SidebarButton());
        } else if (item.kind == SceneItemKind::Container && item.container) {
            if (!item.container->ShownWhen().empty() && item.container->ShownWhen() != ActivePaneId()) {
                continue;
            }
            CollectButtonsInContainer(*item.container, out);
        } else if (item.kind == SceneItemKind::Flexbox) {
            CollectButtonsInFlexbox(item.flexbox, out);
        } else if (item.kind == SceneItemKind::Grid) {
            CollectButtonsInGrid(item.grid, out);
        } else if (item.kind == SceneItemKind::Position) {
            CollectButtonsInPosition(item.position, out);
        }
    }
}

bool RoboScene::IsSidebarToggle(const Button* button) const {
    if (!button) {
        return false;
    }
    for (const SceneItem& item : items_) {
        if (item.kind == SceneItemKind::TitleBar && &item.titleBar.SidebarButton() == button) {
            return true;
        }
    }
    return false;
}

bool RoboScene::ShowOrHideSidebar() {
    ToggleSidebar();
    return true;
}

void RoboScene::ToggleSidebar() {
    sidebarVisible_ = !sidebarVisible_;
    sidebarFrom_ = sidebarAmount_;
    sidebarTo_ = sidebarVisible_ ? 1.0f : 0.0f;
    sidebarElapsed_ = 0.0f;
    sidebarAnimating_ = std::fabs(sidebarTo_ - sidebarFrom_) > 0.001f;
    MarkFullDirty();
}

bool RoboScene::KeyDown(const std::string& key, bool shift) {
    if (focusedInput_ && key != "Tab") {
        return false;
    }
    if (key == "ArrowUp" || key == "ArrowDown" || key == "Home" || key == "End" || key == "ArrowLeft" || key == "ArrowRight") {
        if (!focusedSidebarItem_ || !focusedSidebarItem_->CanActivate()) {
            return false;
        }
        if (key == "ArrowLeft" || key == "ArrowRight") {
            return true;
        }
        std::vector<SidebarItem*> rows;
        if (Sidebar* owner = SidebarOwning(focusedSidebarItem_)) {
            for (SidebarItem& item : owner->Items()) {
                if (item.CanActivate()) {
                    rows.push_back(&item);
                }
                for (SidebarItem& child : item.Children()) {
                    if (child.CanActivate() && child.GetBounds().height > 1.0f) {
                        rows.push_back(&child);
                    }
                }
            }
        }
        if (rows.empty()) {
            return true;
        }
        int index = 0;
        for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
            if (rows[static_cast<std::size_t>(i)] == focusedSidebarItem_) {
                index = i;
            }
        }
        if (key == "Home") {
            index = 0;
        } else if (key == "End") {
            index = static_cast<int>(rows.size()) - 1;
        } else if (key == "ArrowUp") {
            index = std::max(0, index - 1);
        } else {
            index = std::min(static_cast<int>(rows.size()) - 1, index + 1);
        }
        SidebarItem* next = rows[static_cast<std::size_t>(index)];
        sidebarKeyboardPressed_ = false;
        SetFocusedSidebarItem(next, true);
        if (Sidebar* owner = SidebarOwning(next)) {
            const int top = owner->IndexOf(next);
            const SidebarItemBounds& bounds = next->GetBounds();
            if (top >= 0) {
                owner->Select(top);
                OpenSelectedChild(owner, top);
                owner->RevealRail(bounds.y, bounds.height);
            } else {
                owner->SelectChild(next);
                owner->Reveal(bounds.y, bounds.height);
            }
        } else {
            next->SetSelected(true);
        }
        RememberPane(next);
        MarkFullDirty();
        return true;
    }
    if (key == "Tab") {
        if (focusedInput_) {
            SetFocusedInput(nullptr);
        }
        std::vector<FocusSlot> slots;
        for (SceneItem& scene : items_) {
            if (scene.kind == SceneItemKind::Button && scene.button.CanActivate()) {
                FocusSlot slot;
                slot.button = &scene.button;
                slots.push_back(slot);
            } else if (scene.kind == SceneItemKind::TitleBar && scene.titleBar.SidebarButton().CanActivate()) {
                FocusSlot slot;
                slot.button = &scene.titleBar.SidebarButton();
                slots.push_back(slot);
            } else if (scene.kind == SceneItemKind::Sidebar) {
                AppendSidebarSlots(scene.sidebar, slots);
            } else if (scene.kind == SceneItemKind::SidebarItem && scene.sidebarItem.CanActivate()) {
                FocusSlot slot;
                slot.item = &scene.sidebarItem;
                slots.push_back(slot);
            } else if (scene.kind == SceneItemKind::Container && scene.container) {
                std::vector<Button*> buttons;
                CollectButtonsInContainer(*scene.container, buttons);
                for (Button* button : buttons) {
                    if (button->CanActivate()) {
                        FocusSlot slot;
                        slot.button = button;
                        slots.push_back(slot);
                    }
                }
                AppendPaneSlotsContainer(*scene.container, slots);
            } else if (scene.kind == SceneItemKind::Flexbox) {
                std::vector<Button*> buttons;
                CollectButtonsInFlexbox(scene.flexbox, buttons);
                for (Button* button : buttons) {
                    if (button->CanActivate()) {
                        FocusSlot slot;
                        slot.button = button;
                        slots.push_back(slot);
                    }
                }
                AppendPaneSlotsFlex(scene.flexbox, slots);
            } else if (scene.kind == SceneItemKind::Grid) {
                std::vector<Button*> buttons;
                CollectButtonsInGrid(scene.grid, buttons);
                for (Button* button : buttons) {
                    if (button->CanActivate()) {
                        FocusSlot slot;
                        slot.button = button;
                        slots.push_back(slot);
                    }
                }
                AppendPaneSlotsGrid(scene.grid, slots);
            } else if (scene.kind == SceneItemKind::Position) {
                std::vector<Button*> buttons;
                CollectButtonsInPosition(scene.position, buttons);
                for (Button* button : buttons) {
                    if (button->CanActivate()) {
                        FocusSlot slot;
                        slot.button = button;
                        slots.push_back(slot);
                    }
                }
                AppendPaneSlotsPosition(scene.position, slots);
            }
        }
        if (slots.empty()) {
            return false;
        }
        int index = -1;
        for (int i = 0; i < static_cast<int>(slots.size()); ++i) {
            const FocusSlot& slot = slots[static_cast<std::size_t>(i)];
            if ((slot.button && slot.button == focusedButton_) || (slot.item && slot.item == focusedSidebarItem_)) {
                index = i;
            }
        }
        const int count = static_cast<int>(slots.size());
        int next = 0;
        if (index < 0) {
            next = shift ? count - 1 : 0;
        } else if (shift) {
            next = (index - 1 + count) % count;
        } else {
            next = (index + 1) % count;
        }
        keyboardPressed_ = false;
        sidebarKeyboardPressed_ = false;
        const FocusSlot& slot = slots[static_cast<std::size_t>(next)];
        if (slot.button) {
            SetFocusedSidebarItem(nullptr);
            SetFocusedButton(slot.button, true);
        } else {
            SetFocusedButton(nullptr);
            SetFocusedSidebarItem(slot.item, true);
        }
        return true;
    }
    if (focusedSidebarItem_ && focusedSidebarItem_->CanActivate() && (key == "Enter" || key == " ")) {
        sidebarKeyboardPressed_ = true;
        if (focusedSidebarItem_->GetInteraction() != SidebarItemInteraction::Pressed) {
            focusedSidebarItem_->SetInteraction(SidebarItemInteraction::Pressed);
            MarkFullDirty();
        }
        return true;
    }
    if (!focusedButton_ || !focusedButton_->CanActivate()) {
        return false;
    }
    if (key == "Enter" || key == " ") {
        keyboardPressed_ = true;
        if (focusedButton_->GetInteraction() != ButtonInteraction::Pressed) {
            focusedButton_->SetInteraction(ButtonInteraction::Pressed);
            DirtyButton(focusedButton_);
        }
        return true;
    }
    return false;
}

bool RoboScene::KeyUp(const std::string& key) {
    if (key != "Enter" && key != " ") {
        return false;
    }
    bool changed = false;
    if (sidebarKeyboardPressed_) {
        sidebarKeyboardPressed_ = false;
        if (focusedSidebarItem_) {
            const SidebarItemInteraction next = (focusedSidebarItem_ == hoveredSidebarItem_) ? SidebarItemInteraction::Hovered
                                                                                            : SidebarItemInteraction::Normal;
            focusedSidebarItem_->SetInteraction(next);
            ActivateSidebarItem(focusedSidebarItem_);
            changed = true;
        }
    }
    if (!keyboardPressed_) {
        return changed;
    }
    keyboardPressed_ = false;
    if (!focusedButton_) {
        return changed;
    }
    const bool toggle = IsSidebarToggle(focusedButton_);
    const ButtonInteraction next =
        (focusedButton_ == hoveredButton_) ? ButtonInteraction::Hovered : ButtonInteraction::Normal;
    if (focusedButton_->GetInteraction() != next) {
        focusedButton_->SetInteraction(next);
        DirtyButton(focusedButton_);
        changed = true;
    }
    if (toggle) {
        ToggleSidebar();
        changed = true;
    }
    return changed;
}

bool RoboScene::SetFocusedInput(Input* input) {
    if (focusedInput_ == input) {
        return false;
    }
    DirtyInput(focusedInput_);
    if (focusedInput_) {
        const InputInteraction next =
            (focusedInput_ == hoveredInput_) ? InputInteraction::Hovered : InputInteraction::Normal;
        focusedInput_->SetInteraction(next);
    }
    focusedInput_ = input;
    if (focusedInput_) {
        focusedInput_->SetInteraction(InputInteraction::Focused);
        focusedInput_->SetCaretIndex(focusedInput_->GetValue().size());
    }
    DirtyInput(focusedInput_);
    return true;
}

bool RoboScene::PointerMove(float x, float y) {
    selectX_ = x;
    selectY_ = y;
    if (desktop_.IsEnabled()) {
        const int hover = desktop_.HitDock(x, y);
        if (hover != hoveredDock_) {
            hoveredDock_ = hover;
            if (desktop_.DockBarHeight() > 0.0f) {
                MarkDirty(desktop_.DockBarX(), desktop_.DockBarY(), desktop_.DockBarWidth(), desktop_.DockBarHeight());
            } else {
                MarkFullDirty();
            }
        }
        if (hover >= 0) {
            SetHoveredButton(nullptr);
            return true;
        }
    }
    if (TextSelection::IsDragging()) {
        DirtySelectionVisual();
        TextSelection::MovePointer(x, y);
        DirtySelectionVisual();
        if (!needsRender_) {
            MarkFullDirty();
        }
        return true;
    }

    if (draggingScrollbar_) {
        draggingScrollbar_->UpdateDrag(x, y);
        if (draggingScrollbar_ == &pageVertical_ || draggingScrollbar_ == &pageHorizontal_) {
            PullPageScroll();
        } else {
            PullSidebarScroll(draggingScrollbar_);
        }
        RerenderIfPossible();
        return true;
    }

    if (panningContainer_) {
        Scrollbar& bar = panningContainer_->GetScrollbar();
        const float dx = x - panX_;
        const float dy = y - panY_;
        panX_ = x;
        panY_ = y;
        const bool horizontal = bar.GetOrientation() == ScrollbarOrientation::Horizontal;
        const double delta = horizontal ? static_cast<double>(-dx) : static_cast<double>(-dy);
        if (bar.ScrollByPixelsImmediate(delta)) {
            RerenderIfPossible();
        }
        return true;
    }

    if (pressedSidebarItem_) {
        const bool inside = FindSidebarItemAt(x, y) == pressedSidebarItem_;
        const SidebarItemInteraction next = inside ? SidebarItemInteraction::Pressed : SidebarItemInteraction::Normal;
        if (pressedSidebarItem_->GetInteraction() != next) {
            pressedSidebarItem_->SetInteraction(next);
            MarkFullDirty();
            return true;
        }
        return false;
    }

    if (pressedButton_) {
        const bool inside = FindButtonAt(x, y) == pressedButton_;
        const ButtonInteraction next = inside ? ButtonInteraction::Pressed : ButtonInteraction::Normal;
        if (pressedButton_->GetInteraction() != next) {
            pressedButton_->SetInteraction(next);
            DirtyButton(pressedButton_);
            return true;
        }
        return false;
    }

    const bool railChanged = UpdateRailArrows(x, y);
    Scrollbar* scrollbar = FindScrollbarAt(x, y);
    SidebarItem* pane = scrollbar ? nullptr : FindSidebarItemAt(x, y);
    Button* button = (scrollbar || pane) ? nullptr : FindButtonAt(x, y);
    Input* input = (scrollbar || pane || button) ? nullptr : FindInputAt(x, y);

    // Hovering anywhere over a scroll region counts as hovering the region's
    // bar, so auto-hide reveals it and the wheel knows which region to scroll.
    // An interactive child under the pointer keeps the hover it would get.
    if (!scrollbar && !pane && !button && !input) {
        if (Container* container = FindContainerAt(x, y)) {
            if (container->GetStyle().showScrollbar && container->GetScrollbar().IsVisible()) {
                scrollbar = &container->GetScrollbar();
            }
        }
    }

    bool changed = railChanged;
    changed = SetHoveredScrollbar(scrollbar) || changed;
    changed = SetHoveredSidebarItem(pane) || changed;
    changed = SetHoveredButton(button) || changed;
    changed = SetHoveredInput(input) || changed;
    bool motionChanged = false;
    VisitSceneMotion(items_, [&](MotionShape& shape) {
        if (shape.UpdatePointer(x, y)) {
            motionChanged = true;
        }
    });
    if (motionChanged) {
        MarkFullDirty();
        changed = true;
    }
    if (SyncThemeHot(*this, items_, x, y)) {
        MarkFullDirty();
        changed = true;
    }
    if (scrollbar) {
        // Track the arrowhead under the pointer so it can show its hover state.
        const ScrollbarArrow arrow = scrollbar->FindArrowAt(x, y);
        if (scrollbar->GetHotArrow() != arrow) {
            scrollbar->SetHotArrow(arrow);
            DirtyScrollbar(scrollbar);
            changed = true;
        }
    }
    return changed;
}

bool RoboScene::LongPressSelect(float x, float y) {
    selectX_ = x;
    selectY_ = y;
    float oldX = 0.0f;
    float oldY = 0.0f;
    float oldWidth = 0.0f;
    float oldHeight = 0.0f;
    const bool hadVisual = TextSelection::VisualBounds(oldX, oldY, oldWidth, oldHeight);
    TextSelection::LongPress(x, y);
    if (!TextSelection::IsDragging() && !TextSelection::HasSelection()) {
        return false;
    }
    if (hadVisual) {
        MarkDirty(oldX, oldY, oldWidth, oldHeight);
    }
    SetPressedButton(nullptr);
    SetFocusedButton(nullptr);
    SetFocusedInput(nullptr);
    SetFocusedScrollbar(nullptr);
    DirtySelectionVisual();
    return true;
}

bool RoboScene::PointerDown(float x, float y, bool fromTouch, int clicks) {
    if (desktop_.IsEnabled()) {
        const int dock = desktop_.HitDock(x, y);
        if (dock >= 0) {
            if (desktop_.SelectDock(dock)) {
                if (desktop_.DockBarHeight() > 0.0f) {
                    MarkDirty(desktop_.DockBarX(), desktop_.DockBarY(), desktop_.DockBarWidth(), desktop_.DockBarHeight());
                } else {
                    MarkFullDirty();
                }
            }
            return true;
        }
    }
    panningContainer_ = nullptr;
    selectX_ = x;
    selectY_ = y;
    if (!fromTouch && TextSelection::BeginHandleDrag(x, y)) {
        DirtySelectionVisual();
        SetPressedButton(nullptr);
        SetFocusedButton(nullptr);
        SetFocusedInput(nullptr);
        SetFocusedScrollbar(nullptr);
        DirtySelectionVisual();
        return true;
    }
    if (PressRailArrow(x, y)) {
        return true;
    }
    if (SidebarItem* pane = FindSidebarItemAt(x, y)) {
        pressedSidebar_ = SidebarOwning(pane);
        pressedSidebarIndex_ = pressedSidebar_ ? pressedSidebar_->IndexOf(pane) : -1;
        SetPressedButton(nullptr);
        SetFocusedButton(nullptr);
        SetFocusedInput(nullptr);
        SetPressedSidebarItem(pane);
        SetFocusedSidebarItem(pane, false);
        SetHoveredSidebarItem(pane);
        return true;
    }
    SetPressedSidebarItem(nullptr);
    SetFocusedSidebarItem(nullptr);
    if (MotionShape* themeMotion = FindThemeMotionScene(items_, x, y)) {
        SetTheme(theme_ == Theme::Dark ? Theme::Light : Theme::Dark);
        MarkFullDirty();
        return true;
    }
    if (!fromTouch && TextSelection::Hit(x, y) >= 0) {
        DirtySelectionVisual();
        TextSelection::BeginClick(x, y, clicks > 0 ? clicks : 1);
        SetPressedButton(nullptr);
        SetFocusedButton(nullptr);
        SetFocusedInput(nullptr);
        SetFocusedScrollbar(nullptr);
        DirtySelectionVisual();
        return true;
    }
    bool changed = false;
    if (TextSelection::HasCaret() || TextSelection::HasSelection()) {
        DirtySelectionVisual();
        TextSelection::Clear();
        changed = true;
    }
    Scrollbar* scrollbar = FindScrollbarAt(x, y);
    ThemeToggle* themeToggle = scrollbar ? nullptr : FindThemeToggleAt(x, y);
    ThemeIconToggle* themeIcon = (scrollbar || themeToggle) ? nullptr : FindThemeIconToggleAt(x, y);
    MotionShape* themeMotion = (scrollbar || themeToggle || themeIcon) ? nullptr : FindThemeMotionScene(items_, x, y);
    Button* button = (scrollbar || themeToggle || themeIcon || themeMotion) ? nullptr : FindButtonAt(x, y);
    Input* input = (scrollbar || themeToggle || themeIcon || themeMotion || button) ? nullptr : FindInputAt(x, y);
    // Clicking inside a container (not on its track) focuses its scrollbar so
    // keyboard and wheel scrolling land on that region without jumping it.
    Container* container = (scrollbar || themeToggle || themeIcon || themeMotion || button || input) ? nullptr : FindContainerAt(x, y);

    if (scrollbar) {
        changed = SetFocusedInput(nullptr) || changed;
        changed = SetPressedButton(nullptr) || changed;
        changed = SetFocusedButton(nullptr) || changed;
        changed = SetFocusedScrollbar(scrollbar) || changed;
        changed = SetHoveredButton(nullptr) || changed;
        changed = SetHoveredInput(nullptr) || changed;
        const ScrollbarArrow arrow = scrollbar->FindArrowAt(x, y);
        if (arrow != ScrollbarArrow::None) {
            // Clicking a filled arrowhead steps the value instead of dragging.
            scrollbar->SetPressedArrow(arrow);
            scrollbar->SetHotArrow(arrow);
            scrollbar->StepByArrow(arrow);
        } else {
            scrollbar->BeginDrag(x, y);
            draggingScrollbar_ = scrollbar;
        }
        changed = SetHoveredScrollbar(scrollbar) || changed;
        if (scrollbar == &pageVertical_ || scrollbar == &pageHorizontal_) {
            PullPageScroll();
        } else {
            PullSidebarScroll(scrollbar);
        }
        MarkFullDirty();
        changed = true;
    } else if (themeToggle || themeIcon) {
        changed = SetHoveredScrollbar(nullptr) || changed;
        changed = SetFocusedScrollbar(nullptr) || changed;
        changed = SetFocusedInput(nullptr) || changed;
        changed = SetPressedButton(nullptr) || changed;
        changed = SetFocusedButton(nullptr) || changed;
        changed = SetHoveredButton(nullptr) || changed;
        changed = SetHoveredInput(nullptr) || changed;
        const ThemeTogglePart part = themeToggle ? themeToggle->HitPart(x, y) : themeIcon->HitPart(x, y);
        const bool disabled = themeToggle ? themeToggle->IsDisabled() : themeIcon->IsDisabled();
        if (!disabled) {
            if (part == ThemeTogglePart::Light) {
                SetTheme(Theme::Light);
            } else if (part == ThemeTogglePart::Dark) {
                SetTheme(Theme::Dark);
            } else if (part == ThemeTogglePart::Switch) {
                SetTheme(theme_ == Theme::Dark ? Theme::Light : Theme::Dark);
            }
        }
        MarkFullDirty();
        changed = true;
    } else if (themeMotion) {
        changed = SetHoveredScrollbar(nullptr) || changed;
        changed = SetFocusedScrollbar(nullptr) || changed;
        changed = SetFocusedInput(nullptr) || changed;
        changed = SetPressedButton(nullptr) || changed;
        changed = SetFocusedButton(nullptr) || changed;
        changed = SetHoveredButton(nullptr) || changed;
        changed = SetHoveredInput(nullptr) || changed;
        SetTheme(theme_ == Theme::Dark ? Theme::Light : Theme::Dark);
        MarkFullDirty();
        changed = true;
    } else if (button) {
        changed = SetHoveredScrollbar(nullptr) || changed;
        changed = SetFocusedScrollbar(nullptr) || changed;
        changed = SetFocusedInput(nullptr) || changed;
        changed = SetPressedButton(button) || changed;
        changed = SetFocusedButton(button) || changed;
        changed = SetHoveredButton(button) || changed;
        changed = SetHoveredInput(nullptr) || changed;
    } else if (input) {
        changed = SetHoveredScrollbar(nullptr) || changed;
        changed = SetFocusedScrollbar(nullptr) || changed;
        changed = SetPressedButton(nullptr) || changed;
        changed = SetFocusedButton(nullptr) || changed;
        changed = SetFocusedInput(input) || changed;
        changed = SetHoveredInput(input) || changed;
        changed = SetHoveredButton(nullptr) || changed;
    } else {
        Scrollbar* containerBar = nullptr;
        if (container && container->GetStyle().showScrollbar && container->GetScrollbar().IsVisible()) {
            containerBar = &container->GetScrollbar();
        }
        changed = SetFocusedScrollbar(containerBar) || changed;
        changed = SetPressedButton(nullptr) || changed;
        changed = SetFocusedButton(nullptr) || changed;
        changed = SetFocusedInput(nullptr) || changed;
        changed = SetHoveredButton(nullptr) || changed;
        changed = SetHoveredInput(nullptr) || changed;
        changed = SetHoveredScrollbar(containerBar) || changed;
        if (fromTouch && container && container->IsOverflowing()) {
            panningContainer_ = container;
            panX_ = x;
            panY_ = y;
            MarkFullDirty();
            changed = true;
        }
    }

    if (changed && !needsRender_) {
        MarkFullDirty();
    }
    return changed;
}

bool RoboScene::PointerUp(float x, float y) {
    bool changed = false;
    if (TextSelection::IsDragging()) {
        DirtySelectionVisual();
        TextSelection::EndDrag();
        changed = true;
    }
    if (draggingScrollbar_) {
        DirtyScrollbar(draggingScrollbar_);
        draggingScrollbar_->EndDrag();
        hoveredScrollbar_ = nullptr;
        draggingScrollbar_ = nullptr;
        changed = true;
    }
    if (panningContainer_) {
        panningContainer_ = nullptr;
        changed = true;
    }

    auto releaseArrows = [this, &changed](Scrollbar& scrollbar) {
        if (scrollbar.GetPressedArrow() != ScrollbarArrow::None) {
            scrollbar.SetPressedArrow(ScrollbarArrow::None);
            DirtyScrollbar(&scrollbar);
            changed = true;
        }
    };
    VisitSceneScrollbars(items_, releaseArrows);
    releaseArrows(pageVertical_);
    releaseArrows(pageHorizontal_);
    if (ReleaseRailArrows()) {
        changed = true;
    }
    if (UpdateRailArrows(x, y)) {
        changed = true;
    }

    if (pressedSidebarItem_ && FindSidebarItemAt(x, y) == pressedSidebarItem_) {
        ActivateSidebarItem(pressedSidebarItem_);
        changed = true;
    }
    changed = SetPressedSidebarItem(nullptr) || changed;
    pressedSidebar_ = nullptr;
    pressedSidebarIndex_ = -1;

    Button* released = pressedButton_;
    const bool activate = released && FindButtonAt(x, y) == released && released->CanActivate();
    changed = SetPressedButton(nullptr) || changed;
    if (activate && IsSidebarToggle(released)) {
        ToggleSidebar();
        changed = true;
    }
    Scrollbar* scrollbar = FindScrollbarAt(x, y);
    SidebarItem* pane = scrollbar ? nullptr : FindSidebarItemAt(x, y);
    Button* button = (scrollbar || pane) ? nullptr : FindButtonAt(x, y);
    Input* input = (scrollbar || pane || button) ? nullptr : FindInputAt(x, y);

    changed = SetHoveredScrollbar(scrollbar) || changed;
    changed = SetHoveredSidebarItem(pane) || changed;
    changed = SetHoveredButton(button) || changed;
    changed = SetHoveredInput(input) || changed;
    if (changed && !needsRender_) {
        MarkFullDirty();
    }
    return changed;
}

bool RoboScene::PointerLeave() {
    bool changed = false;
    if (pressedButton_) {
        changed = SetPressedButton(nullptr) || changed;
    }
    if (pressedSidebarItem_) {
        changed = SetPressedSidebarItem(nullptr) || changed;
    }
    if (ReleaseRailArrows()) {
        changed = true;
    }
    if (UpdateRailArrows(-100000.0f, -100000.0f)) {
        changed = true;
    }
    if (hoveredSidebarItem_) {
        changed = SetHoveredSidebarItem(nullptr) || changed;
    }
    if (hoveredButton_) {
        changed = SetHoveredButton(nullptr) || changed;
    }
    if (hoveredInput_) {
        changed = SetHoveredInput(nullptr) || changed;
    }
    if (hoveredScrollbar_) {
        changed = SetHoveredScrollbar(nullptr) || changed;
    }
    if (panningContainer_) {
        panningContainer_ = nullptr;
        MarkFullDirty();
        changed = true;
    }
    VisitSceneMotion(items_, [&](MotionShape& shape) {
        if (shape.UpdatePointer(-100000.0f, -100000.0f)) {
            changed = true;
        }
    });
    if (changed && !needsRender_) {
        MarkFullDirty();
    }
    return changed;
}

bool RoboScene::Tick(double deltaMs) {
    bool changed = false;
    bool videoAdvanced = false;
    if (desktop_.IsEnabled() && desktop_.GetBackgroundSource().Advance(deltaMs)) {
        videoAdvanced = true;
        changed = true;
    }
    bool contentMoved = false;
    auto visit = [&changed, &contentMoved, this, deltaMs](Scrollbar& scrollbar) {
        const double value = scrollbar.GetValue();
        if (!scrollbar.Tick(deltaMs)) {
            return;
        }
        changed = true;
        PullSidebarScroll(&scrollbar);
        if (std::fabs(scrollbar.GetValue() - value) > 0.00001) {
            contentMoved = true;
        } else {
            DirtyScrollbar(&scrollbar);
        }
    };
    VisitSceneScrollbars(items_, visit);
    for (SceneItem& item : items_) {
        if (item.kind == SceneItemKind::Sidebar && item.sidebar.TickRail(deltaMs)) {
            contentMoved = true;
            changed = true;
        }
    }
    if (TextSelection::Tick(deltaMs)) {
        DirtySelectionVisual();
        changed = true;
    }
    if (TextSelection::IsDragging()) {
        const float margin = 24.0f;
        float dx = 0.0f;
        float dy = 0.0f;
        if (selectY_ < margin) {
            dy = -std::min(28.0f, margin - selectY_);
        } else if (selectY_ > static_cast<float>(surfaceHeight_) - margin) {
            dy = std::min(28.0f, selectY_ - (static_cast<float>(surfaceHeight_) - margin));
        }
        if (selectX_ < margin) {
            dx = -std::min(28.0f, margin - selectX_);
        } else if (selectX_ > static_cast<float>(surfaceWidth_) - margin) {
            dx = std::min(28.0f, selectX_ - (static_cast<float>(surfaceWidth_) - margin));
        }
        if (dy != 0.0f && maxScrollY_ > 0.0f) {
            const float next = std::max(0.0f, std::min(maxScrollY_, scrollY_ + dy));
            if (next != scrollY_) {
                scrollY_ = next;
                pageVertical_.SetValue(scrollY_ / maxScrollY_);
                contentMoved = true;
                changed = true;
            }
        }
        if (dx != 0.0f && maxScrollX_ > 0.0f) {
            const float next = std::max(0.0f, std::min(maxScrollX_, scrollX_ + dx));
            if (next != scrollX_) {
                scrollX_ = next;
                pageHorizontal_.SetValue(scrollX_ / maxScrollX_);
                contentMoved = true;
                changed = true;
            }
        }
        if (changed) {
            TextSelection::MovePointer(selectX_, selectY_);
        }
    }
    const double verticalValue = pageVertical_.GetValue();
    const double horizontalValue = pageHorizontal_.GetValue();
    const bool verticalTicked = pageVertical_.Tick(deltaMs);
    const bool horizontalTicked = pageHorizontal_.Tick(deltaMs);
    if (verticalTicked || horizontalTicked) {
        const float scrollBeforeX = scrollX_;
        const float scrollBeforeY = scrollY_;
        PullPageScroll();
        if (std::fabs(scrollX_ - scrollBeforeX) > 0.5f || std::fabs(scrollY_ - scrollBeforeY) > 0.5f ||
            std::fabs(pageVertical_.GetValue() - verticalValue) > 0.00001 ||
            std::fabs(pageHorizontal_.GetValue() - horizontalValue) > 0.00001) {
            contentMoved = true;
        } else {
            if (verticalTicked) {
                DirtyScrollbar(&pageVertical_);
            }
            if (horizontalTicked) {
                DirtyScrollbar(&pageHorizontal_);
            }
        }
        changed = true;
    }
    bool motionChanged = false;
    {
        // Tick runs outside Render, so the theme scope has to be opened here.
        // Without it every shape reads light and the switch never moves.
        const FoundationScope foundationScope(ActiveFoundation(), theme_, themeMix_);
        const float mixBefore = themeMix_;
        AdvanceThemeMix(static_cast<float>(deltaMs), 200.0f, themeMixPrimed_, themeMixing_, themeMix_, themeMixFrom_,
                        themeMixTo_, themeMixElapsed_);
        if (themeMixing_ || std::fabs(themeMix_ - mixBefore) > 0.0001f) {
            motionChanged = true;
        }
        if (sidebarAnimating_) {
            sidebarElapsed_ += static_cast<float>(deltaMs);
            const float duration = 320.0f;
            const float t = std::min(1.0f, sidebarElapsed_ / duration);
            MotionEasing easing;
            easing.linear = false;
            easing.x1 = 0.28f;
            easing.y1 = 0.11f;
            easing.x2 = 0.32f;
            easing.y2 = 1.0f;
            sidebarAmount_ = sidebarFrom_ + (sidebarTo_ - sidebarFrom_) * Ease(t, easing);
            if (t >= 1.0f) {
                sidebarAmount_ = sidebarTo_;
                sidebarAnimating_ = false;
            }
            motionChanged = true;
        }
        VisitSceneMotion(items_, [&](MotionShape& shape) {
            if (!shape.NeedsMotion()) {
                return;
            }
            shape.Tick(static_cast<float>(deltaMs), false);
            motionChanged = true;
        });
        auto themeVisit = [&](auto& control) {
            if (!control.NeedsMotion()) {
                return;
            }
            control.Tick(static_cast<float>(deltaMs));
            motionChanged = true;
        };
        VisitSceneThemes(items_, themeVisit);
    }
    if (motionChanged) {
        MarkFullDirty();
        changed = true;
    }
    if (contentMoved) {
        MarkFullDirty();
    }
    if (videoAdvanced && !contentMoved && !motionChanged && chromeReady_ && !fullDirty_) {
        videoOnly_ = true;
        needsRender_ = true;
    } else if (videoAdvanced && !fullDirty_) {
        videoOnly_ = false;
        MarkSurfaceDirty();
    } else if (videoAdvanced) {
        videoOnly_ = false;
    }
    return changed;
}

bool RoboScene::NeedsTick() {
    bool needsTick = false;
    auto visit = [&needsTick](Scrollbar& scrollbar) {
        if (scrollbar.NeedsAnimation()) {
            needsTick = true;
        }
    };
    VisitSceneScrollbars(items_, visit);
    for (const SceneItem& item : items_) {
        if (item.kind == SceneItemKind::Sidebar && item.sidebar.RailNeedsTick()) {
            needsTick = true;
        }
    }
    if (pageVertical_.NeedsAnimation() || pageHorizontal_.NeedsAnimation()) {
        needsTick = true;
    }
    if (TextSelection::IsDragging()) {
        const float margin = 24.0f;
        if (selectY_ < margin || selectY_ > static_cast<float>(surfaceHeight_) - margin || selectX_ < margin ||
            selectX_ > static_cast<float>(surfaceWidth_) - margin) {
            needsTick = true;
        }
    }
    if (TextSelection::NeedsBlink()) {
        needsTick = true;
    }
    {
        const FoundationScope foundationScope(ActiveFoundation(), theme_, themeMix_);
        if (themeMixing_ || !themeMixPrimed_ || sidebarAnimating_) {
            needsTick = true;
        }
        VisitSceneMotion(items_, [&](MotionShape& shape) {
            if (shape.NeedsMotion()) {
                needsTick = true;
            }
        });
        auto themeVisit = [&](auto& control) {
            if (control.NeedsMotion()) {
                needsTick = true;
            }
        };
        VisitSceneThemes(items_, themeVisit);
    }
    return needsTick;
}

bool RoboScene::HasFocusedInput() const {
    return focusedInput_ != nullptr;
}

bool RoboScene::SelectableAt(float x, float y) const {
    return TextSelection::Hit(x, y) >= 0;
}

bool RoboScene::HasCaret() const {
    return TextSelection::HasCaret();
}

bool RoboScene::MoveSelection(int command, bool extend) {
    if (focusedInput_) {
        return false;
    }
    float oldX = 0.0f;
    float oldY = 0.0f;
    float oldWidth = 0.0f;
    float oldHeight = 0.0f;
    const bool hadVisual = TextSelection::VisualBounds(oldX, oldY, oldWidth, oldHeight);
    if (!TextSelection::Move(command, extend)) {
        return false;
    }
    if (hadVisual) {
        MarkDirty(oldX, oldY, oldWidth, oldHeight);
    }
    DirtySelectionVisual();
    return true;
}

bool RoboScene::CopySelection() {
    const std::string text = TextSelection::SelectedText();
    if (text.empty() || !platform_) {
        return false;
    }
    platform_->CopyText(text);
    return true;
}

bool RoboScene::SelectAllText() {
    float oldX = 0.0f;
    float oldY = 0.0f;
    float oldWidth = 0.0f;
    float oldHeight = 0.0f;
    const bool hadVisual = TextSelection::VisualBounds(oldX, oldY, oldWidth, oldHeight);
    if (!TextSelection::SelectAll()) {
        return false;
    }
    if (hadVisual) {
        MarkDirty(oldX, oldY, oldWidth, oldHeight);
    }
    DirtySelectionVisual();
    return true;
}

bool RoboScene::KeyInsert(const std::string& utf8) {
    if (!focusedInput_) {
        return false;
    }
    if (!focusedInput_->InsertText(utf8)) {
        return false;
    }
    MarkFullDirty();
    return true;
}

bool RoboScene::KeyBackspace() {
    if (!focusedInput_) {
        return false;
    }
    if (!focusedInput_->DeleteBackward()) {
        return false;
    }
    MarkFullDirty();
    return true;
}

bool RoboScene::KeyDelete() {
    if (!focusedInput_) {
        return false;
    }
    if (!focusedInput_->DeleteForward()) {
        return false;
    }
    MarkFullDirty();
    return true;
}

bool RoboScene::KeyLeft() {
    if (!focusedInput_) {
        return false;
    }
    if (!focusedInput_->MoveCaretLeft()) {
        return false;
    }
    DirtyInput(focusedInput_);
    return true;
}

bool RoboScene::KeyRight() {
    if (!focusedInput_) {
        return false;
    }
    if (!focusedInput_->MoveCaretRight()) {
        return false;
    }
    DirtyInput(focusedInput_);
    return true;
}

bool RoboScene::KeyEscape() {
    if (focusedInput_) {
        const bool changed = SetFocusedInput(nullptr);
        if (changed) {
            RerenderIfPossible();
        }
        return changed;
    }
    if (focusedScrollbar_) {
        const bool changed = SetFocusedScrollbar(nullptr);
        if (changed) {
            RerenderIfPossible();
        }
        return changed;
    }
    return false;
}

}  // namespace Robo
