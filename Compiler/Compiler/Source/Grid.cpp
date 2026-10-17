#include "Robo/RoboPool.hpp"
#include "Robo/Grid.hpp"
#include "Robo/Container.hpp"
#include "Robo/Flexbox.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <sstream>
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

GridTrack ParseSingleTrack(std::string token) {
    token = Trim(ToLower(token));
    GridTrack track;
    if (token.empty() || token == "auto") {
        track.type = GridTrackType::Auto;
        track.value = 1.0f;
        return track;
    }

    if (token.size() > 2 && token.compare(token.size() - 2, 2, "fr") == 0) {
        track.type = GridTrackType::Fraction;
        const std::string numStr = token.substr(0, token.size() - 2);
        track.value = numStr.empty() ? 1.0f : static_cast<float>(std::atof(numStr.c_str()));
        if (track.value <= 0.0f) {
            track.value = 1.0f;
        }
        return track;
    }

    if (!token.empty() && token.back() == '%') {
        track.type = GridTrackType::Percent;
        track.value = static_cast<float>(std::atof(token.substr(0, token.size() - 1).c_str()));
        return track;
    }

    if (token.size() > 2 && token.compare(token.size() - 2, 2, "px") == 0) {
        token = token.substr(0, token.size() - 2);
    }

    track.type = GridTrackType::Pixel;
    track.value = static_cast<float>(std::atof(token.c_str()));
    return track;
}

}  // namespace

bool GridBounds::Contains(float pointX, float pointY) const {
    return pointX >= x && pointX <= x + width && pointY >= y && pointY <= y + height;
}

Grid::Grid() = default;

void Grid::SetStyle(const GridStyle& style) {
    style_ = style;
    InvalidateLayoutCache();
}

const GridStyle& Grid::GetStyle() const {
    return style_;
}

GridStyle& Grid::GetStyle() {
    return style_;
}

void Grid::AddText(Text text, const GridItemProps& props) {
    GridItem item;
    item.kind = GridItemKind::Text;
    item.props = props;
    item.text = RoboMake<Text>(std::move(text));
    items_.push_back(std::move(item));
}

void Grid::AddSvg(Svg svg, const GridItemProps& props) {
    GridItem item;
    item.kind = GridItemKind::Svg;
    item.props = props;
    item.svg = RoboMake<Svg>(std::move(svg));
    items_.push_back(std::move(item));
}

void Grid::AddBrandLogo(BrandLogo logo, const GridItemProps& props) {
    GridItem item;
    item.kind = GridItemKind::BrandLogo;
    item.props = props;
    item.brandLogo = RoboMake<BrandLogo>(std::move(logo));
    items_.push_back(std::move(item));
}

void Grid::AddTitleBar(TitleBar bar, const GridItemProps& props) {
    GridItem item;
    item.kind = GridItemKind::TitleBar;
    item.props = props;
    item.titleBar = RoboMake<TitleBar>(std::move(bar));
    items_.push_back(std::move(item));
}

void Grid::AddSidebar(Sidebar pane, const GridItemProps& props) {
    GridItem item;
    item.kind = GridItemKind::Sidebar;
    item.props = props;
    item.sidebar = RoboMake<Sidebar>(std::move(pane));
    items_.push_back(std::move(item));
}

void Grid::AddSidebarItem(SidebarItem item, const GridItemProps& props) {
    GridItem entry;
    entry.kind = GridItemKind::SidebarItem;
    entry.props = props;
    entry.sidebarItem = RoboMake<SidebarItem>(std::move(item));
    items_.push_back(std::move(entry));
}

void Grid::AddThemeToggle(ThemeToggle toggle, const GridItemProps& props) {
    GridItem item;
    item.kind = GridItemKind::ThemeToggle;
    item.props = props;
    item.themeToggle = RoboMake<ThemeToggle>(std::move(toggle));
    items_.push_back(std::move(item));
}

void Grid::AddThemeIconToggle(ThemeIconToggle toggle, const GridItemProps& props) {
    GridItem item;
    item.kind = GridItemKind::ThemeIconToggle;
    item.props = props;
    item.themeIconToggle = RoboMake<ThemeIconToggle>(std::move(toggle));
    items_.push_back(std::move(item));
}

