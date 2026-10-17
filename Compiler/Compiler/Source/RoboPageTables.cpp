#include "Robo/RoboPageTables.hpp"

#include "Robo/Container.hpp"
#include "Robo/Flexbox.hpp"
#include "Robo/Heading.hpp"
#include "Robo/Text.hpp"

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

}  // namespace Robo
