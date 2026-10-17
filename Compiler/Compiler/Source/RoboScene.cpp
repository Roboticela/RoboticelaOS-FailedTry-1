#include "Robo/RoboPool.hpp"
#include "Robo/RoboScene.hpp"
#include "Robo/Motion.hpp"
#include "Robo/TextSelection.hpp"
#include "Robo/BrandLogo.hpp"
#include "Robo/BrandLogoPainter.hpp"
#include "Robo/BackgroundPainter.hpp"
#include "Robo/ContextMenuPainter.hpp"
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

#ifdef ROBO_KERNEL
int RoboDevKitOpenPageStep(Robo::RoboScene& scene, int step);
#else
void BuildDevKitPage(Robo::RoboScene& scene);
int BuildDevKitPageStep(Robo::RoboScene& scene, int step);
#endif

namespace Robo {

namespace {

constexpr float DEV_KIT_WIDTH = 1080.0f;
constexpr float DEV_KIT_HEIGHT = 700.0f;
constexpr float DEV_KIT_RADIUS = 12.0f;

bool InsideRoundedWindow(float localX, float localY, float width, float height, float radius) {
    if (width <= 0.0f || height <= 0.0f) {
        return false;
    }
    if (localX < 0.0f || localY < 0.0f || localX >= width || localY >= height) {
        return false;
    }
    const float limit = std::min(radius, std::min(width, height) * 0.5f);
    if (limit <= 0.5f) {
        return true;
    }
    if (localX >= limit && localX < width - limit) {
        return true;
    }
    if (localY >= limit && localY < height - limit) {
        return true;
    }
    float centerX = limit;
    float centerY = limit;
    if (localX >= width - limit) {
        centerX = width - limit;
    }
    if (localY >= height - limit) {
        centerY = height - limit;
    }
    const float dx = localX - centerX;
    const float dy = localY - centerY;
    return dx * dx + dy * dy <= limit * limit;
}

class HostPlatform final : public IPlatform {
public:
    void Bind(IPlatform* assets, FrameBuffer* dest, int destX, int destY, float cornerRadius) {
        assets_ = assets;
        dest_ = dest;
        destX_ = destX;
        destY_ = destY;
        cornerRadius_ = cornerRadius;
        forceFullBlit_ = false;
    }

    void SetForceFullBlit(bool force) {
        forceFullBlit_ = force;
    }

    int GetSurfaceWidth() const override {
        return width_;
    }

    int GetSurfaceHeight() const override {
        return height_;
    }

    float GetPixelScale() const override {
        return pixelScale_;
    }

    void SetSurfaceSize(int width, int height) override {
        width_ = std::max(1, width);
        height_ = std::max(1, height);
    }

    void SetPixelScale(float scale) override {
        pixelScale_ = scale > 0.0f ? scale : 1.0f;
    }

    void Present(const FrameBuffer& buffer) override {
        if (dest_ == nullptr || dest_->GetPixels() == nullptr || buffer.GetPixels() == nullptr) {
            return;
        }
        const int copyWidth = std::min(buffer.GetWidth(), dest_->GetWidth() - destX_);
        const int copyHeight = std::min(buffer.GetHeight(), dest_->GetHeight() - destY_);
        if (copyWidth <= 0 || copyHeight <= 0 || destX_ < 0 || destY_ < 0) {
            return;
        }
        int blitX = 0;
        int blitY = 0;
        int blitW = copyWidth;
        int blitH = copyHeight;
        if (!forceFullBlit_) {
            const int presentW = buffer.GetPresentWidth();
            const int presentH = buffer.GetPresentHeight();
            if (presentW > 0 && presentH > 0 &&
                (presentW < copyWidth || presentH < copyHeight || buffer.GetPresentX() > 0 ||
                 buffer.GetPresentY() > 0)) {
                blitX = std::max(0, buffer.GetPresentX());
                blitY = std::max(0, buffer.GetPresentY());
                blitW = std::min(presentW, copyWidth - blitX);
                blitH = std::min(presentH, copyHeight - blitY);
            }
        }
        if (blitW <= 0 || blitH <= 0) {
            return;
        }
        const std::uint8_t* src = buffer.GetPixels();
        std::uint8_t* dst = dest_->GetPixels();
        const int srcStride = buffer.GetWidth() * 4;
        const int dstStride = dest_->GetWidth() * 4;
        const float radius =
            std::min(cornerRadius_, std::min(static_cast<float>(copyWidth), static_cast<float>(copyHeight)) * 0.5f);
        const float windowW = static_cast<float>(copyWidth);
        const float windowH = static_cast<float>(copyHeight);
        for (int row = 0; row < blitH; ++row) {
            const int srcRow = blitY + row;
            const float localY = static_cast<float>(srcRow) + 0.5f;
            if (radius <= 0.5f || (localY >= radius && localY < windowH - radius)) {
                memcpy(dst + (destY_ + srcRow) * dstStride + (destX_ + blitX) * 4,
                       src + srcRow * srcStride + blitX * 4, static_cast<std::size_t>(blitW) * 4u);
            } else {
                for (int col = 0; col < blitW; ++col) {
                    const int srcCol = blitX + col;
                    const float localX = static_cast<float>(srcCol) + 0.5f;
                    if (!InsideRoundedWindow(localX, localY, windowW, windowH, radius)) {
                        continue;
                    }
                    const std::size_t srcIndex = static_cast<std::size_t>(srcRow * srcStride + srcCol * 4);
                    const std::size_t dstIndex =
                        static_cast<std::size_t>((destY_ + srcRow) * dstStride + (destX_ + srcCol) * 4);
                    dst[dstIndex] = src[srcIndex];
                    dst[dstIndex + 1u] = src[srcIndex + 1u];
                    dst[dstIndex + 2u] = src[srcIndex + 2u];
                    dst[dstIndex + 3u] = src[srcIndex + 3u];
                }
            }
            if ((row & 31) == 31) {
                dest_->Pump();
            }
        }
    }

    void CopyText(const std::string& text) override {
        if (assets_ != nullptr) {
            assets_->CopyText(text);
        }
    }