void Grid::AddSeparator(Separator separator, const GridItemProps& props) {
    GridItem item;
    item.kind = GridItemKind::Separator;
    item.props = props;
    item.separator = RoboMake<Separator>(std::move(separator));
    items_.push_back(std::move(item));
}

void Grid::AddMotionShape(MotionShape shape, const GridItemProps& props) {
    GridItem item;
    item.kind = GridItemKind::MotionShape;
    item.props = props;
    item.motionShape = RoboMake<MotionShape>(std::move(shape));
    items_.push_back(std::move(item));
}

void Grid::AddButton(Button button, const GridItemProps& props) {
    GridItem item;
    item.kind = GridItemKind::Button;
    item.props = props;
    item.button = RoboMake<Button>(std::move(button));
    items_.push_back(std::move(item));
}

void Grid::AddInput(Input input, const GridItemProps& props) {
    GridItem item;
    item.kind = GridItemKind::Input;
    item.props = props;
    item.input = RoboMake<Input>(std::move(input));
    items_.push_back(std::move(item));
}

void Grid::AddScrollbar(Scrollbar scrollbar, const GridItemProps& props) {
    GridItem item;
    item.kind = GridItemKind::Scrollbar;
    item.props = props;
    item.scrollbar = RoboMake<Scrollbar>(std::move(scrollbar));
    items_.push_back(std::move(item));
}

void Grid::AddFlexbox(Flexbox flexbox, const GridItemProps& props) {
    GridItem item;
    item.kind = GridItemKind::Flexbox;
    item.props = props;
    item.flexbox = RoboMake<Flexbox>(std::move(flexbox));
    items_.push_back(std::move(item));
}

void Grid::AddGrid(Grid grid, const GridItemProps& props) {
    GridItem item;
    item.kind = GridItemKind::Grid;
    item.props = props;
    item.grid = RoboMake<Grid>(std::move(grid));
    items_.push_back(std::move(item));
}

void Grid::AddContainer(Container container, const GridItemProps& props) {
    GridItem item;
    item.kind = GridItemKind::Container;
    item.props = props;
    item.container = RoboMake<Container>(std::move(container));
    items_.push_back(std::move(item));
}

void Grid::AddItem(GridItem item) {
    items_.push_back(std::move(item));
    InvalidateLayoutCache();
}

const std::vector<GridItem>& Grid::GetItems() const {
    return items_;
}

std::vector<GridItem>& Grid::GetItems() {
    return items_;
}

void Grid::SetBounds(const GridBounds& bounds) {
    bounds_ = bounds;
}

const GridBounds& Grid::GetBounds() const {
    return bounds_;
}

float Grid::ResolveGapX(float pixelScale) const {
    if (style_.columnGap > 0.0f) {
        return style_.columnGap * pixelScale;
    }
    return style_.gap * pixelScale;
}

float Grid::ResolveGapY(float pixelScale) const {
    if (style_.rowGap > 0.0f) {
        return style_.rowGap * pixelScale;
    }
    return style_.gap * pixelScale;
}

bool Grid::HasLayoutCache(float containerWidth, float pixelScale, float forcedWidth,
                          float forcedHeight) const {
    return layoutCache_.valid && !needsLayout_ &&
           std::fabs(layoutCache_.key.containerWidth - containerWidth) < 0.5f &&
           std::fabs(layoutCache_.key.pixelScale - pixelScale) < 0.001f &&
           std::fabs(layoutCache_.key.forcedWidth - forcedWidth) < 0.5f &&
           std::fabs(layoutCache_.key.forcedHeight - forcedHeight) < 0.5f;
}

const ComputedGridLayout& Grid::LayoutCache() const {
    return layoutCache_.layout;
}

void Grid::StoreLayoutCache(float containerWidth, float pixelScale, float forcedWidth,
                            float forcedHeight, ComputedGridLayout layout) const {
    layoutCache_.layout = std::move(layout);
    layoutCache_.key.containerWidth = containerWidth;
    layoutCache_.key.pixelScale = pixelScale;
    layoutCache_.key.forcedWidth = forcedWidth;
    layoutCache_.key.forcedHeight = forcedHeight;
    layoutCache_.valid = true;
    needsLayout_ = false;
}

