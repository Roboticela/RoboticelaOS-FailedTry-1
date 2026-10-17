#include "Robo/RoboPageTables.hpp"

#include "Robo/Color.hpp"
#include "Robo/Container.hpp"
#include "Robo/Flexbox.hpp"
#include "Robo/FontStyle.hpp"
#include "Robo/Foundation.hpp"
#include "Robo/Heading.hpp"
#include "Robo/Paragraph.hpp"
#include "Robo/Selectable.hpp"
#include "Robo/Text.hpp"
#include "Robo/TextAlign.hpp"

#include <vector>

namespace Robo {

void RoboFillColorSections(Position& content, const RoboColorSection* sections, int count) {
    if (sections == nullptr || count <= 0) {
        return;
    }
    for (int sectionIndex = 0; sectionIndex < count; ++sectionIndex) {
        const RoboColorSection& section = sections[sectionIndex];
        if (section.heading != nullptr && section.heading[0] != '\0') {
            PositionItemProps props;
            Heading heading(2);
            heading.AddPlain(section.heading);
            content.AddText(std::move(heading), props);
        }
        if (section.rows == nullptr) {
            continue;
        }
        for (int rowIndex = 0; rowIndex < section.count; ++rowIndex) {
            const RoboColorSwatch& row = section.rows[rowIndex];
            PositionItemProps props;
            Flexbox rowFlex;
            FlexboxStyle style;
            style.align = FlexAlign::Center;
            style.tokens["gap"] = "space.3";
            rowFlex.SetStyle(style);
            {
                FlexItemProps itemProps;
                Container swatch;
                ContainerStyle swatchStyle;
                swatchStyle.scrollbarStyle.visibility = ScrollbarVisibility::Auto;
                if (row.token != nullptr) {
                    swatchStyle.tokens["backgroundColor"] = row.token;
                }
                if (row.borderToken != nullptr) {
                    swatchStyle.tokens["borderColor"] = row.borderToken;
                }
                swatchStyle.borderWidth = 1.0f;
                swatchStyle.tokens["cornerRadius"] = "radius.md";
                swatchStyle.tokens["height"] = "space.7";
                swatchStyle.tokens["width"] = "space.7";
                swatchStyle.showScrollbar = false;
                swatch.SetStyle(swatchStyle);
                swatch.RefreshVariantDefaults();
                rowFlex.AddContainer(std::move(swatch), itemProps);
            }
            {
                FlexItemProps itemProps;
                Text text;
                TextStyle textStyle;
                textStyle.tokens["color"] = "color.text.primary";
                textStyle.tokens["fontSize"] = "type.body.size";
                text.SetStyle(textStyle);
                text.AddPlain(row.label != nullptr ? row.label : "");
                rowFlex.AddText(std::move(text), itemProps);
            }
            {
                FlexItemProps itemProps;
                Text text;
                TextStyle textStyle;
                textStyle.tokens["color"] = "color.text.secondary";
                textStyle.tokens["fontSize"] = "type.caption.size";
                text.SetStyle(textStyle);
                text.AddPlain(row.hex != nullptr ? row.hex : "");
                rowFlex.AddText(std::move(text), itemProps);
            }
            content.AddFlexbox(std::move(rowFlex), props);
        }
    }
}

void RoboFillTypeSamples(Position& content, const RoboTypeSample* rows, int count) {
    if (rows == nullptr || count <= 0) {
        return;
    }
    for (int index = 0; index < count; ++index) {
        const RoboTypeSample& row = rows[index];
        PositionItemProps props;
        Text text;
        TextStyle style;
        if (row.fontSize != nullptr && row.fontSize[0] != '\0') {
            style.tokens["fontSize"] = row.fontSize;
        }
        if (row.fontWeight != nullptr && row.fontWeight[0] != '\0') {
            style.tokens["fontWeight"] = row.fontWeight;
        }
        if (row.color != nullptr && row.color[0] != '\0') {
            style.tokens["color"] = row.color;
        }
        text.SetStyle(style);
        text.AddPlain(row.label != nullptr ? row.label : "");
        content.AddText(std::move(text), props);
    }
}

void RoboFillSpaceRows(Position& content, const RoboSpaceRow* rows, int count) {
    if (rows == nullptr || count <= 0) {
        return;
    }
    for (int index = 0; index < count; ++index) {
        const RoboSpaceRow& row = rows[index];
        PositionItemProps props;
        Flexbox rowFlex;
        FlexboxStyle style;
        style.align = FlexAlign::Center;
        if (row.gap != nullptr) {
            style.tokens["gap"] = row.gap;
        }
        rowFlex.SetStyle(style);
        for (int mark = 0; mark < 2; ++mark) {
            FlexItemProps itemProps;
            Container swatch;
            ContainerStyle swatchStyle;
            swatchStyle.scrollbarStyle.visibility = ScrollbarVisibility::Auto;
            swatchStyle.tokens["width"] = "space.6";
            swatchStyle.tokens["height"] = "space.6";
            swatchStyle.tokens["backgroundColor"] = "color.brand.primary";
            swatchStyle.tokens["cornerRadius"] = "radius.sm";
            swatchStyle.showScrollbar = false;
            swatch.SetStyle(swatchStyle);
            swatch.RefreshVariantDefaults();
            rowFlex.AddContainer(std::move(swatch), itemProps);
        }
        {
            FlexItemProps itemProps;
            Text text;
            TextStyle textStyle;
            textStyle.tokens["fontSize"] = "type.body.size";
            textStyle.tokens["color"] = "color.text.primary";
            text.SetStyle(textStyle);
            text.AddPlain(row.label != nullptr ? row.label : "");
            rowFlex.AddText(std::move(text), itemProps);
        }
        content.AddFlexbox(std::move(rowFlex), props);
    }
}

void RoboFillRadiusSamples(Position& content, const RoboRadiusSample* rows, int count) {
    if (rows == nullptr || count <= 0) {
        return;
    }
    PositionItemProps props;
    Flexbox wrap;
    FlexboxStyle wrapStyle;
    wrapStyle.align = FlexAlign::FlexStart;
    wrapStyle.wrap = FlexWrap::Wrap;
    wrapStyle.tokens["gap"] = "space.4";
    wrapStyle.tokens["width"] = "";
    wrapStyle.width = "100%";
    wrapStyle.widthLength = Length::Percent(100.0f);
    wrapStyle.hasWidthLength = true;
    wrap.SetStyle(wrapStyle);
    for (int index = 0; index < count; ++index) {
        const RoboRadiusSample& row = rows[index];
        FlexItemProps columnProps;
        Flexbox column;
        FlexboxStyle columnStyle;
        columnStyle.direction = FlexDirection::Column;
        columnStyle.align = FlexAlign::Center;
        columnStyle.tokens["gap"] = "space.2";
        columnStyle.tokens["width"] = "space.10";
        column.SetStyle(columnStyle);
        {
            FlexItemProps itemProps;
            Container swatch;
            ContainerStyle swatchStyle;
            swatchStyle.scrollbarStyle.visibility = ScrollbarVisibility::Auto;
            swatchStyle.tokens["width"] = "space.8";
            swatchStyle.tokens["height"] = "space.8";
            swatchStyle.tokens["backgroundColor"] = "color.brand.primary.subtle";
            if (row.radius != nullptr) {
                swatchStyle.tokens["cornerRadius"] = row.radius;
            }
            swatchStyle.borderWidth = 1.0f;
            swatchStyle.tokens["borderColor"] = "color.brand.primary";
            swatchStyle.showScrollbar = false;
            swatch.SetStyle(swatchStyle);
            swatch.RefreshVariantDefaults();
            column.AddContainer(std::move(swatch), itemProps);
        }
        {
            FlexItemProps itemProps;
            Text text;
            TextStyle textStyle;
            textStyle.tokens["fontSize"] = "type.caption.size";
            textStyle.tokens["color"] = "color.text.secondary";
            text.SetStyle(textStyle);
            text.AddPlain(row.label != nullptr ? row.label : "");
            column.AddText(std::move(text), itemProps);
        }
        wrap.AddFlexbox(std::move(column), columnProps);
    }
    content.AddFlexbox(std::move(wrap), props);
}

void RoboAddMenuLabels(TopBarMenu& menu, const char* const* labels, int count) {
    if (labels == nullptr || count <= 0) {
        return;
    }
    for (int index = 0; index < count; ++index) {
        TopBarMenuItem item;
        item.SetLabel(labels[index] != nullptr ? labels[index] : "");
        menu.AddItem(std::move(item));
    }
}

void RoboFillTextBlocks(Position& content, const RoboTextBlock* rows, int count) {
    if (rows == nullptr || count <= 0) {
        return;
    }
    for (int index = 0; index < count; ++index) {
        const RoboTextBlock& row = rows[index];
        PositionItemProps props;
        if (row.headingLevel > 0) {
            Heading heading(row.headingLevel);
            heading.AddPlain(row.text != nullptr ? row.text : "");
            content.AddText(std::move(heading), props);
            continue;
        }
        Paragraph paragraph;
        if (row.selectable) {
            TextStyle style = paragraph.GetStyle();
            style.selectable = SelectableMode::On;
            paragraph.SetStyle(style);
        }
        paragraph.AddPlain(row.text != nullptr ? row.text : "");
        content.AddText(std::move(paragraph), props);
    }
}

void RoboFillContextMenu(ContextMenu& menu, const RoboMenuEntry* rows, int count) {
    if (rows == nullptr || count <= 0) {
        return;
    }
    std::vector<ContextMenuItem> built(static_cast<std::size_t>(count));
    std::vector<char> attached(static_cast<std::size_t>(count), 0);
    for (int index = 0; index < count; ++index) {
        const RoboMenuEntry& row = rows[index];
        ContextMenuItem& item = built[static_cast<std::size_t>(index)];
        if ((row.flags & RoboMenuSeparator) != 0u) {
            item.SetSeparator(true);
            continue;
        }
        if (row.label != nullptr) {
            item.SetLabel(row.label);
        }
        if ((row.flags & RoboMenuDisabled) != 0u) {
            item.SetDisabled(true);
        }
        if ((row.flags & RoboMenuCheckable) != 0u || (row.flags & RoboMenuChecked) != 0u) {
            item.SetCheckable(true);
            item.SetCheckName("Check");
        }
        if ((row.flags & RoboMenuChecked) != 0u) {
            item.SetChecked(true);
        }
        if (row.icon != nullptr && row.icon[0] != '\0') {
            item.SetIconName(row.icon);
        }
        if ((row.flags & RoboMenuSubmenu) != 0u) {
            item.SetChevronName("Chevron");
        }
        if (row.shortcut != nullptr && row.shortcut[0] != '\0') {
            item.SetShortcutSpec(row.shortcut);
        }
    }
    for (int index = count - 1; index >= 0; --index) {
        const int parent = rows[index].parent;
        if (parent < 0 || parent >= count || parent == index) {
            continue;
        }
        built[static_cast<std::size_t>(parent)].AddItem(std::move(built[static_cast<std::size_t>(index)]));
        attached[static_cast<std::size_t>(index)] = 1;
    }
    for (int index = 0; index < count; ++index) {
        if (attached[static_cast<std::size_t>(index)] || rows[index].parent >= 0) {
            continue;
        }
        menu.AddItem(std::move(built[static_cast<std::size_t>(index)]));
    }
}

void RoboApplyDesktopChrome(RoboScene& scene) {
    scene.SetPagePadding(0.0f);
    Foundation foundationDesktop = Foundation::CopyOf("RoboFoundation");
    foundationDesktop.SetName("Desktop");
    Foundation::Register(foundationDesktop);
    scene.SetFoundation("Desktop");
    scene.EnableDesktop();
    scene.SetDesktopBackground(Color::FromHex("#202838"), "color.bg.primary");
    scene.SetDesktopBackgroundPath("Assets/Background.mp4");
}

void RoboApplySelectionLabelDefaults(SelectionArea& area) {
    SelectionLabelStyle labelStyle = area.GetLabelStyle();
    labelStyle.fontSize = 12.0f;
    labelStyle.fontWeight = FontWeight::Regular;
    labelStyle.color = Color::FromHex("#FFFFFF");
    labelStyle.align = TextAlign::Center;
    labelStyle.lineHeight = 1.25f;
    labelStyle.letterSpacing = 0.0f;
    labelStyle.wrap = true;
    labelStyle.maxWidth = 100.0f;
    labelStyle.maxLines = 2;
    labelStyle.shadowX = 0.0f;
    labelStyle.shadowY = 1.0f;
    labelStyle.shadowBlur = 2.0f;
    labelStyle.shadowColor = Color::FromHex("#000000D9");
    labelStyle.iconGap = 5.0f;
    labelStyle.selectionRadius = 4.0f;
    labelStyle.selectionColor = Color::FromHex("#007AFF");
    area.SetLabelStyle(labelStyle);
}

}  // namespace Robo