    std::vector<std::uint8_t> LoadAsset(const std::string& relativePath) const override {
        if (assets_ == nullptr) {
            return {};
        }
        return assets_->LoadAsset(relativePath);
    }

private:
    IPlatform* assets_ = nullptr;
    FrameBuffer* dest_ = nullptr;
    int destX_ = 0;
    int destY_ = 0;
    int width_ = 1;
    int height_ = 1;
    float pixelScale_ = 1.0f;
    float cornerRadius_ = 0.0f;
    bool forceFullBlit_ = false;
};

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
            ButtonBounds bb = item.button->GetBounds();
            ScaleBounds(bb, inv);
            item.button->SetBounds(bb);
        } else if (item.kind == FlexItemKind::ThemeToggle) {
            item.themeToggle->ScaleBounds(inv);
        } else if (item.kind == FlexItemKind::ThemeIconToggle) {
            item.themeIconToggle->ScaleBounds(inv);
        } else if (item.kind == FlexItemKind::TitleBar) {
            item.titleBar->ThemeIcon().ScaleBounds(inv);
            ButtonBounds sidebarBounds = item.titleBar->SidebarButton().GetBounds();
            ScaleBounds(sidebarBounds, inv);
            item.titleBar->SidebarButton().SetBounds(sidebarBounds);
        } else if (item.kind == FlexItemKind::MotionShape) {
            item.motionShape->ScaleBounds(inv);
        } else if (item.kind == FlexItemKind::Input) {
            InputBounds ib = item.input->GetBounds();
            ScaleBounds(ib, inv);
            item.input->SetBounds(ib);
        } else if (item.kind == FlexItemKind::Scrollbar) {
            ScaleScrollbar(*item.scrollbar, inv);
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
            ButtonBounds bb = item.button->GetBounds();
            ScaleBounds(bb, inv);
            item.button->SetBounds(bb);
        } else if (item.kind == GridItemKind::ThemeToggle) {
            item.themeToggle->ScaleBounds(inv);
        } else if (item.kind == GridItemKind::ThemeIconToggle) {
            item.themeIconToggle->ScaleBounds(inv);
        } else if (item.kind == GridItemKind::TitleBar) {
            item.titleBar->ThemeIcon().ScaleBounds(inv);
            ButtonBounds sidebarBounds = item.titleBar->SidebarButton().GetBounds();
            ScaleBounds(sidebarBounds, inv);
            item.titleBar->SidebarButton().SetBounds(sidebarBounds);
        } else if (item.kind == GridItemKind::MotionShape) {
            item.motionShape->ScaleBounds(inv);
        } else if (item.kind == GridItemKind::Input) {
            InputBounds ib = item.input->GetBounds();
            ScaleBounds(ib, inv);
            item.input->SetBounds(ib);
        } else if (item.kind == GridItemKind::Scrollbar) {
            ScaleScrollbar(*item.scrollbar, inv);
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
            visit(*item.scrollbar);
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
            visit(*item.scrollbar);
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
            visit(*item.scrollbar);
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
// Inactive ShownWhen panes match paint: skip their scrollbars entirely.
template <typename Visit>
void VisitSceneScrollbars(std::vector<SceneItem>& items, const std::string& paneId, Visit& visit) {
    for (SceneItem& item : items) {
        if (item.kind == SceneItemKind::Scrollbar) {
            visit(*item.scrollbar);
        } else if (item.kind == SceneItemKind::Container) {
            if (!item.container->ShownWhen().empty() && item.container->ShownWhen() != paneId) {
                continue;
            }
            VisitScrollbars(*item.container, visit);
        } else if (item.kind == SceneItemKind::Flexbox && item.flexbox) {
            VisitScrollbars(*item.flexbox, visit);
        } else if (item.kind == SceneItemKind::Grid && item.grid) {
            VisitScrollbars(*item.grid, visit);
        } else if (item.kind == SceneItemKind::Position && item.position) {
            VisitScrollbars(*item.position, visit);
        } else if (item.kind == SceneItemKind::Sidebar) {
            visit(item.sidebar->PanelScrollbar());
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
            if (item.button->GetBounds().Contains(x, y)) {
                return item.button.get();
            }
        } else if (item.kind == FlexItemKind::TitleBar) {
            if (item.titleBar->SidebarButton().GetBounds().Contains(x, y)) {
                return &item.titleBar->SidebarButton();
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
            if (item.input->GetBounds().Contains(x, y)) {
                return item.input.get();
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
            if (item.scrollbar->IsVisible() && item.scrollbar->GetBounds().Contains(x, y)) {
                return item.scrollbar.get();
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
            if (item.button->GetBounds().Contains(x, y)) {
                return item.button.get();
            }
        } else if (item.kind == FlexItemKind::TitleBar) {
            if (item.titleBar->SidebarButton().GetBounds().Contains(x, y)) {
                return &item.titleBar->SidebarButton();
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
            if (item.input->GetBounds().Contains(x, y)) {
                return item.input.get();
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
            if (item.scrollbar->IsVisible() && item.scrollbar->GetBounds().Contains(x, y)) {
                return item.scrollbar.get();
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
            if (item.button->GetBounds().Contains(x, y)) {
                return item.button.get();
            }
        } else if (item.kind == GridItemKind::TitleBar) {
            if (item.titleBar->SidebarButton().GetBounds().Contains(x, y)) {
                return &item.titleBar->SidebarButton();
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
            if (item.input->GetBounds().Contains(x, y)) {
                return item.input.get();
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
            if (item.scrollbar->IsVisible() && item.scrollbar->GetBounds().Contains(x, y)) {
                return item.scrollbar.get();
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
            if (item.button->GetBounds().Contains(x, y)) {
                return item.button.get();
            }
        } else if (item.kind == GridItemKind::TitleBar) {
            if (item.titleBar->SidebarButton().GetBounds().Contains(x, y)) {
                return &item.titleBar->SidebarButton();
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
            if (item.input->GetBounds().Contains(x, y)) {
                return item.input.get();
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
            if (item.scrollbar->IsVisible() && item.scrollbar->GetBounds().Contains(x, y)) {
                return item.scrollbar.get();
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
            ButtonBounds bb = item.button->GetBounds();
            ScaleBounds(bb, inv);
            item.button->SetBounds(bb);
        } else if (item.kind == PositionItemKind::ThemeToggle) {
            item.themeToggle->ScaleBounds(inv);
        } else if (item.kind == PositionItemKind::ThemeIconToggle) {
            item.themeIconToggle->ScaleBounds(inv);
        } else if (item.kind == PositionItemKind::TitleBar) {
            item.titleBar->ThemeIcon().ScaleBounds(inv);
            ButtonBounds sidebarBounds = item.titleBar->SidebarButton().GetBounds();
            ScaleBounds(sidebarBounds, inv);
            item.titleBar->SidebarButton().SetBounds(sidebarBounds);
        } else if (item.kind == PositionItemKind::MotionShape) {
            item.motionShape->ScaleBounds(inv);
        } else if (item.kind == PositionItemKind::Input) {
            InputBounds ib = item.input->GetBounds();
            ScaleBounds(ib, inv);
            item.input->SetBounds(ib);
        } else if (item.kind == PositionItemKind::Scrollbar) {
            ScaleScrollbar(*item.scrollbar, inv);
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
            if (item.button->GetBounds().Contains(x, y)) {
                return item.button.get();
            }
        } else if (item.kind == PositionItemKind::TitleBar) {
            if (item.titleBar->SidebarButton().GetBounds().Contains(x, y)) {
                return &item.titleBar->SidebarButton();
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
            if (item.input->GetBounds().Contains(x, y)) {
                return item.input.get();
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
            if (item.scrollbar->IsVisible() && item.scrollbar->GetBounds().Contains(x, y)) {
                return item.scrollbar.get();
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
            if (item.button->GetBounds().Contains(x, y)) {
                return item.button.get();
            }
        } else if (item.kind == PositionItemKind::TitleBar) {
            if (item.titleBar->SidebarButton().GetBounds().Contains(x, y)) {
                return &item.titleBar->SidebarButton();
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
            if (item.input->GetBounds().Contains(x, y)) {
                return item.input.get();
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
            if (item.scrollbar->IsVisible() && item.scrollbar->GetBounds().Contains(x, y)) {
                return item.scrollbar.get();
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
            visit(*item.motionShape);
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
            visit(*item.motionShape);
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
            visit(*item.motionShape);
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
            visit(*item.themeToggle);
        } else if (item.kind == FlexItemKind::ThemeIconToggle) {
            visit(*item.themeIconToggle);
        } else if (item.kind == FlexItemKind::TitleBar) {
            visit(item.titleBar->ThemeIcon());
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
            visit(*item.themeToggle);
        } else if (item.kind == GridItemKind::ThemeIconToggle) {
            visit(*item.themeIconToggle);
        } else if (item.kind == GridItemKind::TitleBar) {
            visit(item.titleBar->ThemeIcon());
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
            visit(*item.themeToggle);
        } else if (item.kind == PositionItemKind::ThemeIconToggle) {
            visit(*item.themeIconToggle);
        } else if (item.kind == PositionItemKind::TitleBar) {
            visit(item.titleBar->ThemeIcon());
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
void VisitSceneThemes(std::vector<SceneItem>& items, const std::string& paneId, Visit& visit) {
    for (SceneItem& item : items) {
        if (item.kind == SceneItemKind::ThemeToggle) {
            visit(*item.themeToggle);
        } else if (item.kind == SceneItemKind::ThemeIconToggle) {
            visit(*item.themeIconToggle);
        } else if (item.kind == SceneItemKind::TitleBar) {
            visit(item.titleBar->ThemeIcon());
        } else if (item.kind == SceneItemKind::Container && item.container) {
            if (!item.container->ShownWhen().empty() && item.container->ShownWhen() != paneId) {
                continue;
            }
            VisitThemeContainer(*item.container, visit);
        } else if (item.kind == SceneItemKind::Flexbox && item.flexbox) {
            VisitThemeFlex(*item.flexbox, visit);
        } else if (item.kind == SceneItemKind::Grid && item.grid) {
            VisitThemeGrid(*item.grid, visit);
        } else if (item.kind == SceneItemKind::Position && item.position) {
            VisitThemePosition(*item.position, visit);
        }
    }
}

void VisitSceneMotion(std::vector<SceneItem>& items, const std::string& paneId,
                      const std::function<void(MotionShape&)>& visit) {
    for (SceneItem& item : items) {
        if (item.kind == SceneItemKind::MotionShape) {
            visit(*item.motionShape);
        } else if (item.kind == SceneItemKind::Container && item.container) {
            if (!item.container->ShownWhen().empty() && item.container->ShownWhen() != paneId) {
                continue;
            }
            VisitMotionContainer(*item.container, visit);
        } else if (item.kind == SceneItemKind::Flexbox && item.flexbox) {
            VisitMotionFlex(*item.flexbox, visit);
        } else if (item.kind == SceneItemKind::Grid && item.grid) {
            VisitMotionGrid(*item.grid, visit);
        } else if (item.kind == SceneItemKind::Position && item.position) {
            VisitMotionPosition(*item.position, visit);
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
        } else if (item.kind == FlexItemKind::MotionShape && item.motionShape->TogglesTheme() &&
                   item.motionShape->Bounds().Contains(x, y)) {
            return item.motionShape.get();
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
        } else if (item.kind == GridItemKind::MotionShape && item.motionShape->TogglesTheme() &&
                   item.motionShape->Bounds().Contains(x, y)) {
            return item.motionShape.get();
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
        } else if (item.kind == PositionItemKind::MotionShape && item.motionShape->TogglesTheme() &&
                   item.motionShape->Bounds().Contains(x, y)) {
            return item.motionShape.get();
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
        } else if (item.kind == SceneItemKind::Flexbox && item.flexbox) {
            if (MotionShape* found = FindThemeMotionFlex(*item.flexbox, x, y)) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Grid && item.grid) {
            if (MotionShape* found = FindThemeMotionGrid(*item.grid, x, y)) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Position && item.position) {
            if (MotionShape* found = FindThemeMotionPosition(*item.position, x, y)) {
                return found;
            }
        } else if (item.kind == SceneItemKind::MotionShape && item.motionShape->TogglesTheme() &&
                   item.motionShape->Bounds().Contains(x, y)) {
            return item.motionShape.get();
        }
    }
    return nullptr;
}

void CollectSceneContainers(std::vector<SceneItem>& items, const std::string& paneId, float x, float y,
                            std::vector<Container*>& out) {
    for (SceneItem& item : items) {
        if (item.kind == SceneItemKind::Container) {
            if (!item.container->ShownWhen().empty() && item.container->ShownWhen() != paneId) {
                continue;
            }
            CollectContainersIn(*item.container, x, y, out);
        } else if (item.kind == SceneItemKind::Flexbox && item.flexbox) {
            CollectContainersIn(*item.flexbox, x, y, out);
        } else if (item.kind == SceneItemKind::Grid && item.grid) {
            CollectContainersIn(*item.grid, x, y, out);
        } else if (item.kind == SceneItemKind::Position && item.position) {
            CollectContainersIn(*item.position, x, y, out);
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
            out.push_back(item.button.get());
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
            out.push_back(item.button.get());
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
            out.push_back(item.button.get());
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
            if (SidebarItem* found = HitPane(*item.sidebarItem, x, y)) {
                return found;
            }
        } else if (item.kind == FlexItemKind::Sidebar) {
            if (SidebarItem* found = item.sidebar->HitItem(x, y)) {
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
            if (SidebarItem* found = HitPane(*item.sidebarItem, x, y)) {
                return found;
            }
        } else if (item.kind == GridItemKind::Sidebar) {
            if (SidebarItem* found = item.sidebar->HitItem(x, y)) {
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
            if (SidebarItem* found = HitPane(*item.sidebarItem, x, y)) {
                return found;
            }
        } else if (item.kind == PositionItemKind::Sidebar) {
            if (SidebarItem* found = item.sidebar->HitItem(x, y)) {
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
        if (item.kind == FlexItemKind::SidebarItem && item.sidebarItem->CanActivate()) {
            FocusSlot slot;
            slot.item = item.sidebarItem.get();
            out.push_back(slot);
        } else if (item.kind == FlexItemKind::Sidebar) {
            AppendSidebarSlots(*item.sidebar, out);
        } else if (item.kind == FlexItemKind::Container && item.container) {
            AppendPaneSlotsContainer(*item.container, out);
        } else if (item.kind == FlexItemKind::Flexbox && item.flexbox) {
            AppendPaneSlotsFlex(*item.flexbox, out);
        }
    }
}

void AppendPaneSlotsGrid(Grid& grid, std::vector<FocusSlot>& out) {
    for (GridItem& item : grid.GetItems()) {
        if (item.kind == GridItemKind::SidebarItem && item.sidebarItem->CanActivate()) {
            FocusSlot slot;
            slot.item = item.sidebarItem.get();
            out.push_back(slot);
        } else if (item.kind == GridItemKind::Sidebar) {
            AppendSidebarSlots(*item.sidebar, out);
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
        if (item.kind == PositionItemKind::SidebarItem && item.sidebarItem->CanActivate()) {
            FocusSlot slot;
            slot.item = item.sidebarItem.get();
            out.push_back(slot);
        } else if (item.kind == PositionItemKind::Sidebar) {
            AppendSidebarSlots(*item.sidebar, out);
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
    hoveredMenu_ = -1;
    focusedSelection_ = -1;
    modifiers_ = 0;
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
    item.text = RoboMake<Text>(std::move(text));
    items_.push_back(std::move(item));
}

void RoboScene::AddButton(Button button) {
    SceneItem item;
    item.kind = SceneItemKind::Button;
    item.button = RoboMake<Button>(std::move(button));
    items_.push_back(std::move(item));
}

void RoboScene::AddInput(Input input) {
    SceneItem item;
    item.kind = SceneItemKind::Input;
    item.input = RoboMake<Input>(std::move(input));
    items_.push_back(std::move(item));
}

void RoboScene::AddScrollbar(Scrollbar scrollbar) {
    SceneItem item;
    item.kind = SceneItemKind::Scrollbar;
    item.scrollbar = RoboMake<Scrollbar>(std::move(scrollbar));
    items_.push_back(std::move(item));
}

void RoboScene::AddContainer(Container container) {
    SceneItem item;
    item.kind = SceneItemKind::Container;
    item.container = RoboMake<Container>(std::move(container));
    items_.push_back(std::move(item));
}

void RoboScene::AddSelectionArea(SelectionArea area) {
    SceneItem item;
    item.kind = SceneItemKind::SelectionArea;
    item.selectionArea = std::move(area);
    items_.push_back(std::move(item));
}

void RoboScene::SetModifiers(int modifiers) {
    modifiers_ = modifiers;
}

void RoboScene::AddFlexbox(Flexbox flexbox) {
    SceneItem item;
    item.kind = SceneItemKind::Flexbox;
    item.flexbox = RoboMake<Flexbox>(std::move(flexbox));
    items_.push_back(std::move(item));
}

void RoboScene::AddGrid(Grid grid) {
    SceneItem item;
    item.kind = SceneItemKind::Grid;
    item.grid = RoboMake<Grid>(std::move(grid));
    items_.push_back(std::move(item));
}

void RoboScene::AddPosition(Position position) {
    SceneItem item;
    item.kind = SceneItemKind::Position;
    item.position = RoboMake<Position>(std::move(position));
    items_.push_back(std::move(item));
}

void RoboScene::AddSvg(Svg svg) {
    SceneItem item;
    item.kind = SceneItemKind::Svg;
    item.svg = RoboMake<Svg>(std::move(svg));
    items_.push_back(std::move(item));
}

void RoboScene::AddBrandLogo(BrandLogo logo) {
    SceneItem item;
    item.kind = SceneItemKind::BrandLogo;
    item.brandLogo = RoboMake<BrandLogo>(std::move(logo));
    items_.push_back(std::move(item));
}

void RoboScene::AddTitleBar(TitleBar bar) {
    SceneItem item;
    item.kind = SceneItemKind::TitleBar;
    item.titleBar = RoboMake<TitleBar>(std::move(bar));
    items_.push_back(std::move(item));
}

void RoboScene::AddSidebar(Sidebar pane) {
    SceneItem item;
    item.kind = SceneItemKind::Sidebar;
    item.sidebar = RoboMake<Sidebar>(std::move(pane));
    items_.push_back(std::move(item));
}

void RoboScene::AddSidebarItem(SidebarItem entry) {
    SceneItem item;
    item.kind = SceneItemKind::SidebarItem;
    item.sidebarItem = RoboMake<SidebarItem>(std::move(entry));
    items_.push_back(std::move(item));
}

void RoboScene::AddThemeToggle(ThemeToggle toggle) {
    SceneItem item;
    item.kind = SceneItemKind::ThemeToggle;
    item.themeToggle = RoboMake<ThemeToggle>(std::move(toggle));
    items_.push_back(std::move(item));
}

void RoboScene::AddThemeIconToggle(ThemeIconToggle toggle) {
    SceneItem item;
    item.kind = SceneItemKind::ThemeIconToggle;
    item.themeIconToggle = RoboMake<ThemeIconToggle>(std::move(toggle));
    items_.push_back(std::move(item));
}

void RoboScene::AddSeparator(Separator separator) {
    SceneItem item;
    item.kind = SceneItemKind::Separator;
    item.separator = RoboMake<Separator>(std::move(separator));
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

void RoboScene::SetTopBarMenu(TopBarMenu menu) {
    desktop_.SetEnabled(true);
    desktop_.SetTopBarMenu(std::move(menu));
}

void RoboScene::SetContextMenu(ContextMenu menu) {
    desktop_.SetEnabled(true);
    desktop_.SetContextMenu(std::move(menu));
}

void RoboScene::SetMenuTransparency(float amount) {
    if (!desktop_.HasTopBarMenu()) {
        return;
    }
    desktop_.GetTopBarMenu().SetTransparency(amount);
    DirtyMenuBar();
}

void RoboScene::AddDockItem(DockItem item) {
    desktop_.SetEnabled(true);
    desktop_.AddDockItem(std::move(item));
}

bool RoboScene::UpdateSelectionHover(float x, float y) {
    bool changed = false;
    for (SceneItem& item : items_) {
        if (item.kind != SceneItemKind::SelectionArea) {
            continue;
        }
        SelectionArea& area = item.selectionArea;
        const int before = area.Hover();
        bool local = false;
        if (area.GetGesture() != SelectionGesture::Idle) {
            local = area.ClearHover();
        } else {
            local = area.SetHoverAt(x, y);
        }
        if (!local) {
            continue;
        }
        changed = true;
        auto dirtyIcon = [&](int index) {
            if (index < 0) {
                return;
            }
            const std::vector<SelectionAreaItem>& icons = area.Items();
            if (index >= static_cast<int>(icons.size())) {
                return;
            }
            const SelectionBounds& bounds = icons[static_cast<std::size_t>(index)].GetBounds();
            if (bounds.width > 0.0f && bounds.height > 0.0f) {
                MarkDirty(bounds.x - 4.0f, bounds.y - 4.0f, bounds.width + 8.0f, bounds.height + 8.0f);
            }
        };
        dirtyIcon(before);
        dirtyIcon(area.Hover());
    }
    return changed;
}

void RoboScene::DirtySelection(SelectionArea& area) {
    const SelectionBounds& bounds = area.GetBounds();
    if (bounds.width > 0.0f && bounds.height > 0.0f) {
        MarkDirty(bounds.x, bounds.y, bounds.width, bounds.height);
    } else {
        MarkFullDirty();
    }
}

SelectionArea* RoboScene::GesturingSelection() {
    for (SceneItem& item : items_) {
        if (item.kind == SceneItemKind::SelectionArea && item.selectionArea.GetGesture() != SelectionGesture::Idle) {
            return &item.selectionArea;
        }
    }
    return nullptr;
}

SelectionArea* RoboScene::SelectionAt(float x, float y) {
    for (int index = static_cast<int>(items_.size()) - 1; index >= 0; --index) {
        SceneItem& item = items_[static_cast<std::size_t>(index)];
        if (item.kind == SceneItemKind::SelectionArea && item.selectionArea.GetBounds().Contains(x, y)) {
            return &item.selectionArea;
        }
    }
    return nullptr;
}

void RoboScene::DirtyMenuBar() {
    if (!desktop_.HasTopBarMenu()) {
        return;
    }
    const TopBarMenu& menu = desktop_.GetTopBarMenu();
    if (menu.BarHeight() > 0.0f) {
        MarkDirty(menu.BarX(), menu.BarY(), menu.BarWidth(), menu.BarHeight());
    } else {
        MarkFullDirty();
    }
}

void RoboScene::DirtyContextMenu() {
    if (!desktop_.HasContextMenu()) {
        return;
    }
    const ContextMenuBounds bounds = desktop_.GetContextMenu().DamageBounds();
    if (bounds.width <= 0.0f || bounds.height <= 0.0f) {
        MarkFullDirty();
        return;
    }
    MarkDirty(bounds.x - 36.0f, bounds.y - 16.0f, bounds.width + 72.0f, bounds.height + 64.0f);
}

void RoboScene::RunContextAction(const std::string& action) {
    if (action != "New Folder") {
        return;
    }
    for (int index = 0; index < static_cast<int>(items_.size()); ++index) {
        SceneItem& item = items_[static_cast<std::size_t>(index)];
        if (item.kind != SceneItemKind::SelectionArea) {
            continue;
        }
        const int count = static_cast<int>(item.selectionArea.Items().size());
        int folders = 0;
        for (SelectionAreaItem& existing : item.selectionArea.Items()) {
            existing.SetSelected(false);
            if (existing.GetOpen().empty()) {
                folders += 1;
            }
        }
        SelectionAreaItem folder;
        folder.SetLabel(folders == 0 ? "untitled folder" : "untitled folder " + std::to_string(folders + 1));
        folder.SetPosition(24.0f + static_cast<float>(count % 6) * 120.0f, 24.0f + static_cast<float>(count / 6) * 124.0f);
        folder.SetSelected(true);
        item.selectionArea.AddItem(std::move(folder));
        item.selectionArea.SetKeyboardFocus(true);
        focusedSelection_ = index;
        DirtySelection(item.selectionArea);
        return;
    }
}

bool RoboScene::PointerContext(float x, float y) {
    if (!desktop_.IsEnabled() || !desktop_.HasContextMenu()) {
        return false;
    }
    if (desktop_.HasTopBarMenu()) {
        const TopBarMenu& bar = desktop_.GetTopBarMenu();
        if (bar.BarHeight() > 0.0f && y >= bar.BarY() && y < bar.BarY() + bar.BarHeight()) {
            return false;
        }
    }
    ContextMenu& menu = desktop_.GetContextMenu();
    contextGuardMs_ = 180.0f;
    if (SelectionArea* area = GesturingSelection()) {
        DirtySelection(*area);
        area->Cancel();
        DirtySelection(*area);
    }
    if (!menu.Open(x, y)) {
        return true;
    }
    DirtyContextMenu();
    return true;
}

void RoboScene::AddMotionShape(MotionShape shape) {
    SceneItem item;
    item.kind = SceneItemKind::MotionShape;
    item.motionShape = RoboMake<MotionShape>(std::move(shape));
    items_.push_back(std::move(item));
}

bool RoboScene::RerenderIfPossible() {
    if (!hasRenderContext_ || !platform_ || !fonts_) {
        return false;
    }
    const float beforeX = scrollX_;
    const float beforeY = scrollY_;
    PullPageScroll();
    if (std::fabs(scrollX_ - beforeX) > 0.01f || std::fabs(scrollY_ - beforeY) > 0.01f ||
        std::fabs(scrollX_ - paintedScrollX_) > 0.01f || std::fabs(scrollY_ - paintedScrollY_) > 0.01f) {
        ShiftPaintedScroll();
        return true;
    }
    MarkContentDirty();
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

void RoboScene::DamageRect(float x, float y, float width, float height) {
    MarkDirty(x, y, width, height);
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
    const float added = std::max(0.0f, right - left) * std::max(0.0f, bottom - top);
    const float surface = static_cast<float>(std::max(1, paintedWidth_)) * static_cast<float>(std::max(1, paintedHeight_));
    if (added > surface * 0.5f) {
        fullDirty_ = true;
        damageOpen_ = false;
    }
}

void RoboScene::MarkDirtyLocal(float x, float y, float width, float height) {
    if (width <= 0.0f || height <= 0.0f) {
        return;
    }
    needsRender_ = true;
    videoOnly_ = false;
    const float pad = 8.0f;
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
    // Never promote DevKit-local damage to a full desktop CaptureChrome paint.
}

void RoboScene::DirtyButton(const Button* button) {
    if (!button) {
        return;
    }
    const ButtonBounds& bounds = button->GetBounds();
    if (bounds.width < 1.0f || bounds.height < 1.0f) {
        // Bounds unknown yet — surface damage, not theme/foundation full wipe.
        MarkSurfaceDirty();
        return;
    }
    MarkDirty(bounds.x - 8.0f, bounds.y - 8.0f, bounds.width + 16.0f, bounds.height + 16.0f);
}

void RoboScene::DirtyInput(const Input* input) {
    if (!input) {
        return;
    }
    const InputBounds& bounds = input->GetBounds();
    float line = static_cast<float>(input->GetStyle().fontSize) * 1.4f;
    if (line < 8.0f || line >= bounds.height) {
        MarkDirty(bounds.x, bounds.y, bounds.width, bounds.height);
        return;
    }
    const float y = bounds.y + (bounds.height - line) * 0.5f;
    MarkDirty(bounds.x, y, bounds.width, line);
}

void RoboScene::DirtyScrollbar(const Scrollbar* scrollbar) {
    if (!scrollbar) {
        return;
    }
    const ScrollbarBounds& bounds = scrollbar->GetBounds();
    MarkDirty(bounds.x, bounds.y, bounds.width, bounds.height);
}

void RoboScene::DirtySidebarItem(const SidebarItem* item) {
    if (!item) {
        return;
    }
    const SidebarItemBounds& bounds = item->GetBounds();
    if (bounds.width < 1.0f || bounds.height < 1.0f) {
        return;
    }
    MarkDirty(bounds.x - 4.0f, bounds.y - 4.0f, bounds.width + 8.0f, bounds.height + 8.0f);
}

void RoboScene::DirtyRailArrows(const Sidebar& sidebar) {
    const SidebarItemBounds& up = sidebar.RailUpBounds();
    if (up.width > 1.0f && up.height > 1.0f) {
        MarkDirty(up.x - 2.0f, up.y - 2.0f, up.width + 4.0f, up.height + 4.0f);
    }
    const SidebarItemBounds& down = sidebar.RailDownBounds();
    if (down.width > 1.0f && down.height > 1.0f) {
        MarkDirty(down.x - 2.0f, down.y - 2.0f, down.width + 4.0f, down.height + 4.0f);
    }
}

void RoboScene::DirtyWallpaperAroundDevKit() {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    DevKitFrame(x, y, width, height);
    if (paintedWidth_ <= 0 || paintedHeight_ <= 0 || width <= 0.0f || height <= 0.0f) {
        MarkSurfaceDirty();
        return;
    }
    const float surfaceW = static_cast<float>(paintedWidth_);
    const float surfaceH = static_cast<float>(paintedHeight_);
    if (y > 0.0f) {
        MarkDirty(0.0f, 0.0f, surfaceW, y);
    }
    if (y + height < surfaceH) {
        MarkDirty(0.0f, y + height, surfaceW, surfaceH - (y + height));
    }
    if (x > 0.0f) {
        MarkDirty(0.0f, y, x, height);
    }
    if (x + width < surfaceW) {
        MarkDirty(x + width, y, surfaceW - (x + width), height);
    }
}

bool RoboScene::PaneIsActive(const Container& container) const {
    if (container.ShownWhen().empty()) {
        return true;
    }
    return container.ShownWhen() == activePaneId_;
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

void RoboScene::DirtyThemeBounds(float x, float y, float width, float height) {
    MarkDirty(x, y, width, height);
}

void RoboScene::GetLastPresentRect(int& x, int& y, int& width, int& height) const {
    if (platform_ != nullptr) {
        x = platform_->GetPresentX();
        y = platform_->GetPresentY();
        width = platform_->GetPresentWidth();
        height = platform_->GetPresentHeight();
        if (width > 0 && height > 0) {
            return;
        }
    }
    x = frame_.GetPresentX();
    y = frame_.GetPresentY();
    width = frame_.GetPresentWidth();
    height = frame_.GetPresentHeight();
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
    // Scroll moves content; only full-surface damage is safe (MarkSurfaceDirty).
    if (std::fabs(scrollX_ - paintedScrollX_) > 0.5f || std::fabs(scrollY_ - paintedScrollY_) > 0.5f) {
        const bool fullSurface =
            dirtyLeft_ <= 0.5f && dirtyTop_ <= 0.5f && dirtyRight_ + 0.5f >= static_cast<float>(paintedWidth_) &&
            dirtyBottom_ + 0.5f >= static_cast<float>(paintedHeight_);
        if (!fullSurface) {
            return false;
        }
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
        if (item.kind == SceneItemKind::Sidebar && &item.sidebar->PanelScrollbar() == bar) {
            item.sidebar->PullPanelScroll();
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

void RoboScene::EnsureMenuMark() {
    if (menuMarkTried_ || platform_ == nullptr || !desktop_.HasTopBarMenu()) {
        return;
    }
    menuMarkTried_ = true;
    const std::vector<std::uint8_t> bytes = platform_->LoadAsset("Assets/RoboticelaLogo.svg");
    if (bytes.empty()) {
        return;
    }
    Svg mark;
    if (!LoadSvgDocument(bytes.data(), bytes.size(), mark)) {
        return;
    }
    desktop_.GetTopBarMenu().SetMark(std::move(mark));
}

float RoboScene::SceneScale() const {
    if (paintedScale_ > 0.0f) {
        return paintedScale_;
    }
    if (lastPaintScale_ > 0.0f) {
        return lastPaintScale_;
    }
    return 1.0f;
}

void RoboScene::DevKitFrame(float& x, float& y, float& width, float& height) const {
    const float scale = SceneScale();
    x = devKitX_ * scale;
    y = devKitY_ * scale;
    width = devKitW_ * scale;
    height = devKitH_ * scale;
}

bool RoboScene::PointerOnDevKit(float x, float y) const {
    if (!devKitOpen_) {
        return false;
    }
    float left = 0.0f;
    float top = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    DevKitFrame(left, top, width, height);
    return InsideRoundedWindow(x - left, y - top, width, height, DEV_KIT_RADIUS * SceneScale());
}

void RoboScene::DirtyDevKitWindow() {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    DevKitFrame(x, y, width, height);
    MarkDirtyLocal(x, y, width, height);
    wallpaperAroundDevKit_ = true;
    devKitPaintDirty_ = true;
    // Geometry/open/theme: retained surface must be rebuilt.
    devKitSurfaceValid_ = false;
}

void RoboScene::DirtyDevKitWindowSoft() {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    DevKitFrame(x, y, width, height);
    MarkDirtyLocal(x, y, width, height);
    wallpaperAroundDevKit_ = true;
    devKitPaintDirty_ = true;
}

void RoboScene::PumpPaint() {
    if (paintPump_ != nullptr) {
        paintPump_(paintPumpCookie_);
    }
}

void RoboScene::DirtyDevKitFromNested() {
    if (devKitScene_ == nullptr) {
        DirtyDevKitWindow();
        return;
    }
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    DevKitFrame(x, y, width, height);
    // Hover/content: dirty only the nested damage (translated). Nested NeedsRender drives rebuild;
    // do not force a full retained-surface invalidate on every hover.
    if (devKitScene_->damageOpen_ && !devKitScene_->fullDirty_) {
        const float pad = 8.0f;
        const float left = x + devKitScene_->dirtyLeft_ - pad;
        const float top = y + devKitScene_->dirtyTop_ - pad;
        const float right = x + devKitScene_->dirtyRight_ + pad;
        const float bottom = y + devKitScene_->dirtyBottom_ + pad;
        MarkDirty(left, top, std::max(1.0f, right - left), std::max(1.0f, bottom - top));
    } else {
        MarkDirty(x, y, width, height);
        if (devKitScene_->fullDirty_) {
            devKitPaintDirty_ = true;
        }
    }
}

void RoboScene::SetPaneEnsure(PaneEnsureFn fn) {
    paneEnsureFn_ = fn;
}

void RoboScene::SetRailChildrenEnsure(RailChildrenFn fn) {
    railChildrenFn_ = fn;
}

bool RoboScene::HasPaneContainer(const char* paneId) const {
    if (paneId == nullptr || paneId[0] == '\0') {
        return false;
    }
    for (const SceneItem& item : items_) {
        if (item.kind != SceneItemKind::Container || !item.container) {
            continue;
        }
        if (item.container->ShownWhen() == paneId) {
            return true;
        }
    }
    return false;
}

bool RoboScene::TakeParkedPane(const char* paneId) {
    if (paneId == nullptr) {
        return false;
    }
    for (std::size_t index = 0; index < parkedPanes_.size(); ++index) {
        SceneItem& item = parkedPanes_[index];
        if (item.kind != SceneItemKind::Container || !item.container) {
            continue;
        }
        if (item.container->ShownWhen() != paneId) {
            continue;
        }
        items_.push_back(std::move(item));
        parkedPanes_.erase(parkedPanes_.begin() + static_cast<std::ptrdiff_t>(index));
        return true;
    }
    return false;
}

void RoboScene::DropPane(const std::string& paneId) {
    auto drop = [&](std::vector<SceneItem>& list) {
        for (std::size_t index = list.size(); index > 0; --index) {
            const std::size_t at = index - 1;
            SceneItem& item = list[at];
            if (item.kind != SceneItemKind::Container || !item.container) {
                continue;
            }
            if (item.container->ShownWhen() == paneId) {
                list.erase(list.begin() + static_cast<std::ptrdiff_t>(at));
            }
        }
    };
    drop(items_);
    drop(parkedPanes_);
}

void RoboScene::EnsurePaneContent() {
    const std::string& id = ActivePaneId();
    if (id.empty() || HasPaneContainer(id.c_str())) {
        return;
    }
    if (TakeParkedPane(id.c_str())) {
        return;
    }
    if (paneEnsureFn_ == nullptr) {
        return;
    }
    paneEnsureFn_(*this, id.c_str());
}

void RoboScene::EvictInactivePanes(const std::string& keepPaneId) {
    for (std::size_t index = items_.size(); index > 0; --index) {
        const std::size_t at = index - 1;
        SceneItem& item = items_[at];
        if (item.kind != SceneItemKind::Container || !item.container) {
            continue;
        }
        const std::string shown = item.container->ShownWhen();
        if (shown.empty() || shown == keepPaneId) {
            continue;
        }
        parkedPanes_.push_back(std::move(item));
        items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(at));
    }
}

int RoboScene::GalleryLimit() const {
    return galleryLimit_;
}

void RoboScene::SetGalleryCount(int count) {
    galleryCount_ = count;
}

int RoboScene::GalleryFirst() const {
    return galleryFirst_;
}

void RoboScene::OpenLauncher(const char* names, int stride, int count) {
    launcherCount_ = 0;
    launcherOpen_ = false;
    if (names == nullptr || stride < 40 || count <= 1) {
        return;
    }
    if (count > 8) {
        count = 8;
    }
    for (int index = 0; index < count; ++index) {
        const char* name = names + index * stride;
        for (int letter = 0; letter < 47; ++letter) {
            launcherNames_[index][letter] = name[letter];
            if (name[letter] == 0) {
                break;
            }
        }
        launcherNames_[index][47] = 0;
        launcherCount_ += 1;
    }
    launcherOpen_ = launcherCount_ > 1;
    if (launcherOpen_) {
        MarkDirty(32.0f, 72.0f, 320.0f, static_cast<float>(36 * launcherCount_ + 16));
    }
}

void RoboScene::SetLauncherPick(void (*pick)(int index)) {
    launcherPick_ = pick;
}

bool RoboScene::WallpaperOnly() const {
    return wallpaperOnly_;
}

bool RoboScene::WallpaperDue() const {
    return wallpaperClockMs_ >= wallpaperPresentDueMs_;
}

void RoboScene::AdvanceClock(double deltaMs) {
    if (deltaMs > 0.0) {
        wallpaperClockMs_ += deltaMs;
    }
}

void RoboScene::MarkContentDirty() {
    if (paintedContentW_ > 1.0f && paintedContentH_ > 1.0f) {
        MarkDirty(paintedContentX_, paintedContentY_, paintedContentW_, paintedContentH_);
        return;
    }
    MarkDirty(0.0f, 0.0f, static_cast<float>(paintedWidth_), static_cast<float>(paintedHeight_));
}

void RoboScene::ShiftPaintedScroll() {
    const float dy = scrollY_ - paintedScrollY_;
    const float dx = scrollX_ - paintedScrollX_;
    const int shiftY = static_cast<int>(std::lround(dy));
    const int shiftX = static_cast<int>(std::lround(dx));
    if (shiftX == 0 && shiftY == 0) {
        paintedScrollX_ = scrollX_;
        paintedScrollY_ = scrollY_;
        return;
    }
    if (!frameReady_ || frame_.GetPixels() == nullptr || paintedWidth_ <= 0 || paintedHeight_ <= 0) {
        MarkContentDirty();
        paintedScrollX_ = scrollX_;
        paintedScrollY_ = scrollY_;
        return;
    }
    int clipX = static_cast<int>(paintedContentX_);
    int clipY = static_cast<int>(paintedContentY_);
    int clipW = static_cast<int>(paintedContentW_);
    int clipH = static_cast<int>(paintedContentH_);
    if (clipW <= 0 || clipH <= 0) {
        clipX = 0;
        clipY = 0;
        clipW = paintedWidth_;
        clipH = paintedHeight_;
    }
    if (clipX < 0) {
        clipW += clipX;
        clipX = 0;
    }
    if (clipY < 0) {
        clipH += clipY;
        clipY = 0;
    }
    if (clipX + clipW > paintedWidth_) {
        clipW = paintedWidth_ - clipX;
    }
    if (clipY + clipH > paintedHeight_) {
        clipH = paintedHeight_ - clipY;
    }
    const bool bothAxes = shiftX != 0 && shiftY != 0;
    if (clipW <= 0 || clipH <= 0 || bothAxes || shiftY <= -clipH || shiftY >= clipH || shiftX <= -clipW || shiftX >= clipW) {
        MarkDirty(static_cast<float>(clipX), static_cast<float>(clipY), static_cast<float>(clipW), static_cast<float>(clipH));
        paintedScrollX_ = scrollX_;
        paintedScrollY_ = scrollY_;
        return;
    }
    uint8_t* pixels = frame_.GetPixels();
    const std::size_t stride = static_cast<std::size_t>(paintedWidth_) * 4u;
    if (shiftY != 0) {
        const int rows = shiftY > 0 ? shiftY : -shiftY;
        const std::size_t rowBytes = static_cast<std::size_t>(clipW) * 4u;
        uint8_t* base = pixels + static_cast<std::size_t>(clipY) * stride + static_cast<std::size_t>(clipX) * 4u;
        if (shiftY > 0) {
            for (int row = 0; row < clipH - rows; ++row) {
                memmove(base + static_cast<std::size_t>(row) * stride, base + static_cast<std::size_t>(row + rows) * stride, rowBytes);
            }
            MarkDirty(static_cast<float>(clipX), static_cast<float>(clipY + clipH - rows), static_cast<float>(clipW), static_cast<float>(rows));
        } else {
            for (int row = clipH - 1; row >= rows; --row) {
                memmove(base + static_cast<std::size_t>(row) * stride, base + static_cast<std::size_t>(row - rows) * stride, rowBytes);
            }
            MarkDirty(static_cast<float>(clipX), static_cast<float>(clipY), static_cast<float>(clipW), static_cast<float>(rows));
        }
    } else {
        const int columns = shiftX > 0 ? shiftX : -shiftX;
        const std::size_t keep = static_cast<std::size_t>(clipW - columns) * 4u;
        uint8_t* base = pixels + static_cast<std::size_t>(clipY) * stride + static_cast<std::size_t>(clipX) * 4u;
        if (shiftX > 0) {
            for (int row = 0; row < clipH; ++row) {
                uint8_t* line = base + static_cast<std::size_t>(row) * stride;
                memmove(line, line + static_cast<std::size_t>(columns) * 4u, keep);
            }
            MarkDirty(static_cast<float>(clipX + clipW - columns), static_cast<float>(clipY), static_cast<float>(columns), static_cast<float>(clipH));
        } else {
            for (int row = 0; row < clipH; ++row) {
                uint8_t* line = base + static_cast<std::size_t>(row) * stride;
                memmove(line + static_cast<std::size_t>(columns) * 4u, line, keep);
            }
            MarkDirty(static_cast<float>(clipX), static_cast<float>(clipY), static_cast<float>(columns), static_cast<float>(clipH));
        }
    }
    paintedScrollX_ = scrollX_;
    paintedScrollY_ = scrollY_;
}

void RoboScene::CollectTextLineDamage() {
    for (SceneItem& item : items_) {
        if (item.kind != SceneItemKind::Text || !item.text) {
            continue;
        }
        int first = 0;
        int last = 0;
        if (!item.text->TakeDirtyLines(first, last)) {
            continue;
        }
        const TextLayoutCache& cache = item.text->LayoutCache();
        if (first < 0 || last < first || cache.lines.empty()) {
            continue;
        }
        const int clampedLast = std::min(last, static_cast<int>(cache.lines.size()) - 1);
        const TextLayoutLine& start = cache.lines[static_cast<std::size_t>(first)];
        const TextLayoutLine& end = cache.lines[static_cast<std::size_t>(clampedLast)];
        const float top = cache.originY + start.y;
        const float bottom = cache.originY + end.y + end.height;
        MarkDirty(cache.originX, top, std::max(cache.width, 1.0f), std::max(1.0f, bottom - top));
    }
}

void RoboScene::SetActivePaneId(const std::string& paneId) {
    if (paneId.empty() || paneId == activePaneId_) {
        if (!paneId.empty()) {
            EnsurePaneContent();
        }
        return;
    }
    activePaneId_ = paneId;
    pageVertical_.SetValue(0);
    scrollY_ = 0.0f;
    SetFocusedButton(nullptr);
    SetFocusedInput(nullptr);
    EvictInactivePanes(activePaneId_);
    EnsurePaneContent();
    MarkSurfaceDirty();
}

bool RoboScene::EnsureSelectedRailChildren(Sidebar* owner) {
    if (owner == nullptr || railChildrenFn_ == nullptr) {
        return false;
    }
    const int selected = owner->Selected();
    if (selected < 0) {
        return false;
    }
    return railChildrenFn_(*owner, selected);
}

void RoboScene::EnsureDevKit() {
    if (devKitScene_ != nullptr) {
        return;
    }
    devKitScene_ = new RoboScene();
    if (devKitScene_ == nullptr) {
        return;
    }
    // Build across ticks (micro-step spiral). Zero heavy work on PointerDown.
    devKitBuildStep_ = 0;
    DirtyDevKitWindowSoft();
}

bool RoboScene::DevKitNeedsPaint() const {
    // While open, desktop/wallpaper may wipe DevKit pixels; composite every frame path that paints.
    return devKitOpen_;
}

void RoboScene::OpenDevKit() {
    EnsureDevKit();
    if (devKitScene_ == nullptr) {
        return;
    }
    devKitOpen_ = true;
    devKitFocus_ = true;
    const float scale = SceneScale();
    if (paintedWidth_ > 0 && paintedHeight_ > 0) {
        const float availW = static_cast<float>(paintedWidth_) / scale;
        const float availH = static_cast<float>(paintedHeight_) / scale;
        devKitW_ = std::min(DEV_KIT_WIDTH, std::max(640.0f, availW - 64.0f));
        devKitH_ = std::min(DEV_KIT_HEIGHT, std::max(420.0f, availH - 64.0f));
        devKitX_ = std::max(16.0f, (availW - devKitW_) * 0.5f);
        devKitY_ = std::max(40.0f, (availH - devKitH_) * 0.5f);
    } else {
        devKitW_ = DEV_KIT_WIDTH;
        devKitH_ = DEV_KIT_HEIGHT;
    }
    // Stub window only — chrome/rails/pane fill in Tick with the paint pump.
    devKitRailFillPending_ = true;
    DirtyDevKitWindowSoft();
}

bool RoboScene::ForwardDevKitMove(float x, float y) {
    if (!devKitOpen_ || devKitScene_ == nullptr || !devKitFocus_ || !PointerOnDevKit(x, y)) {
        return false;
    }
    float left = 0.0f;
    float top = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    DevKitFrame(left, top, width, height);
    devKitScene_->SetModifiers(modifiers_);
    if (devKitScene_->PointerMove(x - left, y - top)) {
        DirtyDevKitFromNested();
        return true;
    }
    return false;
}

bool RoboScene::ForwardDevKitPointer(float x, float y, bool down, bool up, bool fromTouch, int clicks) {
    if (!devKitOpen_ || devKitScene_ == nullptr || !PointerOnDevKit(x, y)) {
        return false;
    }
    float left = 0.0f;
    float top = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    DevKitFrame(left, top, width, height);
    (void)width;
    (void)height;
    devKitScene_->SetModifiers(modifiers_);
    bool changed = false;
    if (down) {
        changed = devKitScene_->PointerDown(x - left, y - top, fromTouch, clicks);
        // Rail/pane clicks may leave budgeted child fills unfinished.
        if (changed) {
            devKitRailFillPending_ = true;
        }
    }
    if (up) {
        changed = devKitScene_->PointerUp(x - left, y - top) || changed;
    }
    if (changed) {
        DirtyDevKitFromNested();
    }
    return true;
}

bool RoboScene::ForwardDevKitWheel(float x, float y, float deltaX, float deltaY) {
    if (!devKitOpen_ || devKitScene_ == nullptr || !PointerOnDevKit(x, y)) {
        return false;
    }
    float left = 0.0f;
    float top = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    DevKitFrame(left, top, width, height);
    (void)width;
    (void)height;
    if (devKitScene_->Wheel(x - left, y - top, deltaX, deltaY)) {
        DirtyDevKitFromNested();
        return true;
    }
    return false;
}

void RoboScene::PaintDevKit() {
    CompositeDevKit();
}

void RoboScene::CompositeDevKit() {
    if (!devKitOpen_ || fonts_ == nullptr || platform_ == nullptr) {
        return;
    }
    EnsureDevKit();
    if (devKitScene_ == nullptr) {
        return;
    }
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    DevKitFrame(x, y, width, height);
    const bool nestedNeeds =
        !devKitSurfaceValid_ || devKitPaintDirty_ || devKitScene_->NeedsRender() || devKitScene_->fullDirty_;
    const bool parentHitsDevKit =
        fullDirty_ || devKitParentWiped_ ||
        (damageOpen_ && dirtyRight_ > x && dirtyLeft_ < x + width && dirtyBottom_ > y && dirtyTop_ < y + height);
    // Skip when neither nested content nor parent damage touches the window.
    if (!nestedNeeds && !parentHitsDevKit) {
        return;
    }
    const float scale = SceneScale();
    const int surfaceW = std::max(1, static_cast<int>(std::lround(width)));
    const int surfaceH = std::max(1, static_cast<int>(std::lround(height)));
    HostPlatform host;
    host.Bind(platform_, &frame_, static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y)),
              DEV_KIT_RADIUS * scale);
    host.SetSurfaceSize(surfaceW, surfaceH);
    host.SetPixelScale(scale);
    // Parent wiped the window (wallpaper / large damage): restore full retained pixels.
    const bool wipeRestore = devKitParentWiped_ || (parentHitsDevKit && !nestedNeeds);
    host.SetForceFullBlit(wipeRestore);
    frame_.PushClipRect(x, y, width, height);
    // Forward shell pump so nested paint/blit cannot freeze the cursor.
    devKitScene_->SetPaintPump(paintPump_, paintPumpCookie_);
    if (nestedNeeds) {
        const Color page = devKitScene_->GetBackground();
        // Do not EnsureRailChildren / EnsurePaneContent here — Tick micro-steps own that work.
        host.SetForceFullBlit(wipeRestore || !devKitSurfaceValid_);
        if (devKitScene_->items_.empty()) {
            // Stub: empty nested scene — fill window so open is visible immediately.
            frame_.FillRect(static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y)),
                            static_cast<int>(std::lround(width)), static_cast<int>(std::lround(height)),
                            page.alpha > 0 ? page : Color::FromHex("#FFFFFF"));
            PumpPaint();
        } else {
            devKitScene_->Render(host, *fonts_, page.alpha > 0 ? page : Color::FromHex("#FFFFFF"), 0.0f);
        }
        devKitPaintDirty_ = false;
        devKitSurfaceValid_ = true;
    } else if (devKitScene_->frame_.GetPixels() != nullptr &&
               devKitScene_->frame_.GetWidth() == surfaceW &&
               devKitScene_->frame_.GetHeight() == surfaceH) {
        devKitScene_->frame_.SetPresentRect(0, 0, surfaceW, surfaceH);
        host.SetForceFullBlit(true);
        host.Present(devKitScene_->frame_);
    } else {
        const Color page = devKitScene_->GetBackground();
        host.SetForceFullBlit(true);
        if (devKitScene_->items_.empty()) {
            frame_.FillRect(static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y)),
                            static_cast<int>(std::lround(width)), static_cast<int>(std::lround(height)),
                            page.alpha > 0 ? page : Color::FromHex("#FFFFFF"));
            PumpPaint();
        } else {
            devKitScene_->Render(host, *fonts_, page.alpha > 0 ? page : Color::FromHex("#FFFFFF"), 0.0f);
        }
        devKitPaintDirty_ = false;
        devKitSurfaceValid_ = true;
    }
    frame_.PopClip();
    devKitParentWiped_ = false;
}

void RoboScene::EnsureFolderIcon() {
    if (folderIconTried_ || platform_ == nullptr) {
        return;
    }
    folderIconTried_ = true;
    const std::vector<std::uint8_t> bytes = platform_->LoadAsset("Assets/EmptyFolder.svg");
    if (bytes.empty()) {
        return;
    }
    Svg icon;
    if (!LoadSvgDocument(bytes.data(), bytes.size(), icon)) {
        return;
    }
    folderIcon_ = std::move(icon);
}

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
    BackgroundPainter::Paint(frame_, desktop_.GetBackgroundSource(), paintedWidth_, paintedHeight_, 0, 0, paintedWidth_,
                             paintedHeight_);
    ApplyChrome(frame_);
    EnsureMenuMark();
    EnsureFolderIcon();
    // Live-paint full desktop chrome (menu, AppShell, dock). Stale CaptureChrome must not snap dock hover back.
    if (fonts_ != nullptr && desktop_.IsEnabled()) {
        const float barScale = paintedScale_ > 0.0f ? paintedScale_ : std::max(0.5f, platform.GetPixelScale());
        if (desktop_.HasTopBarMenu()) {
            desktop_.GetTopBarMenu().ApplyTheme(theme_ == Theme::Dark);
        }
        const DesktopChrome chrome =
            DesktopPainter::Layout(static_cast<float>(paintedWidth_), static_cast<float>(paintedHeight_), barScale,
                                   desktop_.HasTopBarMenu(), desktop_.HasAppShell(), !desktop_.Dock().empty());
        DesktopPainter::Paint(frame_, *fonts_, desktop_, chrome, barScale, hoveredDock_);
    }
    if (TextSelection::ShowsHandles()) {
        float startX = 0.0f;
        float startY = 0.0f;
        float endX = 0.0f;
        float endY = 0.0f;
        float lineHeight = 0.0f;
        if (TextSelection::HandleGeometry(startX, startY, endX, endY, lineHeight)) {
            const float radius = std::max(5.0f, lineHeight * 0.22f);
            const Color handle = Color::FromHex("#007AFF");
            frame_.FillRoundedRect(startX - radius, startY + lineHeight - radius * 0.15f, radius * 2.0f, radius * 2.0f,
                                   radius, handle, 1.0f);
            frame_.FillRoundedRect(endX - radius, endY + lineHeight - radius * 0.15f, radius * 2.0f, radius * 2.0f, radius,
                                   handle, 1.0f);
        }
    }
    if (devKitOpen_) {
        // Full wallpaper paint wiped the DevKit rect — restore retained pixels.
        devKitParentWiped_ = true;
        CompositeDevKit();
    }
    if (fonts_ != nullptr && desktop_.HasContextMenu() && desktop_.GetContextMenu().IsVisible()) {
        ContextMenu& contextMenu = desktop_.GetContextMenu();
        const bool contextDark = desktop_.HasTopBarMenu() ? desktop_.GetTopBarMenu().IsDark() : theme_ == Theme::Dark;
        contextMenu.ApplyTheme(contextDark);
        const float menuTop = desktop_.HasTopBarMenu() ? desktop_.GetTopBarMenu().BarHeight() : 0.0f;
        float dockInset = 0.0f;
        if (desktop_.DockBarHeight() > 0.0f) {
            dockInset = std::max(0.0f, static_cast<float>(paintedHeight_) - desktop_.DockBarY());
        }
        const float menuScale = paintedScale_ > 0.0f ? paintedScale_ : std::max(0.5f, platform.GetPixelScale());
        ContextMenuPainter::Paint(frame_, *fonts_, contextMenu, menuScale, static_cast<float>(paintedWidth_),
                                  static_cast<float>(paintedHeight_), menuTop, dockInset);
    }
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
    ApplyChromeRect(frame, 0, 0, frame.GetWidth(), frame.GetHeight());
}

void RoboScene::ApplyChromeRect(FrameBuffer& frame, int x, int y, int width, int height) {
    std::uint8_t* pixels = frame.GetPixels();
    const int frameW = frame.GetWidth();
    const int frameH = frame.GetHeight();
    const std::size_t count = chromeMask_.size();
    if (pixels == nullptr || chromePixels_.size() != count * 4u || frameW <= 0 || frameH <= 0) {
        return;
    }
    if (static_cast<std::size_t>(frameW) * static_cast<std::size_t>(frameH) != count) {
        return;
    }
    int left = std::max(0, x);
    int top = std::max(0, y);
    int right = std::min(frameW, x + width);
    int bottom = std::min(frameH, y + height);
    for (int row = top; row < bottom; ++row) {
        for (int col = left; col < right; ++col) {
            const std::size_t index = static_cast<std::size_t>(row) * static_cast<std::size_t>(frameW) +
                                      static_cast<std::size_t>(col);
            if (chromeMask_[index] == 0) {
                continue;
            }
            pixels[index * 4u] = chromePixels_[index * 4u];
            pixels[index * 4u + 1u] = chromePixels_[index * 4u + 1u];
            pixels[index * 4u + 2u] = chromePixels_[index * 4u + 2u];
            pixels[index * 4u + 3u] = chromePixels_[index * 4u + 3u];
        }
    }
}

bool RoboScene::PresentWallpaperAroundDevKit(IPlatform& platform) {
    if (!chromeReady_ || platform_ == nullptr || paintedWidth_ <= 0 || paintedHeight_ <= 0) {
        return false;
    }
    if (frame_.GetWidth() != paintedWidth_ || frame_.GetHeight() != paintedHeight_) {
        return false;
    }
    float kitX = 0.0f;
    float kitY = 0.0f;
    float kitW = 0.0f;
    float kitH = 0.0f;
    DevKitFrame(kitX, kitY, kitW, kitH);
    const int surfaceW = paintedWidth_;
    const int surfaceH = paintedHeight_;
    const int left = std::max(0, static_cast<int>(std::floor(kitX)));
    const int top = std::max(0, static_cast<int>(std::floor(kitY)));
    const int right = std::min(surfaceW, static_cast<int>(std::ceil(kitX + kitW)));
    const int bottom = std::min(surfaceH, static_cast<int>(std::ceil(kitY + kitH)));
    if (kitW <= 1.0f || kitH <= 1.0f || right <= left || bottom <= top) {
        return PresentVideoFrame(platform);
    }

    auto paintBand = [&](int x, int y, int width, int height) {
        if (width <= 0 || height <= 0) {
            return;
        }
        BackgroundPainter::Paint(frame_, desktop_.GetBackgroundSource(), surfaceW, surfaceH, x, y, width, height);
        ApplyChromeRect(frame_, x, y, width, height);
    };
    paintBand(0, 0, surfaceW, top);
    paintBand(0, bottom, surfaceW, surfaceH - bottom);
    paintBand(0, top, left, bottom - top);
    paintBand(right, top, surfaceW - right, bottom - top);

    EnsureMenuMark();
    EnsureFolderIcon();
    // Live-paint chrome only inside bands so DevKit interior pixels stay retained.
    if (fonts_ != nullptr && desktop_.IsEnabled()) {
        const float barScale = paintedScale_ > 0.0f ? paintedScale_ : std::max(0.5f, platform.GetPixelScale());
        if (desktop_.HasTopBarMenu()) {
            desktop_.GetTopBarMenu().ApplyTheme(theme_ == Theme::Dark);
        }
        const DesktopChrome chrome =
            DesktopPainter::Layout(static_cast<float>(surfaceW), static_cast<float>(surfaceH), barScale,
                                   desktop_.HasTopBarMenu(), desktop_.HasAppShell(), !desktop_.Dock().empty());
        auto paintChromeBand = [&](int x, int y, int width, int height) {
            if (width <= 0 || height <= 0) {
                return;
            }
            frame_.PushClipRect(static_cast<float>(x), static_cast<float>(y), static_cast<float>(width),
                                static_cast<float>(height));
            DesktopPainter::Paint(frame_, *fonts_, desktop_, chrome, barScale, hoveredDock_);
            frame_.PopClip();
        };
        paintChromeBand(0, 0, surfaceW, top);
        paintChromeBand(0, bottom, surfaceW, surfaceH - bottom);
        paintChromeBand(0, top, left, bottom - top);
        paintChromeBand(right, top, surfaceW - right, bottom - top);
    }

    // Bands do not cover the DevKit interior — skip wipe-restore unless nested content needs paint.
    const bool nestedNeeds = devKitPaintDirty_ || !devKitSurfaceValid_ ||
                             (devKitScene_ != nullptr && (devKitScene_->NeedsRender() || devKitScene_->fullDirty_));
    if (nestedNeeds) {
        CompositeDevKit();
    }

    // AABB of the L-shaped bands is the full surface for an inset window; Present still
    // uploads that rect, but paint/chrome/DevKit wipe work above is what we avoided.
    frame_.SetPresentRect(0, 0, surfaceW, surfaceH);
    platform.Present(frame_);
    needsRender_ = false;
    fullDirty_ = false;
    damageOpen_ = false;
    frameReady_ = true;
    return true;
}

void RoboScene::Render(IPlatform& platform, FontEngine& fonts, const Color& background, float padding) {
    if (wallpaperAroundDevKit_ && !fullDirty_ && !damageOpen_ && PresentWallpaperAroundDevKit(platform)) {
        wallpaperAroundDevKit_ = false;
        videoOnly_ = false;
        return;
    }
    wallpaperAroundDevKit_ = false;
    if (videoOnly_ && !fullDirty_ && !damageOpen_ && PresentVideoFrame(platform)) {
        videoOnly_ = false;
        return;
    }
    videoOnly_ = false;
    const FoundationScope foundationScope(ActiveFoundation(), theme_, themeMix_);
    platform_ = &platform;
    fonts_ = &fonts;
    EnsureMenuMark();
    EnsureFolderIcon();
    Color lightPage = background;
    Color darkPage = Color::FromHex("#000000");
    ActiveFoundation().LookupColor("bg.primary", Theme::Light, lightPage);
    ActiveFoundation().LookupColor("bg.primary", Theme::Dark, darkPage);
    background_ = LerpPageColor(lightPage, darkPage, themeMix_);
    if (desktop_.IsEnabled() && desktop_.HasBackground()) {
        desktop_.GetBackgroundSource().EnsureLoaded(platform);
        if (!desktop_.GetBackgroundSource().PaintsMedia()) {
            background_ = ResolveColor(desktop_.GetBackground(), desktop_.GetBackgroundToken());
            if (background_.red > 240 && background_.green > 240 && background_.blue > 240) {
                background_ = Color::FromHex("#202838");
            }
        }
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
    frame.BeginDisplayList();
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

    float verticalReserve = paintedVerticalReserve_;
    float horizontalReserve = paintedHorizontalReserve_;
    float titleBarBottom = 0.0f;
    for (int pass = 0; pass < 2; ++pass) {
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
        // chromeBase_ is only needed for CaptureChrome on full media paints — skip O(W×H) memcpy on partial.
        if (!partial && frame.GetPixels() != nullptr) {
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
                                                   desktop_.HasTopBarMenu(), desktop_.HasAppShell(),
                                                   !desktop_.Dock().empty());
            frame.PushClipRect(0.0f, desktopChrome.contentY, static_cast<float>(width), desktopChrome.contentHeight);
            desktopClip = true;
            paintedContentX_ = 0.0f;
            paintedContentY_ = desktopChrome.contentY;
            paintedContentW_ = static_cast<float>(width);
            paintedContentH_ = desktopChrome.contentHeight;
            cursorY = desktopChrome.contentY + pad - scrollY_;
            flowEdge = cursorY;
        }

        for (int i = 0; i < static_cast<int>(items_.size()); ++i) {
            frame.Pump();
            SceneItem& item = items_[static_cast<std::size_t>(i)];
            const float yBefore = cursorY;
            bool edgeSet = false;
            if (item.kind == SceneItemKind::Text) {
                if (partial && item.text) {
                    const TextLayoutCache& cache = item.text->LayoutCache();
                    if (cache.valid && cache.width > 1.0f && cache.height > 1.0f &&
                        frame.MissesClip(static_cast<int>(cache.originX), static_cast<int>(cache.originY),
                                         static_cast<int>(cache.width), static_cast<int>(std::max(cache.height, cache.advanceY)))) {
                        cursorY += cache.advanceY > 0.0f ? cache.advanceY : cache.height;
                        continue;
                    }
                }
                cursorY += TextPainter::Paint(frame, fonts, *item.text, columnX, cursorY, columnW, paintScale);
                contentRight = std::max(contentRight, columnX + columnW + scrollX_);
            } else if (item.kind == SceneItemKind::Button) {
                if (partial && item.button) {
                    const ButtonBounds& prior = item.button->GetBounds();
                    if (prior.width > 1.0f && prior.height > 1.0f &&
                        frame.MissesClip(static_cast<int>(prior.x), static_cast<int>(prior.y), static_cast<int>(prior.width),
                                         static_cast<int>(prior.height))) {
                        cursorY += prior.height;
                        continue;
                    }
                }
                const ButtonPaintResult painted =
                    ButtonPainter::Paint(frame, fonts, *item.button, columnX, cursorY, columnW, paintScale);
                cursorY += painted.advanceY;
                const ButtonBounds& bounds = item.button->GetBounds();
                contentRight = std::max(contentRight, bounds.x + bounds.width + scrollX_);
            } else if (item.kind == SceneItemKind::Input) {
                if (partial && item.input) {
                    const InputBounds& prior = item.input->GetBounds();
                    if (prior.width > 1.0f && prior.height > 1.0f &&
                        frame.MissesClip(static_cast<int>(prior.x), static_cast<int>(prior.y), static_cast<int>(prior.width),
                                         static_cast<int>(prior.height))) {
                        cursorY += prior.height;
                        continue;
                    }
                }
                if (item.input.get() == focusedInput_) {
                    item.input->SetInteraction(InputInteraction::Focused);
                } else if (item.input.get() == hoveredInput_ && item.input->GetInteraction() != InputInteraction::Focused) {
                    item.input->SetInteraction(InputInteraction::Hovered);
                } else if (item.input.get() != focusedInput_) {
                    item.input->SetInteraction(InputInteraction::Normal);
                }

                const InputPaintResult painted =
                    InputPainter::Paint(frame, fonts, *item.input, columnX, cursorY, columnW, paintScale);
                cursorY += painted.advanceY;
                const InputBounds& bounds = item.input->GetBounds();
                contentRight = std::max(contentRight, bounds.x + bounds.width + scrollX_);
            } else if (item.kind == SceneItemKind::Scrollbar) {
                if (item.scrollbar.get() == draggingScrollbar_) {
                    item.scrollbar->SetInteraction(ScrollbarInteraction::Dragging);
                } else if (item.scrollbar.get() == hoveredScrollbar_) {
                    item.scrollbar->SetInteraction(ScrollbarInteraction::Hovered);
                } else {
                    item.scrollbar->SetInteraction(ScrollbarInteraction::Normal);
                }

                const ScrollbarPaintResult painted =
                    ScrollbarPainter::Paint(frame, *item.scrollbar, columnX, cursorY, columnW, paintScale);
                cursorY += painted.advanceY;
                const ScrollbarBounds& bounds = item.scrollbar->GetBounds();
                contentRight = std::max(contentRight, bounds.x + bounds.width + scrollX_);
            } else if (item.kind == SceneItemKind::Container) {
                if (!item.container->ShownWhen().empty() && item.container->ShownWhen() != ActivePaneId()) {
                    item.container->SetBounds(InputBounds{});
                    continue;
                }
                if (partial) {
                    const InputBounds& prior = item.container->GetBounds();
                    if (prior.width > 1.0f && prior.height > 1.0f &&
                        frame.MissesClip(static_cast<int>(prior.x), static_cast<int>(prior.y),
                                         static_cast<int>(prior.width), static_cast<int>(prior.height))) {
                        cursorY += prior.height;
                        contentRight = std::max(contentRight, prior.x + prior.width + scrollX_);
                        continue;
                    }
                }
                const ContainerPaintResult painted =
                    ContainerPainter::Paint(frame, fonts, *item.container, columnX, cursorY, columnW, paintScale);
                cursorY += painted.advanceY;
                const InputBounds& bounds = item.container->GetBounds();
                contentRight = std::max(contentRight, bounds.x + bounds.width + scrollX_);
            } else if (item.kind == SceneItemKind::Flexbox && item.flexbox) {
                if (partial) {
                    const FlexboxBounds& prior = item.flexbox->GetBounds();
                    if (prior.width > 1.0f && prior.height > 1.0f &&
                        frame.MissesClip(static_cast<int>(prior.x), static_cast<int>(prior.y),
                                         static_cast<int>(prior.width), static_cast<int>(prior.height))) {
                        cursorY += prior.height;
                        contentRight = std::max(contentRight, prior.x + prior.width + scrollX_);
                        continue;
                    }
                }
                const FlexboxPaintResult painted =
                    FlexboxPainter::Paint(frame, fonts, *item.flexbox, columnX, cursorY, columnW, paintScale);
                cursorY += painted.advanceY;
                const FlexboxBounds& bounds = item.flexbox->GetBounds();
                contentRight = std::max(contentRight, bounds.x + bounds.width + scrollX_);
            } else if (item.kind == SceneItemKind::Grid && item.grid) {
                if (partial) {
                    const GridBounds& prior = item.grid->GetBounds();
                    if (prior.width > 1.0f && prior.height > 1.0f &&
                        frame.MissesClip(static_cast<int>(prior.x), static_cast<int>(prior.y),
                                         static_cast<int>(prior.width), static_cast<int>(prior.height))) {
                        cursorY += prior.height;
                        contentRight = std::max(contentRight, prior.x + prior.width + scrollX_);
                        continue;
                    }
                }
                const GridPaintResult painted =
                    GridPainter::Paint(frame, fonts, *item.grid, columnX, cursorY, columnW, paintScale);
                cursorY += painted.advanceY;
                const GridBounds& bounds = item.grid->GetBounds();
                contentRight = std::max(contentRight, bounds.x + bounds.width + scrollX_);
            } else if (item.kind == SceneItemKind::Svg) {
                if (partial && item.svg) {
                    const SvgBounds& prior = item.svg->GetBounds();
                    if (prior.width > 1.0f && prior.height > 1.0f &&
                        frame.MissesClip(static_cast<int>(prior.x), static_cast<int>(prior.y), static_cast<int>(prior.width),
                                         static_cast<int>(prior.height))) {
                        cursorY += prior.height;
                        continue;
                    }
                }
                const SvgPaintResult painted =
                    SvgPainter::Paint(frame, *item.svg, columnX, cursorY, columnW, paintScale);
                cursorY += painted.advanceY;
                const SvgBounds& bounds = item.svg->GetBounds();
                contentRight = std::max(contentRight, bounds.x + bounds.width + scrollX_);
            } else if (item.kind == SceneItemKind::BrandLogo) {
                const BrandLogoPaintResult painted =
                    BrandLogoPainter::Paint(frame, fonts, *item.brandLogo, columnX, cursorY, paintScale);
                cursorY += painted.advanceY;
                const BrandLogoMeasureResult measured = BrandLogoPainter::Measure(fonts, *item.brandLogo, paintScale);
                contentRight = std::max(contentRight, columnX + measured.width + scrollX_);
            } else if (item.kind == SceneItemKind::TitleBar) {
                item.titleBar->SetSidebarOpen(sidebarVisible_);
                item.titleBar->SetSidebarAmount(sidebarAmount_);
                const float barX = 0.0f;
                const float barY = cursorY + scrollY_;
                const float barWidth = static_cast<float>(width);
                const TitleBarPaintResult painted =
                    TitleBarPainter::Paint(frame, fonts, *item.titleBar, barX, barY, barWidth, paintScale);
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
                const float paneWidth = item.sidebar->LayoutWidth(sidebarAmount_) * paintScale;
                if (paneWidth > 0.5f) {
                    frame.PushClipRect(columnX, surfaceTop, paneWidth, sidebarHeight);
                    SidebarPainter::Paint(frame, fonts, *item.sidebar, columnX, surfaceTop, sidebarHeight, paintScale, sidebarAmount_);
                    frame.PopClip();
                    columnX += paneWidth;
                    columnW = std::max(0.0f, columnW - paneWidth);
                }
                cursorY = flowEdge;
                edgeSet = true;
                contentRight = std::max(contentRight, columnX + scrollX_);
            } else if (item.kind == SceneItemKind::SidebarItem) {
                const SidebarItemMeasureResult measured = SidebarPainter::MeasureItem(fonts, *item.sidebarItem, paintScale);
                SidebarPainter::PaintItem(frame, fonts, *item.sidebarItem, columnX, cursorY, measured.width, measured.height, paintScale, false);
                cursorY += measured.height + 8.0f * paintScale;
                contentRight = std::max(contentRight, columnX + measured.width + scrollX_);
            } else if (item.kind == SceneItemKind::Separator) {
                const bool vertical = item.separator->GetOrientation() == SeparatorOrientation::Vertical;
                const SeparatorMeasureResult measured =
                    SeparatorPainter::Measure(fonts, *item.separator, columnW, 0.0f, paintScale);
                const float boxWidth = vertical ? measured.width : columnW;
                const float boxHeight = measured.height;
                const SeparatorPaintResult painted =
                    SeparatorPainter::Paint(frame, fonts, *item.separator, columnX, cursorY, boxWidth, boxHeight, paintScale);
                cursorY += painted.advanceY;
                contentRight = std::max(contentRight, columnX + painted.width + scrollX_);
            } else if (item.kind == SceneItemKind::ThemeToggle) {
                const ThemeTogglePaintResult painted =
                    ThemeTogglePainter::Paint(frame, fonts, *item.themeToggle, columnX, cursorY, columnW, paintScale);
                cursorY += painted.advanceY;
                contentRight = std::max(contentRight, columnX + item.themeToggle->GetBounds().width + scrollX_);
            } else if (item.kind == SceneItemKind::ThemeIconToggle) {
                const ThemeIconTogglePaintResult painted =
                    ThemeIconTogglePainter::Paint(frame, *item.themeIconToggle, columnX, cursorY, paintScale);
                cursorY += painted.advanceY;
                contentRight = std::max(contentRight, columnX + item.themeIconToggle->GetBounds().width + scrollX_);
            } else if (item.kind == SceneItemKind::MotionShape) {
                item.motionShape->Paint(frame, columnX, cursorY, columnW, item.motionShape->Height() * paintScale, paintScale);
                const float advance = item.motionShape->Height() * paintScale + 12.0f * paintScale;
                cursorY += advance;
                contentRight = std::max(contentRight, columnX + item.motionShape->Width() * paintScale + scrollX_);
            } else if (item.kind == SceneItemKind::SelectionArea) {
                float areaX = columnX;
                float areaY = cursorY;
                float areaW = columnW;
                float areaH = 420.0f * paintScale;
                if (desktop_.IsEnabled()) {
                    areaX = 0.0f;
                    areaY = desktopChrome.contentY;
                    areaW = static_cast<float>(width);
                    areaH = desktopChrome.contentHeight;
                }
                const bool selectionDark = desktop_.HasTopBarMenu() ? desktop_.GetTopBarMenu().IsDark() : theme_ == Theme::Dark;
                SelectionAreaPainter::Paint(frame, fonts, item.selectionArea, areaX, areaY, areaW, areaH, paintScale,
                                            selectionDark, folderIcon_.GetShapes().empty() ? nullptr : &folderIcon_);
                if (!desktop_.IsEnabled()) {
                    cursorY += areaH;
                    contentRight = std::max(contentRight, areaX + areaW + scrollX_);
                }
            } else if (item.kind == SceneItemKind::Position && item.position) {
                const PositionPaintResult painted =
                    PositionPainter::Paint(frame, fonts, *item.position, columnX, cursorY, columnW, paintScale);
                cursorY += painted.advanceY;
                const InputBounds& bounds = item.position->GetBounds();
                contentRight = std::max(contentRight, bounds.x + bounds.width + scrollX_);
            }
            if (!edgeSet) {
                flowEdge = cursorY;
            }
        }
        if (desktopClip) {
            frame.PopClip();
            frame.Pump();
            if (desktop_.HasTopBarMenu()) {
                desktop_.GetTopBarMenu().ApplyTheme(theme_ == Theme::Dark);
            }
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
        if (!partial && pass < 1 && (nextVertical != verticalReserve || nextHorizontal != horizontalReserve || scrollClamped)) {
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
    if (!partial && desktop_.IsEnabled() && desktop_.GetBackgroundSource().PaintsMedia()) {
        CaptureChrome();
    }
    if (partial) {
        frame.PopClip();
    }
    if (devKitOpen_) {
        CompositeDevKit();
    }
    if (desktop_.HasContextMenu() && desktop_.GetContextMenu().IsVisible()) {
        ContextMenu& contextMenu = desktop_.GetContextMenu();
        const bool contextDark = desktop_.HasTopBarMenu() ? desktop_.GetTopBarMenu().IsDark() : theme_ == Theme::Dark;
        contextMenu.ApplyTheme(contextDark);
        float menuTop = 0.0f;
        if (desktop_.HasTopBarMenu()) {
            menuTop = desktop_.GetTopBarMenu().BarHeight();
        }
        float dockInset = 0.0f;
        if (desktop_.DockBarHeight() > 0.0f) {
            dockInset = std::max(0.0f, static_cast<float>(height) - desktop_.DockBarY());
        }
        ContextMenuPainter::Paint(frame, fonts, contextMenu, paintScale, static_cast<float>(width), static_cast<float>(height), menuTop, dockInset);
    }
    if (!partial || damageWidth <= 0 || damageHeight <= 0) {
        damageX = 0;
        damageY = 0;
        damageWidth = width;
        damageHeight = height;
    }
    if (launcherOpen_ && launcherCount_ > 0) {
        frame.FillRect(32, 72, 320, 36 * launcherCount_ + 16, Color::FromHex("#1C1C1E"), 0.94f);
        for (int index = 0; index < launcherCount_; ++index) {
            fonts.DrawString(frame, 48.0f, 100.0f + static_cast<float>(index) * 36.0f, launcherNames_[index], 16.0f,
                             FontFaceId::Medium, Color::FromHex("#F5F5F7"), 1.0f, 0.0f, true);
        }
    }
    frame.SetPump(nullptr, nullptr);
    frame.EndDisplayList();
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
    CollectTextLineDamage();
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
    // Zoom changes paint scale; retained layout keys must remesaure.
    MarkFullDirty();
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
        if (item.kind == FlexItemKind::ThemeToggle && item.themeToggle->GetBounds().Contains(x, y)) {
            return item.themeToggle.get();
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
        if (item.kind == FlexItemKind::ThemeIconToggle && item.themeIconToggle->GetBounds().Contains(x, y)) {
            return item.themeIconToggle.get();
        }
        if (item.kind == FlexItemKind::TitleBar && item.titleBar->ThemeIcon().GetBounds().Contains(x, y)) {
            return &item.titleBar->ThemeIcon();
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
        if (item.kind == GridItemKind::ThemeToggle && item.themeToggle->GetBounds().Contains(x, y)) {
            return item.themeToggle.get();
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
        if (item.kind == GridItemKind::ThemeIconToggle && item.themeIconToggle->GetBounds().Contains(x, y)) {
            return item.themeIconToggle.get();
        }
        if (item.kind == GridItemKind::TitleBar && item.titleBar->ThemeIcon().GetBounds().Contains(x, y)) {
            return &item.titleBar->ThemeIcon();
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
        if (item.kind == PositionItemKind::ThemeToggle && item.themeToggle->GetBounds().Contains(x, y)) {
            return item.themeToggle.get();
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
        if (item.kind == PositionItemKind::ThemeIconToggle && item.themeIconToggle->GetBounds().Contains(x, y)) {
            return item.themeIconToggle.get();
        }
        if (item.kind == PositionItemKind::TitleBar && item.titleBar->ThemeIcon().GetBounds().Contains(x, y)) {
            return &item.titleBar->ThemeIcon();
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
            const ThemeIconHot next = HotFor(*item.themeIconToggle, x, y);
            if (item.themeIconToggle->GetHot() != next) {
                item.themeIconToggle->SetHot(next);
                changed = true;
            }
        } else if (item.kind == FlexItemKind::TitleBar) {
            const ThemeIconHot next = HotFor(item.titleBar->ThemeIcon(), x, y);
            if (item.titleBar->ThemeIcon().GetHot() != next) {
                item.titleBar->ThemeIcon().SetHot(next);
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
            const ThemeIconHot next = HotFor(*item.themeIconToggle, x, y);
            if (item.themeIconToggle->GetHot() != next) {
                item.themeIconToggle->SetHot(next);
                changed = true;
            }
        } else if (item.kind == GridItemKind::TitleBar) {
            const ThemeIconHot next = HotFor(item.titleBar->ThemeIcon(), x, y);
            if (item.titleBar->ThemeIcon().GetHot() != next) {
                item.titleBar->ThemeIcon().SetHot(next);
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
            const ThemeIconHot next = HotFor(*item.themeIconToggle, x, y);
            if (item.themeIconToggle->GetHot() != next) {
                item.themeIconToggle->SetHot(next);
                changed = true;
            }
        } else if (item.kind == PositionItemKind::TitleBar) {
            const ThemeIconHot next = HotFor(item.titleBar->ThemeIcon(), x, y);
            if (item.titleBar->ThemeIcon().GetHot() != next) {
                item.titleBar->ThemeIcon().SetHot(next);
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
        if (item.kind == SceneItemKind::ThemeToggle && item.themeToggle->GetBounds().Contains(x, y)) {
            return item.themeToggle.get();
        }
        if (item.kind == SceneItemKind::Container) {
            if (ThemeToggle* found = FindThemeToggleInContainer(*item.container, x, y)) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Flexbox && item.flexbox) {
            if (ThemeToggle* found = FindThemeToggleInFlexbox(*item.flexbox, x, y)) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Grid && item.grid) {
            if (ThemeToggle* found = FindThemeToggleInGrid(*item.grid, x, y)) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Position && item.position) {
            if (ThemeToggle* found = FindThemeToggleInPosition(*item.position, x, y)) {
                return found;
            }
        }
    }
    return nullptr;
}

ThemeIconToggle* RoboScene::FindThemeIconToggleAt(float x, float y) {
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        SceneItem& item = items_[static_cast<std::size_t>(i)];
        if (item.kind == SceneItemKind::ThemeIconToggle && item.themeIconToggle->GetBounds().Contains(x, y)) {
            return item.themeIconToggle.get();
        }
        if (item.kind == SceneItemKind::TitleBar && item.titleBar->ThemeIcon().GetBounds().Contains(x, y)) {
            return &item.titleBar->ThemeIcon();
        }
        if (item.kind == SceneItemKind::Container) {
            if (ThemeIconToggle* found = FindThemeIconInContainer(*item.container, x, y)) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Flexbox && item.flexbox) {
            if (ThemeIconToggle* found = FindThemeIconInFlexbox(*item.flexbox, x, y)) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Grid && item.grid) {
            if (ThemeIconToggle* found = FindThemeIconInGrid(*item.grid, x, y)) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Position && item.position) {
            if (ThemeIconToggle* found = FindThemeIconInPosition(*item.position, x, y)) {
                return found;
            }
        }
    }
    return nullptr;
}

void DirtyThemeControl(RoboScene& scene, float x, float y, float width, float height) {
    if (width < 1.0f || height < 1.0f) {
        scene.DamageRect(0.0f, 0.0f, 1.0f, 1.0f);
        return;
    }
    scene.DamageRect(x - 4.0f, y - 4.0f, width + 8.0f, height + 8.0f);
}

bool SyncThemeHot(RoboScene& scene, std::vector<SceneItem>& items, const std::string& paneId, float x, float y) {
    bool changed = false;
    for (SceneItem& item : items) {
        if (item.kind == SceneItemKind::ThemeIconToggle) {
            const ThemeIconHot next = HotFor(*item.themeIconToggle, x, y);
            if (item.themeIconToggle->GetHot() != next) {
                item.themeIconToggle->SetHot(next);
                const ThemeToggleBounds& bounds = item.themeIconToggle->GetBounds();
                DirtyThemeControl(scene, bounds.x, bounds.y, bounds.width, bounds.height);
                changed = true;
            }
        } else if (item.kind == SceneItemKind::TitleBar) {
            const ThemeIconHot next = HotFor(item.titleBar->ThemeIcon(), x, y);
            if (item.titleBar->ThemeIcon().GetHot() != next) {
                item.titleBar->ThemeIcon().SetHot(next);
                const ThemeToggleBounds& bounds = item.titleBar->ThemeIcon().GetBounds();
                DirtyThemeControl(scene, bounds.x, bounds.y, bounds.width, bounds.height);
                changed = true;
            }
        } else if (item.kind == SceneItemKind::Container && item.container) {
            if (!item.container->ShownWhen().empty() && item.container->ShownWhen() != paneId) {
                continue;
            }
            if (SyncHotInContainer(*item.container, x, y)) {
                const InputBounds& bounds = item.container->GetBounds();
                DirtyThemeControl(scene, bounds.x, bounds.y, bounds.width, bounds.height);
                changed = true;
            }
        } else if (item.kind == SceneItemKind::Flexbox && item.flexbox) {
            if (SyncHotInFlexbox(*item.flexbox, x, y)) {
                const FlexboxBounds& bounds = item.flexbox->GetBounds();
                DirtyThemeControl(scene, bounds.x, bounds.y, bounds.width, bounds.height);
                changed = true;
            }
        } else if (item.kind == SceneItemKind::Grid && item.grid) {
            if (SyncHotInGrid(*item.grid, x, y)) {
                const GridBounds& bounds = item.grid->GetBounds();
                DirtyThemeControl(scene, bounds.x, bounds.y, bounds.width, bounds.height);
                changed = true;
            }
        } else if (item.kind == SceneItemKind::Position && item.position) {
            if (SyncHotInPosition(*item.position, x, y)) {
                const InputBounds& bounds = item.position->GetBounds();
                DirtyThemeControl(scene, bounds.x, bounds.y, bounds.width, bounds.height);
                changed = true;
            }
        }
    }
    return changed;
}

Button* RoboScene::FindButtonAt(float x, float y) {
    for (int i = static_cast<int>(items_.size()) - 1; i >= 0; --i) {
        SceneItem& item = items_[static_cast<std::size_t>(i)];
        if (item.kind == SceneItemKind::Button) {
            if (item.button->GetBounds().Contains(x, y)) {
                return item.button.get();
            }
        } else if (item.kind == SceneItemKind::TitleBar) {
            if (item.titleBar->SidebarButton().GetBounds().Contains(x, y)) {
                return &item.titleBar->SidebarButton();
            }
        } else if (item.kind == SceneItemKind::Container) {
            if (!PaneIsActive(*item.container)) {
                continue;
            }
            Button* found = FindButtonInContainer(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Flexbox && item.flexbox) {
            Button* found = FindButtonInFlexbox(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Grid && item.grid) {
            Button* found = FindButtonInGrid(*item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Position && item.position) {
            Button* found = FindButtonInPosition(*item.position, x, y);
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
            if (item.input->GetBounds().Contains(x, y)) {
                return item.input.get();
            }
        } else if (item.kind == SceneItemKind::Container) {
            if (!PaneIsActive(*item.container)) {
                continue;
            }
            Input* found = FindInputInContainer(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Flexbox && item.flexbox) {
            Input* found = FindInputInFlexbox(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Grid && item.grid) {
            Input* found = FindInputInGrid(*item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Position && item.position) {
            Input* found = FindInputInPosition(*item.position, x, y);
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
            Scrollbar& bar = item.sidebar->PanelScrollbar();
            if (bar.IsVisible() && bar.GetBounds().Contains(x, y)) {
                return &bar;
            }
        } else if (item.kind == SceneItemKind::Scrollbar) {
            if (item.scrollbar->IsVisible() && item.scrollbar->GetBounds().Contains(x, y)) {
                return item.scrollbar.get();
            }
        } else if (item.kind == SceneItemKind::Container) {
            if (!PaneIsActive(*item.container)) {
                continue;
            }
            Scrollbar* found = FindScrollbarInContainer(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Flexbox && item.flexbox) {
            Scrollbar* found = FindScrollbarInFlexbox(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Grid && item.grid) {
            Scrollbar* found = FindScrollbarInGrid(*item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Position && item.position) {
            Scrollbar* found = FindScrollbarInPosition(*item.position, x, y);
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
            if (item.button->GetBounds().Contains(x, y)) {
                return item.button.get();
            }
        } else if (item.kind == SceneItemKind::TitleBar) {
            if (item.titleBar->SidebarButton().GetBounds().Contains(x, y)) {
                return &item.titleBar->SidebarButton();
            }
        } else if (item.kind == SceneItemKind::Container) {
            if (!PaneIsActive(*item.container)) {
                continue;
            }
            const Button* found = FindButtonInContainerConst(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Flexbox && item.flexbox) {
            const Button* found = FindButtonInFlexboxConst(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Grid && item.grid) {
            const Button* found = FindButtonInGridConst(*item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Position && item.position) {
            const Button* found = FindButtonInPositionConst(*item.position, x, y);
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
            if (item.input->GetBounds().Contains(x, y)) {
                return item.input.get();
            }
        } else if (item.kind == SceneItemKind::Container) {
            if (!PaneIsActive(*item.container)) {
                continue;
            }
            const Input* found = FindInputInContainerConst(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Flexbox && item.flexbox) {
            const Input* found = FindInputInFlexboxConst(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Grid && item.grid) {
            const Input* found = FindInputInGridConst(*item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Position && item.position) {
            const Input* found = FindInputInPositionConst(*item.position, x, y);
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
            const Scrollbar& bar = item.sidebar->PanelScrollbar();
            if (bar.IsVisible() && bar.GetBounds().Contains(x, y)) {
                return &bar;
            }
        } else if (item.kind == SceneItemKind::Scrollbar) {
            if (item.scrollbar->IsVisible() && item.scrollbar->GetBounds().Contains(x, y)) {
                return item.scrollbar.get();
            }
        } else if (item.kind == SceneItemKind::Container) {
            if (!PaneIsActive(*item.container)) {
                continue;
            }
            const Scrollbar* found = FindScrollbarInContainerConst(*item.container, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Flexbox && item.flexbox) {
            const Scrollbar* found = FindScrollbarInFlexboxConst(*item.flexbox, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Grid && item.grid) {
            const Scrollbar* found = FindScrollbarInGridConst(*item.grid, x, y);
            if (found) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Position && item.position) {
            const Scrollbar* found = FindScrollbarInPositionConst(*item.position, x, y);
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
            if (!PaneIsActive(*item.container)) {
                continue;
            }
            found = FindContainerInContainer(*item.container, x, y);
        } else if (item.kind == SceneItemKind::Flexbox && item.flexbox) {
            found = FindContainerInFlexbox(*item.flexbox, x, y);
        } else if (item.kind == SceneItemKind::Grid && item.grid) {
            found = FindContainerInGrid(*item.grid, x, y);
        } else if (item.kind == SceneItemKind::Position && item.position) {
            found = FindContainerInPosition(*item.position, x, y);
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
    CollectSceneContainers(items_, ActivePaneId(), x, y, containers);
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
    VisitSceneScrollbars(items_, ActivePaneId(), visit);
    visit(pageVertical_);
    visit(pageHorizontal_);
    return best;
}

bool RoboScene::Wheel(float x, float y, float deltaX, float deltaY) {
    if (ForwardDevKitWheel(x, y, deltaX, deltaY)) {
        return true;
    }
    if (desktop_.HasContextMenu() && deltaY != 0.0f && desktop_.GetContextMenu().IsVisible() && desktop_.GetContextMenu().Contains(x, y)) {
        ContextMenu& contextMenu = desktop_.GetContextMenu();
        const float step = deltaY > 0.0f ? 0.08f : -0.08f;
        contextMenu.SetTransparency(contextMenu.GetTransparency() + step);
        DirtyContextMenu();
        return true;
    }
    if (desktop_.HasTopBarMenu() && deltaY != 0.0f) {
        const TopBarMenu& menu = desktop_.GetTopBarMenu();
        if (menu.BarHeight() > 0.0f && y >= menu.BarY() && y < menu.BarY() + menu.BarHeight()) {
            const float step = deltaY > 0.0f ? 0.08f : -0.08f;
            SetMenuTransparency(menu.GetTransparency() + step);
            return true;
        }
    }
    if (std::fabs(deltaY) >= std::fabs(deltaX)) {
        for (SceneItem& item : items_) {
            if (item.kind == SceneItemKind::Sidebar && item.sidebar->Contains(x, y) && item.sidebar->ScrollBy(deltaY, x)) {
                MarkContentDirty();
                return true;
            }
        }
    }
    // Prefer the bar whose axis matches the gesture, then any scroller under
    // the cursor. A mouse wheel over a horizontal bar still moves it.
    const bool wantHorizontal = std::fabs(deltaX) > std::fabs(deltaY);
    Scrollbar* target = nullptr;
    std::vector<Container*> containers;
    CollectSceneContainers(items_, ActivePaneId(), x, y, containers);
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
    CollectSceneContainers(items_, ActivePaneId(), x, y, containers);
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
        if (item.kind == SceneItemKind::Sidebar && item.sidebar->HitRailArrow(x, y) != SidebarRailArrow::None) {
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
            if (SidebarItem* found = item.sidebar->HitItem(x, y)) {
                return found;
            }
        } else if (item.kind == SceneItemKind::SidebarItem) {
            if (item.sidebarItem->CanActivate() && item.sidebarItem->GetBounds().Contains(x, y)) {
                return item.sidebarItem.get();
            }
        } else if (item.kind == SceneItemKind::Container && item.container) {
            if (SidebarItem* found = FindPaneInContainer(*item.container, x, y)) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Flexbox && item.flexbox) {
            if (SidebarItem* found = FindPaneInFlexbox(*item.flexbox, x, y)) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Grid && item.grid) {
            if (SidebarItem* found = FindPaneInGrid(*item.grid, x, y)) {
                return found;
            }
        } else if (item.kind == SceneItemKind::Position && item.position) {
            if (SidebarItem* found = FindPaneInPosition(*item.position, x, y)) {
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
    DirtySidebarItem(hoveredSidebarItem_);
    if (hoveredSidebarItem_ && hoveredSidebarItem_ != pressedSidebarItem_) {
        hoveredSidebarItem_->SetInteraction(SidebarItemInteraction::Normal);
    }
    hoveredSidebarItem_ = item;
    if (hoveredSidebarItem_ && hoveredSidebarItem_ != pressedSidebarItem_) {
        hoveredSidebarItem_->SetInteraction(SidebarItemInteraction::Hovered);
    }
    DirtySidebarItem(hoveredSidebarItem_);
    return true;
}

bool RoboScene::SetPressedSidebarItem(SidebarItem* item) {
    if (item && !item->CanActivate()) {
        item = nullptr;
    }
    if (pressedSidebarItem_ == item) {
        return false;
    }
    DirtySidebarItem(pressedSidebarItem_);
    if (pressedSidebarItem_) {
        const SidebarItemInteraction next =
            (pressedSidebarItem_ == hoveredSidebarItem_) ? SidebarItemInteraction::Hovered : SidebarItemInteraction::Normal;
        pressedSidebarItem_->SetInteraction(next);
    }
    pressedSidebarItem_ = item;
    if (pressedSidebarItem_) {
        pressedSidebarItem_->SetInteraction(SidebarItemInteraction::Pressed);
    }
    DirtySidebarItem(pressedSidebarItem_);
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
            DirtySidebarItem(focusedSidebarItem_);
            return true;
        }
        return false;
    }
    DirtySidebarItem(focusedSidebarItem_);
    if (focusedSidebarItem_) {
        focusedSidebarItem_->SetFocused(false);
    }
    focusedSidebarItem_ = item;
    if (focusedSidebarItem_) {
        focusedSidebarItem_->SetFocused(true);
        focusedSidebarItem_->SetFocusVisible(showRing);
    }
    DirtySidebarItem(focusedSidebarItem_);
    return true;
}

bool RoboScene::UpdateRailArrows(float x, float y) {
    bool changed = false;
    for (SceneItem& item : items_) {
        if (item.kind != SceneItemKind::Sidebar) {
            continue;
        }
        const SidebarRailArrow hit = item.sidebar->HitRailArrow(x, y);
        if (item.sidebar->SetHotRailArrow(hit)) {
            DirtyRailArrows(*item.sidebar);
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
        const SidebarRailArrow hit = item.sidebar->HitRailArrow(x, y);
        if (hit == SidebarRailArrow::None) {
            continue;
        }
        item.sidebar->SetPressedRailArrow(hit);
        item.sidebar->SetHotRailArrow(hit);
        item.sidebar->StepRail(hit);
        DirtyRailArrows(*item.sidebar);
        pressed = true;
    }
    if (!pressed) {
        return false;
    }
    SetPressedSidebarItem(nullptr);
    SetPressedButton(nullptr);
    SetFocusedButton(nullptr);
    SetFocusedInput(nullptr);
    MarkSurfaceDirty();
    return true;
}

bool RoboScene::ReleaseRailArrows() {
    bool changed = false;
    for (SceneItem& item : items_) {
        if (item.kind != SceneItemKind::Sidebar) {
            continue;
        }
        if (item.sidebar->SetPressedRailArrow(SidebarRailArrow::None)) {
            DirtyRailArrows(*item.sidebar);
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
        if (scene.sidebar->IndexOf(item) >= 0) {
            return scene.sidebar.get();
        }
        for (SidebarItem& row : scene.sidebar->Items()) {
            for (SidebarItem& child : row.Children()) {
                if (&child == item) {
                    return scene.sidebar.get();
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
        DirtySidebarItem(item);
        item->SetSelected(true);
        DirtySidebarItem(item);
        MarkSurfaceDirty();
        return;
    }
    const int index = owner->IndexOf(item);
    if (index >= 0) {
        const bool activity = owner->GetVariant() == SidebarVariant::Activity;
        const bool same = index == owner->Selected();
        DirtySidebarItem(item);
        owner->Select(index);
        if (EnsureSelectedRailChildren(owner)) {
            // Nested DevKit: parent Tick continues budgeted fills.
            // Desktop scene: NeedsTick stays warm via sidebar animation / dirty.
        }
        if (activity && same && sidebarVisible_) {
            ToggleSidebar();
        } else if (activity && !sidebarVisible_) {
            ToggleSidebar();
        }
        OpenSelectedChild(owner, index);
        RememberPane(item);
        EnsurePaneContent();
        DirtySidebarItem(item);
        DirtyRailArrows(*owner);
        MarkSurfaceDirty();
        return;
    }
    DirtySidebarItem(item);
    owner->SelectChild(item);
    RememberPane(item);
    EnsurePaneContent();
    DirtySidebarItem(item);
    MarkSurfaceDirty();
}

const std::string& RoboScene::ActivePaneId() {
    if (!activePaneId_.empty()) {
        return activePaneId_;
    }
    for (SceneItem& scene : items_) {
        if (scene.kind != SceneItemKind::Sidebar) {
            continue;
        }
        for (const SidebarItem& row : scene.sidebar->Items()) {
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
    EvictInactivePanes(activePaneId_);
    EnsurePaneContent();
    MarkSurfaceDirty();
}

void RoboScene::CollectButtons(std::vector<Button*>& out) {
    for (SceneItem& item : items_) {
        if (item.kind == SceneItemKind::Button) {
            out.push_back(item.button.get());
        } else if (item.kind == SceneItemKind::TitleBar) {
            out.push_back(&item.titleBar->SidebarButton());
        } else if (item.kind == SceneItemKind::Container && item.container) {
            if (!item.container->ShownWhen().empty() && item.container->ShownWhen() != ActivePaneId()) {
                continue;
            }
            CollectButtonsInContainer(*item.container, out);
        } else if (item.kind == SceneItemKind::Flexbox && item.flexbox) {
            CollectButtonsInFlexbox(*item.flexbox, out);
        } else if (item.kind == SceneItemKind::Grid && item.grid) {
            CollectButtonsInGrid(*item.grid, out);
        } else if (item.kind == SceneItemKind::Position && item.position) {
            CollectButtonsInPosition(*item.position, out);
        }
    }
}

bool RoboScene::IsSidebarToggle(const Button* button) const {
    if (!button) {
        return false;
    }
    for (const SceneItem& item : items_) {
        if (item.kind == SceneItemKind::TitleBar && &item.titleBar->SidebarButton() == button) {
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
    MarkSurfaceDirty();
}

bool RoboScene::KeyDown(const std::string& key, bool shift) {
    if (devKitOpen_ && devKitFocus_ && devKitScene_ != nullptr) {
        if (devKitScene_->KeyDown(key, shift)) {
            DirtyDevKitFromNested();
            return true;
        }
    }
    if (desktop_.HasContextMenu() && desktop_.GetContextMenu().IsVisible()) {
        ContextMenu& contextMenu = desktop_.GetContextMenu();
        if (contextMenu.KeyDown(key)) {
            const std::string action = contextMenu.ConsumeAction();
            DirtyContextMenu();
            if (!action.empty()) {
                RunContextAction(action);
            }
            return true;
        }
    }
    if (focusedInput_ && key != "Tab") {
        return false;
    }
    if (focusedSelection_ >= 0 && focusedSelection_ < static_cast<int>(items_.size())) {
        SceneItem& item = items_[static_cast<std::size_t>(focusedSelection_)];
        if (item.kind == SceneItemKind::SelectionArea && item.selectionArea.HasKeyboardFocus()) {
            if (item.selectionArea.KeyDown(key, shift, modifiers_)) {
                DirtySelection(item.selectionArea);
                return true;
            }
        }
    }
    bool selectionOwnsKeys = false;
    if (focusedSelection_ >= 0 && focusedSelection_ < static_cast<int>(items_.size())) {
        const SceneItem& item = items_[static_cast<std::size_t>(focusedSelection_)];
        selectionOwnsKeys = item.kind == SceneItemKind::SelectionArea && item.selectionArea.HasKeyboardFocus();
    }
    if (desktop_.HasTopBarMenu() && !focusedInput_ && !focusedSidebarItem_ && !selectionOwnsKeys) {
        TopBarMenu& menu = desktop_.GetTopBarMenu();
        if (key == "ArrowLeft" || key == "ArrowRight") {
            if (menu.MoveFocus(key == "ArrowRight" ? 1 : -1)) {
                DirtyMenuBar();
            }
            return true;
        }
        if ((key == "Enter" || key == " ") && menu.KeyboardFocus() && menu.Focus() >= 0) {
            menu.Select(menu.Focus());
            DirtyMenuBar();
            return true;
        }
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
        MarkSurfaceDirty();
        return true;
    }
    if (key == "Tab") {
        if (focusedInput_) {
            SetFocusedInput(nullptr);
        }
        std::vector<FocusSlot> slots;
        for (SceneItem& scene : items_) {
            if (scene.kind == SceneItemKind::Button && scene.button->CanActivate()) {
                FocusSlot slot;
                slot.button = scene.button.get();
                slots.push_back(slot);
            } else if (scene.kind == SceneItemKind::TitleBar && scene.titleBar->SidebarButton().CanActivate()) {
                FocusSlot slot;
                slot.button = &scene.titleBar->SidebarButton();
                slots.push_back(slot);
            } else if (scene.kind == SceneItemKind::Sidebar) {
                AppendSidebarSlots(*scene.sidebar, slots);
            } else if (scene.kind == SceneItemKind::SidebarItem && scene.sidebarItem->CanActivate()) {
                FocusSlot slot;
                slot.item = scene.sidebarItem.get();
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
            } else if (scene.kind == SceneItemKind::Flexbox && scene.flexbox) {
                std::vector<Button*> buttons;
                CollectButtonsInFlexbox(*scene.flexbox, buttons);
                for (Button* button : buttons) {
                    if (button->CanActivate()) {
                        FocusSlot slot;
                        slot.button = button;
                        slots.push_back(slot);
                    }
                }
                AppendPaneSlotsFlex(*scene.flexbox, slots);
            } else if (scene.kind == SceneItemKind::Grid && scene.grid) {
                std::vector<Button*> buttons;
                CollectButtonsInGrid(*scene.grid, buttons);
                for (Button* button : buttons) {
                    if (button->CanActivate()) {
                        FocusSlot slot;
                        slot.button = button;
                        slots.push_back(slot);
                    }
                }
                AppendPaneSlotsGrid(*scene.grid, slots);
            } else if (scene.kind == SceneItemKind::Position && scene.position) {
                std::vector<Button*> buttons;
                CollectButtonsInPosition(*scene.position, buttons);
                for (Button* button : buttons) {
                    if (button->CanActivate()) {
                        FocusSlot slot;
                        slot.button = button;
                        slots.push_back(slot);
                    }
                }
                AppendPaneSlotsPosition(*scene.position, slots);
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
            DirtySidebarItem(focusedSidebarItem_);
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
    if (desktop_.HasContextMenu() && desktop_.GetContextMenu().IsVisible()) {
        ContextMenu& contextMenu = desktop_.GetContextMenu();
        if (contextMenu.PointerMove(x, y)) {
            DirtyContextMenu();
        }
        if (contextMenu.Contains(x, y)) {
            return true;
        }
    }
    if (ForwardDevKitMove(x, y)) {
        return true;
    }
    if (SelectionArea* area = GesturingSelection()) {
        if (area->ClearHover()) {
            DirtySelection(*area);
        }
        if (area->PointerMove(x, y)) {
            DirtySelection(*area);
            return true;
        }
    }
    const bool folderHover = desktop_.IsEnabled() && UpdateSelectionHover(x, y);
    selectX_ = x;
    selectY_ = y;
    if (desktop_.IsEnabled()) {
        if (desktop_.HasTopBarMenu()) {
            TopBarMenu& menu = desktop_.GetTopBarMenu();
            const bool insideMenu = menu.BarHeight() > 0.0f && y >= menu.BarY() && y < menu.BarY() + menu.BarHeight();
            const int menuHover = insideMenu ? menu.Hit(x, y) : -1;
            bool barChanged = false;
            if (menuHover != hoveredMenu_) {
                hoveredMenu_ = menuHover;
                menu.SetHover(menuHover);
                DirtyMenuBar();
                barChanged = true;
            }
            if (insideMenu) {
                if (hoveredDock_ != -1) {
                    hoveredDock_ = -1;
                    if (desktop_.DockBarHeight() > 0.0f) {
                        MarkDirty(desktop_.DockBarX(), desktop_.DockBarY(), desktop_.DockBarWidth(), desktop_.DockBarHeight());
                    }
                    barChanged = true;
                }
                barChanged = SetHoveredButton(nullptr) || barChanged;
                return barChanged || folderHover;
            }
        }
        const int hover = desktop_.HitDock(x, y);
        if (hover != hoveredDock_) {
            hoveredDock_ = hover;
            if (desktop_.DockBarHeight() > 0.0f) {
                MarkDirty(desktop_.DockBarX(), desktop_.DockBarY(), desktop_.DockBarWidth(), desktop_.DockBarHeight());
            } else {
                MarkSurfaceDirty();
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
        needsRender_ = true;
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
            DirtySidebarItem(pressedSidebarItem_);
            return true;
        }
        return folderHover;
    }

    if (pressedButton_) {
        const bool inside = FindButtonAt(x, y) == pressedButton_;
        const ButtonInteraction next = inside ? ButtonInteraction::Pressed : ButtonInteraction::Normal;
        if (pressedButton_->GetInteraction() != next) {
            pressedButton_->SetInteraction(next);
            DirtyButton(pressedButton_);
            return true;
        }
        return folderHover;
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
    VisitSceneMotion(items_, ActivePaneId(), [&](MotionShape& shape) {
        if (shape.UpdatePointer(x, y)) {
            motionChanged = true;
        }
    });
    if (motionChanged) {
        MarkSurfaceDirty();
        changed = true;
    }
    if (SyncThemeHot(*this, items_, ActivePaneId(), x, y)) {
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
    return changed || folderHover;
}

bool RoboScene::LongPressSelect(float x, float y) {
    if (SelectionArea* area = SelectionAt(x, y)) {
        if (area->LongPress(x, y)) {
            DirtySelection(*area);
            return true;
        }
    }
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
    if (launcherOpen_) {
        const float row = (y - 80.0f) / 36.0f;
        if (x >= 32.0f && x <= 352.0f && row >= 0.0f && row < static_cast<float>(launcherCount_)) {
            const int index = static_cast<int>(row);
            if (launcherPick_ != nullptr) {
                launcherPick_(index);
            }
            launcherOpen_ = false;
            MarkDirty(32.0f, 72.0f, 320.0f, static_cast<float>(36 * launcherCount_ + 16));
            return true;
        }
        launcherOpen_ = false;
        MarkDirty(32.0f, 72.0f, 320.0f, static_cast<float>(36 * launcherCount_ + 16));
    }
    if (desktop_.HasContextMenu() && desktop_.GetContextMenu().IsVisible()) {
        ContextMenu& contextMenu = desktop_.GetContextMenu();
        if (contextMenu.Panel().width < 1.0f || contextGuardMs_ > 0.0f) {
            return true;
        }
    }
    if (desktop_.HasContextMenu() && desktop_.GetContextMenu().IsVisible()) {
        ContextMenu& contextMenu = desktop_.GetContextMenu();
        if (contextMenu.Contains(x, y)) {
            contextMenu.PointerDown(x, y);
            DirtyContextMenu();
            return true;
        }
        DirtyContextMenu();
        contextMenu.Close();
        DirtyContextMenu();
        if ((modifiers_ & 2) == 0) {
            return true;
        }
    }
    if ((modifiers_ & 2) != 0 && !fromTouch && PointerContext(x, y)) {
        return true;
    }
    if (devKitOpen_) {
        if (PointerOnDevKit(x, y)) {
            devKitFocus_ = true;
            ForwardDevKitPointer(x, y, true, false, fromTouch, clicks);
            return true;
        }
        devKitFocus_ = false;
    }
    if (desktop_.IsEnabled()) {
        if (desktop_.HasTopBarMenu()) {
            TopBarMenu& menu = desktop_.GetTopBarMenu();
            const bool insideMenu = menu.BarHeight() > 0.0f && y >= menu.BarY() && y < menu.BarY() + menu.BarHeight();
            if (insideMenu) {
                if (focusedSelection_ >= 0 && focusedSelection_ < static_cast<int>(items_.size())) {
                    SceneItem& item = items_[static_cast<std::size_t>(focusedSelection_)];
                    if (item.kind == SceneItemKind::SelectionArea) {
                        item.selectionArea.SetKeyboardFocus(false);
                    }
                    focusedSelection_ = -1;
                }
                const int slot = menu.Hit(x, y);
                if (slot >= 0 && menu.SlotEnabled(slot)) {
                    menu.Press(slot);
                } else {
                    menu.SetPressed(-1);
                    menu.ClearSelection();
                }
                DirtyMenuBar();
                return true;
            }
            if (menu.ClearSelection()) {
                DirtyMenuBar();
            }
        }
        const int dock = desktop_.HitDock(x, y);
        if (dock >= 0) {
            if (desktop_.SelectDock(dock)) {
                if (desktop_.DockBarHeight() > 0.0f) {
                    MarkDirty(desktop_.DockBarX(), desktop_.DockBarY(), desktop_.DockBarWidth(), desktop_.DockBarHeight());
                } else {
                    MarkSurfaceDirty();
                }
            }
            return true;
        }
    }
    if (SelectionArea* area = SelectionAt(x, y)) {
        for (SceneItem& item : items_) {
            if (item.kind == SceneItemKind::SelectionArea && &item.selectionArea != area) {
                item.selectionArea.SetKeyboardFocus(false);
            }
        }
        for (int index = 0; index < static_cast<int>(items_.size()); ++index) {
            if (&items_[static_cast<std::size_t>(index)].selectionArea == area &&
                items_[static_cast<std::size_t>(index)].kind == SceneItemKind::SelectionArea) {
                focusedSelection_ = index;
            }
        }
        area->PointerDown(x, y, fromTouch, clicks, modifiers_);
        const std::string opened = area->ConsumeOpen();
        if (opened == "DevKit") {
            OpenDevKit();
            DirtyDevKitWindowSoft();
        } else {
            DirtySelection(*area);
        }
        return true;
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
        DirtyScrollbar(scrollbar);
        if (scrollbar == &pageVertical_ || scrollbar == &pageHorizontal_) {
            ShiftPaintedScroll();
        } else {
            MarkContentDirty();
        }
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
        // SetTheme already MarkFullDirty when the theme value changes.
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
            MarkSurfaceDirty();
            changed = true;
        }
    }

    if (changed) {
        needsRender_ = true;
    }
    return changed;
}

bool RoboScene::PointerUp(float x, float y) {
    bool changed = false;
    if (ForwardDevKitPointer(x, y, false, true, false, 1)) {
        changed = true;
    }
    if (desktop_.HasContextMenu() && desktop_.GetContextMenu().IsVisible() && desktop_.GetContextMenu().Pressed() >= 0) {
        ContextMenu& contextMenu = desktop_.GetContextMenu();
        contextMenu.PointerUp(x, y);
        const std::string action = contextMenu.ConsumeAction();
        DirtyContextMenu();
        if (!action.empty()) {
            RunContextAction(action);
        }
        changed = true;
    }
    if (SelectionArea* area = GesturingSelection()) {
        area->PointerUp(x, y);
        DirtySelection(*area);
        changed = true;
    }
    if (desktop_.HasTopBarMenu() && desktop_.GetTopBarMenu().Pressed() >= 0) {
        TopBarMenu& menu = desktop_.GetTopBarMenu();
        menu.ReleasePress(menu.Hit(x, y));
        DirtyMenuBar();
        changed = true;
    }
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
    VisitSceneScrollbars(items_, ActivePaneId(), releaseArrows);
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
    if (changed) {
        needsRender_ = true;
    }
    return changed;
}

bool RoboScene::PointerLeave() {
    bool changed = false;
    if (hoveredMenu_ != -1) {
        hoveredMenu_ = -1;
        if (desktop_.HasTopBarMenu()) {
            desktop_.GetTopBarMenu().SetHover(-1);
            DirtyMenuBar();
        }
        changed = true;
    }
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
        MarkSurfaceDirty();
        changed = true;
    }
    VisitSceneMotion(items_, ActivePaneId(), [&](MotionShape& shape) {
        if (shape.UpdatePointer(-100000.0f, -100000.0f)) {
            MarkSurfaceDirty();
            changed = true;
        }
    });
    if (changed) {
        needsRender_ = true;
    }
    return changed;
}

bool RoboScene::Tick(double deltaMs) {
    if (contextGuardMs_ > 0.0f) {
        contextGuardMs_ -= static_cast<float>(deltaMs);
        if (contextGuardMs_ < 0.0f) {
            contextGuardMs_ = 0.0f;
        }
    }
    wallpaperClockMs_ += deltaMs;
    bool changed = false;
    if (devKitOpen_ && devKitScene_ != nullptr && devKitBuildStep_ >= 0) {
        PumpPaint();
#ifdef ROBO_KERNEL
        devKitBuildStep_ = RoboDevKitOpenPageStep(*devKitScene_, devKitBuildStep_);
#else
        devKitBuildStep_ = BuildDevKitPageStep(*devKitScene_, devKitBuildStep_);
#endif
        PumpPaint();
        if (devKitBuildStep_ < 0) {
            devKitRailFillPending_ = true;
        }
        DirtyDevKitWindowSoft();
        changed = true;
    } else if (devKitOpen_ && devKitScene_ != nullptr && devKitRailFillPending_) {
        // Budgeted EnsureRailChildren on the nested scene (≤8 kids/call).
        for (SceneItem& item : devKitScene_->items_) {
            if (item.kind != SceneItemKind::Sidebar || !item.sidebar) {
                continue;
            }
            const int selected = item.sidebar->Selected();
            if (selected < 0 || selected >= static_cast<int>(item.sidebar->Items().size())) {
                devKitRailFillPending_ = false;
                break;
            }
            const std::size_t before =
                item.sidebar->Items()[static_cast<std::size_t>(selected)].Children().size();
            PumpPaint();
            devKitRailFillPending_ = devKitScene_->EnsureSelectedRailChildren(item.sidebar.get());
            PumpPaint();
            const std::size_t after =
                item.sidebar->Items()[static_cast<std::size_t>(selected)].Children().size();
            if (devKitRailFillPending_ || after != before) {
                DirtyDevKitWindowSoft();
                changed = true;
            }
            break;
        }
    }
    if (desktop_.HasContextMenu() && desktop_.GetContextMenu().IsVisible()) {
        if (desktop_.GetContextMenu().Tick(static_cast<float>(deltaMs))) {
            DirtyContextMenu();
            changed = true;
        }
    }
    // Cap decode and Present together (~15 FPS) so idle does not burn JPEG decode for dropped frames.
    // While DevKit is focused, freeze wallpaper under the opaque window so idle can hlt.
    constexpr double ROBO_WALLPAPER_PRESENT_MS = 66.0;
    bool videoAdvanced = false;
    const bool freezeWallpaper = devKitOpen_ && devKitFocus_;
    if (!freezeWallpaper && desktop_.IsEnabled() && desktop_.GetBackgroundSource().IsAnimated()) {
        if (wallpaperClockMs_ >= wallpaperPresentDueMs_) {
            if (desktop_.GetBackgroundSource().Advance(deltaMs + ROBO_WALLPAPER_PRESENT_MS)) {
                videoAdvanced = true;
            }
            wallpaperPresentDueMs_ = wallpaperClockMs_ + ROBO_WALLPAPER_PRESENT_MS;
        }
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
    VisitSceneScrollbars(items_, ActivePaneId(), visit);
    if (devKitOpen_ && devKitScene_ != nullptr && devKitScene_->Tick(deltaMs)) {
        DirtyDevKitFromNested();
        changed = true;
    }
    for (SceneItem& item : items_) {
        if (item.kind == SceneItemKind::SelectionArea) {
            item.selectionArea.Advance(static_cast<float>(deltaMs));
        }
        if (item.kind == SceneItemKind::Sidebar && item.sidebar->TickRail(deltaMs)) {
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
        VisitSceneMotion(items_, ActivePaneId(), [&](MotionShape& shape) {
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
        VisitSceneThemes(items_, ActivePaneId(), themeVisit);
    }
    if (motionChanged) {
        if (themeMixing_) {
            MarkFullDirty();
        } else {
            MarkSurfaceDirty();
        }
        changed = true;
    }
    if (contentMoved) {
        ShiftPaintedScroll();
    }
    if (activePaneId_ == "icons" && galleryCount_ > galleryLimit_ && maxScrollY_ > 1.0f) {
        bool slide = false;
        if (scrollY_ > maxScrollY_ * 0.75f && galleryFirst_ + galleryLimit_ < galleryCount_) {
            galleryFirst_ += galleryLimit_;
            if (galleryFirst_ + galleryLimit_ > galleryCount_) {
                galleryFirst_ = galleryCount_ - galleryLimit_;
            }
            if (galleryFirst_ < 0) {
                galleryFirst_ = 0;
            }
            scrollY_ = maxScrollY_ * 0.2f;
            slide = true;
        } else if (scrollY_ < maxScrollY_ * 0.12f && galleryFirst_ > 0) {
            galleryFirst_ -= galleryLimit_;
            if (galleryFirst_ < 0) {
                galleryFirst_ = 0;
            }
            scrollY_ = maxScrollY_ * 0.55f;
            slide = true;
        }
        if (slide) {
            pageVertical_.SetValue(maxScrollY_ > 0.0f ? static_cast<double>(scrollY_ / maxScrollY_) : 0.0);
            paintedScrollY_ = scrollY_;
            DropPane("icons");
            EnsurePaneContent();
            MarkContentDirty();
        }
    }
    if (videoAdvanced) {
        changed = true;
    }
    const bool contextOpen = desktop_.HasContextMenu() && desktop_.GetContextMenu().IsVisible();
    const bool liveOverlay = contextOpen || sidebarAnimating_ || themeMixing_ || GesturingSelection() != nullptr ||
                             TextSelection::ShowsHandles() || TextSelection::IsDragging();
    // With DevKit open: banded wallpaper Present (no wipe of window interior).
    // Without DevKit: full PresentVideoFrame path.
    if (videoAdvanced && !contentMoved && !motionChanged && !liveOverlay && chromeReady_ && !fullDirty_ &&
        !damageOpen_) {
        if (devKitOpen_) {
            wallpaperAroundDevKit_ = true;
            videoOnly_ = false;
        } else {
            videoOnly_ = true;
            wallpaperAroundDevKit_ = false;
        }
        needsRender_ = true;
    } else if (videoAdvanced && !fullDirty_) {
        videoOnly_ = false;
        wallpaperAroundDevKit_ = false;
        MarkSurfaceDirty();
    } else if (videoAdvanced) {
        videoOnly_ = false;
        wallpaperAroundDevKit_ = false;
    }
    return changed;
}

bool RoboScene::NeedsTick() {
    wallpaperOnly_ = false;
    if (devKitOpen_ && devKitBuildStep_ >= 0) {
        return true;
    }
    if (devKitOpen_ && devKitRailFillPending_) {
        return true;
    }
    if (devKitOpen_ && devKitScene_ != nullptr && devKitScene_->NeedsTick()) {
        return true;
    }
    bool needsTick = false;
    auto visit = [&needsTick](Scrollbar& scrollbar) {
        if (scrollbar.NeedsAnimation()) {
            needsTick = true;
        }
    };
    VisitSceneScrollbars(items_, activePaneId_, visit);
    for (const SceneItem& item : items_) {
        if (item.kind == SceneItemKind::Sidebar && item.sidebar->RailNeedsTick()) {
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
        VisitSceneMotion(items_, activePaneId_, [&](MotionShape& shape) {
            if (shape.NeedsMotion()) {
                needsTick = true;
            }
        });
        auto themeVisit = [&](auto& control) {
            if (control.NeedsMotion()) {
                needsTick = true;
            }
        };
        VisitSceneThemes(items_, activePaneId_, themeVisit);
    }
    const bool wallpaper = desktop_.IsEnabled() && desktop_.GetBackgroundSource().IsAnimated() && !(devKitOpen_ && devKitFocus_);
    wallpaperOnly_ = wallpaper && !needsTick;
    if (needsTick) {
        return true;
    }
    return wallpaper && wallpaperClockMs_ >= wallpaperPresentDueMs_;
}

bool RoboScene::HasFocusedInput() const {
    if (focusedInput_ != nullptr) {
        return true;
    }
    return devKitOpen_ && devKitFocus_ && devKitScene_ != nullptr && devKitScene_->HasFocusedInput();
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
    if (devKitOpen_ && devKitFocus_ && devKitScene_ != nullptr && devKitScene_->KeyInsert(utf8)) {
        DirtyDevKitFromNested();
        return true;
    }
    if (!focusedInput_) {
        return false;
    }
    if (!focusedInput_->InsertText(utf8)) {
        return false;
    }
    DirtyInput(focusedInput_);
    return true;
}

bool RoboScene::KeyBackspace() {
    if (devKitOpen_ && devKitFocus_ && devKitScene_ != nullptr && devKitScene_->KeyBackspace()) {
        DirtyDevKitFromNested();
        return true;
    }
    if (!focusedInput_) {
        return false;
    }
    if (!focusedInput_->DeleteBackward()) {
        return false;
    }
    DirtyInput(focusedInput_);
    return true;
}

bool RoboScene::KeyDelete() {
    if (devKitOpen_ && devKitFocus_ && devKitScene_ != nullptr && devKitScene_->KeyDelete()) {
        DirtyDevKitFromNested();
        return true;
    }
    if (!focusedInput_) {
        return false;
    }
    if (!focusedInput_->DeleteForward()) {
        return false;
    }
    DirtyInput(focusedInput_);
    return true;
}

bool RoboScene::KeyLeft() {
    if (devKitOpen_ && devKitFocus_ && devKitScene_ != nullptr && devKitScene_->KeyLeft()) {
        DirtyDevKitFromNested();
        return true;
    }
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
    if (devKitOpen_ && devKitFocus_ && devKitScene_ != nullptr && devKitScene_->KeyRight()) {
        DirtyDevKitFromNested();
        return true;
    }
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
    if (devKitOpen_ && devKitFocus_ && devKitScene_ != nullptr && devKitScene_->KeyEscape()) {
        DirtyDevKitFromNested();
        return true;
    }
    if (desktop_.HasContextMenu() && desktop_.GetContextMenu().IsVisible()) {
        desktop_.GetContextMenu().KeyDown("Escape");
        DirtyContextMenu();
        return true;
    }
    if (focusedSelection_ >= 0 && focusedSelection_ < static_cast<int>(items_.size())) {
        SceneItem& item = items_[static_cast<std::size_t>(focusedSelection_)];
        if (item.kind == SceneItemKind::SelectionArea && item.selectionArea.Cancel()) {
            DirtySelection(item.selectionArea);
            return true;
        }
    }
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
