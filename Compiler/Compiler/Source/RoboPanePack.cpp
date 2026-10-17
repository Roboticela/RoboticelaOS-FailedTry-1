#include "Robo/RoboPanePack.hpp"

#include "Robo/Button.hpp"
#include "Robo/Container.hpp"
#include "Robo/Flexbox.hpp"
#include "Robo/Grid.hpp"
#include "Robo/Heading.hpp"
#include "Robo/Icon.hpp"
#include "Robo/Input.hpp"
#include "Robo/Paragraph.hpp"
#include "Robo/Position.hpp"
#include "Robo/RoboIconPack.hpp"
#include "Robo/Selectable.hpp"
#include "Robo/Text.hpp"

#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace Robo {

namespace {

enum PaneKind : std::uint16_t {
    PaneKindContainer = 1,
    PaneKindFlexbox = 2,
    PaneKindHeading = 3,
    PaneKindParagraph = 4,
    PaneKindText = 5,
    PaneKindButton = 6,
    PaneKindInput = 7,
    PaneKindIcon = 8,
    PaneKindIconButton = 9,
    PaneKindGrid = 10
};

struct PaneAttr {
    std::uint32_t key = 0;
    std::uint32_t value = 0;
};

struct PaneNode {
    std::uint16_t kind = 0;
    std::uint32_t text = 0xFFFFFFFFu;
    std::vector<PaneAttr> attrs;
    std::vector<std::uint32_t> children;
};

struct PaneDef {
    std::uint32_t id = 0;
    std::uint32_t root = 0;
    std::vector<PaneNode> nodes;
};

std::vector<std::uint8_t> gPack;
std::vector<std::string> gPool;
std::vector<PaneDef> gPanes;
bool gLoaded = false;

std::uint16_t Read16(const std::uint8_t* data, std::uint32_t size, std::uint32_t offset) {
    if (offset + 2u > size) {
        return 0;
    }
    return static_cast<std::uint16_t>(data[offset] | (static_cast<std::uint16_t>(data[offset + 1u]) << 8));
}

std::uint32_t Read32(const std::uint8_t* data, std::uint32_t size, std::uint32_t offset) {
    if (offset + 4u > size) {
        return 0;
    }
    return static_cast<std::uint32_t>(data[offset]) | (static_cast<std::uint32_t>(data[offset + 1u]) << 8) |
           (static_cast<std::uint32_t>(data[offset + 2u]) << 16) | (static_cast<std::uint32_t>(data[offset + 3u]) << 24);
}

const char* Pool(std::uint32_t index) {
    if (index == 0xFFFFFFFFu || index >= gPool.size()) {
        return "";
    }
    return gPool[index].c_str();
}

bool SameName(const char* left, const char* right) {
    if (left == nullptr || right == nullptr) {
        return false;
    }
    while (*left != '\0' && *left == *right) {
        ++left;
        ++right;
    }
    return *left == '\0' && *right == '\0';
}

const char* Attr(const PaneNode& node, const char* key) {
    for (const PaneAttr& attr : node.attrs) {
        if (SameName(Pool(attr.key), key)) {
            return Pool(attr.value);
        }
    }
    return nullptr;
}

std::string TokenOf(const char* value) {
    if (value == nullptr) {
        return {};
    }
    const std::size_t length = strlen(value);
    if (length >= 2 && value[0] == '{' && value[length - 1] == '}') {
        return std::string(value + 1, length - 2);
    }
    return {};
}

void ApplyLength(std::map<std::string, std::string>& tokens, const char* field, Length& length, bool& hasLength,
                 std::string& legacy, const char* value) {
    if (value == nullptr) {
        return;
    }
    const std::string token = TokenOf(value);
    if (!token.empty()) {
        tokens[field] = token;
        return;
    }
    legacy = value;
    if (strcmp(value, "auto") == 0 || value[0] == '\0') {
        length = Length::Auto();
    } else {
        const std::size_t lengthBytes = strlen(value);
        if (lengthBytes > 0 && value[lengthBytes - 1] == '%') {
            length = Length::Percent(static_cast<float>(std::atof(value)));
        } else {
            length = Length::Px(static_cast<float>(std::atof(value)));
        }
    }
    hasLength = true;
}

void ApplyContainerStyle(ContainerStyle& style, const PaneNode& node) {
    style.scrollbarStyle.visibility = ScrollbarVisibility::Auto;
    for (const PaneAttr& attr : node.attrs) {
        const char* key = Pool(attr.key);
        const char* value = Pool(attr.value);
        if (SameName(key, "when")) {
            continue;
        }
        if (SameName(key, "width")) {
            ApplyLength(style.tokens, "width", style.widthLength, style.hasWidthLength, style.width, value);
        } else if (SameName(key, "height")) {
            ApplyLength(style.tokens, "height", style.heightLength, style.hasHeightLength, style.height, value);
        } else if (SameName(key, "gap")) {
            const std::string token = TokenOf(value);
            if (!token.empty()) {
                style.tokens["gap"] = token;
            } else {
                style.gap = static_cast<float>(std::atof(value));
            }
        } else if (SameName(key, "padding") || SameName(key, "paddingTop") || SameName(key, "paddingBottom") ||
                   SameName(key, "paddingLeft") || SameName(key, "paddingRight")) {
            const std::string token = TokenOf(value);
            if (!token.empty()) {
                if (SameName(key, "padding")) {
                    style.tokens["paddingTop"] = token;
                    style.tokens["paddingBottom"] = token;
                    style.tokens["paddingLeft"] = token;
                    style.tokens["paddingRight"] = token;
                } else {
                    style.tokens[key] = token;
                }
            }
        } else if (SameName(key, "background") || SameName(key, "backgroundColor")) {
            const std::string token = TokenOf(value);
            if (!token.empty()) {
                style.tokens["backgroundColor"] = token;
            } else {
                style.backgroundColor = Color::FromHex(value);
            }
        } else if (SameName(key, "borderColor")) {
            const std::string token = TokenOf(value);
            if (!token.empty()) {
                style.tokens["borderColor"] = token;
            } else {
                style.borderColor = Color::FromHex(value);
            }
        } else if (SameName(key, "borderWidth")) {
            style.borderWidth = static_cast<float>(std::atof(value));
        } else if (SameName(key, "cornerRadius")) {
            const std::string token = TokenOf(value);
            if (!token.empty()) {
                style.tokens["cornerRadius"] = token;
            } else {
                style.cornerRadius = static_cast<float>(std::atof(value));
            }
        } else if (SameName(key, "showScrollbar")) {
            style.showScrollbar = !(SameName(value, "false") || SameName(value, "0") || SameName(value, "no"));
        }
    }
}

void ApplyGridStyle(GridStyle& style, const PaneNode& node) {
    for (const PaneAttr& attr : node.attrs) {
        const char* key = Pool(attr.key);
        const char* value = Pool(attr.value);
        if (SameName(key, "columnCount") || SameName(key, "columns")) {
            style.columnCount = std::atoi(value);
        } else if (SameName(key, "rowCount") || SameName(key, "rows")) {
            style.rowCount = std::atoi(value);
        } else if (SameName(key, "gap")) {
            const std::string token = TokenOf(value);
            if (!token.empty()) {
                style.tokens["gap"] = token;
            } else {
                style.gap = static_cast<float>(std::atof(value));
            }
        } else if (SameName(key, "width")) {
            ApplyLength(style.tokens, "width", style.widthLength, style.hasWidthLength, style.width, value);
        } else if (SameName(key, "height")) {
            ApplyLength(style.tokens, "height", style.heightLength, style.hasHeightLength, style.height, value);
        }
    }
}

void ApplyFlexStyle(FlexboxStyle& style, const PaneNode& node) {
    for (const PaneAttr& attr : node.attrs) {
        const char* key = Pool(attr.key);
        const char* value = Pool(attr.value);
        if (SameName(key, "direction") || SameName(key, "flexDirection")) {
            if (SameName(value, "column") || SameName(value, "col")) {
                style.direction = FlexDirection::Column;
            } else if (SameName(value, "column-reverse")) {
                style.direction = FlexDirection::ColumnReverse;
            } else if (SameName(value, "row-reverse")) {
                style.direction = FlexDirection::RowReverse;
            } else {
                style.direction = FlexDirection::Row;
            }
        } else if (SameName(key, "align") || SameName(key, "alignItems")) {
            if (SameName(value, "center") || SameName(value, "middle")) {
                style.align = FlexAlign::Center;
            } else if (SameName(value, "end") || SameName(value, "flex-end") || SameName(value, "bottom")) {
                style.align = FlexAlign::FlexEnd;
            } else if (SameName(value, "start") || SameName(value, "flex-start") || SameName(value, "top")) {
                style.align = FlexAlign::FlexStart;
            } else {
                style.align = FlexAlign::Stretch;
            }
        } else if (SameName(key, "justify") || SameName(key, "justifyContent")) {
            if (SameName(value, "center")) {
                style.justify = FlexJustify::Center;
            } else if (SameName(value, "end") || SameName(value, "flex-end")) {
                style.justify = FlexJustify::FlexEnd;
            } else if (SameName(value, "space-between")) {
                style.justify = FlexJustify::SpaceBetween;
            } else {
                style.justify = FlexJustify::FlexStart;
            }
        } else if (SameName(key, "wrap") || SameName(key, "flexWrap")) {
            if (SameName(value, "wrap")) {
                style.wrap = FlexWrap::Wrap;
            } else if (SameName(value, "wrap-reverse")) {
                style.wrap = FlexWrap::WrapReverse;
            } else {
                style.wrap = FlexWrap::NoWrap;
            }
        } else if (SameName(key, "gap")) {
            const std::string token = TokenOf(value);
            if (!token.empty()) {
                style.tokens["gap"] = token;
            } else {
                style.gap = static_cast<float>(std::atof(value));
            }
        } else if (SameName(key, "width")) {
            ApplyLength(style.tokens, "width", style.widthLength, style.hasWidthLength, style.width, value);
        } else if (SameName(key, "height")) {
            ApplyLength(style.tokens, "height", style.heightLength, style.hasHeightLength, style.height, value);
        }
    }
}

void ApplyTextStyle(TextStyle& style, const PaneNode& node) {
    for (const PaneAttr& attr : node.attrs) {
        const char* key = Pool(attr.key);
        const char* value = Pool(attr.value);
        if (SameName(key, "fontSize")) {
            const std::string token = TokenOf(value);
            if (!token.empty()) {
                style.tokens["fontSize"] = token;
            }
        } else if (SameName(key, "fontWeight")) {
            const std::string token = TokenOf(value);
            if (!token.empty()) {
                style.tokens["fontWeight"] = token;
            }
        } else if (SameName(key, "color")) {
            const std::string token = TokenOf(value);
            if (!token.empty()) {
                style.tokens["color"] = token;
            } else {
                style.color = Color::FromHex(value);
            }
        } else if (SameName(key, "selectable")) {
            if (!(SameName(value, "false") || SameName(value, "0") || SameName(value, "no"))) {
                style.selectable = SelectableMode::On;
            }
        }
    }
}

ButtonVariant ParseButtonVariantValue(const char* value) {
    if (value == nullptr) {
        return ButtonVariant::Filled;
    }
    if (SameName(value, "tinted")) {
        return ButtonVariant::Tinted;
    }
    if (SameName(value, "gray") || SameName(value, "secondary")) {
        return ButtonVariant::Gray;
    }
    if (SameName(value, "plain") || SameName(value, "ghost")) {
        return ButtonVariant::Plain;
    }
    if (SameName(value, "destructive") || SameName(value, "danger")) {
        return ButtonVariant::Destructive;
    }
    return ButtonVariant::Filled;
}

struct BuildCtx {
    const PaneDef* pane = nullptr;
};

void BuildIntoPosition(BuildCtx& ctx, Position& content, std::uint32_t index);
void BuildIntoFlex(BuildCtx& ctx, Flexbox& flex, std::uint32_t index);
void BuildIntoGrid(BuildCtx& ctx, Grid& grid, std::uint32_t index);

void BuildChildrenPosition(BuildCtx& ctx, Position& content, const PaneNode& node) {
    for (std::uint32_t child : node.children) {
        BuildIntoPosition(ctx, content, child);
    }
}

void BuildChildrenFlex(BuildCtx& ctx, Flexbox& flex, const PaneNode& node) {
    for (std::uint32_t child : node.children) {
        BuildIntoFlex(ctx, flex, child);
    }
}

void BuildChildrenGrid(BuildCtx& ctx, Grid& grid, const PaneNode& node) {
    for (std::uint32_t child : node.children) {
        BuildIntoGrid(ctx, grid, child);
    }
}

void BuildIntoPosition(BuildCtx& ctx, Position& content, std::uint32_t index) {
    if (ctx.pane == nullptr || index >= ctx.pane->nodes.size()) {
        return;
    }
    const PaneNode& node = ctx.pane->nodes[index];
    PositionItemProps props;
    switch (node.kind) {
        case PaneKindContainer: {
            Container container;
            ContainerStyle style;
            ApplyContainerStyle(style, node);
            container.SetStyle(style);
            container.RefreshVariantDefaults();
            BuildChildrenPosition(ctx, container.GetContent(), node);
            content.AddContainer(std::move(container), props);
            break;
        }
        case PaneKindFlexbox: {
            Flexbox flex;
            FlexboxStyle style;
            ApplyFlexStyle(style, node);
            flex.SetStyle(style);
            BuildChildrenFlex(ctx, flex, node);
            content.AddFlexbox(std::move(flex), props);
            break;
        }
        case PaneKindHeading: {
            int level = 1;
            if (const char* levelText = Attr(node, "level")) {
                level = std::atoi(levelText);
            }
            Heading heading(level);
            TextStyle style = heading.GetStyle();
            ApplyTextStyle(style, node);
            heading.SetStyle(style);
            heading.AddPlain(Pool(node.text));
            content.AddText(std::move(heading), props);
            break;
        }
        case PaneKindParagraph: {
            Paragraph paragraph;
            TextStyle style = paragraph.GetStyle();
            ApplyTextStyle(style, node);
            paragraph.SetStyle(style);
            paragraph.AddPlain(Pool(node.text));
            content.AddText(std::move(paragraph), props);
            break;
        }
        case PaneKindText: {
            Text text;
            TextStyle style;
            ApplyTextStyle(style, node);
            text.SetStyle(style);
            text.AddPlain(Pool(node.text));
            content.AddText(std::move(text), props);
            break;
        }
        case PaneKindButton:
        case PaneKindIconButton: {
            Button button;
            ButtonStyle style;
            if (const char* variant = Attr(node, "variant")) {
                style.variant = ParseButtonVariantValue(variant);
            }
            button.SetStyle(style);
            button.ApplyVariantDefaults();
            if (node.kind == PaneKindIconButton) {
                button.SetIconOnly(true);
            }
            if (const char* label = Attr(node, "label")) {
                button.SetLabel(label);
            } else {
                button.SetLabel(Pool(node.text));
            }
            if (const char* icon = Attr(node, "icon")) {
                button.SetIcon(RoboBakeNamedIcon(icon, 20.0f));
            }
            // Buttons as position children: wrap in a flex row of one.
            Flexbox row;
            FlexItemProps itemProps;
            row.AddButton(std::move(button), itemProps);
            content.AddFlexbox(std::move(row), props);
            break;
        }
        case PaneKindInput: {
            Input input;
            InputStyle style;
            if (const char* variant = Attr(node, "variant")) {
                if (SameName(variant, "filled")) {
                    style.variant = InputVariant::Filled;
                }
            }
            input.SetStyle(style);
            if (const char* placeholder = Attr(node, "placeholder")) {
                input.SetPlaceholder(placeholder);
            }
            content.AddInput(std::move(input), props);
            break;
        }
        case PaneKindIcon: {
            const char* name = Attr(node, "name");
            Icon icon;
            if (name != nullptr && RoboFillIconByName(icon, name)) {
                for (const PaneAttr& attr : node.attrs) {
                    const char* key = Pool(attr.key);
                    const char* value = Pool(attr.value);
                    if (SameName(key, "name")) {
                        continue;
                    }
                    const std::string token = TokenOf(value);
                    if (!token.empty()) {
                        icon.SetToken(key, token);
                    }
                }
                content.AddSvg(icon.Bake(), props);
            }
            break;
        }
        case PaneKindGrid: {
            Grid grid;
            GridStyle style;
            ApplyGridStyle(style, node);
            grid.SetStyle(style);
            BuildChildrenGrid(ctx, grid, node);
            content.AddGrid(std::move(grid), props);
            break;
        }
        default:
            break;
    }
}

void BuildIntoFlex(BuildCtx& ctx, Flexbox& flex, std::uint32_t index) {
    if (ctx.pane == nullptr || index >= ctx.pane->nodes.size()) {
        return;
    }
    const PaneNode& node = ctx.pane->nodes[index];
    FlexItemProps props;
    switch (node.kind) {
        case PaneKindContainer: {
            Container container;
            ContainerStyle style;
            ApplyContainerStyle(style, node);
            container.SetStyle(style);
            container.RefreshVariantDefaults();
            BuildChildrenPosition(ctx, container.GetContent(), node);
            flex.AddContainer(std::move(container), props);
            break;
        }
        case PaneKindFlexbox: {
            Flexbox child;
            FlexboxStyle style;
            ApplyFlexStyle(style, node);
            child.SetStyle(style);
            BuildChildrenFlex(ctx, child, node);
            flex.AddFlexbox(std::move(child), props);
            break;
        }
        case PaneKindHeading: {
            int level = 1;
            if (const char* levelText = Attr(node, "level")) {
                level = std::atoi(levelText);
            }
            Heading heading(level);
            TextStyle style = heading.GetStyle();
            ApplyTextStyle(style, node);
            heading.SetStyle(style);
            heading.AddPlain(Pool(node.text));
            flex.AddText(std::move(heading), props);
            break;
        }
        case PaneKindParagraph: {
            Paragraph paragraph;
            TextStyle style = paragraph.GetStyle();
            ApplyTextStyle(style, node);
            paragraph.SetStyle(style);
            paragraph.AddPlain(Pool(node.text));
            flex.AddText(std::move(paragraph), props);
            break;
        }
        case PaneKindText: {
            Text text;
            TextStyle style;
            ApplyTextStyle(style, node);
            text.SetStyle(style);
            text.AddPlain(Pool(node.text));
            flex.AddText(std::move(text), props);
            break;
        }
        case PaneKindButton:
        case PaneKindIconButton: {
            Button button;
            ButtonStyle style;
            if (const char* variant = Attr(node, "variant")) {
                style.variant = ParseButtonVariantValue(variant);
            }
            button.SetStyle(style);
            button.ApplyVariantDefaults();
            if (node.kind == PaneKindIconButton) {
                button.SetIconOnly(true);
            }
            if (const char* label = Attr(node, "label")) {
                button.SetLabel(label);
            } else {
                button.SetLabel(Pool(node.text));
            }
            if (const char* icon = Attr(node, "icon")) {
                button.SetIcon(RoboBakeNamedIcon(icon, 20.0f));
            }
            flex.AddButton(std::move(button), props);
            break;
        }
        case PaneKindInput: {
            Input input;
            InputStyle style;
            if (const char* variant = Attr(node, "variant")) {
                if (SameName(variant, "filled")) {
                    style.variant = InputVariant::Filled;
                }
            }
            input.SetStyle(style);
            if (const char* placeholder = Attr(node, "placeholder")) {
                input.SetPlaceholder(placeholder);
            }
            flex.AddInput(std::move(input), props);
            break;
        }
        case PaneKindIcon: {
            const char* name = Attr(node, "name");
            Icon icon;
            if (name != nullptr && RoboFillIconByName(icon, name)) {
                for (const PaneAttr& attr : node.attrs) {
                    const char* key = Pool(attr.key);
                    const char* value = Pool(attr.value);
                    if (SameName(key, "name")) {
                        continue;
                    }
                    const std::string token = TokenOf(value);
                    if (!token.empty()) {
                        icon.SetToken(key, token);
                    }
                }
                flex.AddSvg(icon.Bake(), props);
            }
            break;
        }
        case PaneKindGrid: {
            // Flexbox has no AddGrid; nest the grid in a thin container.
            Container wrap;
            ContainerStyle wrapStyle;
            wrapStyle.scrollbarStyle.visibility = ScrollbarVisibility::Auto;
            wrapStyle.showScrollbar = false;
            wrap.SetStyle(wrapStyle);
            wrap.RefreshVariantDefaults();
            Grid grid;
            GridStyle style;
            ApplyGridStyle(style, node);
            grid.SetStyle(style);
            BuildChildrenGrid(ctx, grid, node);
            PositionItemProps gridProps;
            wrap.GetContent().AddGrid(std::move(grid), gridProps);
            flex.AddContainer(std::move(wrap), props);
            break;
        }
        default:
            break;
    }
}

void BuildIntoGrid(BuildCtx& ctx, Grid& grid, std::uint32_t index) {
    if (ctx.pane == nullptr || index >= ctx.pane->nodes.size()) {
        return;
    }
    const PaneNode& node = ctx.pane->nodes[index];
    GridItemProps props;
    switch (node.kind) {
        case PaneKindContainer: {
            Container container;
            ContainerStyle style;
            ApplyContainerStyle(style, node);
            container.SetStyle(style);
            container.RefreshVariantDefaults();
            BuildChildrenPosition(ctx, container.GetContent(), node);
            grid.AddContainer(std::move(container), props);
            break;
        }
        case PaneKindFlexbox: {
            Flexbox flex;
            FlexboxStyle style;
            ApplyFlexStyle(style, node);
            flex.SetStyle(style);
            BuildChildrenFlex(ctx, flex, node);
            grid.AddFlexbox(std::move(flex), props);
            break;
        }
        case PaneKindHeading: {
            int level = 1;
            if (const char* levelText = Attr(node, "level")) {
                level = std::atoi(levelText);
            }
            Heading heading(level);
            TextStyle style = heading.GetStyle();
            ApplyTextStyle(style, node);
            heading.SetStyle(style);
            heading.AddPlain(Pool(node.text));
            grid.AddText(std::move(heading), props);
            break;
        }
        case PaneKindParagraph: {
            Paragraph paragraph;
            TextStyle style = paragraph.GetStyle();
            ApplyTextStyle(style, node);
            paragraph.SetStyle(style);
            paragraph.AddPlain(Pool(node.text));
            grid.AddText(std::move(paragraph), props);
            break;
        }
        case PaneKindText: {
            Text text;
            TextStyle style;
            ApplyTextStyle(style, node);
            text.SetStyle(style);
            text.AddPlain(Pool(node.text));
            grid.AddText(std::move(text), props);
            break;
        }
        case PaneKindButton:
        case PaneKindIconButton: {
            Button button;
            ButtonStyle style;
            if (const char* variant = Attr(node, "variant")) {
                style.variant = ParseButtonVariantValue(variant);
            }
            button.SetStyle(style);
            button.ApplyVariantDefaults();
            if (node.kind == PaneKindIconButton) {
                button.SetIconOnly(true);
            }
            if (const char* label = Attr(node, "label")) {
                button.SetLabel(label);
            } else {
                button.SetLabel(Pool(node.text));
            }
            if (const char* icon = Attr(node, "icon")) {
                button.SetIcon(RoboBakeNamedIcon(icon, 20.0f));
            }
            grid.AddButton(std::move(button), props);
            break;
        }
        case PaneKindInput: {
            Input input;
            InputStyle style;
            if (const char* variant = Attr(node, "variant")) {
                if (SameName(variant, "filled")) {
                    style.variant = InputVariant::Filled;
                }
            }
            input.SetStyle(style);
            if (const char* placeholder = Attr(node, "placeholder")) {
                input.SetPlaceholder(placeholder);
            }
            grid.AddInput(std::move(input), props);
            break;
        }
        case PaneKindIcon: {
            const char* name = Attr(node, "name");
            Icon icon;
            if (name != nullptr && RoboFillIconByName(icon, name)) {
                for (const PaneAttr& attr : node.attrs) {
                    const char* key = Pool(attr.key);
                    const char* value = Pool(attr.value);
                    if (SameName(key, "name")) {
                        continue;
                    }
                    const std::string token = TokenOf(value);
                    if (!token.empty()) {
                        icon.SetToken(key, token);
                    }
                }
                grid.AddSvg(icon.Bake(), props);
            }
            break;
        }
        case PaneKindGrid: {
            Grid child;
            GridStyle style;
            ApplyGridStyle(style, node);
            child.SetStyle(style);
            BuildChildrenGrid(ctx, child, node);
            grid.AddGrid(std::move(child), props);
            break;
        }
        default:
            break;
    }
}

bool BuildRoot(RoboScene& scene, const PaneDef& pane) {
    if (pane.root >= pane.nodes.size()) {
        return false;
    }
    const PaneNode& root = pane.nodes[pane.root];
    if (root.kind != PaneKindContainer) {
        return false;
    }
    Container container;
    ContainerStyle style;
    ApplyContainerStyle(style, root);
    container.SetStyle(style);
    container.RefreshVariantDefaults();
    if (const char* when = Attr(root, "when")) {
        container.SetShownWhen(when);
    } else {
        container.SetShownWhen(Pool(pane.id));
    }
    BuildCtx ctx;
    ctx.pane = &pane;
    BuildChildrenPosition(ctx, container.GetContent(), root);
    scene.AddContainer(std::move(container));
    return true;
}

}  // namespace