void Grid::InvalidateLayoutCache() const {
    layoutCache_.valid = false;
    layoutCache_.layout = ComputedGridLayout{};
    needsLayout_ = true;
    needsPaint_ = true;
}

bool Grid::NeedsLayout() const {
    return needsLayout_ || !layoutCache_.valid;
}

void Grid::SetNeedsLayout(bool needs) const {
    needsLayout_ = needs;
    if (needs) {
        layoutCache_.valid = false;
        needsPaint_ = true;
    }
}

bool Grid::NeedsPaint() const {
    return needsPaint_;
}

void Grid::SetNeedsPaint(bool needs) const {
    needsPaint_ = needs;
}

std::vector<GridTrack> ParseGridTracks(const std::string& value) {
    std::string s = Trim(value);
    if (s.empty()) {
        return {};
    }

    bool isAllDigits = true;
    for (char c : s) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            isAllDigits = false;
            break;
        }
    }
    if (isAllDigits) {
        const int count = std::atoi(s.c_str());
        if (count > 0) {
            std::vector<GridTrack> result;
            result.reserve(static_cast<std::size_t>(count));
            for (int i = 0; i < count; ++i) {
                result.push_back({GridTrackType::Fraction, 1.0f});
            }
            return result;
        }
    }

    std::vector<GridTrack> tracks;
    std::size_t pos = 0;
    while (pos < s.size()) {
        while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t' || s[pos] == ',')) {
            ++pos;
        }
        if (pos >= s.size()) {
            break;
        }

        if (s.compare(pos, 7, "repeat(") == 0 || s.compare(pos, 7, "REPEAT(") == 0) {
            const std::size_t closeParen = s.find(')', pos + 7);
            if (closeParen != std::string::npos) {
                const std::string inner = s.substr(pos + 7, closeParen - (pos + 7));
                const std::size_t comma = inner.find(',');
                if (comma != std::string::npos) {
                    const int repeatCount = std::atoi(Trim(inner.substr(0, comma)).c_str());
                    const GridTrack repeatedTrack = ParseSingleTrack(inner.substr(comma + 1));
                    if (repeatCount > 0) {
                        for (int r = 0; r < repeatCount; ++r) {
                            tracks.push_back(repeatedTrack);
                        }
                    }
                }
                pos = closeParen + 1;
                continue;
            }
        }

        std::size_t nextSpace = pos;
        while (nextSpace < s.size() && s[nextSpace] != ' ' && s[nextSpace] != '\t' && s[nextSpace] != ',') {
            ++nextSpace;
        }
        const std::string token = s.substr(pos, nextSpace - pos);
        tracks.push_back(ParseSingleTrack(token));
        pos = nextSpace;
    }

    return tracks;
}

GridAutoFlow ParseGridAutoFlow(const std::string& value) {
    const std::string lower = ToLower(Trim(value));
    if (lower == "column" || lower == "col") {
        return GridAutoFlow::Column;
    }
    return GridAutoFlow::Row;
}

GridAlignment ParseGridAlignment(const std::string& value) {
    const std::string lower = ToLower(Trim(value));
    if (lower == "start" || lower == "left" || lower == "top" || lower == "flex-start") {
        return GridAlignment::Start;
    }
    if (lower == "center" || lower == "middle") {
        return GridAlignment::Center;
    }
    if (lower == "end" || lower == "right" || lower == "bottom" || lower == "flex-end") {
        return GridAlignment::End;
    }
    if (lower == "auto") {
        return GridAlignment::Auto;
    }
    return GridAlignment::Stretch;
}

GridJustifyContent ParseGridJustifyContent(const std::string& value) {
    const std::string lower = ToLower(Trim(value));
    if (lower == "center") {
        return GridJustifyContent::Center;
    }
    if (lower == "end" || lower == "flex-end") {
        return GridJustifyContent::End;
    }
    if (lower == "space-between" || lower == "between") {
        return GridJustifyContent::SpaceBetween;
    }
    if (lower == "space-around" || lower == "around") {
        return GridJustifyContent::SpaceAround;
    }
    if (lower == "space-evenly" || lower == "evenly") {
        return GridJustifyContent::SpaceEvenly;
    }
    return GridJustifyContent::Start;
}

}  // namespace Robo