void RoboPaneAdd(const std::uint8_t* data, std::uint32_t size) {
    if (data == nullptr || size < 12u) {
        return;
    }
    if (data[0] != 'R' || data[1] != 'P' || data[2] != 'A' || data[3] != 'N' || data[4] != '1') {
        return;
    }
    gPack.assign(data, data + size);
    gPool.clear();
    gPanes.clear();
    const std::uint32_t poolCount = Read32(data, size, 8u);
    if (poolCount > 100000u) {
        return;
    }
    std::uint32_t cursor = 12u;
    gPool.reserve(poolCount);
    for (std::uint32_t index = 0; index < poolCount; ++index) {
        if (cursor + 2u > size) {
            return;
        }
        const std::uint16_t length = Read16(data, size, cursor);
        cursor += 2u;
        if (cursor + length > size) {
            return;
        }
        std::string entry;
        if (length > 0) {
            entry.assign(reinterpret_cast<const char*>(data + cursor), length);
            while (!entry.empty() && entry.back() == '\0') {
                entry.pop_back();
            }
        }
        cursor += length;
        gPool.push_back(std::move(entry));
    }
    if (cursor + 4u > size) {
        return;
    }
    const std::uint32_t paneCount = Read32(data, size, cursor);
    cursor += 4u;
    if (paneCount > 1000u) {
        return;
    }
    for (std::uint32_t paneIndex = 0; paneIndex < paneCount; ++paneIndex) {
        if (cursor + 12u > size) {
            break;
        }
        PaneDef pane;
        pane.id = Read32(data, size, cursor);
        pane.root = Read32(data, size, cursor + 4u);
        const std::uint32_t nodeCount = Read32(data, size, cursor + 8u);
        cursor += 12u;
        if (nodeCount > 100000u) {
            break;
        }
        pane.nodes.reserve(nodeCount);
        for (std::uint32_t nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
            if (cursor + 12u > size) {
                pane.nodes.clear();
                break;
            }
            PaneNode node;
            node.kind = Read16(data, size, cursor);
            const std::uint16_t attrCount = Read16(data, size, cursor + 2u);
            const std::uint16_t childCount = Read16(data, size, cursor + 4u);
            cursor += 8u;
            node.text = Read32(data, size, cursor);
            cursor += 4u;
            node.attrs.reserve(attrCount);
            for (std::uint16_t attrIndex = 0; attrIndex < attrCount; ++attrIndex) {
                if (cursor + 8u > size) {
                    break;
                }
                PaneAttr attr;
                attr.key = Read32(data, size, cursor);
                attr.value = Read32(data, size, cursor + 4u);
                cursor += 8u;
                node.attrs.push_back(attr);
            }
            node.children.reserve(childCount);
            for (std::uint16_t childIndex = 0; childIndex < childCount; ++childIndex) {
                if (cursor + 4u > size) {
                    break;
                }
                node.children.push_back(Read32(data, size, cursor));
                cursor += 4u;
            }
            pane.nodes.push_back(std::move(node));
        }
        if (!pane.nodes.empty()) {
            gPanes.push_back(std::move(pane));
        }
    }
}

void RoboPaneLoad(const IPlatform& platform) {
    if (gLoaded) {
        return;
    }
    gLoaded = true;
    const char* paths[] = {"ROBOPANE.PAK", "Assets/ROBOPANE.PAK"};
    for (const char* path : paths) {
        const std::vector<std::uint8_t> bytes = platform.LoadAsset(path);
        if (!bytes.empty()) {
            RoboPaneAdd(bytes.data(), static_cast<std::uint32_t>(bytes.size()));
            return;
        }
    }
}

bool RoboPaneContains(const char* paneId) {
    if (paneId == nullptr) {
        return false;
    }
    for (const PaneDef& pane : gPanes) {
        if (SameName(Pool(pane.id), paneId)) {
            return true;
        }
    }
    return false;
}

bool RoboPaneBuild(RoboScene& scene, const char* paneId) {
    if (paneId == nullptr || paneId[0] == '\0' || scene.HasPaneContainer(paneId)) {
        return false;
    }
    for (const PaneDef& pane : gPanes) {
        if (SameName(Pool(pane.id), paneId)) {
            return BuildRoot(scene, pane);
        }
    }
    return false;
}

}  // namespace Robo
