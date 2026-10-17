#include "XmlToCpp.hpp"
#include "IconPack.hpp"
#include "XmlParser.hpp"

#include "Robo/Foundation.hpp"

#include <cctype>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace Robo {

namespace {

bool IsPaneItem(const std::string& name) {
    return name == "SidebarItem" || name == "PaneItem" || name == "PaneItems";
}

bool IsWhitespaceText(const std::string& text) {
    for (char ch : text) {
        if (ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r') {
            return false;
        }
    }
    return true;
}

std::string SanitizePaneId(const std::string& id) {
    std::string out;
    out.reserve(id.size());
    for (char ch : id) {
        if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')) {
            out.push_back(ch);
        } else {
            out.push_back('_');
        }
    }
    if (out.empty()) {
        out = "pane";
    }
    if (out[0] >= '0' && out[0] <= '9') {
        out.insert(out.begin(), '_');
    }
    return out;
}

struct CatalogEntry {
    std::string id;
    std::string label;
    std::string icon;
    int parent = -1;
    bool section = false;
    bool selected = false;
};

void CollectCatalogEntries(const XmlNode& node, int parent, std::vector<CatalogEntry>& entries) {
    for (const XmlNode& child : node.children) {
        if (!IsPaneItem(child.name)) {
            continue;
        }
        CatalogEntry entry;
        entry.parent = parent;
        if (child.attributes.count("id") != 0) {
            entry.id = child.attributes.at("id");
        }
        if (child.attributes.count("label") != 0) {
            entry.label = child.attributes.at("label");
        } else {
            for (const XmlNode& textChild : child.children) {
                if (textChild.isText && !IsWhitespaceText(textChild.text)) {
                    entry.label += textChild.text;
                }
            }
        }
        if (child.attributes.count("icon") != 0) {
            entry.icon = child.attributes.at("icon");
        }
        const auto section = child.attributes.find("section");
        const auto variant = child.attributes.find("variant");
        entry.section = (section != child.attributes.end() &&
                         (section->second == "true" || section->second == "1" || section->second == "section")) ||
                        (variant != child.attributes.end() && variant->second == "section");
        const auto selected = child.attributes.find("selected");
        entry.selected = selected != child.attributes.end() &&
                         (selected->second == "true" || selected->second == "1");
        const int index = static_cast<int>(entries.size());
        entries.push_back(entry);
        CollectCatalogEntries(child, index, entries);
    }
}

bool IsSelectionAreaName(const std::string& name) {
    return name == "SelectionArea" || name == "MarqueeSelection" || name == "RubberBandSelection" ||
           name == "DragBoxSelection";
}

bool IsSelectionItemName(const std::string& name) {
    return name == "SelectionAreaItem" || name == "MarqueeSelectionItem" || name == "RubberBandSelectionItem" ||
           name == "DragBoxSelectionItem";
}

const char* MapFlexAlign(const std::string& value) {
    if (value == "flex-start" || value == "start" || value == "top") {
        return "Robo::FlexAlign::FlexStart";
    }
    if (value == "center" || value == "middle") {
        return "Robo::FlexAlign::Center";
    }
    if (value == "flex-end" || value == "end" || value == "bottom") {
        return "Robo::FlexAlign::FlexEnd";
    }
    if (value == "auto") {
        return "Robo::FlexAlign::Auto";
    }
    if (value == "stretch") {
        return "Robo::FlexAlign::Stretch";
    }
    return nullptr;
}

const char* MapFlexDirection(const std::string& value) {
    if (value == "column" || value == "col") {
        return "Robo::FlexDirection::Column";
    }
    if (value == "column-reverse" || value == "col-reverse") {
        return "Robo::FlexDirection::ColumnReverse";
    }
    if (value == "row-reverse") {
        return "Robo::FlexDirection::RowReverse";
    }
    if (value == "row") {
        return "Robo::FlexDirection::Row";
    }
    return nullptr;
}

const char* MapFlexJustify(const std::string& value) {
    if (value == "center") {
        return "Robo::FlexJustify::Center";
    }
    if (value == "flex-end" || value == "end" || value == "right") {
        return "Robo::FlexJustify::FlexEnd";
    }
    if (value == "space-between" || value == "between") {
        return "Robo::FlexJustify::SpaceBetween";
    }
    if (value == "space-around" || value == "around") {
        return "Robo::FlexJustify::SpaceAround";
    }
    if (value == "space-evenly" || value == "evenly") {
        return "Robo::FlexJustify::SpaceEvenly";
    }
    if (value == "flex-start" || value == "start" || value == "left") {
        return "Robo::FlexJustify::FlexStart";
    }
    return nullptr;
}

const char* MapFlexWrap(const std::string& value) {
    if (value == "wrap") {
        return "Robo::FlexWrap::Wrap";
    }
    if (value == "wrap-reverse" || value == "wrapReverse") {
        return "Robo::FlexWrap::WrapReverse";
    }
    if (value == "nowrap" || value == "no-wrap") {
        return "Robo::FlexWrap::NoWrap";
    }
    return nullptr;
}

const char* MapGridAlignment(const std::string& value) {
    if (value == "start" || value == "left" || value == "top" || value == "flex-start") {
        return "Robo::GridAlignment::Start";
    }
    if (value == "center" || value == "middle") {
        return "Robo::GridAlignment::Center";
    }
    if (value == "end" || value == "right" || value == "bottom" || value == "flex-end") {
        return "Robo::GridAlignment::End";
    }
    if (value == "auto") {
        return "Robo::GridAlignment::Auto";
    }
    if (value == "stretch") {
        return "Robo::GridAlignment::Stretch";
    }
    return nullptr;
}

const char* MapGridJustifyContent(const std::string& value) {
    if (value == "center") {
        return "Robo::GridJustifyContent::Center";
    }
    if (value == "end" || value == "flex-end") {
        return "Robo::GridJustifyContent::End";
    }
    if (value == "space-between" || value == "between") {
        return "Robo::GridJustifyContent::SpaceBetween";
    }
    if (value == "space-around" || value == "around") {
        return "Robo::GridJustifyContent::SpaceAround";
    }
    if (value == "space-evenly" || value == "evenly") {
        return "Robo::GridJustifyContent::SpaceEvenly";
    }
    if (value == "start" || value == "flex-start") {
        return "Robo::GridJustifyContent::Start";
    }
    return nullptr;
}

const char* MapGridAutoFlow(const std::string& value) {
    if (value == "column" || value == "col") {
        return "Robo::GridAutoFlow::Column";
    }
    if (value == "row") {
        return "Robo::GridAutoFlow::Row";
    }
    return nullptr;
}

const char* MapSeparatorVariant(const std::string& value) {
    if (value == "subtle") {
        return "Robo::SeparatorVariant::Subtle";
    }
    if (value == "dashed") {
        return "Robo::SeparatorVariant::Dashed";
    }
    if (value == "dotted") {
        return "Robo::SeparatorVariant::Dotted";
    }
    if (value == "fade") {
        return "Robo::SeparatorVariant::Fade";
    }
    if (value == "solid") {
        return "Robo::SeparatorVariant::Solid";
    }
    return nullptr;
}

const char* MapSeparatorSize(const std::string& value) {
    if (value == "sm" || value == "small") {
        return "Robo::SeparatorSize::Small";
    }
    if (value == "lg" || value == "large") {
        return "Robo::SeparatorSize::Large";
    }
    if (value == "md" || value == "medium") {
        return "Robo::SeparatorSize::Medium";
    }
    return nullptr;
}

const char* MapSeparatorTone(const std::string& value) {
    if (value == "brand") {
        return "Robo::SeparatorTone::Brand";
    }
    if (value == "success") {
        return "Robo::SeparatorTone::Success";
    }
    if (value == "warning") {
        return "Robo::SeparatorTone::Warning";
    }
    if (value == "danger") {
        return "Robo::SeparatorTone::Danger";
    }
    if (value == "default") {
        return "Robo::SeparatorTone::Default";
    }
    return nullptr;
}

const char* MapSeparatorAlign(const std::string& value) {
    if (value == "start" || value == "left") {
        return "Robo::SeparatorAlign::Start";
    }
    if (value == "end" || value == "right") {
        return "Robo::SeparatorAlign::End";
    }
    if (value == "center") {
        return "Robo::SeparatorAlign::Center";
    }
    return nullptr;
}

const char* MapSeparatorOrientation(const std::string& value) {
    if (value == "vertical") {
        return "Robo::SeparatorOrientation::Vertical";
    }
    if (value == "horizontal") {
        return "Robo::SeparatorOrientation::Horizontal";
    }
    return nullptr;
}

const char* MapTextAlign(const std::string& value) {
    if (value == "center") {
        return "Robo::TextAlign::Center";
    }
    if (value == "right") {
        return "Robo::TextAlign::Right";
    }
    if (value == "justify") {
        return "Robo::TextAlign::Justify";
    }
    if (value == "left" || value == "start") {
        return "Robo::TextAlign::Left";
    }
    return nullptr;
}

const char* MapFontWeight(const std::string& value) {
    if (value == "medium" || value == "500") {
        return "Robo::FontWeight::Medium";
    }
    if (value == "semibold" || value == "semiBold" || value == "600") {
        return "Robo::FontWeight::SemiBold";
    }
    if (value == "bold" || value == "700") {
        return "Robo::FontWeight::Bold";
    }
    if (value == "regular" || value == "400" || value == "normal") {
        return "Robo::FontWeight::Regular";
    }
    return nullptr;
}

const char* MapFontStyle(const std::string& value) {
    if (value == "italic") {
        return "Robo::FontStyleKind::Italic";
    }
    if (value == "normal") {
        return "Robo::FontStyleKind::Normal";
    }
    return nullptr;
}

const char* MapTextDecoration(const std::string& value) {
    if (value == "underline") {
        return "Robo::TextDecoration::Underline";
    }
    if (value == "line-through" || value == "lineThrough") {
        return "Robo::TextDecoration::LineThrough";
    }
    if (value == "none") {
        return "Robo::TextDecoration::None";
    }
    return nullptr;
}

const char* MapButtonVariant(const std::string& value) {
    if (value == "tinted") {
        return "Robo::ButtonVariant::Tinted";
    }
    if (value == "gray" || value == "grey" || value == "secondary") {
        return "Robo::ButtonVariant::Gray";
    }
    if (value == "plain" || value == "tertiary") {
        return "Robo::ButtonVariant::Plain";
    }
    if (value == "destructive" || value == "danger") {
        return "Robo::ButtonVariant::Destructive";
    }
    if (value == "outline") {
        return "Robo::ButtonVariant::Outline";
    }
    if (value == "filled" || value == "primary") {
        return "Robo::ButtonVariant::Filled";
    }
    return nullptr;
}

const char* MapSelectable(const std::string& value) {
    if (value == "true" || value == "1" || value == "yes" || value == "on") {
        return "Robo::SelectableMode::On";
    }
    if (value == "false" || value == "0" || value == "no" || value == "off") {
        return "Robo::SelectableMode::Off";
    }
    if (value == "inherit") {
        return "Robo::SelectableMode::Inherit";
    }
    return nullptr;
}

const char* MapSidebarVariant(const std::string& value) {
    if (value == "activity" || value == "dual" || value == "wide") {
        return "Robo::SidebarVariant::Activity";
    }
    if (value == "options" || value == "normal" || value == "buttons") {
        return "Robo::SidebarVariant::Normal";
    }
    return nullptr;
}

}  // namespace

void XmlToCpp::EmitBool(std::ostringstream& code, int indent, const char* lhs, const std::string& value, bool fallback,
                        bool defaultValue) const {
    bool known = false;
    bool parsed = fallback;
    if (value == "true" || value == "yes" || value == "1") {
        known = true;
        parsed = true;
    } else if (value == "false" || value == "no" || value == "0") {
        known = true;
        parsed = false;
    }
    if (known && parsed == defaultValue) {
        return;
    }
    if (known) {
        code << Indent(indent) << lhs << " = " << (parsed ? "true" : "false") << ";\n";
        return;
    }
    code << Indent(indent) << lhs << " = Robo::ParseBool(\"" << EscapeCpp(value) << "\", " << (fallback ? "true" : "false")
         << ");\n";
}

void XmlToCpp::EmitEnum(std::ostringstream& code, int indent, const char* lhs, const std::string& value,
                        const char* (*map)(const std::string&), const char* parseName, const char* defaultExpr) const {
    const char* expr = map != nullptr ? map(value) : nullptr;
    if (expr != nullptr && defaultExpr != nullptr && std::string(expr) == defaultExpr) {
        return;
    }
    if (expr != nullptr) {
        code << Indent(indent) << lhs << " = " << expr << ";\n";
        return;
    }
    code << Indent(indent) << lhs << " = Robo::" << parseName << "(\"" << EscapeCpp(value) << "\");\n";
}

std::string XmlToCpp::Indent(int level) const {
    return std::string(static_cast<std::size_t>(level) * 4, ' ');
}

std::string XmlToCpp::EscapeCpp(const std::string& value) const {
    std::string result;
    result.reserve(value.size());
    for (char ch : value) {
        switch (ch) {
            case '\\':
                result += "\\\\";
                break;
            case '"':
                result += "\\\"";
                break;
            case '\n':
                result += "\\n";
                break;
            case '\r':
                result += "\\r";
                break;
            case '\t':
                result += "\\t";
                break;
            default:
                result.push_back(ch);
                break;
        }
    }
    return result;
}

std::string XmlToCpp::FloatLiteral(const std::string& value) const {
    // A bare "16f" is not valid C++, so every numeric attribute is normalised
    // into a real float literal before it reaches the generated source.
    std::string trimmed = value;
    const std::size_t first = trimmed.find_first_not_of(" 	");
    if (first == std::string::npos) {
        return "0.0f";
    }
    trimmed.erase(0, first);
    trimmed.erase(trimmed.find_last_not_of(" 	") + 1);
    if (trimmed.find('.') == std::string::npos && trimmed.find('e') == std::string::npos &&
        trimmed.find('E') == std::string::npos) {
        trimmed += ".0";
    }
    return trimmed + "f";
}

std::string XmlToCpp::ExtractToken(const std::string& value) const {
    std::string trimmed = value;
    const std::size_t first = trimmed.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    trimmed.erase(0, first);
    trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);
    if (trimmed.size() < 2 || trimmed.front() != '{' || trimmed.back() != '}') {
        return {};
    }
    std::string token = trimmed.substr(1, trimmed.size() - 2);
    const std::size_t tokenFirst = token.find_first_not_of(" \t");
    if (tokenFirst == std::string::npos) {
        throw std::runtime_error("Unknown foundation token " + trimmed);
    }
    token.erase(0, tokenFirst);
    token.erase(token.find_last_not_of(" \t") + 1);
    if (!Foundation::IsKnownToken(token)) {
        throw std::runtime_error("Unknown foundation token {" + token + "}");
    }
    return token;
}

void XmlToCpp::WarnLiteral(const std::string& element, const std::string& attribute, const std::string& value) const {
    std::cerr << "warning: custom value \"" << value << "\" used for " << attribute;
    if (!element.empty()) {
        std::cerr << " on <" << element << ">";
    }
    std::cerr << "; use a RoboFoundation token\n";
}

bool XmlToCpp::IsFreeLength(const std::string& value) const {
    std::string trimmed = value;
    const std::size_t first = trimmed.find_first_not_of(" \t");
    if (first == std::string::npos) {
        return true;
    }
    trimmed.erase(0, first);
    trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
    return trimmed == "auto" || trimmed.find('%') != std::string::npos;
}

std::string XmlToCpp::BindColor(int indent,
                                const std::string& object,
                                const std::string& field,
                                const std::string& value,
                                const std::string& element) const {
    const std::string token = ExtractToken(value);
    if (!token.empty()) {
        return Indent(indent) + object + ".tokens[\"" + field + "\"] = \"" + EscapeCpp(token) + "\";\n";
    }
    WarnLiteral(element, field, value);
    return Indent(indent) + object + ".tokens[\"" + field + "\"] = \"\";\n" + Indent(indent) + object + "." + field +
           " = Robo::Color::FromHex(\"" + EscapeCpp(value) + "\");\n";
}

std::string XmlToCpp::BindFloat(int indent,
                                const std::string& object,
                                const std::string& field,
                                const std::string& value,
                                const std::string& element) const {
    const std::string token = ExtractToken(value);
    if (!token.empty()) {
        return Indent(indent) + object + ".tokens[\"" + field + "\"] = \"" + EscapeCpp(token) + "\";\n";
    }
    WarnLiteral(element, field, value);
    return Indent(indent) + object + ".tokens[\"" + field + "\"] = \"\";\n" + Indent(indent) + object + "." + field +
           " = " + FloatLiteral(value) + ";\n";
}

std::string XmlToCpp::BindLength(int indent,
                                 const std::string& object,
                                 const std::string& field,
                                 const std::string& value,
                                 const std::string& element) const {
    const std::string token = ExtractToken(value);
    if (!token.empty()) {
        return Indent(indent) + object + ".tokens[\"" + field + "\"] = \"" + EscapeCpp(token) + "\";\n";
    }
    if (!IsFreeLength(value)) {
        WarnLiteral(element, field, value);
    }
    // Emit typed Length alongside the legacy string so measure/paint skip parse.
    std::string lengthExpr = "Robo::Length::Auto()";
    if (value == "auto" || value.empty()) {
        lengthExpr = "Robo::Length::Auto()";
    } else if (!value.empty() && value.back() == '%') {
        lengthExpr = "Robo::Length::Percent(" + FloatLiteral(value.substr(0, value.size() - 1)) + ")";
    } else if (value.size() > 3 && value.compare(value.size() - 3, 3, "rem") == 0) {
        lengthExpr = "Robo::Length::Rem(" + FloatLiteral(value.substr(0, value.size() - 3)) + ")";
    } else if (value.size() > 2 && value.compare(value.size() - 2, 2, "px") == 0) {
        lengthExpr = "Robo::Length::Px(" + FloatLiteral(value.substr(0, value.size() - 2)) + ")";
    } else {
        lengthExpr = "Robo::Length::Px(" + FloatLiteral(value) + ")";
    }
    const std::string lengthField = field + "Length";
    const std::string hasField = "has" + std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(field[0])))) +
                                 field.substr(1) + "Length";
    return Indent(indent) + object + "." + field + " = \"" + EscapeCpp(value) + "\";\n" + Indent(indent) + object +
           "." + lengthField + " = " + lengthExpr + ";\n" + Indent(indent) + object + "." + hasField + " = true;\n";
}

std::string XmlToCpp::BindPropsLength(int indent, const std::string& object, const std::string& field,
                                      const std::string& value) const {
    std::string lengthExpr = "Robo::Length::Auto()";
    if (value == "auto" || value.empty()) {
        lengthExpr = "Robo::Length::Auto()";
    } else if (!value.empty() && value.back() == '%') {
        lengthExpr = "Robo::Length::Percent(" + FloatLiteral(value.substr(0, value.size() - 1)) + ")";
    } else if (value.size() > 3 && value.compare(value.size() - 3, 3, "rem") == 0) {
        lengthExpr = "Robo::Length::Rem(" + FloatLiteral(value.substr(0, value.size() - 3)) + ")";
    } else if (value.size() > 2 && value.compare(value.size() - 2, 2, "px") == 0) {
        lengthExpr = "Robo::Length::Px(" + FloatLiteral(value.substr(0, value.size() - 2)) + ")";
    } else {
        lengthExpr = "Robo::Length::Px(" + FloatLiteral(value) + ")";
    }
    const std::string lengthField = field + "Length";
    const std::string hasField = "has" + std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(field[0])))) +
                                 field.substr(1) + "Length";
    return Indent(indent) + object + "." + field + " = \"" + EscapeCpp(value) + "\";\n" + Indent(indent) + object +
           "." + lengthField + " = " + lengthExpr + ";\n" + Indent(indent) + object + "." + hasField + " = true;\n";
}

std::string XmlToCpp::BindWeight(int indent, const std::string& object, const std::string& value, const std::string& element) const {
    const std::string token = ExtractToken(value);
    if (!token.empty()) {
        return Indent(indent) + object + ".tokens[\"fontWeight\"] = \"" + EscapeCpp(token) + "\";\n";
    }
    WarnLiteral(element, "fontWeight", value);
    const char* expr = MapFontWeight(value);
    if (expr != nullptr && std::string(expr) == "Robo::FontWeight::Regular") {
        return {};
    }
    const std::string assigned = expr != nullptr ? expr : "Robo::ParseFontWeight(\"" + EscapeCpp(value) + "\")";
    return Indent(indent) + object + ".tokens[\"fontWeight\"] = \"\";\n" + Indent(indent) + object +
           ".fontWeight = " + assigned + ";\n";
}

std::string XmlToCpp::BindSides(int indent,
                                const std::string& object,
                                const std::string& kind,
                                const std::string& value,
                                const std::string& element) const {
    std::vector<std::string> fields;
    if (kind == "paddingX") {
        fields = {"paddingLeft", "paddingRight"};
    } else if (kind == "paddingY") {
        fields = {"paddingTop", "paddingBottom"};
    } else if (kind == "margin") {
        fields = {"marginTop", "marginBottom", "marginLeft", "marginRight"};
    } else {
        fields = {"paddingTop", "paddingBottom", "paddingLeft", "paddingRight"};
    }
    const std::string token = ExtractToken(value);
    std::string code;
    if (!token.empty()) {
        for (const std::string& field : fields) {
            code += Indent(indent) + object + ".tokens[\"" + field + "\"] = \"" + EscapeCpp(token) + "\";\n";
        }
        return code;
    }
    WarnLiteral(element, kind, value);
    for (const std::string& field : fields) {
        code += Indent(indent) + object + ".tokens[\"" + field + "\"] = \"\";\n";
        code += Indent(indent) + object + "." + field + " = " + FloatLiteral(value) + ";\n";
    }
    return code;
}

void XmlToCpp::CollectFoundations(const XmlNode& node) const {
    if (node.name == "Foundation") {
        const auto name = node.attributes.find("name");
        if (name == node.attributes.end() || name->second.empty()) {
            throw std::runtime_error("Foundation requires a name");
        }
        if (name->second == "RoboFoundation") {
            throw std::runtime_error("RoboFoundation is sealed and cannot be redefined");
        }
        foundations_.insert(name->second);
    }
    for (const XmlNode& child : node.children) {
        CollectFoundations(child);
    }
}

std::string XmlToCpp::EmitFoundation(const XmlNode& node, int indent) const {
    const auto nameAttribute = node.attributes.find("name");
    if (nameAttribute == node.attributes.end() || nameAttribute->second.empty()) {
        throw std::runtime_error("Foundation requires a name");
    }
    if (nameAttribute->second == "RoboFoundation") {
        throw std::runtime_error("RoboFoundation is sealed and cannot be redefined");
    }
    const std::string name = nameAttribute->second;
    std::string from = "RoboFoundation";
    bool hasFrom = false;
    const auto fromAttribute = node.attributes.find("from");
    if (fromAttribute != node.attributes.end() && !fromAttribute->second.empty()) {
        from = fromAttribute->second;
        hasFrom = true;
        if (emittedFoundations_.count(from) == 0 && from != "RoboFoundation") {
            throw std::runtime_error("Foundation " + name + " copies " + from + " before it is defined");
        }
    }

    std::ostringstream code;
    const std::string variable = "foundation" + name;
    code << Indent(indent) << "Robo::Foundation " << variable << " = Robo::Foundation::CopyOf(\"" << EscapeCpp(from)
         << "\");\n";
    code << Indent(indent) << variable << ".SetName(\"" << EscapeCpp(name) << "\");\n";

    auto requireSource = [&](const std::string& source) {
        if (emittedFoundations_.count(source) == 0 && source != "RoboFoundation") {
            throw std::runtime_error("Foundation layer copies " + source + " before it is defined");
        }
    };

    bool sawLayer = hasFrom;
    for (const XmlNode& child : node.children) {
        if (child.isText) {
            continue;
        }
        const auto layerFrom = child.attributes.find("from");
        const bool copyLayer = layerFrom != child.attributes.end() && !layerFrom->second.empty();
        if (child.name == "Colors" || child.name == "Typography" || child.name == "Spacing" || child.name == "Radius" ||
            child.name == "Layout") {
            sawLayer = true;
            if (copyLayer) {
                requireSource(layerFrom->second);
                std::string method = "CopyColorsFrom";
                if (child.name == "Typography") {
                    method = "CopyTypographyFrom";
                } else if (child.name == "Spacing") {
                    method = "CopySpacingFrom";
                } else if (child.name == "Radius") {
                    method = "CopyRadiusFrom";
                } else if (child.name == "Layout") {
                    method = "CopyLayoutFrom";
                }
                code << Indent(indent) << "{\n";
                code << Indent(indent + 1) << "const Robo::Foundation* source = Robo::Foundation::Find(\""
                     << EscapeCpp(layerFrom->second) << "\");\n";
                code << Indent(indent + 1) << "if (source != nullptr) " << variable << "." << method << "(*source);\n";
                code << Indent(indent) << "}\n";
            }
            for (const XmlNode& item : child.children) {
                if (item.isText) {
                    continue;
                }
                if (item.name == "Color") {
                    const std::string colorName = item.attributes.count("name") ? item.attributes.at("name") : "";
                    const std::string light = item.attributes.count("light") ? item.attributes.at("light") : "";
                    const std::string dark = item.attributes.count("dark") ? item.attributes.at("dark") : light;
                    if (colorName.empty() || !Foundation::IsKnownToken("color." + colorName)) {
                        throw std::runtime_error("Unknown color token " + colorName);
                    }
                    code << Indent(indent) << variable << ".SetColor(\"" << EscapeCpp(colorName)
                         << "\", Robo::Color::FromHex(\"" << EscapeCpp(light) << "\"), Robo::Color::FromHex(\""
                         << EscapeCpp(dark) << "\"));\n";
                }
            }
        } else if (child.name == "Color") {
            sawLayer = true;
            const std::string colorName = child.attributes.count("name") ? child.attributes.at("name") : "";
            const std::string light = child.attributes.count("light") ? child.attributes.at("light") : "";
            const std::string dark = child.attributes.count("dark") ? child.attributes.at("dark") : light;
            if (colorName.empty() || !Foundation::IsKnownToken("color." + colorName)) {
                throw std::runtime_error("Unknown color token " + colorName);
            }
            code << Indent(indent) << variable << ".SetColor(\"" << EscapeCpp(colorName) << "\", Robo::Color::FromHex(\""
                 << EscapeCpp(light) << "\"), Robo::Color::FromHex(\"" << EscapeCpp(dark) << "\"));\n";
        } else if (child.name == "Space") {
            sawLayer = true;
            const std::string spaceName = child.attributes.count("name") ? child.attributes.at("name") : "";
            const std::string spaceValue = child.attributes.count("value") ? child.attributes.at("value") : "";
            if (!Foundation::IsKnownToken("space." + spaceName)) {
                throw std::runtime_error("Unknown space token " + spaceName);
            }
            code << Indent(indent) << variable << ".SetSpace(\"" << EscapeCpp(spaceName) << "\", " << FloatLiteral(spaceValue)
                 << ");\n";
        } else if (child.name == "Radius") {
            sawLayer = true;
            const std::string radiusName = child.attributes.count("name") ? child.attributes.at("name") : "";
            const std::string radiusValue = child.attributes.count("value") ? child.attributes.at("value") : "";
            if (!Foundation::IsKnownToken("radius." + radiusName)) {
                throw std::runtime_error("Unknown radius token " + radiusName);
            }
            code << Indent(indent) << variable << ".SetRadius(\"" << EscapeCpp(radiusName) << "\", "
                 << FloatLiteral(radiusValue) << ");\n";
        } else if (child.name == "Type") {
            sawLayer = true;
            const std::string role = child.attributes.count("name") ? child.attributes.at("name") : "";
            if (!Foundation::IsKnownToken("type." + role + ".size")) {
                throw std::runtime_error("Unknown type role " + role);
            }
            const Foundation& base = Foundation::RoboFoundation();
            float size = 16.0f;
            float mobile = 16.0f;
            float weight = 400.0f;
            float tracking = 0.0f;
            float leading = 1.4f;
            base.LookupFloat("type." + role + ".size", size);
            base.LookupFloat("type." + role + ".mobile", mobile);
            base.LookupFloat("type." + role + ".weight", weight);
            base.LookupFloat("type." + role + ".tracking", tracking);
            base.LookupFloat("type." + role + ".leading", leading);
            auto number = [&](const char* attribute, float fallback) {
                const auto found = child.attributes.find(attribute);
                if (found == child.attributes.end()) {
                    return FloatLiteral(std::to_string(fallback));
                }
                return FloatLiteral(found->second);
            };
            code << Indent(indent) << variable << ".SetType(\"" << EscapeCpp(role) << "\", " << number("size", size)
                 << ", " << number("mobile", mobile) << ", static_cast<int>(" << number("weight", weight) << "), "
                 << number("tracking", tracking) << ", " << number("leading", leading) << ");\n";
        } else if (child.name == "LayoutValue") {
            sawLayer = true;
            const std::string layoutName = child.attributes.count("name") ? child.attributes.at("name") : "";
            const std::string layoutValue = child.attributes.count("value") ? child.attributes.at("value") : "";
            if (!Foundation::IsKnownToken("layout." + layoutName)) {
                throw std::runtime_error("Unknown layout token " + layoutName);
            }
            code << Indent(indent) << variable << ".SetLayout(\"" << EscapeCpp(layoutName) << "\", "
                 << FloatLiteral(layoutValue) << ");\n";
        }
    }
    if (!sawLayer && !hasFrom) {
        throw std::runtime_error("Foundation " + name + " must copy RoboFoundation or name the layers it copies");
    }
    code << Indent(indent) << "Robo::Foundation::Register(" << variable << ");\n";
    emittedFoundations_.insert(name);
    return code.str();
}

std::string XmlToCpp::EmitFoundationUse(const XmlNode& node, int indent) const {
    const auto nameAttribute = node.attributes.find("name");
    if (nameAttribute == node.attributes.end() || foundations_.count(nameAttribute->second) == 0) {
        throw std::runtime_error("FoundationUse names a foundation that does not exist");
    }
    const std::string when = node.attributes.count("when") ? node.attributes.at("when") : "";
    std::ostringstream code;
    if (when.empty()) {
        code << Indent(indent) << "scene.SetFoundation(\"" << EscapeCpp(nameAttribute->second) << "\");\n";
    } else if (when == "dark") {
        code << Indent(indent) << "scene.SetFoundationForTheme(Robo::Theme::Dark, \"" << EscapeCpp(nameAttribute->second)
             << "\");\n";
    } else if (when == "light") {
        code << Indent(indent) << "scene.SetFoundationForTheme(Robo::Theme::Light, \"" << EscapeCpp(nameAttribute->second)
             << "\");\n";
    } else {
        throw std::runtime_error("FoundationUse when must be light or dark");
    }
    return code.str();
}

std::string XmlToCpp::ConvertNode(const XmlNode& node, int indent) const {
    std::ostringstream code;
    if (node.isText) {
        return code.str();
    }

    if (node.name == "Foundation") {
        return EmitFoundation(node, indent);
    }
    if (node.name == "FoundationUse") {
        return EmitFoundationUse(node, indent);
    }
    if (node.name == "Keyframes") {
        EmitKeyframes(code, node, indent);
        return code.str();
    }

    if (node.name == "Robo") {
        const XmlNode* sidebarNode = nullptr;
        std::vector<CatalogEntry> catalog;
        std::string sidebarVariant;
        std::string defaultPaneId;
        std::vector<const XmlNode*> lazyPanes;
        std::vector<const XmlNode*> eagerNodes;
        for (const XmlNode& child : node.children) {
            if (child.name == "Foundation" || child.name == "FoundationUse" || child.name == "Keyframes" ||
                child.isText) {
                continue;
            }
            if (child.name == "Sidebar" || child.name == "Pane") {
                // Inline items in the page XML: emit the Sidebar as eager UI.
                // Empty Sidebar + ROBOCAT.PAK remains the pack-driven catalog path.
                bool hasInlineItems = false;
                for (const XmlNode& item : child.children) {
                    if (IsPaneItem(item.name)) {
                        hasInlineItems = true;
                        break;
                    }
                }
                if (hasInlineItems) {
                    eagerNodes.push_back(&child);
                    continue;
                }
                sidebarNode = &child;
                if (child.attributes.count("variant") != 0) {
                    sidebarVariant = child.attributes.at("variant");
                }
                CollectCatalogEntries(child, -1, catalog);
                continue;
            }
            if (child.name == "Container" && child.attributes.count("when") != 0) {
                lazyPanes.push_back(&child);
                continue;
            }
            eagerNodes.push_back(&child);
        }
        for (const CatalogEntry& entry : catalog) {
            if (entry.selected && !entry.id.empty()) {
                defaultPaneId = entry.id;
                break;
            }
        }
        if (defaultPaneId.empty()) {
            for (const CatalogEntry& entry : catalog) {
                if (!entry.id.empty() && !entry.section) {
                    defaultPaneId = entry.id;
                    break;
                }
            }
        }
        if (defaultPaneId.empty() && !lazyPanes.empty()) {
            defaultPaneId = lazyPanes.front()->attributes.at("when");
        }

        if (!catalog.empty()) {
            code << Indent(indent) << "static bool EnsureRailChildren(Robo::Sidebar& sidebar, int railIndex) {\n";
            code << Indent(indent + 1) << "return Robo::RoboEnsureCatalogChildren(sidebar, railIndex, "
                                         "Robo::RoboCatalogRows(), Robo::RoboCatalogCount());\n";
            code << Indent(indent) << "}\n\n";
            (void)sidebarNode;
            (void)sidebarVariant;
        }

        if (!lazyPanes.empty()) {
            code << Indent(indent) << "static void EnsurePane(Robo::RoboScene& scene, const char* paneId) {\n";
            code << Indent(indent + 1) << "Robo::RoboPaneBuild(scene, paneId);\n";
            code << Indent(indent) << "}\n\n";
        }

        int eagerPart = 0;
        for (const XmlNode* eager : eagerNodes) {
            code << Indent(indent) << "static void BuildEagerPart" << eagerPart << "(Robo::RoboScene& scene) {\n";
            code << ConvertNode(*eager, indent + 1);
            code << Indent(indent) << "}\n\n";
            ++eagerPart;
        }

        bool desktopChrome = false;
        bool catalogChrome = false;
        {
            bool hasDesktopFoundation = false;
            bool hasDesktopUse = false;
            bool hasAppFoundation = false;
            bool hasAppUse = false;
            for (const XmlNode& child : node.children) {
                if (child.name == "Foundation" && child.attributes.count("name") != 0 &&
                    child.attributes.count("from") != 0 && child.attributes.at("from") == "RoboFoundation" &&
                    child.children.empty()) {
                    if (child.attributes.at("name") == "Desktop") {
                        hasDesktopFoundation = true;
                    } else {
                        hasAppFoundation = true;
                    }
                }
                if (child.name == "FoundationUse" && child.attributes.count("name") != 0) {
                    if (child.attributes.at("name") == "Desktop") {
                        hasDesktopUse = true;
                    } else {
                        hasAppUse = true;
                    }
                }
            }
            desktopChrome = catalog.empty() && lazyPanes.empty() && hasDesktopFoundation && hasDesktopUse;
            catalogChrome = !catalog.empty() && hasAppFoundation && hasAppUse;
        }

        code << Indent(indent) << "// Generated from Robo XML (catalog + lazy panes)\n";
        code << Indent(indent) << "void BuildPage(Robo::RoboScene& scene) {\n";
        if (desktopChrome) {
            code << Indent(indent + 1) << "Robo::RoboApplyDesktopChrome(scene);\n";
        } else if (catalogChrome) {
            code << Indent(indent + 1) << "Robo::RoboApplyCatalogChrome(scene);\n";
        } else {
            if (node.attributes.count("padding") != 0) {
                code << Indent(indent + 1) << "scene.SetPagePadding(" << FloatLiteral(node.attributes.at("padding"))
                     << ");\n";
            }
            for (const XmlNode& child : node.children) {
                if (child.name == "Foundation") {
                    code << ConvertNode(child, indent + 1);
                }
            }
            for (const XmlNode& child : node.children) {
                if (child.name == "FoundationUse") {
                    code << ConvertNode(child, indent + 1);
                }
            }
        }
        for (const XmlNode& child : node.children) {
            if (child.name == "Keyframes") {
                code << ConvertNode(child, indent + 1);
            }
        }
        for (int i = 0; i < eagerPart; ++i) {
            code << Indent(indent + 1) << "BuildEagerPart" << i << "(scene);\n";
        }
        if (!catalog.empty()) {
            code << Indent(indent + 1) << "Robo::RoboBuildSidebarFromCatalog(scene);\n";
            code << Indent(indent + 1) << "scene.SetRailChildrenEnsure(&EnsureRailChildren);\n";
        }
        if (!lazyPanes.empty()) {
            code << Indent(indent + 1) << "scene.SetPaneEnsure(&EnsurePane);\n";
        }
        if (!defaultPaneId.empty()) {
            code << Indent(indent + 1) << "scene.SetActivePaneId(\"" << EscapeCpp(defaultPaneId) << "\");\n";
        }
        code << Indent(indent) << "}\n\n";
        const bool hasRails = !catalog.empty();
        const bool hasPaneHook = !lazyPanes.empty();
        const bool hasPaneBuild = !defaultPaneId.empty();
        code << Indent(indent)
             << "// Micro-steps: 0=chrome, 1..N=eager parts (one/tick), 100+n=rails, 200=hooks, 300=pane.\n";
        code << Indent(indent) << "int BuildPageStep(Robo::RoboScene& scene, int step) {\n";
        code << Indent(indent + 1) << "if (step == 0) {\n";
        if (desktopChrome) {
            code << Indent(indent + 2) << "Robo::RoboApplyDesktopChrome(scene);\n";
        } else if (catalogChrome) {
            code << Indent(indent + 2) << "Robo::RoboApplyCatalogChrome(scene);\n";
        } else {
            if (node.attributes.count("padding") != 0) {
                code << Indent(indent + 2) << "scene.SetPagePadding(" << FloatLiteral(node.attributes.at("padding"))
                     << ");\n";
            }
            for (const XmlNode& child : node.children) {
                if (child.name == "Foundation") {
                    code << ConvertNode(child, indent + 2);
                }
            }
            for (const XmlNode& child : node.children) {
                if (child.name == "FoundationUse") {
                    code << ConvertNode(child, indent + 2);
                }
            }
        }
        for (const XmlNode& child : node.children) {
            if (child.name == "Keyframes") {
                code << ConvertNode(child, indent + 2);
            }
        }
        if (eagerPart > 0) {
            code << Indent(indent + 2) << "return 1;\n";
        } else if (hasRails) {
            code << Indent(indent + 2) << "return 100;\n";
        } else if (hasPaneHook || hasPaneBuild) {
            code << Indent(indent + 2) << "return 200;\n";
        } else {
            code << Indent(indent + 2) << "return -1;\n";
        }
        code << Indent(indent + 1) << "}\n";
        if (eagerPart > 0) {
            code << Indent(indent + 1) << "if (step >= 1 && step <= " << eagerPart << ") {\n";
            code << Indent(indent + 2) << "switch (step) {\n";
            for (int i = 0; i < eagerPart; ++i) {
                code << Indent(indent + 3) << "case " << (i + 1) << ": BuildEagerPart" << i << "(scene); break;\n";
            }
            code << Indent(indent + 3) << "default: break;\n";
            code << Indent(indent + 2) << "}\n";
            code << Indent(indent + 2) << "if (step < " << eagerPart << ") {\n";
            code << Indent(indent + 3) << "return step + 1;\n";
            code << Indent(indent + 2) << "}\n";
            if (hasRails) {
                code << Indent(indent + 2) << "return 100;\n";
            } else if (hasPaneHook || hasPaneBuild) {
                code << Indent(indent + 2) << "return 200;\n";
            } else {
                code << Indent(indent + 2) << "return -1;\n";
            }
            code << Indent(indent + 1) << "}\n";
        }
        if (hasRails) {
            code << Indent(indent + 1) << "if (step >= 100 && step < 200) {\n";
            code << Indent(indent + 2) << "const int railStart = step - 100;\n";
            code << Indent(indent + 2) << "const int next = Robo::RoboBuildSidebarRailsStep(scene, railStart, 4);\n";
            code << Indent(indent + 2) << "if (next < Robo::RoboCountCatalogRails(Robo::RoboCatalogRows(), "
                                         "Robo::RoboCatalogCount())) {\n";
            code << Indent(indent + 3) << "return 100 + next;\n";
            code << Indent(indent + 2) << "}\n";
            code << Indent(indent + 2) << "if (Robo::Sidebar* sidebar = Robo::RoboFindSidebar(scene)) {\n";
            code << Indent(indent + 3) << "sidebar->Select(Robo::RoboDefaultCatalogRail(Robo::RoboCatalogRows(), "
                                         "Robo::RoboCatalogCount()));\n";
            code << Indent(indent + 2) << "}\n";
            if (hasPaneHook || hasPaneBuild) {
                code << Indent(indent + 2) << "return 200;\n";
            } else {
                code << Indent(indent + 2) << "scene.SetRailChildrenEnsure(&EnsureRailChildren);\n";
                code << Indent(indent + 2) << "return -1;\n";
            }
            code << Indent(indent + 1) << "}\n";
        }
        if (hasPaneHook || hasPaneBuild || hasRails) {
            code << Indent(indent + 1) << "if (step == 200) {\n";
            if (hasRails) {
                code << Indent(indent + 2) << "scene.SetRailChildrenEnsure(&EnsureRailChildren);\n";
            }
            if (hasPaneHook) {
                code << Indent(indent + 2) << "scene.SetPaneEnsure(&EnsurePane);\n";
            }
            if (hasPaneBuild) {
                code << Indent(indent + 2) << "return 300;\n";
            } else {
                code << Indent(indent + 2) << "return -1;\n";
            }
            code << Indent(indent + 1) << "}\n";
        }
        if (hasPaneBuild) {
            code << Indent(indent + 1) << "if (step == 300) {\n";
            code << Indent(indent + 2) << "scene.SetActivePaneId(\"" << EscapeCpp(defaultPaneId) << "\");\n";
            code << Indent(indent + 2) << "return -1;\n";
            code << Indent(indent + 1) << "}\n";
        }
        code << Indent(indent + 1) << "(void)scene;\n";
        code << Indent(indent + 1) << "return -1;\n";
        code << Indent(indent) << "}\n";
        return code.str();
    }

    if (IsSelectionAreaName(node.name)) {
        EmitSelectionArea(code, node, indent);
        return code.str();
    }

    if (node.name == "Text") {
        code << Indent(indent) << "{\n";
        code << Indent(indent + 1) << "Robo::Text text;\n";
        code << Indent(indent + 1) << "Robo::TextStyle style;\n";

        for (const auto& attribute : node.attributes) {
            if (attribute.first == "fontSize") {
                code << BindFloat(indent + 1, "style", "fontSize", attribute.second, node.name);
            } else if (attribute.first == "color") {
                code << BindColor(indent + 1, "style", "color", attribute.second, node.name);
            } else if (attribute.first == "align") {
                EmitEnum(code, indent + 1, "style.align", attribute.second, MapTextAlign, "ParseTextAlign",
                         "Robo::TextAlign::Left");
            } else if (attribute.first == "wrap") {
                const bool wrap = !(attribute.second == "false" || attribute.second == "0" || attribute.second == "no");
                if (!wrap) {
                    code << Indent(indent + 1) << "style.wrap = false;\n";
                }
            } else if (attribute.first == "fontWeight") {
                code << BindWeight(indent + 1, "style", attribute.second, node.name);
            } else if (attribute.first == "fontStyle") {
                EmitEnum(code, indent + 1, "style.fontStyle", attribute.second, MapFontStyle, "ParseFontStyle",
                         "Robo::FontStyleKind::Normal");
            } else if (attribute.first == "textDecoration") {
                EmitEnum(code, indent + 1, "style.textDecoration", attribute.second, MapTextDecoration, "ParseTextDecoration",
                         "Robo::TextDecoration::None");
            } else if (attribute.first == "lineHeight") {
                code << BindFloat(indent + 1, "style", "lineHeight", attribute.second, node.name);
            } else if (attribute.first == "letterSpacing") {
                code << BindFloat(indent + 1, "style", "letterSpacing", attribute.second, node.name);
            } else if (attribute.first == "opacity") {
                code << Indent(indent + 1) << "style.opacity = " << attribute.second << ";\n";
            } else if (attribute.first == "width") {
                code << BindLength(indent + 1, "style", "width", attribute.second, node.name);
            } else if (attribute.first == "maxWidth") {
                code << BindLength(indent + 1, "style", "maxWidth", attribute.second, node.name);
            }
        }

        EmitSelectable(code, indent + 1, "style", node);
        code << Indent(indent + 1) << "text.SetStyle(style);\n";

        for (const XmlNode& child : node.children) {
            if (child.isText) {
                code << Indent(indent + 1) << "text.AddPlain(\"" << EscapeCpp(child.text) << "\");\n";
            } else if (child.name == "Span" || child.name == "span") {
                code << Indent(indent + 1) << "{\n";
                code << Indent(indent + 2) << "Robo::TextRun run;\n";
                std::string content;
                for (const XmlNode& spanChild : child.children) {
                    if (spanChild.isText) {
                        content += spanChild.text;
                    }
                }
                code << Indent(indent + 2) << "run.content = \"" << EscapeCpp(content) << "\";\n";
                for (const auto& attribute : child.attributes) {
                    if (attribute.first == "fontSize") {
                        code << Indent(indent + 2) << "run.fontSize = " << attribute.second << ";\n";
                        code << Indent(indent + 2) << "run.hasFontSize = true;\n";
                    } else if (attribute.first == "color") {
                        code << BindColor(indent + 2, "run", "color", attribute.second, node.name);
                        code << Indent(indent + 2) << "run.hasColor = true;\n";
                    } else if (attribute.first == "fontWeight") {
                        EmitEnum(code, indent + 2, "run.fontWeight", attribute.second, MapFontWeight, "ParseFontWeight",
                                 "Robo::FontWeight::Regular");
                        code << Indent(indent + 2) << "run.hasFontWeight = true;\n";
                    } else if (attribute.first == "fontStyle") {
                        EmitEnum(code, indent + 2, "run.fontStyle", attribute.second, MapFontStyle, "ParseFontStyle",
                                 "Robo::FontStyleKind::Normal");
                        code << Indent(indent + 2) << "run.hasFontStyle = true;\n";
                    } else if (attribute.first == "textDecoration") {
                        EmitEnum(code, indent + 2, "run.textDecoration", attribute.second, MapTextDecoration, "ParseTextDecoration",
                                 "Robo::TextDecoration::None");
                        code << Indent(indent + 2) << "run.hasTextDecoration = true;\n";
                    } else if (attribute.first == "letterSpacing") {
                        code << Indent(indent + 2) << "run.letterSpacing = " << attribute.second << ";\n";
                        code << Indent(indent + 2) << "run.hasLetterSpacing = true;\n";
                    } else if (attribute.first == "opacity") {
                        code << Indent(indent + 2) << "run.opacity = " << attribute.second << ";\n";
                        code << Indent(indent + 2) << "run.hasOpacity = true;\n";
                    }
                }
                code << Indent(indent + 2) << "text.AddRun(run);\n";
                code << Indent(indent + 1) << "}\n";
            }
        }

        code << Indent(indent + 1) << "scene.AddText(std::move(text));\n";
        code << Indent(indent) << "}\n";
        return code.str();
    }

    const bool isHeading =
        node.name == "Heading" || node.name == "heading" ||
        (node.name.size() == 2 && (node.name[0] == 'H' || node.name[0] == 'h') && node.name[1] >= '1' &&
         node.name[1] <= '6') ||
        (node.name.size() == 8 && (node.name.compare(0, 7, "Heading") == 0 || node.name.compare(0, 7, "heading") == 0) &&
         node.name[7] >= '1' && node.name[7] <= '6');
    const bool isParagraph =
        node.name == "Paragraph" || node.name == "paragraph" || node.name == "P" || node.name == "p";

    if (isHeading || isParagraph) {
        int level = 1;
        if (isHeading) {
            for (const auto& attribute : node.attributes) {
                if (attribute.first == "level") {
                    level = std::atoi(attribute.second.c_str());
                }
            }
            if (node.name.size() == 2 && node.name[1] >= '1' && node.name[1] <= '6') {
                level = node.name[1] - '0';
            }
            if (node.name.size() == 8 && node.name[7] >= '1' && node.name[7] <= '6') {
                level = node.name[7] - '0';
            }
        }

        code << Indent(indent) << "{\n";
        if (isHeading) {
            code << Indent(indent + 1) << "Robo::Heading text(" << level << ");\n";
        } else {
            code << Indent(indent + 1) << "Robo::Paragraph text;\n";
        }
        bool styleTouched = false;
        std::ostringstream styleCode;
        for (const auto& attribute : node.attributes) {
            if (attribute.first == "fontSize") {
                styleCode << BindFloat(indent + 1, "style", "fontSize", attribute.second, node.name);
                styleTouched = true;
            } else if (attribute.first == "color") {
                styleCode << BindColor(indent + 1, "style", "color", attribute.second, node.name);
                styleTouched = true;
            } else if (attribute.first == "align") {
                EmitEnum(styleCode, indent + 1, "style.align", attribute.second, MapTextAlign, "ParseTextAlign",
                         "Robo::TextAlign::Left");
                styleTouched = true;
            } else if (attribute.first == "wrap") {
                const bool wrap = !(attribute.second == "false" || attribute.second == "0" || attribute.second == "no");
                if (!wrap) {
                    styleCode << Indent(indent + 1) << "style.wrap = false;\n";
                    styleTouched = true;
                }
            } else if (attribute.first == "fontWeight") {
                styleCode << BindWeight(indent + 1, "style", attribute.second, node.name);
                styleTouched = true;
            } else if (attribute.first == "lineHeight") {
                styleCode << BindFloat(indent + 1, "style", "lineHeight", attribute.second, node.name);
                styleTouched = true;
            } else if (attribute.first == "width") {
                styleCode << BindLength(indent + 1, "style", "width", attribute.second, node.name);
                styleTouched = true;
            } else if (attribute.first == "maxWidth") {
                styleCode << BindLength(indent + 1, "style", "maxWidth", attribute.second, node.name);
                styleTouched = true;
            }
        }
        {
            std::ostringstream selectableCode;
            EmitSelectable(selectableCode, indent + 1, "style", node);
            if (!selectableCode.str().empty()) {
                styleCode << selectableCode.str();
                styleTouched = true;
            }
        }
        if (styleTouched) {
            code << Indent(indent + 1) << "Robo::TextStyle style = text.GetStyle();\n";
            code << styleCode.str();
            code << Indent(indent + 1) << "text.SetStyle(style);\n";
        }

        for (const XmlNode& child : node.children) {
            if (child.isText) {
                code << Indent(indent + 1) << "text.AddPlain(\"" << EscapeCpp(child.text) << "\");\n";
            } else if (child.name == "Span" || child.name == "span") {
                code << Indent(indent + 1) << "{\n";
                code << Indent(indent + 2) << "Robo::TextRun run;\n";
                std::string content;
                for (const XmlNode& spanChild : child.children) {
                    if (spanChild.isText) {
                        content += spanChild.text;
                    }
                }
                code << Indent(indent + 2) << "run.content = \"" << EscapeCpp(content) << "\";\n";
                code << Indent(indent + 2) << "text.AddRun(run);\n";
                code << Indent(indent + 1) << "}\n";
            }
        }

        code << Indent(indent + 1) << "scene.AddText(std::move(text));\n";
        code << Indent(indent) << "}\n";
        return code.str();
    }

    if (node.name == "Button" || node.name == "IconButton") {
        code << Indent(indent) << "{\n";
        code << Indent(indent + 1) << "Robo::Button button;\n";
        code << Indent(indent + 1) << "Robo::ButtonStyle style;\n";

        std::string label;
        for (const auto& attribute : node.attributes) {
            if (attribute.first == "title" || attribute.first == "label") {
                label = attribute.second;
            } else if (attribute.first == "variant" || attribute.first == "style") {
                EmitEnum(code, indent + 1, "style.variant", attribute.second, MapButtonVariant, "ParseButtonVariant",
                         "Robo::ButtonVariant::Filled");
            } else if (attribute.first == "fontSize") {
                code << BindFloat(indent + 1, "style", "fontSize", attribute.second, node.name);
            } else if (attribute.first == "fontWeight") {
                code << BindWeight(indent + 1, "style", attribute.second, node.name);
            } else if (attribute.first == "color") {
                code << BindColor(indent + 1, "style", "textColor", attribute.second, node.name);
                code << Indent(indent + 1) << "style.hasCustomTextColor = true;\n";
            } else if (attribute.first == "background" || attribute.first == "backgroundColor") {
                code << BindColor(indent + 1, "style", "backgroundColor", attribute.second, node.name);
                code << Indent(indent + 1) << "style.hasCustomBackground = true;\n";
            } else if (attribute.first == "cornerRadius") {
                code << BindFloat(indent + 1, "style", "cornerRadius", attribute.second, node.name);
            } else if (attribute.first == "paddingX") {
                code << BindFloat(indent + 1, "style", "paddingX", attribute.second, node.name);
            } else if (attribute.first == "paddingY") {
                code << BindFloat(indent + 1, "style", "paddingY", attribute.second, node.name);
            } else if (attribute.first == "align") {
                EmitEnum(code, indent + 1, "style.align", attribute.second, MapTextAlign, "ParseTextAlign",
                         "Robo::TextAlign::Left");
            }
        }

        if (label.empty()) {
            for (const XmlNode& child : node.children) {
                if (child.isText) {
                    label += child.text;
                }
            }
        }

        EmitSelectable(code, indent + 1, "style", node);
        code << Indent(indent + 1) << "button.SetStyle(style);\n";
        code << Indent(indent + 1) << "button.ApplyVariantDefaults();\n";
        code << Indent(indent + 1) << "button.SetLabel(\"" << EscapeCpp(label) << "\");\n";
        EmitButtonFlags(code, indent + 1, node);
        code << Indent(indent + 1) << "scene.AddButton(std::move(button));\n";
        code << Indent(indent) << "}\n";
        return code.str();
    }

    if (node.name == "Input" || node.name == "TextField" || node.name == "TextInput") {
        code << Indent(indent) << "{\n";
        code << Indent(indent + 1) << "Robo::Input input;\n";
        code << Indent(indent + 1) << "Robo::InputStyle style;\n";

        std::string value;
        std::string placeholder;
        for (const auto& attribute : node.attributes) {
            if (attribute.first == "value" || attribute.first == "text") {
                value = attribute.second;
            } else if (attribute.first == "placeholder" || attribute.first == "hint") {
                placeholder = attribute.second;
            } else if (attribute.first == "variant" || attribute.first == "style") {
                code << Indent(indent + 1) << "style.variant = Robo::ParseInputVariant(\"" << EscapeCpp(attribute.second)
                     << "\");\n";
            } else if (attribute.first == "fontSize") {
                code << BindFloat(indent + 1, "style", "fontSize", attribute.second, node.name);
            } else if (attribute.first == "fontWeight") {
                code << BindWeight(indent + 1, "style", attribute.second, node.name);
            } else if (attribute.first == "color") {
                code << BindColor(indent + 1, "style", "textColor", attribute.second, node.name);
                code << Indent(indent + 1) << "style.hasCustomTextColor = true;\n";
            } else if (attribute.first == "background" || attribute.first == "backgroundColor") {
                code << BindColor(indent + 1, "style", "backgroundColor", attribute.second, node.name);
                code << Indent(indent + 1) << "style.hasCustomBackground = true;\n";
            } else if (attribute.first == "cornerRadius") {
                code << BindFloat(indent + 1, "style", "cornerRadius", attribute.second, node.name);
            } else if (attribute.first == "width") {
                code << BindLength(indent + 1, "style", "width", attribute.second, node.name);
            } else if (attribute.first == "align") {
                EmitEnum(code, indent + 1, "style.align", attribute.second, MapTextAlign, "ParseTextAlign",
                         "Robo::TextAlign::Left");
            }
        }

        if (value.empty()) {
            for (const XmlNode& child : node.children) {
                if (child.isText) {
                    value += child.text;
                }
            }
        }

        EmitSelectable(code, indent + 1, "style", node);
        code << Indent(indent + 1) << "input.SetStyle(style);\n";
        code << Indent(indent + 1) << "input.ApplyVariantDefaults();\n";
        code << Indent(indent + 1) << "input.SetPlaceholder(\"" << EscapeCpp(placeholder) << "\");\n";
        code << Indent(indent + 1) << "input.SetValue(\"" << EscapeCpp(value) << "\");\n";
        code << Indent(indent + 1) << "scene.AddInput(std::move(input));\n";
        code << Indent(indent) << "}\n";
        return code.str();
    }

    if (node.name == "Scrollbar" || node.name == "scrollbar") {
        code << Indent(indent) << "{\n";
        EmitScrollbar(code, node, indent + 1, "scrollbar");
        code << Indent(indent + 1) << "scene.AddScrollbar(std::move(scrollbar));\n";
        code << Indent(indent) << "}\n";
        return code.str();
    }

    if (node.name == "Container" || node.name == "container") {
        code << Indent(indent) << "{\n";
        EmitContainer(code, node, indent + 1, "scene");
        code << Indent(indent) << "}\n";
        return code.str();
    }

    if (node.name == "Flexbox" || node.name == "flexbox" || node.name == "Flex" || node.name == "flex") {
        code << Indent(indent) << "{\n";
        EmitFlexbox(code, node, indent + 1, "scene");
        code << Indent(indent) << "}\n";
        return code.str();
    }

    if (node.name == "Grid" || node.name == "grid") {
        code << Indent(indent) << "{\n";
        EmitGrid(code, node, indent + 1, "scene");
        code << Indent(indent) << "}\n";
        return code.str();
    }

    if (node.name == "Position" || node.name == "position" || node.name == "Positioned" || node.name == "positioned") {
        code << Indent(indent) << "{\n";
        EmitPosition(code, node, indent + 1, "scene");
        code << Indent(indent) << "}\n";
        return code.str();
    }

    if (node.name == "BrandLogo") {
        code << Indent(indent) << "{\n";
        EmitBrandLogo(code, node, indent + 1, "brandLogo");
        code << Indent(indent + 1) << "scene.AddBrandLogo(std::move(brandLogo));\n";
        code << Indent(indent) << "}\n";
        return code.str();
    }

    if (node.name == "Desktop") {
        EmitDesktop(code, node, indent);
        return code.str();
    }

    if (node.name == "TitleBar" || node.name == "Navbar" || node.name == "NavBar") {
        code << Indent(indent) << "{\n";
        EmitTitleBar(code, node, indent + 1, "titleBar");
        code << Indent(indent + 1) << "scene.AddTitleBar(std::move(titleBar));\n";
        code << Indent(indent) << "}\n";
        return code.str();
    }

    if (node.name == "Sidebar" || node.name == "Pane") {
        code << Indent(indent) << "{\n";
        EmitSidebar(code, node, indent + 1, "sidebar");
        code << Indent(indent + 1) << "scene.AddSidebar(std::move(sidebar));\n";
        code << Indent(indent) << "}\n";
        return code.str();
    }

    if (IsPaneItem(node.name)) {
        code << Indent(indent) << "{\n";
        EmitSidebarItem(code, node, indent + 1, "sidebarItem");
        code << Indent(indent + 1) << "scene.AddSidebarItem(std::move(sidebarItem));\n";
        code << Indent(indent) << "}\n";
        return code.str();
    }

    if (node.name == "Separator" || node.name == "Divider") {
        code << Indent(indent) << "{\n";
        EmitSeparator(code, node, indent + 1, "separator");
        code << Indent(indent + 1) << "scene.AddSeparator(std::move(separator));\n";
        code << Indent(indent) << "}\n";
        return code.str();
    }

    if (node.name == "ThemeToggle") {
        code << Indent(indent) << "{\n";
        EmitThemeToggle(code, node, indent + 1, "themeToggle");
        code << Indent(indent + 1) << "scene.AddThemeToggle(std::move(themeToggle));\n";
        code << Indent(indent) << "}\n";
        return code.str();
    }

    if (node.name == "ThemeIconToggle") {
        code << Indent(indent) << "{\n";
        EmitThemeIconToggle(code, node, indent + 1, "themeIconToggle");
        code << Indent(indent + 1) << "scene.AddThemeIconToggle(std::move(themeIconToggle));\n";
        code << Indent(indent) << "}\n";
        return code.str();
    }

    if (node.name == "Shape" || node.name == "MotionShape") {
        const std::string shapeName = NextShapeName();
        code << Indent(indent) << "{\n";
        EmitShape(code, node, indent + 1, shapeName);
        code << Indent(indent + 1) << "scene.AddMotionShape(std::move(" << shapeName << "));\n";
        code << Indent(indent) << "}\n";
        return code.str();
    }

    if (TryEmitGraphic(code, node, indent + 1, "scene", false)) {
        return code.str();
    }

    if (!node.name.empty()) {
        std::cerr << "Skipping unknown XML element <" << node.name << ">\n";
    }
    return code.str();
}

void XmlToCpp::EmitScrollbar(std::ostringstream& code, const XmlNode& node, int indent, const std::string& targetVar) const {
    code << Indent(indent) << "Robo::Scrollbar " << targetVar << ";\n";
    code << Indent(indent) << "Robo::ScrollbarStyle style;\n";

    // Presets come first so that explicit attributes emitted below win.
    const auto variantIt = node.attributes.find("variant");
    if (variantIt != node.attributes.end() &&
        (variantIt->second == "thin" || variantIt->second == "overlay")) {
        code << Indent(indent) << "style.thickness = 14.0;\n";
        code << Indent(indent) << "style.cornerRadius = 7.0;\n";
        code << Indent(indent) << "style.minThumbLength = 28.0;\n";
        code << Indent(indent) << "style.arrowSize = 16.0;\n";
        code << Indent(indent) << "style.showArrows = true;\n";
    }

    for (const auto& attribute : node.attributes) {
        if (attribute.first == "orientation" || attribute.first == "direction") {
            code << Indent(indent) << "style.orientation = Robo::ParseScrollbarOrientation(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "thickness" || attribute.first == "size") {
            code << Indent(indent) << "style.thickness = " << attribute.second << ";\n";
        } else if (attribute.first == "length" || attribute.first == "trackLength") {
            code << Indent(indent) << "style.length = " << attribute.second << ";\n";
        } else if (attribute.first == "cornerRadius" || attribute.first == "radius") {
            code << BindFloat(indent, "style", "cornerRadius", attribute.second, node.name);
        } else if (attribute.first == "minThumbLength" || attribute.first == "minThumb") {
            code << Indent(indent) << "style.minThumbLength = " << attribute.second << ";\n";
        } else if (attribute.first == "trackColor" || attribute.first == "track") {
            code << BindColor(indent, "style", "trackColor", attribute.second, node.name);
            code << Indent(indent) << "style.hasCustomTrackColor = true;\n";
        } else if (attribute.first == "thumbColor" || attribute.first == "color") {
            code << BindColor(indent, "style", "thumbColor", attribute.second, node.name);
            code << Indent(indent) << "style.hasCustomThumbColor = true;\n";
        } else if (attribute.first == "thumbHoverColor") {
            code << BindColor(indent, "style", "thumbHoverColor", attribute.second, node.name);
            code << Indent(indent) << "style.hasCustomThumbHoverColor = true;\n";
        } else if (attribute.first == "thumbActiveColor") {
            code << BindColor(indent, "style", "thumbActiveColor", attribute.second, node.name);
            code << Indent(indent) << "style.hasCustomThumbActiveColor = true;\n";
        } else if (attribute.first == "opacity") {
            code << Indent(indent) << "style.opacity = " << attribute.second << ";\n";
        } else if (attribute.first == "visibility" || attribute.first == "showWhen") {
            code << Indent(indent) << "style.visibility = Robo::ParseScrollbarVisibility(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "arrows" || attribute.first == "showArrows" ||
                   attribute.first == "arrow") {
            EmitBool(code, indent, "style.showArrows", attribute.second, false, true);
        } else if (attribute.first == "arrowSize") {
            code << Indent(indent) << "style.arrowSize = " << attribute.second << ";\n";
        } else if (attribute.first == "arrowGap") {
            code << Indent(indent) << "style.arrowGap = " << attribute.second << ";\n";
        } else if (attribute.first == "arrowColor") {
            code << BindColor(indent, "style", "arrowColor", attribute.second, node.name);
            code << Indent(indent) << "style.hasCustomArrowColor = true;\n";
        } else if (attribute.first == "arrowHoverColor") {
            code << BindColor(indent, "style", "arrowHoverColor", attribute.second, node.name);
            code << Indent(indent) << "style.hasCustomArrowHoverColor = true;\n";
        } else if (attribute.first == "arrowActiveColor") {
            code << BindColor(indent, "style", "arrowActiveColor", attribute.second, node.name);
            code << Indent(indent) << "style.hasCustomArrowActiveColor = true;\n";
        } else if (attribute.first == "autoHide" || attribute.first == "autohide") {
            EmitBool(code, indent, "style.autoHide", attribute.second, false, false);
        } else if (attribute.first == "autoHideDelay" || attribute.first == "hideDelay" ||
                   attribute.first == "hideAfter") {
            code << Indent(indent) << "style.autoHideDelay = " << attribute.second << ";\n";
        } else if (attribute.first == "fadeDuration" || attribute.first == "fade") {
            code << Indent(indent) << "style.fadeDuration = " << attribute.second << ";\n";
        } else if (attribute.first == "hiddenOpacity" || attribute.first == "idleOpacity") {
            code << Indent(indent) << "style.hiddenOpacity = " << attribute.second << ";\n";
        }
    }

    code << Indent(indent) << targetVar << ".SetStyle(style);\n";
    code << Indent(indent) << targetVar << ".ApplyVariantDefaults();\n";

    for (const auto& attribute : node.attributes) {
        if (attribute.first == "value") {
            code << Indent(indent) << targetVar << ".SetValue(" << attribute.second << ");\n";
        } else if (attribute.first == "contentSize" || attribute.first == "content") {
            code << Indent(indent) << targetVar << ".SetContentSize(" << attribute.second << ");\n";
        } else if (attribute.first == "viewportSize" || attribute.first == "viewport" ||
                   attribute.first == "pageSize") {
            code << Indent(indent) << targetVar << ".SetViewportSize(" << attribute.second << ");\n";
        } else if (attribute.first == "thumbRatio" || attribute.first == "thumbSize") {
            code << Indent(indent) << targetVar << ".SetThumbRatio(" << attribute.second << ");\n";
        }
    }
}

void XmlToCpp::EmitContainer(std::ostringstream& code,
                             const XmlNode& node,
                             int indent,
                             const std::string& targetVar,
                             bool emitChildren) const {
    const std::string containerVar = (targetVar == "scene") ? "container" : targetVar;
    code << Indent(indent) << "Robo::Container " << containerVar << ";\n";
    code << Indent(indent) << "Robo::ContainerStyle style;\n";
    // A container hides its bar unless the content overflows, like an overlay.
    code << Indent(indent) << "style.scrollbarStyle.visibility = Robo::ScrollbarVisibility::Auto;\n";

    // Presets come first so the explicit attributes emitted below win.
    const auto variantIt = node.attributes.find("variant");
    if (variantIt != node.attributes.end() &&
        (variantIt->second == "thin" || variantIt->second == "overlay")) {
        code << Indent(indent) << "style.scrollbarStyle.thickness = 14.0;\n";
        code << Indent(indent) << "style.scrollbarStyle.cornerRadius = 7.0;\n";
        code << Indent(indent) << "style.scrollbarStyle.minThumbLength = 28.0;\n";
        code << Indent(indent) << "style.scrollbarStyle.arrowSize = 16.0;\n";
        code << Indent(indent) << "style.scrollbarStyle.showArrows = true;\n";
    }

    for (const auto& attribute : node.attributes) {
        if (attribute.first == "layout") {
            code << Indent(indent) << "style.layout = Robo::ParseContainerLayout(\"" << EscapeCpp(attribute.second)
                 << "\");\n";
        } else if (attribute.first == "orientation" || attribute.first == "direction" || attribute.first == "axis" ||
            attribute.first == "scrollAxis") {
            code << Indent(indent) << "style.orientation = Robo::ParseContainerOrientation(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "width") {
            code << BindLength(indent, "style", "width", attribute.second, node.name);
        } else if (attribute.first == "height") {
            code << BindLength(indent, "style", "height", attribute.second, node.name);
        } else if (attribute.first == "minWidth") {
            code << Indent(indent) << "style.minWidth = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "maxWidth") {
            code << Indent(indent) << "style.maxWidth = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "minHeight") {
            code << Indent(indent) << "style.minHeight = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "maxHeight") {
            code << Indent(indent) << "style.maxHeight = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "padding") {
code << BindSides(indent, "style", "padding", attribute.second, node.name);
        } else if (attribute.first == "paddingX") {
code << BindSides(indent, "style", "paddingX", attribute.second, node.name);
        } else if (attribute.first == "paddingY") {
code << BindSides(indent, "style", "paddingY", attribute.second, node.name);
        } else if (attribute.first == "gap" || attribute.first == "itemGap") {
            code << BindFloat(indent, "style", "gap", attribute.second, node.name);
        } else if (attribute.first == "background" || attribute.first == "backgroundColor") {
            code << BindColor(indent, "style", "backgroundColor", attribute.second, node.name);
        } else if (attribute.first == "borderColor") {
            code << BindColor(indent, "style", "borderColor", attribute.second, node.name);
        } else if (attribute.first == "borderWidth") {
            code << Indent(indent) << "style.borderWidth = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "cornerRadius" || attribute.first == "borderRadius") {
            code << BindFloat(indent, "style", "cornerRadius", attribute.second, node.name);
        } else if (attribute.first == "opacity") {
            code << Indent(indent) << "style.opacity = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "clip" || attribute.first == "clipToBounds") {
            EmitBool(code, indent, "style.clipToBounds", attribute.second, true, true);
        } else if (attribute.first == "showScrollbar" || attribute.first == "scrollbar") {
            EmitBool(code, indent, "style.showScrollbar", attribute.second, true, true);
        } else if (attribute.first == "scrollbarGap") {
            code << Indent(indent) << "style.scrollbarGap = " << attribute.second << ";\n";
        } else if (attribute.first == "contentSize" || attribute.first == "contentExtent" ||
                   attribute.first == "scrollSize") {
            code << Indent(indent) << "style.contentExtent = " << attribute.second << ";\n";
        } else if (attribute.first == "viewportSize" || attribute.first == "viewportExtent") {
            code << Indent(indent) << "style.viewportExtent = " << attribute.second << ";\n";
        } else if (attribute.first == "variant") {
            // Handled by the preset above.
        } else if (attribute.first == "thickness" || attribute.first == "size") {
            code << Indent(indent) << "style.scrollbarStyle.thickness = " << attribute.second << ";\n";
        } else if (attribute.first == "barRadius") {
            code << Indent(indent) << "style.scrollbarStyle.cornerRadius = " << attribute.second << ";\n";
        } else if (attribute.first == "minThumbLength") {
            code << Indent(indent) << "style.scrollbarStyle.minThumbLength = " << attribute.second << ";\n";
        } else if (attribute.first == "trackColor") {
            code << BindColor(indent, "style.scrollbarStyle", "trackColor", attribute.second, node.name);
            code << Indent(indent) << "style.scrollbarStyle.hasCustomTrackColor = true;\n";
        } else if (attribute.first == "thumbColor") {
            code << BindColor(indent, "style.scrollbarStyle", "thumbColor", attribute.second, node.name);
            code << Indent(indent) << "style.scrollbarStyle.hasCustomThumbColor = true;\n";
        } else if (attribute.first == "thumbHoverColor") {
            code << BindColor(indent, "style.scrollbarStyle", "thumbHoverColor", attribute.second, node.name);
            code << Indent(indent) << "style.scrollbarStyle.hasCustomThumbHoverColor = true;\n";
        } else if (attribute.first == "thumbActiveColor") {
            code << BindColor(indent, "style.scrollbarStyle", "thumbActiveColor", attribute.second, node.name);
            code << Indent(indent) << "style.scrollbarStyle.hasCustomThumbActiveColor = true;\n";
        } else if (attribute.first == "arrows" || attribute.first == "showArrows" || attribute.first == "arrow") {
            EmitBool(code, indent, "style.scrollbarStyle.showArrows", attribute.second, false, true);
        } else if (attribute.first == "arrowColor") {
            code << BindColor(indent, "style.scrollbarStyle", "arrowColor", attribute.second, node.name);
            code << Indent(indent) << "style.scrollbarStyle.hasCustomArrowColor = true;\n";
        } else if (attribute.first == "autoHide" || attribute.first == "autohide") {
            EmitBool(code, indent, "style.scrollbarStyle.autoHide", attribute.second, false, false);
        } else if (attribute.first == "autoHideDelay") {
            code << Indent(indent) << "style.scrollbarStyle.autoHideDelay = " << attribute.second << ";\n";
        } else if (attribute.first == "fadeDuration") {
            code << Indent(indent) << "style.scrollbarStyle.fadeDuration = " << attribute.second << ";\n";
        } else if (attribute.first == "hiddenOpacity") {
            code << Indent(indent) << "style.scrollbarStyle.hiddenOpacity = " << attribute.second << ";\n";
        } else if (attribute.first == "visibility") {
            code << Indent(indent) << "style.scrollbarStyle.visibility = Robo::ParseScrollbarVisibility(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        }
    }

    EmitSelectable(code, indent, "style", node);
    code << Indent(indent) << containerVar << ".SetStyle(style);\n";
    code << Indent(indent) << containerVar << ".RefreshVariantDefaults();\n";
    const auto when = node.attributes.find("when");
    if (when != node.attributes.end()) {
        code << Indent(indent) << containerVar << ".SetShownWhen(\"" << EscapeCpp(when->second) << "\");\n";
    }

    std::string layout = "stack";
    const auto layoutIt = node.attributes.find("layout");
    if (layoutIt != node.attributes.end()) {
        layout = layoutIt->second;
    }

    if (emitChildren) {
        if (layout == "flex" || layout == "flexbox") {
            const std::string flexName = containerVar + "Flex";
            EmitFlexbox(code, node, indent, flexName);
            code << Indent(indent) << flexName << ".GetStyle().width = \"auto\";\n";
            code << Indent(indent) << flexName << ".GetStyle().height = \"auto\";\n";
            code << Indent(indent) << flexName << ".GetStyle().paddingTop = 0.0f;\n";
            code << Indent(indent) << flexName << ".GetStyle().paddingRight = 0.0f;\n";
            code << Indent(indent) << flexName << ".GetStyle().paddingBottom = 0.0f;\n";
            code << Indent(indent) << flexName << ".GetStyle().paddingLeft = 0.0f;\n";
            code << Indent(indent) << flexName << ".GetStyle().backgroundColor = Robo::Color::FromRgba(0, 0, 0, 0);\n";
            code << Indent(indent) << flexName << ".GetStyle().borderWidth = 0.0f;\n";
            code << Indent(indent) << containerVar << ".SetFlexbox(std::move(" << flexName << "));\n";
        } else if (layout == "grid") {
            const std::string gridName = containerVar + "Grid";
            EmitGrid(code, node, indent, gridName);
            code << Indent(indent) << gridName << ".GetStyle().width = \"auto\";\n";
            code << Indent(indent) << gridName << ".GetStyle().height = \"auto\";\n";
            code << Indent(indent) << gridName << ".GetStyle().paddingTop = 0.0f;\n";
            code << Indent(indent) << gridName << ".GetStyle().paddingRight = 0.0f;\n";
            code << Indent(indent) << gridName << ".GetStyle().paddingBottom = 0.0f;\n";
            code << Indent(indent) << gridName << ".GetStyle().paddingLeft = 0.0f;\n";
            code << Indent(indent) << gridName << ".GetStyle().backgroundColor = Robo::Color::FromRgba(0, 0, 0, 0);\n";
            code << Indent(indent) << gridName << ".GetStyle().borderWidth = 0.0f;\n";
            code << Indent(indent) << containerVar << ".SetGrid(std::move(" << gridName << "));\n";
        } else {
            for (const XmlNode& child : node.children) {
                if (!child.isText) {
                    EmitPositionChild(code, child, indent, containerVar + ".GetContent()");
                }
            }
        }
    }

    for (const auto& attribute : node.attributes) {
        if (attribute.first == "value" || attribute.first == "scroll" || attribute.first == "scrollValue") {
            code << Indent(indent) << containerVar << ".SetScrollValue(" << attribute.second << ");\n";
        }
    }

    if (emitChildren && targetVar == "scene") {
        code << Indent(indent) << "scene.AddContainer(std::move(" << containerVar << "));\n";
    }
}

void XmlToCpp::EmitFlexbox(std::ostringstream& code, const XmlNode& node, int indent, const std::string& targetVar) const {
    const std::string flexVar = (targetVar == "scene") ? "flexbox" : targetVar;
    code << Indent(indent) << "Robo::Flexbox " << flexVar << ";\n";
    code << Indent(indent) << "Robo::FlexboxStyle style;\n";

    for (const auto& attribute : node.attributes) {
        if (attribute.first == "direction" || attribute.first == "flexDirection") {
            EmitEnum(code, indent, "style.direction", attribute.second, MapFlexDirection, "ParseFlexDirection",
                     "Robo::FlexDirection::Row");
        } else if (attribute.first == "justify" || attribute.first == "justifyContent") {
            EmitEnum(code, indent, "style.justify", attribute.second, MapFlexJustify, "ParseFlexJustify",
                     "Robo::FlexJustify::FlexStart");
        } else if (attribute.first == "align" || attribute.first == "alignItems") {
            EmitEnum(code, indent, "style.align", attribute.second, MapFlexAlign, "ParseFlexAlign",
                     "Robo::FlexAlign::Stretch");
        } else if (attribute.first == "wrap" || attribute.first == "flexWrap") {
            EmitEnum(code, indent, "style.wrap", attribute.second, MapFlexWrap, "ParseFlexWrap", "Robo::FlexWrap::NoWrap");
        } else if (attribute.first == "gap") {
            code << BindFloat(indent, "style", "gap", attribute.second, node.name);
        } else if (attribute.first == "gapX" || attribute.first == "columnGap") {
            code << BindFloat(indent, "style", "columnGap", attribute.second, node.name);
        } else if (attribute.first == "gapY" || attribute.first == "rowGap") {
            code << BindFloat(indent, "style", "rowGap", attribute.second, node.name);
        } else if (attribute.first == "width") {
            code << BindLength(indent, "style", "width", attribute.second, node.name);
        } else if (attribute.first == "height") {
            code << BindLength(indent, "style", "height", attribute.second, node.name);
        } else if (attribute.first == "minWidth") {
            code << Indent(indent) << "style.minWidth = " << attribute.second << ";\n";
        } else if (attribute.first == "maxWidth") {
            code << Indent(indent) << "style.maxWidth = " << attribute.second << ";\n";
        } else if (attribute.first == "padding") {
code << BindSides(indent, "style", "padding", attribute.second, node.name);
        } else if (attribute.first == "paddingX") {
code << BindSides(indent, "style", "paddingX", attribute.second, node.name);
        } else if (attribute.first == "paddingY") {
code << BindSides(indent, "style", "paddingY", attribute.second, node.name);
        } else if (attribute.first == "margin") {
code << BindSides(indent, "style", "margin", attribute.second, node.name);
        } else if (attribute.first == "background" || attribute.first == "backgroundColor") {
            code << BindColor(indent, "style", "backgroundColor", attribute.second, node.name);
        } else if (attribute.first == "borderColor") {
            code << BindColor(indent, "style", "borderColor", attribute.second, node.name);
        } else if (attribute.first == "borderWidth") {
            code << Indent(indent) << "style.borderWidth = " << attribute.second << ";\n";
        } else if (attribute.first == "cornerRadius" || attribute.first == "borderRadius") {
            code << BindFloat(indent, "style", "cornerRadius", attribute.second, node.name);
        } else if (attribute.first == "opacity") {
            code << Indent(indent) << "style.opacity = " << attribute.second << ";\n";
        }
    }

    EmitSelectable(code, indent, "style", node);
    code << Indent(indent) << flexVar << ".SetStyle(style);\n";

    for (const XmlNode& child : node.children) {
        if (child.isText) {
            std::string content = child.text;
            bool hasContent = false;
            for (char c : content) {
                if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                    hasContent = true;
                    break;
                }
            }
            if (hasContent) {
                code << Indent(indent) << "{\n";
                code << Indent(indent + 1) << "Robo::Text rawText;\n";
                code << Indent(indent + 1) << "rawText.AddPlain(\"" << EscapeCpp(content) << "\");\n";
                code << Indent(indent + 1) << flexVar << ".AddText(std::move(rawText));\n";
                code << Indent(indent) << "}\n";
            }
            continue;
        }

        EmitFlexChild(code, child, indent, flexVar);
    }

    if (targetVar == "scene") {
        code << Indent(indent) << "scene.AddFlexbox(std::move(" << flexVar << "));\n";
    }
}

void XmlToCpp::EmitFlexChild(std::ostringstream& code, const XmlNode& child, int indent, const std::string& parentVar) const {
    code << Indent(indent) << "{\n";
    code << Indent(indent + 1) << "Robo::FlexItemProps props;\n";
    for (const auto& attribute : child.attributes) {
        if (attribute.first == "flex" || attribute.first == "flexGrow") {
            code << Indent(indent + 1) << "props.flexGrow = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "flexShrink") {
            code << Indent(indent + 1) << "props.flexShrink = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "flexBasis") {
            if (!attribute.second.empty() && attribute.second.back() == '%') {
                code << Indent(indent + 1) << "props.flexBasisString = \"" << EscapeCpp(attribute.second) << "\";\n";
            } else {
                code << Indent(indent + 1) << "props.flexBasis = " << FloatLiteral(attribute.second) << ";\n";
            }
        } else if (attribute.first == "alignSelf") {
            EmitEnum(code, indent + 1, "props.alignSelf", attribute.second, MapFlexAlign, "ParseFlexAlign",
                     "Robo::FlexAlign::Auto");
        } else if (attribute.first == "width") {
            if (ExtractToken(attribute.second).empty()) {
                if (!IsFreeLength(attribute.second)) {
                    WarnLiteral(child.name, "width", attribute.second);
                }
                code << BindPropsLength(indent + 1, "props", "width", attribute.second);
            }
        } else if (attribute.first == "height") {
            if (ExtractToken(attribute.second).empty()) {
                if (!IsFreeLength(attribute.second)) {
                    WarnLiteral(child.name, "height", attribute.second);
                }
                code << BindPropsLength(indent + 1, "props", "height", attribute.second);
            }
        } else if (attribute.first == "minWidth") {
            if (ExtractToken(attribute.second).empty()) {
                code << BindPropsLength(indent + 1, "props", "minWidth", attribute.second);
            }
        } else if (attribute.first == "maxWidth") {
            if (ExtractToken(attribute.second).empty()) {
                code << BindPropsLength(indent + 1, "props", "maxWidth", attribute.second);
            }
        } else if (attribute.first == "minHeight") {
            if (ExtractToken(attribute.second).empty()) {
                code << BindPropsLength(indent + 1, "props", "minHeight", attribute.second);
            }
        } else if (attribute.first == "maxHeight") {
            if (ExtractToken(attribute.second).empty()) {
                code << BindPropsLength(indent + 1, "props", "maxHeight", attribute.second);
            }
        } else if (attribute.first == "margin") {
            code << Indent(indent + 1) << "props.marginTop = " << attribute.second << ";\n";
            code << Indent(indent + 1) << "props.marginBottom = " << attribute.second << ";\n";
            code << Indent(indent + 1) << "props.marginLeft = " << attribute.second << ";\n";
            code << Indent(indent + 1) << "props.marginRight = " << attribute.second << ";\n";
        }
    }

    if (child.name == "Button" || child.name == "IconButton") {
        code << Indent(indent + 1) << "Robo::Button button;\n";
        code << Indent(indent + 1) << "Robo::ButtonStyle style;\n";
        std::string label;
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "title" || attribute.first == "label") {
                label = attribute.second;
            } else if (attribute.first == "variant" || attribute.first == "style") {
                EmitEnum(code, indent + 1, "style.variant", attribute.second, MapButtonVariant, "ParseButtonVariant",
                         "Robo::ButtonVariant::Filled");
            } else if (attribute.first == "fontSize") {
                code << BindFloat(indent + 1, "style", "fontSize", attribute.second, child.name);
            } else if (attribute.first == "fontWeight") {
                code << BindWeight(indent + 1, "style", attribute.second, child.name);
            } else if (attribute.first == "color") {
                code << BindColor(indent + 1, "style", "textColor", attribute.second, child.name);
                code << Indent(indent + 1) << "style.hasCustomTextColor = true;\n";
            } else if (attribute.first == "background" || attribute.first == "backgroundColor") {
                code << BindColor(indent + 1, "style", "backgroundColor", attribute.second, child.name);
                code << Indent(indent + 1) << "style.hasCustomBackground = true;\n";
            } else if (attribute.first == "cornerRadius") {
                code << BindFloat(indent + 1, "style", "cornerRadius", attribute.second, child.name);
            } else if (attribute.first == "paddingX") {
                code << BindFloat(indent + 1, "style", "paddingX", attribute.second, child.name);
            } else if (attribute.first == "paddingY") {
                code << BindFloat(indent + 1, "style", "paddingY", attribute.second, child.name);
            }
        }
        if (label.empty()) {
            for (const XmlNode& c : child.children) {
                if (c.isText) {
                    label += c.text;
                }
            }
        }
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "button.SetStyle(style);\n";
        code << Indent(indent + 1) << "button.ApplyVariantDefaults();\n";
        code << Indent(indent + 1) << "button.SetLabel(\"" << EscapeCpp(label) << "\");\n";
        EmitButtonFlags(code, indent + 1, child);
        code << Indent(indent + 1) << parentVar << ".AddButton(std::move(button), props);\n";
    } else if (child.name == "Input" || child.name == "TextField" || child.name == "TextInput") {
        code << Indent(indent + 1) << "Robo::Input input;\n";
        code << Indent(indent + 1) << "Robo::InputStyle style;\n";
        std::string placeholder;
        std::string value;
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "placeholder" || attribute.first == "hint") {
                placeholder = attribute.second;
            } else if (attribute.first == "value" || attribute.first == "text") {
                value = attribute.second;
            } else if (attribute.first == "variant" || attribute.first == "style") {
                code << Indent(indent + 1) << "style.variant = Robo::ParseInputVariant(\"" << EscapeCpp(attribute.second) << "\");\n";
            } else if (attribute.first == "fontSize") {
                code << BindFloat(indent + 1, "style", "fontSize", attribute.second, child.name);
            } else if (attribute.first == "color") {
                code << BindColor(indent + 1, "style", "textColor", attribute.second, child.name);
                code << Indent(indent + 1) << "style.hasCustomTextColor = true;\n";
            } else if (attribute.first == "background" || attribute.first == "backgroundColor") {
                code << BindColor(indent + 1, "style", "backgroundColor", attribute.second, child.name);
                code << Indent(indent + 1) << "style.hasCustomBackground = true;\n";
            } else if (attribute.first == "cornerRadius") {
                code << BindFloat(indent + 1, "style", "cornerRadius", attribute.second, child.name);
            }
        }
        if (value.empty()) {
            for (const XmlNode& c : child.children) {
                if (c.isText) {
                    value += c.text;
                }
            }
        }
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "input.SetStyle(style);\n";
        code << Indent(indent + 1) << "input.ApplyVariantDefaults();\n";
        code << Indent(indent + 1) << "input.SetPlaceholder(\"" << EscapeCpp(placeholder) << "\");\n";
        code << Indent(indent + 1) << "input.SetValue(\"" << EscapeCpp(value) << "\");\n";
        code << Indent(indent + 1) << parentVar << ".AddInput(std::move(input), props);\n";
    } else if (child.name == "Scrollbar" || child.name == "scrollbar") {
        EmitScrollbar(code, child, indent + 1, "scrollbar");
        code << Indent(indent + 1) << parentVar << ".AddScrollbar(std::move(scrollbar), props);\n";
    } else if (child.name == "Container" || child.name == "container") {
        const std::string nestedContainer = "nestedContainer" + std::to_string(indent + 1);
        EmitContainer(code, child, indent + 1, nestedContainer);
        code << Indent(indent + 1) << parentVar << ".AddContainer(std::move(" << nestedContainer
             << "), props);\n";
    } else if (child.name == "Text") {
        code << Indent(indent + 1) << "Robo::Text text;\n";
        code << Indent(indent + 1) << "Robo::TextStyle style;\n";
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "fontSize") {
                code << BindFloat(indent + 1, "style", "fontSize", attribute.second, child.name);
            } else if (attribute.first == "color") {
                code << BindColor(indent + 1, "style", "color", attribute.second, child.name);
            } else if (attribute.first == "fontWeight") {
                code << BindWeight(indent + 1, "style", attribute.second, child.name);
            }
        }
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "text.SetStyle(style);\n";
        for (const XmlNode& c : child.children) {
            if (c.isText) {
                code << Indent(indent + 1) << "text.AddPlain(\"" << EscapeCpp(c.text) << "\");\n";
            }
        }
        code << Indent(indent + 1) << parentVar << ".AddText(std::move(text), props);\n";
    } else if (child.name == "Heading" || child.name == "heading" ||
               (child.name.size() == 2 && (child.name[0] == 'H' || child.name[0] == 'h') && child.name[1] >= '1' && child.name[1] <= '6') ||
               (child.name.size() == 8 && (child.name.compare(0, 7, "Heading") == 0 || child.name.compare(0, 7, "heading") == 0) && child.name[7] >= '1' && child.name[7] <= '6')) {
        int level = 1;
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "level") {
                level = std::atoi(attribute.second.c_str());
            }
        }
        if (child.name.size() == 2 && child.name[1] >= '1' && child.name[1] <= '6') {
            level = child.name[1] - '0';
        }
        if (child.name.size() == 8 && child.name[7] >= '1' && child.name[7] <= '6') {
            level = child.name[7] - '0';
        }
        code << Indent(indent + 1) << "Robo::Heading heading(" << level << ");\n";
        code << Indent(indent + 1) << "Robo::TextStyle style = heading.GetStyle();\n";
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "color") {
                code << BindColor(indent + 1, "style", "color", attribute.second, child.name);
            }
        }
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "heading.SetStyle(style);\n";
        for (const XmlNode& c : child.children) {
            if (c.isText) {
                code << Indent(indent + 1) << "heading.AddPlain(\"" << EscapeCpp(c.text) << "\");\n";
            }
        }
        code << Indent(indent + 1) << parentVar << ".AddText(std::move(heading), props);\n";
    } else if (child.name == "Paragraph" || child.name == "paragraph" || child.name == "P" || child.name == "p") {
        code << Indent(indent + 1) << "Robo::Paragraph paragraph;\n";
        code << Indent(indent + 1) << "Robo::TextStyle style = paragraph.GetStyle();\n";
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "color") {
                code << BindColor(indent + 1, "style", "color", attribute.second, child.name);
            }
        }
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "paragraph.SetStyle(style);\n";
        for (const XmlNode& c : child.children) {
            if (c.isText) {
                code << Indent(indent + 1) << "paragraph.AddPlain(\"" << EscapeCpp(c.text) << "\");\n";
            }
        }
        code << Indent(indent + 1) << parentVar << ".AddText(std::move(paragraph), props);\n";
    } else if (child.name == "Flexbox" || child.name == "flexbox" || child.name == "Flex" || child.name == "flex") {
        const std::string childFlexVar = "nestedFlex" + std::to_string(indent + 1);
        EmitFlexbox(code, child, indent + 1, childFlexVar);
        code << Indent(indent + 1) << parentVar << ".AddFlexbox(std::move(" << childFlexVar << "), props);\n";
    } else if (child.name == "Grid" || child.name == "grid") {
        const std::string childGridVar = "nestedGrid" + std::to_string(indent + 1);
        EmitGrid(code, child, indent + 1, childGridVar);
        code << Indent(indent + 1) << parentVar << ".AddGrid(std::move(childGridVar), props);\n";
    } else if (child.name == "BrandLogo") {
        EmitBrandLogo(code, child, indent + 1, "brandLogo");
        code << Indent(indent + 1) << parentVar << ".AddBrandLogo(std::move(brandLogo), props);\n";
    } else if (child.name == "TitleBar" || child.name == "Navbar" || child.name == "NavBar") {
        EmitTitleBar(code, child, indent + 1, "titleBar");
        code << Indent(indent + 1) << parentVar << ".AddTitleBar(std::move(titleBar), props);\n";
    } else if (child.name == "Sidebar" || child.name == "Pane") {
        EmitSidebar(code, child, indent + 1, "sidebar");
        code << Indent(indent + 1) << parentVar << ".AddSidebar(std::move(sidebar), props);\n";
    } else if (IsPaneItem(child.name)) {
        EmitSidebarItem(code, child, indent + 1, "sidebarItem");
        code << Indent(indent + 1) << parentVar << ".AddSidebarItem(std::move(sidebarItem), props);\n";
    } else if (child.name == "Separator" || child.name == "Divider") {
        EmitSeparator(code, child, indent + 1, "separator");
        code << Indent(indent + 1) << parentVar << ".AddSeparator(std::move(separator), props);\n";
    } else if (child.name == "ThemeToggle") {
        EmitThemeToggle(code, child, indent + 1, "themeToggle");
        code << Indent(indent + 1) << parentVar << ".AddThemeToggle(std::move(themeToggle), props);\n";
    } else if (child.name == "ThemeIconToggle") {
        EmitThemeIconToggle(code, child, indent + 1, "themeIconToggle");
        code << Indent(indent + 1) << parentVar << ".AddThemeIconToggle(std::move(themeIconToggle), props);\n";
    } else if (child.name == "Shape" || child.name == "MotionShape") {
        const std::string shapeName = NextShapeName();
        EmitShape(code, child, indent + 1, shapeName);
        code << Indent(indent + 1) << parentVar << ".AddMotionShape(std::move(" << shapeName << "), props);\n";
    } else if (!TryEmitGraphic(code, child, indent + 1, parentVar, true)) {
        if (!child.name.empty()) {
            std::cerr << "Skipping unknown XML element <" << child.name << ">\n";
        }
    }

    code << Indent(indent) << "}\n";
}

void XmlToCpp::EmitGrid(std::ostringstream& code, const XmlNode& node, int indent, const std::string& targetVar) const {
    const std::string gridVar = (targetVar == "scene") ? "grid" : targetVar;
    code << Indent(indent) << "Robo::Grid " << gridVar << ";\n";
    code << Indent(indent) << "Robo::GridStyle style;\n";

    for (const auto& attribute : node.attributes) {
        if (attribute.first == "columns" || attribute.first == "templateColumns" || attribute.first == "gridTemplateColumns") {
            code << Indent(indent) << "style.columnsString = \"" << EscapeCpp(attribute.second) << "\";\n";
            code << Indent(indent) << "style.templateColumns = Robo::ParseGridTracks(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "rows" || attribute.first == "templateRows" || attribute.first == "gridTemplateRows") {
            code << Indent(indent) << "style.rowsString = \"" << EscapeCpp(attribute.second) << "\";\n";
            code << Indent(indent) << "style.templateRows = Robo::ParseGridTracks(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "columnCount") {
            code << Indent(indent) << "style.columnCount = " << attribute.second << ";\n";
        } else if (attribute.first == "rowCount") {
            code << Indent(indent) << "style.rowCount = " << attribute.second << ";\n";
        } else if (attribute.first == "gap") {
            code << BindFloat(indent, "style", "gap", attribute.second, node.name);
        } else if (attribute.first == "gapX" || attribute.first == "columnGap") {
            code << BindFloat(indent, "style", "columnGap", attribute.second, node.name);
        } else if (attribute.first == "gapY" || attribute.first == "rowGap") {
            code << BindFloat(indent, "style", "rowGap", attribute.second, node.name);
        } else if (attribute.first == "autoFlow") {
            EmitEnum(code, indent, "style.autoFlow", attribute.second, MapGridAutoFlow, "ParseGridAutoFlow",
                     "Robo::GridAutoFlow::Row");
        } else if (attribute.first == "justifyItems" || attribute.first == "justify") {
            EmitEnum(code, indent, "style.justifyItems", attribute.second, MapGridAlignment, "ParseGridAlignment",
                     "Robo::GridAlignment::Stretch");
        } else if (attribute.first == "alignItems" || attribute.first == "align") {
            EmitEnum(code, indent, "style.alignItems", attribute.second, MapGridAlignment, "ParseGridAlignment",
                     "Robo::GridAlignment::Stretch");
        } else if (attribute.first == "justifyContent") {
            EmitEnum(code, indent, "style.justifyContent", attribute.second, MapGridJustifyContent,
                     "ParseGridJustifyContent", "Robo::GridJustifyContent::Start");
        } else if (attribute.first == "width") {
            code << BindLength(indent, "style", "width", attribute.second, node.name);
        } else if (attribute.first == "height") {
            code << BindLength(indent, "style", "height", attribute.second, node.name);
        } else if (attribute.first == "minWidth") {
            code << Indent(indent) << "style.minWidth = " << attribute.second << ";\n";
        } else if (attribute.first == "maxWidth") {
            code << Indent(indent) << "style.maxWidth = " << attribute.second << ";\n";
        } else if (attribute.first == "minHeight") {
            code << Indent(indent) << "style.minHeight = " << attribute.second << ";\n";
        } else if (attribute.first == "maxHeight") {
            code << Indent(indent) << "style.maxHeight = " << attribute.second << ";\n";
        } else if (attribute.first == "padding") {
code << BindSides(indent, "style", "padding", attribute.second, node.name);
        } else if (attribute.first == "paddingX") {
code << BindSides(indent, "style", "paddingX", attribute.second, node.name);
        } else if (attribute.first == "paddingY") {
code << BindSides(indent, "style", "paddingY", attribute.second, node.name);
        } else if (attribute.first == "margin") {
code << BindSides(indent, "style", "margin", attribute.second, node.name);
        } else if (attribute.first == "background" || attribute.first == "backgroundColor") {
            code << BindColor(indent, "style", "backgroundColor", attribute.second, node.name);
        } else if (attribute.first == "borderColor") {
            code << BindColor(indent, "style", "borderColor", attribute.second, node.name);
        } else if (attribute.first == "borderWidth") {
            code << Indent(indent) << "style.borderWidth = " << attribute.second << ";\n";
        } else if (attribute.first == "cornerRadius" || attribute.first == "borderRadius") {
            code << BindFloat(indent, "style", "cornerRadius", attribute.second, node.name);
        } else if (attribute.first == "opacity") {
            code << Indent(indent) << "style.opacity = " << attribute.second << ";\n";
        }
    }

    EmitSelectable(code, indent, "style", node);
    code << Indent(indent) << gridVar << ".SetStyle(style);\n";

    for (const XmlNode& child : node.children) {
        if (child.isText) {
            std::string content = child.text;
            bool hasContent = false;
            for (char c : content) {
                if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                    hasContent = true;
                    break;
                }
            }
            if (hasContent) {
                code << Indent(indent) << "{\n";
                code << Indent(indent + 1) << "Robo::Text rawText;\n";
                code << Indent(indent + 1) << "rawText.AddPlain(\"" << EscapeCpp(content) << "\");\n";
                code << Indent(indent + 1) << gridVar << ".AddText(std::move(rawText));\n";
                code << Indent(indent) << "}\n";
            }
            continue;
        }

        EmitGridChild(code, child, indent, gridVar);
    }

    if (targetVar == "scene") {
        code << Indent(indent) << "scene.AddGrid(std::move(" << gridVar << "));\n";
    }
}

void XmlToCpp::EmitGridChild(std::ostringstream& code, const XmlNode& child, int indent, const std::string& parentVar) const {
    code << Indent(indent) << "{\n";
    code << Indent(indent + 1) << "Robo::GridItemProps props;\n";
    for (const auto& attribute : child.attributes) {
        if (attribute.first == "column" || attribute.first == "gridColumn" || attribute.first == "col") {
            code << Indent(indent + 1) << "props.column = " << attribute.second << ";\n";
        } else if (attribute.first == "row" || attribute.first == "gridRow") {
            code << Indent(indent + 1) << "props.row = " << attribute.second << ";\n";
        } else if (attribute.first == "columnSpan" || attribute.first == "colSpan") {
            code << Indent(indent + 1) << "props.columnSpan = " << attribute.second << ";\n";
        } else if (attribute.first == "rowSpan") {
            code << Indent(indent + 1) << "props.rowSpan = " << attribute.second << ";\n";
        } else if (attribute.first == "justifySelf") {
            code << Indent(indent + 1) << "props.justifySelf = Robo::ParseGridAlignment(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "alignSelf") {
            code << Indent(indent + 1) << "props.alignSelf = Robo::ParseGridAlignment(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "width") {
            if (ExtractToken(attribute.second).empty()) {
                if (!IsFreeLength(attribute.second)) {
                    WarnLiteral(child.name, "width", attribute.second);
                }
                code << BindPropsLength(indent + 1, "props", "width", attribute.second);
            }
        } else if (attribute.first == "height") {
            if (ExtractToken(attribute.second).empty()) {
                if (!IsFreeLength(attribute.second)) {
                    WarnLiteral(child.name, "height", attribute.second);
                }
                code << BindPropsLength(indent + 1, "props", "height", attribute.second);
            }
        } else if (attribute.first == "minWidth") {
            if (ExtractToken(attribute.second).empty()) {
                code << BindPropsLength(indent + 1, "props", "minWidth", attribute.second);
            }
        } else if (attribute.first == "maxWidth") {
            if (ExtractToken(attribute.second).empty()) {
                code << BindPropsLength(indent + 1, "props", "maxWidth", attribute.second);
            }
        } else if (attribute.first == "minHeight") {
            if (ExtractToken(attribute.second).empty()) {
                code << BindPropsLength(indent + 1, "props", "minHeight", attribute.second);
            }
        } else if (attribute.first == "maxHeight") {
            if (ExtractToken(attribute.second).empty()) {
                code << BindPropsLength(indent + 1, "props", "maxHeight", attribute.second);
            }
        } else if (attribute.first == "margin") {
            code << Indent(indent + 1) << "props.marginTop = " << attribute.second << ";\n";
            code << Indent(indent + 1) << "props.marginBottom = " << attribute.second << ";\n";
            code << Indent(indent + 1) << "props.marginLeft = " << attribute.second << ";\n";
            code << Indent(indent + 1) << "props.marginRight = " << attribute.second << ";\n";
        }
    }

    if (child.name == "Button" || child.name == "IconButton") {
        code << Indent(indent + 1) << "Robo::Button button;\n";
        code << Indent(indent + 1) << "Robo::ButtonStyle style;\n";
        std::string label;
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "title" || attribute.first == "label") {
                label = attribute.second;
            } else if (attribute.first == "variant" || attribute.first == "style") {
                EmitEnum(code, indent + 1, "style.variant", attribute.second, MapButtonVariant, "ParseButtonVariant",
                         "Robo::ButtonVariant::Filled");
            } else if (attribute.first == "fontSize") {
                code << BindFloat(indent + 1, "style", "fontSize", attribute.second, child.name);
            } else if (attribute.first == "fontWeight") {
                code << BindWeight(indent + 1, "style", attribute.second, child.name);
            } else if (attribute.first == "color") {
                code << BindColor(indent + 1, "style", "textColor", attribute.second, child.name);
                code << Indent(indent + 1) << "style.hasCustomTextColor = true;\n";
            } else if (attribute.first == "background" || attribute.first == "backgroundColor") {
                code << BindColor(indent + 1, "style", "backgroundColor", attribute.second, child.name);
                code << Indent(indent + 1) << "style.hasCustomBackground = true;\n";
            } else if (attribute.first == "cornerRadius") {
                code << BindFloat(indent + 1, "style", "cornerRadius", attribute.second, child.name);
            } else if (attribute.first == "paddingX") {
                code << BindFloat(indent + 1, "style", "paddingX", attribute.second, child.name);
            } else if (attribute.first == "paddingY") {
                code << BindFloat(indent + 1, "style", "paddingY", attribute.second, child.name);
            }
        }
        if (label.empty()) {
            for (const XmlNode& c : child.children) {
                if (c.isText) {
                    label += c.text;
                }
            }
        }
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "button.SetStyle(style);\n";
        code << Indent(indent + 1) << "button.ApplyVariantDefaults();\n";
        code << Indent(indent + 1) << "button.SetLabel(\"" << EscapeCpp(label) << "\");\n";
        EmitButtonFlags(code, indent + 1, child);
        code << Indent(indent + 1) << parentVar << ".AddButton(std::move(button), props);\n";
    } else if (child.name == "Input" || child.name == "TextField" || child.name == "TextInput") {
        code << Indent(indent + 1) << "Robo::Input input;\n";
        code << Indent(indent + 1) << "Robo::InputStyle style;\n";
        std::string placeholder;
        std::string value;
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "placeholder" || attribute.first == "hint") {
                placeholder = attribute.second;
            } else if (attribute.first == "value" || attribute.first == "text") {
                value = attribute.second;
            } else if (attribute.first == "variant" || attribute.first == "style") {
                code << Indent(indent + 1) << "style.variant = Robo::ParseInputVariant(\"" << EscapeCpp(attribute.second) << "\");\n";
            } else if (attribute.first == "fontSize") {
                code << BindFloat(indent + 1, "style", "fontSize", attribute.second, child.name);
            } else if (attribute.first == "color") {
                code << BindColor(indent + 1, "style", "textColor", attribute.second, child.name);
                code << Indent(indent + 1) << "style.hasCustomTextColor = true;\n";
            } else if (attribute.first == "background" || attribute.first == "backgroundColor") {
                code << BindColor(indent + 1, "style", "backgroundColor", attribute.second, child.name);
                code << Indent(indent + 1) << "style.hasCustomBackground = true;\n";
            } else if (attribute.first == "cornerRadius") {
                code << BindFloat(indent + 1, "style", "cornerRadius", attribute.second, child.name);
            }
        }
        if (value.empty()) {
            for (const XmlNode& c : child.children) {
                if (c.isText) {
                    value += c.text;
                }
            }
        }
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "input.SetStyle(style);\n";
        code << Indent(indent + 1) << "input.ApplyVariantDefaults();\n";
        code << Indent(indent + 1) << "input.SetPlaceholder(\"" << EscapeCpp(placeholder) << "\");\n";
        code << Indent(indent + 1) << "input.SetValue(\"" << EscapeCpp(value) << "\");\n";
        code << Indent(indent + 1) << parentVar << ".AddInput(std::move(input), props);\n";
    } else if (child.name == "Scrollbar" || child.name == "scrollbar") {
        EmitScrollbar(code, child, indent + 1, "scrollbar");
        code << Indent(indent + 1) << parentVar << ".AddScrollbar(std::move(scrollbar), props);\n";
    } else if (child.name == "Container" || child.name == "container") {
        const std::string nestedContainer = "nestedContainer" + std::to_string(indent + 1);
        EmitContainer(code, child, indent + 1, nestedContainer);
        code << Indent(indent + 1) << parentVar << ".AddContainer(std::move(" << nestedContainer
             << "), props);\n";
    } else if (child.name == "Text") {
        code << Indent(indent + 1) << "Robo::Text text;\n";
        code << Indent(indent + 1) << "Robo::TextStyle style;\n";
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "fontSize") {
                code << BindFloat(indent + 1, "style", "fontSize", attribute.second, child.name);
            } else if (attribute.first == "color") {
                code << BindColor(indent + 1, "style", "color", attribute.second, child.name);
            } else if (attribute.first == "fontWeight") {
                code << BindWeight(indent + 1, "style", attribute.second, child.name);
            }
        }
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "text.SetStyle(style);\n";
        for (const XmlNode& c : child.children) {
            if (c.isText) {
                code << Indent(indent + 1) << "text.AddPlain(\"" << EscapeCpp(c.text) << "\");\n";
            }
        }
        code << Indent(indent + 1) << parentVar << ".AddText(std::move(text), props);\n";
    } else if (child.name == "Heading" || child.name == "heading" ||
               (child.name.size() == 2 && (child.name[0] == 'H' || child.name[0] == 'h') && child.name[1] >= '1' && child.name[1] <= '6') ||
               (child.name.size() == 8 && (child.name.compare(0, 7, "Heading") == 0 || child.name.compare(0, 7, "heading") == 0) && child.name[7] >= '1' && child.name[7] <= '6')) {
        int level = 1;
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "level") {
                level = std::atoi(attribute.second.c_str());
            }
        }
        if (child.name.size() == 2 && child.name[1] >= '1' && child.name[1] <= '6') {
            level = child.name[1] - '0';
        }
        if (child.name.size() == 8 && child.name[7] >= '1' && child.name[7] <= '6') {
            level = child.name[7] - '0';
        }
        code << Indent(indent + 1) << "Robo::Heading heading(" << level << ");\n";
        code << Indent(indent + 1) << "Robo::TextStyle style = heading.GetStyle();\n";
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "color") {
                code << BindColor(indent + 1, "style", "color", attribute.second, child.name);
            }
        }
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "heading.SetStyle(style);\n";
        for (const XmlNode& c : child.children) {
            if (c.isText) {
                code << Indent(indent + 1) << "heading.AddPlain(\"" << EscapeCpp(c.text) << "\");\n";
            }
        }
        code << Indent(indent + 1) << parentVar << ".AddText(std::move(heading), props);\n";
    } else if (child.name == "Paragraph" || child.name == "paragraph" || child.name == "P" || child.name == "p") {
        code << Indent(indent + 1) << "Robo::Paragraph paragraph;\n";
        code << Indent(indent + 1) << "Robo::TextStyle style = paragraph.GetStyle();\n";
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "color") {
                code << BindColor(indent + 1, "style", "color", attribute.second, child.name);
            }
        }
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "paragraph.SetStyle(style);\n";
        for (const XmlNode& c : child.children) {
            if (c.isText) {
                code << Indent(indent + 1) << "paragraph.AddPlain(\"" << EscapeCpp(c.text) << "\");\n";
            }
        }
        code << Indent(indent + 1) << parentVar << ".AddText(std::move(paragraph), props);\n";
    } else if (child.name == "Flexbox" || child.name == "flexbox" || child.name == "Flex" || child.name == "flex") {
        const std::string childFlexVar = "nestedFlex" + std::to_string(indent + 1);
        EmitFlexbox(code, child, indent + 1, childFlexVar);
        code << Indent(indent + 1) << parentVar << ".AddFlexbox(std::move(childFlexVar), props);\n";
    } else if (child.name == "Grid" || child.name == "grid") {
        const std::string childGridVar = "nestedGrid" + std::to_string(indent + 1);
        EmitGrid(code, child, indent + 1, childGridVar);
        code << Indent(indent + 1) << parentVar << ".AddGrid(std::move(childGridVar), props);\n";
    } else if (child.name == "BrandLogo") {
        EmitBrandLogo(code, child, indent + 1, "brandLogo");
        code << Indent(indent + 1) << parentVar << ".AddBrandLogo(std::move(brandLogo), props);\n";
    } else if (child.name == "TitleBar" || child.name == "Navbar" || child.name == "NavBar") {
        EmitTitleBar(code, child, indent + 1, "titleBar");
        code << Indent(indent + 1) << parentVar << ".AddTitleBar(std::move(titleBar), props);\n";
    } else if (child.name == "Sidebar" || child.name == "Pane") {
        EmitSidebar(code, child, indent + 1, "sidebar");
        code << Indent(indent + 1) << parentVar << ".AddSidebar(std::move(sidebar), props);\n";
    } else if (IsPaneItem(child.name)) {
        EmitSidebarItem(code, child, indent + 1, "sidebarItem");
        code << Indent(indent + 1) << parentVar << ".AddSidebarItem(std::move(sidebarItem), props);\n";
    } else if (child.name == "Separator" || child.name == "Divider") {
        EmitSeparator(code, child, indent + 1, "separator");
        code << Indent(indent + 1) << parentVar << ".AddSeparator(std::move(separator), props);\n";
    } else if (child.name == "ThemeToggle") {
        EmitThemeToggle(code, child, indent + 1, "themeToggle");
        code << Indent(indent + 1) << parentVar << ".AddThemeToggle(std::move(themeToggle), props);\n";
    } else if (child.name == "ThemeIconToggle") {
        EmitThemeIconToggle(code, child, indent + 1, "themeIconToggle");
        code << Indent(indent + 1) << parentVar << ".AddThemeIconToggle(std::move(themeIconToggle), props);\n";
    } else if (child.name == "Shape" || child.name == "MotionShape") {
        const std::string shapeName = NextShapeName();
        EmitShape(code, child, indent + 1, shapeName);
        code << Indent(indent + 1) << parentVar << ".AddMotionShape(std::move(" << shapeName << "), props);\n";
    } else if (!TryEmitGraphic(code, child, indent + 1, parentVar, true)) {
        if (!child.name.empty()) {
            std::cerr << "Skipping unknown XML element <" << child.name << ">\n";
        }
    }

    code << Indent(indent) << "}\n";
}

void XmlToCpp::EmitPosition(std::ostringstream& code, const XmlNode& node, int indent, const std::string& targetVar) const {
    const std::string posVar = (targetVar == "scene") ? "position" : targetVar;
    code << Indent(indent) << "Robo::Position " << posVar << ";\n";
    code << Indent(indent) << "Robo::PositionStyle style;\n";

    for (const auto& attribute : node.attributes) {
        if (attribute.first == "mode" || attribute.first == "position") {
            code << Indent(indent) << "style.mode = Robo::ParsePositionMode(\"" << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "anchor") {
            code << Indent(indent) << "style.anchor = Robo::ParsePositionAnchor(\"" << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "left" || attribute.first == "x") {
            code << Indent(indent) << "style.hasLeft = true;\n";
            code << Indent(indent) << "style.left = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "top" || attribute.first == "y") {
            code << Indent(indent) << "style.hasTop = true;\n";
            code << Indent(indent) << "style.top = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "right") {
            code << Indent(indent) << "style.hasRight = true;\n";
            code << Indent(indent) << "style.right = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "bottom") {
            code << Indent(indent) << "style.hasBottom = true;\n";
            code << Indent(indent) << "style.bottom = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "width") {
            code << BindLength(indent, "style", "width", attribute.second, node.name);
        } else if (attribute.first == "height") {
            code << BindLength(indent, "style", "height", attribute.second, node.name);
        } else if (attribute.first == "minWidth") {
            code << Indent(indent) << "style.minWidth = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "maxWidth") {
            code << Indent(indent) << "style.maxWidth = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "minHeight") {
            code << Indent(indent) << "style.minHeight = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "maxHeight") {
            code << Indent(indent) << "style.maxHeight = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "zIndex") {
            code << Indent(indent) << "style.zIndex = " << attribute.second << ";\n";
        } else if (attribute.first == "background" || attribute.first == "backgroundColor") {
            code << BindColor(indent, "style", "backgroundColor", attribute.second, node.name);
        } else if (attribute.first == "borderColor") {
            code << BindColor(indent, "style", "borderColor", attribute.second, node.name);
        } else if (attribute.first == "borderWidth") {
            code << Indent(indent) << "style.borderWidth = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "cornerRadius" || attribute.first == "borderRadius") {
            code << BindFloat(indent, "style", "cornerRadius", attribute.second, node.name);
        } else if (attribute.first == "padding") {
code << BindSides(indent, "style", "padding", attribute.second, node.name);
        }
    }

    EmitSelectable(code, indent, "style", node);
    code << Indent(indent) << posVar << ".SetStyle(style);\n";

    for (const XmlNode& child : node.children) {
        if (!child.isText) {
            EmitPositionChild(code, child, indent, posVar);
        }
    }

    if (targetVar == "scene") {
        code << Indent(indent) << "scene.AddPosition(std::move(" << posVar << "));\n";
    }
}

void XmlToCpp::EmitPositionChild(std::ostringstream& code, const XmlNode& child, int indent, const std::string& parentVar) const {
    code << Indent(indent) << "{\n";
    code << Indent(indent + 1) << "Robo::PositionItemProps props;\n";

    for (const auto& attribute : child.attributes) {
        if (attribute.first == "left" || attribute.first == "x") {
            code << Indent(indent + 1) << "props.hasLeft = true;\n";
            code << Indent(indent + 1) << "props.left = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "top" || attribute.first == "y") {
            code << Indent(indent + 1) << "props.hasTop = true;\n";
            code << Indent(indent + 1) << "props.top = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "right") {
            code << Indent(indent + 1) << "props.hasRight = true;\n";
            code << Indent(indent + 1) << "props.right = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "bottom") {
            code << Indent(indent + 1) << "props.hasBottom = true;\n";
            code << Indent(indent + 1) << "props.bottom = " << FloatLiteral(attribute.second) << ";\n";
        } else if (attribute.first == "zIndex") {
            code << Indent(indent + 1) << "props.zIndex = " << attribute.second << ";\n";
        } else if (attribute.first == "width") {
            if (ExtractToken(attribute.second).empty()) {
                if (!IsFreeLength(attribute.second)) {
                    WarnLiteral(child.name, "width", attribute.second);
                }
                code << BindPropsLength(indent + 1, "props", "width", attribute.second);
            }
        } else if (attribute.first == "height") {
            if (ExtractToken(attribute.second).empty()) {
                if (!IsFreeLength(attribute.second)) {
                    WarnLiteral(child.name, "height", attribute.second);
                }
                code << BindPropsLength(indent + 1, "props", "height", attribute.second);
            }
        } else if (attribute.first == "minWidth") {
            if (ExtractToken(attribute.second).empty()) {
                code << BindPropsLength(indent + 1, "props", "minWidth", attribute.second);
            }
        } else if (attribute.first == "maxWidth") {
            if (ExtractToken(attribute.second).empty()) {
                code << BindPropsLength(indent + 1, "props", "maxWidth", attribute.second);
            }
        } else if (attribute.first == "minHeight") {
            if (ExtractToken(attribute.second).empty()) {
                code << BindPropsLength(indent + 1, "props", "minHeight", attribute.second);
            }
        } else if (attribute.first == "maxHeight") {
            if (ExtractToken(attribute.second).empty()) {
                code << BindPropsLength(indent + 1, "props", "maxHeight", attribute.second);
            }
        } else if (attribute.first == "mode" || attribute.first == "position") {
            code << Indent(indent + 1) << "props.mode = Robo::ParsePositionMode(\"" << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "anchor") {
            code << Indent(indent + 1) << "props.anchor = Robo::ParsePositionAnchor(\"" << EscapeCpp(attribute.second) << "\");\n";
        }
    }

    if (child.name == "Button" || child.name == "IconButton") {
        code << Indent(indent + 1) << "Robo::Button button;\n";
        code << Indent(indent + 1) << "Robo::ButtonStyle style;\n";
        std::string label;
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "variant") {
                EmitEnum(code, indent + 1, "style.variant", attribute.second, MapButtonVariant, "ParseButtonVariant",
                         "Robo::ButtonVariant::Filled");
            } else if (attribute.first == "color") {
                code << BindColor(indent + 1, "style", "textColor", attribute.second, child.name);
                code << Indent(indent + 1) << "style.hasCustomTextColor = true;\n";
            } else if (attribute.first == "background" || attribute.first == "backgroundColor") {
                code << BindColor(indent + 1, "style", "backgroundColor", attribute.second, child.name);
                code << Indent(indent + 1) << "style.hasCustomBackground = true;\n";
            } else if (attribute.first == "cornerRadius") {
                code << BindFloat(indent + 1, "style", "cornerRadius", attribute.second, child.name);
            } else if (attribute.first == "label") {
                label = attribute.second;
            }
        }
        if (label.empty()) {
            for (const XmlNode& c : child.children) {
                if (c.isText) {
                    label += c.text;
                }
            }
        }
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "button.SetStyle(style);\n";
        code << Indent(indent + 1) << "button.ApplyVariantDefaults();\n";
        code << Indent(indent + 1) << "button.SetLabel(\"" << EscapeCpp(label) << "\");\n";
        EmitButtonFlags(code, indent + 1, child);
        code << Indent(indent + 1) << parentVar << ".AddButton(std::move(button), props);\n";
    } else if (child.name == "Input" || child.name == "TextField" || child.name == "TextInput") {
        code << Indent(indent + 1) << "Robo::Input input;\n";
        code << Indent(indent + 1) << "Robo::InputStyle style;\n";
        std::string placeholder;
        std::string value;
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "variant") {
                code << Indent(indent + 1) << "style.variant = Robo::ParseInputVariant(\"" << EscapeCpp(attribute.second) << "\");\n";
            } else if (attribute.first == "placeholder") {
                placeholder = attribute.second;
            } else if (attribute.first == "value") {
                value = attribute.second;
            }
        }
        if (value.empty()) {
            for (const XmlNode& c : child.children) {
                if (c.isText) {
                    value += c.text;
                }
            }
        }
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "input.SetStyle(style);\n";
        code << Indent(indent + 1) << "input.ApplyVariantDefaults();\n";
        code << Indent(indent + 1) << "input.SetPlaceholder(\"" << EscapeCpp(placeholder) << "\");\n";
        code << Indent(indent + 1) << "input.SetValue(\"" << EscapeCpp(value) << "\");\n";
        code << Indent(indent + 1) << parentVar << ".AddInput(std::move(input), props);\n";
    } else if (child.name == "Scrollbar" || child.name == "scrollbar") {
        EmitScrollbar(code, child, indent + 1, "scrollbar");
        code << Indent(indent + 1) << parentVar << ".AddScrollbar(std::move(scrollbar), props);\n";
    } else if (child.name == "Container" || child.name == "container") {
        const std::string nestedContainer = "nestedContainer" + std::to_string(indent + 1);
        EmitContainer(code, child, indent + 1, nestedContainer);
        code << Indent(indent + 1) << parentVar << ".AddContainer(std::move(" << nestedContainer
             << "), props);\n";
    } else if (child.name == "Text") {
        code << Indent(indent + 1) << "Robo::Text text;\n";
        code << Indent(indent + 1) << "Robo::TextStyle style;\n";
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "fontSize") {
                code << BindFloat(indent + 1, "style", "fontSize", attribute.second, child.name);
            } else if (attribute.first == "fontWeight") {
                code << BindWeight(indent + 1, "style", attribute.second, child.name);
            } else if (attribute.first == "color") {
                code << BindColor(indent + 1, "style", "color", attribute.second, child.name);
            }
        }
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "text.SetStyle(style);\n";
        for (const XmlNode& c : child.children) {
            if (c.isText) {
                code << Indent(indent + 1) << "text.AddPlain(\"" << EscapeCpp(c.text) << "\");\n";
            }
        }
        code << Indent(indent + 1) << parentVar << ".AddText(std::move(text), props);\n";
    } else if (child.name == "Heading" || child.name == "heading" ||
               (child.name.size() == 2 && (child.name[0] == 'H' || child.name[0] == 'h') && child.name[1] >= '1' && child.name[1] <= '6')) {
        int level = (child.name.size() == 2) ? (child.name[1] - '0') : 1;
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "level") {
                level = std::atoi(attribute.second.c_str());
            }
        }
        code << Indent(indent + 1) << "Robo::Heading heading(" << level << ");\n";
        code << Indent(indent + 1) << "Robo::TextStyle style = heading.GetStyle();\n";
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "heading.SetStyle(style);\n";
        for (const XmlNode& c : child.children) {
            if (c.isText) {
                code << Indent(indent + 1) << "heading.AddPlain(\"" << EscapeCpp(c.text) << "\");\n";
            }
        }
        code << Indent(indent + 1) << parentVar << ".AddText(std::move(heading), props);\n";
    } else if (child.name == "Paragraph" || child.name == "paragraph" || child.name == "P" || child.name == "p") {
        code << Indent(indent + 1) << "Robo::Paragraph paragraph;\n";
        code << Indent(indent + 1) << "Robo::TextStyle style = paragraph.GetStyle();\n";
        EmitSelectable(code, indent + 1, "style", child);
        code << Indent(indent + 1) << "paragraph.SetStyle(style);\n";
        for (const XmlNode& c : child.children) {
            if (c.isText) {
                code << Indent(indent + 1) << "paragraph.AddPlain(\"" << EscapeCpp(c.text) << "\");\n";
            }
        }
        code << Indent(indent + 1) << parentVar << ".AddText(std::move(paragraph), props);\n";
    } else if (child.name == "Flexbox" || child.name == "flexbox") {
        const std::string childFlexVar = "nestedFlex" + std::to_string(indent + 1);
        EmitFlexbox(code, child, indent + 1, childFlexVar);
        code << Indent(indent + 1) << parentVar << ".AddFlexbox(std::move(" << childFlexVar << "), props);\n";
    } else if (child.name == "Grid" || child.name == "grid") {
        const std::string childGridVar = "nestedGrid" + std::to_string(indent + 1);
        EmitGrid(code, child, indent + 1, childGridVar);
        code << Indent(indent + 1) << parentVar << ".AddGrid(std::move(" << childGridVar << "), props);\n";
    } else if (child.name == "Position" || child.name == "position") {
        const std::string childPosVar = "nestedPos" + std::to_string(indent + 1);
        EmitPosition(code, child, indent + 1, childPosVar);
        code << Indent(indent + 1) << parentVar << ".AddPosition(std::move(" << childPosVar << "), props);\n";
    } else if (child.name == "BrandLogo") {
        EmitBrandLogo(code, child, indent + 1, "brandLogo");
        code << Indent(indent + 1) << parentVar << ".AddBrandLogo(std::move(brandLogo), props);\n";
    } else if (child.name == "TitleBar" || child.name == "Navbar" || child.name == "NavBar") {
        EmitTitleBar(code, child, indent + 1, "titleBar");
        code << Indent(indent + 1) << parentVar << ".AddTitleBar(std::move(titleBar), props);\n";
    } else if (child.name == "Sidebar" || child.name == "Pane") {
        EmitSidebar(code, child, indent + 1, "sidebar");
        code << Indent(indent + 1) << parentVar << ".AddSidebar(std::move(sidebar), props);\n";
    } else if (IsPaneItem(child.name)) {
        EmitSidebarItem(code, child, indent + 1, "sidebarItem");
        code << Indent(indent + 1) << parentVar << ".AddSidebarItem(std::move(sidebarItem), props);\n";
    } else if (child.name == "Separator" || child.name == "Divider") {
        EmitSeparator(code, child, indent + 1, "separator");
        code << Indent(indent + 1) << parentVar << ".AddSeparator(std::move(separator), props);\n";
    } else if (child.name == "ThemeToggle") {
        EmitThemeToggle(code, child, indent + 1, "themeToggle");
        code << Indent(indent + 1) << parentVar << ".AddThemeToggle(std::move(themeToggle), props);\n";
    } else if (child.name == "ThemeIconToggle") {
        EmitThemeIconToggle(code, child, indent + 1, "themeIconToggle");
        code << Indent(indent + 1) << parentVar << ".AddThemeIconToggle(std::move(themeIconToggle), props);\n";
    } else if (child.name == "Shape" || child.name == "MotionShape") {
        const std::string shapeName = NextShapeName();
        EmitShape(code, child, indent + 1, shapeName);
        code << Indent(indent + 1) << parentVar << ".AddMotionShape(std::move(" << shapeName << "), props);\n";
    } else if (!TryEmitGraphic(code, child, indent + 1, parentVar, true)) {
        if (!child.name.empty()) {
            std::cerr << "Skipping unknown XML element <" << child.name << ">\n";
        }
    }

    code << Indent(indent) << "}\n";
}

void XmlToCpp::EmitSidebarItem(std::ostringstream& code,
                             const XmlNode& node,
                             int indent,
                             const std::string& variable,
                             int nestDepth) const {
    code << Indent(indent) << "Robo::SidebarItem " << variable << ";\n";
    std::string label;
    std::string iconName;
    if (node.attributes.count("label") != 0) {
        label = node.attributes.at("label");
    } else {
        for (const XmlNode& child : node.children) {
            if (child.isText && !IsWhitespaceText(child.text)) {
                label += child.text;
            }
        }
    }
    if (node.attributes.count("icon") != 0) {
        iconName = node.attributes.at("icon");
    }
    code << Indent(indent) << variable << ".SetLabel(\"" << EscapeCpp(label) << "\");\n";
    const auto itemId = node.attributes.find("id");
    if (itemId != node.attributes.end()) {
        code << Indent(indent) << variable << ".SetId(\"" << EscapeCpp(itemId->second) << "\");\n";
    }
    const auto selected = node.attributes.find("selected");
    if (selected != node.attributes.end() && (selected->second == "true" || selected->second == "1")) {
        code << Indent(indent) << variable << ".SetSelected(true);\n";
    }
    const auto section = node.attributes.find("section");
    const auto itemVariant = node.attributes.find("variant");
    const bool isSection = (section != node.attributes.end() &&
                            (section->second == "true" || section->second == "1" || section->second == "section")) ||
                           (itemVariant != node.attributes.end() && itemVariant->second == "section");
    if (isSection) {
        code << Indent(indent) << variable << ".SetSection(true);\n";
    }
    const auto disabled = node.attributes.find("disabled");
    if (disabled != node.attributes.end() && (disabled->second == "true" || disabled->second == "1")) {
        code << Indent(indent) << variable << ".SetDisabled(true);\n";
    }
    const auto loading = node.attributes.find("loading");
    if (loading != node.attributes.end() && (loading->second == "true" || loading->second == "1")) {
        code << Indent(indent) << variable << ".SetLoading(true);\n";
    }
    const auto icon = icons_.find(iconName);
    // Nested PaneItem icons are deferred: baking thousands of SVGs on open freezes the OS BSP.
    // Top-level rail icons (nestDepth == 0) still bake so the activity sidebar has marks.
    if (nestDepth == 0 && !iconName.empty() && icon != icons_.end()) {
        EmitPackedIcon(code, indent, "icon", iconName, 20.0f, variable + ".SetIcon(std::move(icon));");
    }
    int childIndex = 0;
    for (const XmlNode& child : node.children) {
        if (!IsPaneItem(child.name)) {
            continue;
        }
        const std::string childName = variable + "Child" + std::to_string(childIndex++);
        code << Indent(indent) << "{\n";
        EmitSidebarItem(code, child, indent + 1, childName, nestDepth + 1);
        code << Indent(indent + 1) << variable << ".AddChild(std::move(" << childName << "));\n";
        code << Indent(indent) << "}\n";
    }
}

void XmlToCpp::EmitSidebar(std::ostringstream& code,
                             const XmlNode& node,
                             int indent,
                             const std::string& variable) const {
    code << Indent(indent) << "Robo::Sidebar " << variable << ";\n";
    if (node.attributes.count("variant") != 0) {
        const char* variantExpr = MapSidebarVariant(node.attributes.at("variant"));
        if (variantExpr != nullptr) {
            code << Indent(indent) << variable << ".SetVariant(" << variantExpr << ");\n";
        } else {
            code << Indent(indent) << variable << ".SetVariant(Robo::ParseSidebarVariant(\""
                 << EscapeCpp(node.attributes.at("variant")) << "\"));\n";
        }
    }
    int itemIndex = 0;
    for (const XmlNode& child : node.children) {
        if (!IsPaneItem(child.name)) {
            continue;
        }
        const std::string itemName = variable + "Item" + std::to_string(itemIndex++);
        code << Indent(indent) << "{\n";
        EmitSidebarItem(code, child, indent + 1, itemName);
        code << Indent(indent + 1) << variable << ".AddItem(std::move(" << itemName << "));\n";
        code << Indent(indent) << "}\n";
    }
}

void XmlToCpp::EmitSeparator(std::ostringstream& code,
                             const XmlNode& node,
                             int indent,
                             const std::string& variable) const {
    code << Indent(indent) << "Robo::Separator " << variable << ";\n";
    std::string label;
    if (node.attributes.count("label") != 0) {
        label = node.attributes.at("label");
    }
    auto emitSetEnum = [&](const char* method, const std::string& value, const char* (*map)(const std::string&),
                           const char* parseName, const char* defaultExpr) {
        const char* expr = map != nullptr ? map(value) : nullptr;
        if (expr != nullptr && defaultExpr != nullptr && std::string(expr) == defaultExpr) {
            return;
        }
        if (expr != nullptr) {
            code << Indent(indent) << variable << "." << method << "(" << expr << ");\n";
            return;
        }
        code << Indent(indent) << variable << "." << method << "(Robo::" << parseName << "(\"" << EscapeCpp(value)
             << "\"));\n";
    };
    for (const auto& attribute : node.attributes) {
        if (attribute.first == "variant") {
            emitSetEnum("SetVariant", attribute.second, MapSeparatorVariant, "ParseSeparatorVariant",
                        "Robo::SeparatorVariant::Solid");
        } else if (attribute.first == "size") {
            emitSetEnum("SetSize", attribute.second, MapSeparatorSize, "ParseSeparatorSize",
                        "Robo::SeparatorSize::Medium");
        } else if (attribute.first == "tone") {
            emitSetEnum("SetTone", attribute.second, MapSeparatorTone, "ParseSeparatorTone",
                        "Robo::SeparatorTone::Default");
        } else if (attribute.first == "align") {
            emitSetEnum("SetAlign", attribute.second, MapSeparatorAlign, "ParseSeparatorAlign",
                        "Robo::SeparatorAlign::Center");
        } else if (attribute.first == "orientation") {
            emitSetEnum("SetOrientation", attribute.second, MapSeparatorOrientation, "ParseSeparatorOrientation",
                        "Robo::SeparatorOrientation::Horizontal");
        } else if (attribute.first == "inset" && attribute.second != "false") {
            code << Indent(indent) << variable << ".SetInset(true);\n";
        } else if (attribute.first == "flush" && attribute.second != "false") {
            code << Indent(indent) << variable << ".SetFlush(true);\n";
        } else if (attribute.first == "disabled" && attribute.second != "false") {
            code << Indent(indent) << variable << ".SetDisabled(true);\n";
        }
    }
    if (label.empty()) {
        for (const XmlNode& child : node.children) {
            if (child.isText) {
                label += child.text;
            }
        }
    }
    if (!label.empty()) {
        code << Indent(indent) << variable << ".SetLabel(\"" << EscapeCpp(label) << "\");\n";
    }
}

void XmlToCpp::EmitDockItem(std::ostringstream& code,
                            const XmlNode& node,
                            int indent,
                            const std::string& variable) const {
    code << Indent(indent) << "Robo::DockItem " << variable << ";\n";
    std::string label;
    std::string iconName;
    bool selected = false;
    if (node.attributes.count("label") != 0) {
        label = node.attributes.at("label");
    }
    for (const auto& attribute : node.attributes) {
        if (attribute.first == "icon") {
            iconName = attribute.second;
        } else if (attribute.first == "selected" && attribute.second != "false" && attribute.second != "0") {
            selected = true;
        }
    }
    for (const XmlNode& child : node.children) {
        if (child.isText) {
            label += child.text;
        }
    }
    code << Indent(indent) << variable << ".SetLabel(\"" << EscapeCpp(label) << "\");\n";
    if (selected) {
        code << Indent(indent) << variable << ".SetSelected(true);\n";
    }
    const auto icon = icons_.find(iconName);
    if (!iconName.empty() && icon != icons_.end()) {
        EmitPackedIcon(code, indent, "icon", iconName, 22.0f, variable + ".SetIcon(std::move(icon));");
    }
}

void XmlToCpp::EmitSelectionLabelStyle(std::ostringstream& code,
                                         const XmlNode& node,
                                         int indent,
                                         const std::string& variable,
                                         const std::string& seed) const {
    const bool labeled = node.attributes.count("fontSize") != 0 || node.attributes.count("fontWeight") != 0 ||
                         node.attributes.count("color") != 0 || node.attributes.count("align") != 0 ||
                         node.attributes.count("lineHeight") != 0 || node.attributes.count("letterSpacing") != 0 ||
                         node.attributes.count("wrap") != 0 || node.attributes.count("maxWidth") != 0 ||
                         node.attributes.count("maxLines") != 0 || node.attributes.count("shadowX") != 0 ||
                         node.attributes.count("shadowY") != 0 || node.attributes.count("shadowBlur") != 0 ||
                         node.attributes.count("shadowColor") != 0 || node.attributes.count("iconGap") != 0 ||
                         node.attributes.count("selectionRadius") != 0 || node.attributes.count("selectionColor") != 0;
    if (!labeled) {
        return;
    }
    // Desktop chrome uses the shared defaults helper (no per-field dump).
    if (seed.empty()) {
        code << Indent(indent) << "Robo::RoboApplySelectionLabelDefaults(" << variable << ");\n";
        return;
    }
    code << Indent(indent) << "{\n";
    const std::string source = seed;
    code << Indent(indent + 1) << "Robo::SelectionLabelStyle labelStyle = " << source << ".GetLabelStyle();\n";
    if (node.attributes.count("fontSize") != 0) {
        code << Indent(indent + 1) << "labelStyle.fontSize = " << FloatLiteral(node.attributes.at("fontSize")) << ";\n";
    }
    if (node.attributes.count("fontWeight") != 0) {
        EmitEnum(code, indent + 1, "labelStyle.fontWeight", node.attributes.at("fontWeight"), MapFontWeight, "ParseFontWeight",
                 "Robo::FontWeight::Regular");
    }
    if (node.attributes.count("color") != 0) {
        code << Indent(indent + 1) << "labelStyle.color = Robo::Color::FromHex(\"" << EscapeCpp(node.attributes.at("color"))
             << "\");\n";
    }
    if (node.attributes.count("align") != 0) {
        EmitEnum(code, indent + 1, "labelStyle.align", node.attributes.at("align"), MapTextAlign, "ParseTextAlign",
                 "Robo::TextAlign::Left");
    }
    if (node.attributes.count("lineHeight") != 0) {
        code << Indent(indent + 1) << "labelStyle.lineHeight = " << FloatLiteral(node.attributes.at("lineHeight")) << ";\n";
    }
    if (node.attributes.count("letterSpacing") != 0) {
        code << Indent(indent + 1) << "labelStyle.letterSpacing = " << FloatLiteral(node.attributes.at("letterSpacing"))
             << ";\n";
    }
    if (node.attributes.count("wrap") != 0) {
        const std::string wrap = node.attributes.at("wrap");
        const bool enabled = !(wrap == "false" || wrap == "0" || wrap == "no");
        code << Indent(indent + 1) << "labelStyle.wrap = " << (enabled ? "true" : "false") << ";\n";
    }
    if (node.attributes.count("maxWidth") != 0) {
        code << Indent(indent + 1) << "labelStyle.maxWidth = " << FloatLiteral(node.attributes.at("maxWidth")) << ";\n";
    }
    if (node.attributes.count("maxLines") != 0) {
        code << Indent(indent + 1) << "labelStyle.maxLines = " << node.attributes.at("maxLines") << ";\n";
    }
    if (node.attributes.count("shadowX") != 0) {
        code << Indent(indent + 1) << "labelStyle.shadowX = " << FloatLiteral(node.attributes.at("shadowX")) << ";\n";
    }
    if (node.attributes.count("shadowY") != 0) {
        code << Indent(indent + 1) << "labelStyle.shadowY = " << FloatLiteral(node.attributes.at("shadowY")) << ";\n";
    }
    if (node.attributes.count("shadowBlur") != 0) {
        code << Indent(indent + 1) << "labelStyle.shadowBlur = " << FloatLiteral(node.attributes.at("shadowBlur")) << ";\n";
    }
    if (node.attributes.count("shadowColor") != 0) {
        code << Indent(indent + 1) << "labelStyle.shadowColor = Robo::Color::FromHex(\""
             << EscapeCpp(node.attributes.at("shadowColor")) << "\");\n";
    }
    if (node.attributes.count("iconGap") != 0) {
        code << Indent(indent + 1) << "labelStyle.iconGap = " << FloatLiteral(node.attributes.at("iconGap")) << ";\n";
    }
    if (node.attributes.count("selectionRadius") != 0) {
        code << Indent(indent + 1) << "labelStyle.selectionRadius = " << FloatLiteral(node.attributes.at("selectionRadius"))
             << ";\n";
    }
    if (node.attributes.count("selectionColor") != 0) {
        code << Indent(indent + 1) << "labelStyle.selectionColor = Robo::Color::FromHex(\""
             << EscapeCpp(node.attributes.at("selectionColor")) << "\");\n";
    }
    code << Indent(indent + 1) << variable << ".SetLabelStyle(labelStyle);\n";
    code << Indent(indent) << "}\n";
}

void XmlToCpp::EmitSelectionArea(std::ostringstream& code, const XmlNode& node, int indent) const {
    code << Indent(indent) << "{\n";
    code << Indent(indent + 1) << "Robo::SelectionArea selectionArea;\n";
    if (node.attributes.count("rule") != 0) {
        const std::string rule = node.attributes.at("rule");
        const bool contain = rule == "contain" || rule == "contained" || rule == "inside";
        code << Indent(indent + 1) << "selectionArea.SetInclusion("
             << (contain ? "Robo::SelectionInclusion::Contain" : "Robo::SelectionInclusion::Intersect") << ");\n";
    }
    if (node.attributes.count("mode") != 0) {
        const std::string mode = node.attributes.at("mode");
        std::string value = "Robo::SelectionCombine::Replace";
        if (mode == "add") {
            value = "Robo::SelectionCombine::Add";
        } else if (mode == "subtract") {
            value = "Robo::SelectionCombine::Subtract";
        } else if (mode == "toggle") {
            value = "Robo::SelectionCombine::Toggle";
        }
        code << Indent(indent + 1) << "selectionArea.SetCombine(" << value << ");\n";
    }
    if (node.attributes.count("threshold") != 0) {
        code << Indent(indent + 1) << "selectionArea.SetThreshold(" << FloatLiteral(node.attributes.at("threshold"))
             << ");\n";
    }
    EmitSelectionLabelStyle(code, node, indent + 1, "selectionArea");
    int itemIndex = 0;
    for (const XmlNode& item : node.children) {
        if (!IsSelectionItemName(item.name)) {
            continue;
        }
        std::string label;
        if (item.attributes.count("label") != 0) {
            label = item.attributes.at("label");
        }
        for (const XmlNode& text : item.children) {
            if (text.isText) {
                label += text.text;
            }
        }
        const std::string itemName = "selectionAreaItem" + std::to_string(itemIndex++);
        code << Indent(indent + 1) << "Robo::SelectionAreaItem " << itemName << ";\n";
        code << Indent(indent + 1) << itemName << ".SetLabel(\"" << EscapeCpp(label) << "\");\n";
        if (item.attributes.count("open") != 0) {
            code << Indent(indent + 1) << itemName << ".SetOpen(\"" << EscapeCpp(item.attributes.at("open")) << "\");\n";
        }
        if (item.attributes.count("x") != 0 || item.attributes.count("y") != 0) {
            const std::string x = item.attributes.count("x") != 0 ? FloatLiteral(item.attributes.at("x")) : "0.0f";
            const std::string y = item.attributes.count("y") != 0 ? FloatLiteral(item.attributes.at("y")) : "0.0f";
            code << Indent(indent + 1) << itemName << ".SetPosition(" << x << ", " << y << ");\n";
        }
        if (item.attributes.count("width") != 0 || item.attributes.count("height") != 0) {
            const std::string width =
                item.attributes.count("width") != 0 ? FloatLiteral(item.attributes.at("width")) : "104.0f";
            const std::string height =
                item.attributes.count("height") != 0 ? FloatLiteral(item.attributes.at("height")) : "108.0f";
            code << Indent(indent + 1) << itemName << ".SetSize(" << width << ", " << height << ");\n";
        }
        auto flag = [&](const char* name) {
            return item.attributes.count(name) != 0 && item.attributes.at(name) != "false" && item.attributes.at(name) != "0";
        };
        if (flag("selected")) {
            code << Indent(indent + 1) << itemName << ".SetSelected(true);\n";
        }
        if (flag("locked") || flag("disabled")) {
            code << Indent(indent + 1) << itemName << ".SetLocked(true);\n";
        }
        EmitSelectionLabelStyle(code, item, indent + 1, itemName, "selectionArea");
        code << Indent(indent + 1) << "selectionArea.AddItem(std::move(" << itemName << "));\n";
    }
    code << Indent(indent + 1) << "scene.AddSelectionArea(std::move(selectionArea));\n";
    code << Indent(indent) << "}\n";
}

void XmlToCpp::EmitDesktop(std::ostringstream& code, const XmlNode& node, int indent) const {
    // EnableDesktop + background live in RoboApplyDesktopChrome for thin DesktopPage.
    int dockIndex = 0;
    for (const XmlNode& child : node.children) {
        if (child.name == "Background") {
            continue;
        }
        if (child.name == "AppShell") {
            std::string title;
            if (child.attributes.count("title") != 0) {
                title = child.attributes.at("title");
            } else {
                for (const XmlNode& text : child.children) {
                    if (text.isText) {
                        title += text.text;
                    }
                }
            }
            code << Indent(indent) << "scene.SetAppShellTitle(\"" << EscapeCpp(title) << "\");\n";
            for (const XmlNode& content : child.children) {
                if (!content.isText) {
                    code << ConvertNode(content, indent);
                }
            }
        } else if (child.name == "TopBarMenu") {
            EmitTopBarMenu(code, child, indent);
        } else if (child.name == "ContextMenu") {
            EmitContextMenu(code, child, indent);
        } else if (IsSelectionAreaName(child.name)) {
            EmitSelectionArea(code, child, indent);
        } else if (child.name == "Dock") {
            for (const XmlNode& item : child.children) {
                if (item.name != "DockItem") {
                    continue;
                }
                const std::string itemName = "dockItem" + std::to_string(dockIndex++);
                code << Indent(indent) << "{\n";
                EmitDockItem(code, item, indent + 1, itemName);
                code << Indent(indent + 1) << "scene.AddDockItem(std::move(" << itemName << "));\n";
                code << Indent(indent) << "}\n";
            }
        }
    }
}

void XmlToCpp::EmitTopBarMenu(std::ostringstream& code, const XmlNode& node, int indent) const {
    code << Indent(indent) << "{\n";
    code << Indent(indent + 1) << "Robo::TopBarMenu topBarMenu;\n";
    std::string app = "RoboticelaOS";
    if (node.attributes.count("app") != 0) {
        app = node.attributes.at("app");
    } else if (node.attributes.count("title") != 0) {
        app = node.attributes.at("title");
    }
    code << Indent(indent + 1) << "topBarMenu.SetApp(\"" << EscapeCpp(app) << "\");\n";
    if (node.attributes.count("appearance") != 0) {
        const std::string appearance = node.attributes.at("appearance");
        const bool dark = appearance == "dark" || appearance == "Dark";
        code << Indent(indent + 1) << "topBarMenu.SetAppearanceDark(" << (dark ? "true" : "false") << ");\n";
    }
    if (node.attributes.count("transparent") != 0) {
        const std::string transparent = node.attributes.at("transparent");
        std::string amount = "0.0f";
        if (transparent == "true") {
            amount = "0.35f";
        } else if (transparent != "false" && transparent != "0") {
            amount = FloatLiteral(transparent);
        }
        code << Indent(indent + 1) << "topBarMenu.SetTransparency(" << amount << ");\n";
    }
    if (node.attributes.count("clock") != 0) {
        code << Indent(indent + 1) << "topBarMenu.SetClock(\"" << EscapeCpp(node.attributes.at("clock")) << "\");\n";
    }
    std::vector<std::string> menuLabels;
    bool menuPlain = true;
    for (const XmlNode& item : node.children) {
        if (item.name != "TopBarMenuItem") {
            continue;
        }
        auto menuFlag = [&](const char* name) {
            return item.attributes.count(name) != 0 && item.attributes.at(name) != "false" && item.attributes.at(name) != "0";
        };
        if (menuFlag("selected") || menuFlag("disabled") || menuFlag("separator") || menuFlag("submenu")) {
            menuPlain = false;
        }
        std::string label;
        if (item.attributes.count("label") != 0) {
            label = item.attributes.at("label");
        }
        for (const XmlNode& text : item.children) {
            if (text.isText) {
                label += text.text;
            }
        }
        menuLabels.push_back(label);
    }
    if (menuPlain && menuLabels.size() > 1) {
        code << Indent(indent + 1) << "static const char* kRoboMenuLabels[] = {\n";
        for (const std::string& label : menuLabels) {
            code << Indent(indent + 2) << "\"" << EscapeCpp(label) << "\",\n";
        }
        code << Indent(indent + 1) << "};\n";
        code << Indent(indent + 1) << "Robo::RoboAddMenuLabels(topBarMenu, kRoboMenuLabels, "
             << static_cast<int>(menuLabels.size()) << ");\n";
    } else {
        int itemIndex = 0;
        for (const XmlNode& item : node.children) {
            if (item.name != "TopBarMenuItem") {
                continue;
            }
            std::string label;
            if (item.attributes.count("label") != 0) {
                label = item.attributes.at("label");
            }
            for (const XmlNode& text : item.children) {
                if (text.isText) {
                    label += text.text;
                }
            }
            const std::string itemName = "topBarMenuItem" + std::to_string(itemIndex++);
            code << Indent(indent + 1) << "Robo::TopBarMenuItem " << itemName << ";\n";
            code << Indent(indent + 1) << itemName << ".SetLabel(\"" << EscapeCpp(label) << "\");\n";
            auto menuFlag = [&](const char* name) {
                return item.attributes.count(name) != 0 && item.attributes.at(name) != "false" &&
                       item.attributes.at(name) != "0";
            };
            if (menuFlag("selected")) {
                code << Indent(indent + 1) << itemName << ".SetSelected(true);\n";
            }
            if (menuFlag("disabled")) {
                code << Indent(indent + 1) << itemName << ".SetDisabled(true);\n";
            }
            if (menuFlag("separator")) {
                code << Indent(indent + 1) << itemName << ".SetSeparator(true);\n";
            }
            if (menuFlag("submenu")) {
                code << Indent(indent + 1) << itemName << ".SetSubmenu(true);\n";
            }
            code << Indent(indent + 1) << "topBarMenu.AddItem(std::move(" << itemName << "));\n";
        }
    }
    code << Indent(indent + 1) << "scene.SetTopBarMenu(std::move(topBarMenu));\n";
    code << Indent(indent) << "}\n";
}

void XmlToCpp::EmitContextMenuItem(std::ostringstream& code, const XmlNode& node, int indent, const std::string& variable, int& index) const {
    (void)code;
    (void)node;
    (void)indent;
    (void)variable;
    (void)index;
}

void XmlToCpp::EmitContextMenu(std::ostringstream& code, const XmlNode& node, int indent) const {
    struct MenuRow {
        std::string label;
        std::string icon;
        std::string shortcut;
        int parent = -1;
        unsigned flags = 0;
    };
    std::vector<MenuRow> rows;
    std::function<void(const XmlNode&, int)> collect = [&](const XmlNode& item, int parent) {
        MenuRow row;
        row.parent = parent;
        if (item.name == "ContextMenuSeparator" ||
            (item.attributes.count("separator") != 0 && item.attributes.at("separator") != "false")) {
            row.flags |= 1u;
            rows.push_back(row);
            return;
        }
        if (item.attributes.count("label") != 0) {
            row.label = item.attributes.at("label");
        } else {
            for (const XmlNode& text : item.children) {
                if (text.isText) {
                    row.label += text.text;
                }
            }
        }
        if (item.attributes.count("icon") != 0) {
            row.icon = item.attributes.at("icon");
        }
        if (item.attributes.count("shortcut") != 0) {
            row.shortcut = item.attributes.at("shortcut");
        }
        auto flag = [&](const char* name) {
            return item.attributes.count(name) != 0 && item.attributes.at(name) != "false" &&
                   item.attributes.at(name) != "0";
        };
        if (flag("disabled")) {
            row.flags |= 8u;
        }
        if (flag("checkable")) {
            row.flags |= 2u;
        }
        if (flag("checked")) {
            row.flags |= 4u | 2u;
        }
        bool submenu = flag("submenu");
        for (const XmlNode& child : item.children) {
            if (child.name == "ContextMenuItem" || child.name == "ContextMenuSeparator") {
                submenu = true;
            }
        }
        if (submenu) {
            row.flags |= 16u;
        }
        const int index = static_cast<int>(rows.size());
        rows.push_back(row);
        for (const XmlNode& child : item.children) {
            if (child.name == "ContextMenuItem" || child.name == "ContextMenuSeparator") {
                collect(child, index);
            }
        }
    };
    for (const XmlNode& item : node.children) {
        if (item.name == "ContextMenuItem" || item.name == "ContextMenuSeparator") {
            collect(item, -1);
        }
    }

    code << Indent(indent) << "{\n";
    code << Indent(indent + 1) << "Robo::ContextMenu contextMenu;\n";
    if (node.attributes.count("appearance") != 0) {
        const std::string appearance = node.attributes.at("appearance");
        const bool dark = appearance == "dark" || appearance == "Dark";
        code << Indent(indent + 1) << "contextMenu.SetAppearanceDark(" << (dark ? "true" : "false") << ");\n";
    }
    if (node.attributes.count("transparent") != 0) {
        const std::string transparent = node.attributes.at("transparent");
        std::string amount = "0.0f";
        if (transparent == "true") {
            amount = "0.35f";
        } else if (transparent != "false" && transparent != "0") {
            amount = FloatLiteral(transparent);
        }
        code << Indent(indent + 1) << "contextMenu.SetTransparency(" << amount << ");\n";
    }
    code << Indent(indent + 1) << "static const Robo::RoboMenuEntry kRoboMenuRows[] = {\n";
    for (const MenuRow& row : rows) {
        code << Indent(indent + 2) << "{\"" << EscapeCpp(row.label) << "\", \"" << EscapeCpp(row.icon) << "\", \""
             << EscapeCpp(row.shortcut) << "\", " << row.parent << ", " << row.flags << "u},\n";
    }
    if (rows.empty()) {
        code << Indent(indent + 2) << "{nullptr, nullptr, nullptr, -1, 0u},\n";
    }
    code << Indent(indent + 1) << "};\n";
    code << Indent(indent + 1)
         << "Robo::RoboFillContextMenu(contextMenu, kRoboMenuRows, "
         << (rows.empty() ? 0 : static_cast<int>(rows.size())) << ");\n";
    code << Indent(indent + 1) << "scene.SetContextMenu(std::move(contextMenu));\n";
    code << Indent(indent) << "}\n";
}

void XmlToCpp::EmitTitleBar(std::ostringstream& code,
                              const XmlNode& node,
                              int indent,
                              const std::string& variable) const {
    code << Indent(indent) << "Robo::TitleBar " << variable << ";\n";
    std::string title;
    if (node.attributes.count("title") != 0) {
        title = node.attributes.at("title");
    }
    for (const XmlNode& child : node.children) {
        if (child.isText) {
            title += child.text;
        }
    }
    code << Indent(indent) << variable << ".SetTitle(\"" << EscapeCpp(title) << "\");\n";

    if (icons_.find("Sidebar") != icons_.end() && icons_.find("SidebarHidden") != icons_.end()) {
        code << Indent(indent) << variable << ".SetSidebarIconNames(\"Sidebar\", \"SidebarHidden\");\n";
    }
}

void XmlToCpp::EmitBrandLogo(std::ostringstream& code,
                               const XmlNode& node,
                               int indent,
                               const std::string& variable) const {
    code << Indent(indent) << "Robo::BrandLogo " << variable << ";\n";
    for (const auto& attribute : node.attributes) {
        if (attribute.first == "variant") {
            code << Indent(indent) << variable << ".SetVariant(Robo::ParseBrandLogoVariant(\"" << EscapeCpp(attribute.second) << "\"));\n";
        } else if (attribute.first == "size") {
            const std::string token = ExtractToken(attribute.second);
            if (!token.empty()) {
                code << Indent(indent) << variable << ".SetSize(Robo::ResolveFloat(20.0f, \"" << token << "\"));\n";
            } else {
                code << Indent(indent) << variable << ".SetSize(" << FloatLiteral(attribute.second) << ");\n";
            }
        } else if (attribute.first == "primary") {
            code << Indent(indent) << variable << ".SetPrimary(\"" << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "secondary") {
            code << Indent(indent) << variable << ".SetSecondary(\"" << EscapeCpp(attribute.second) << "\");\n";
        }
    }
}

void XmlToCpp::EmitThemeToggle(std::ostringstream& code,
                                 const XmlNode& node,
                                 int indent,
                                 const std::string& variable) const {
    code << Indent(indent) << "Robo::ThemeToggle " << variable << ";\n";
    bool compact = false;
    for (const auto& attribute : node.attributes) {
        if (attribute.first == "variant") {
            code << Indent(indent) << variable << ".SetVariant(Robo::ParseThemeToggleVariant(\"" << EscapeCpp(attribute.second) << "\"));\n";
        } else if (attribute.first == "compact") {
            compact = attribute.second == "true" || attribute.second == "1";
        } else if (attribute.first == "disabled") {
            const bool disabled = attribute.second == "true" || attribute.second == "1";
            code << Indent(indent) << variable << ".SetDisabled(" << (disabled ? "true" : "false") << ");\n";
        }
    }
    if (compact) {
        code << Indent(indent) << variable << ".SetVariant(Robo::ThemeToggleVariant::Compact);\n";
    }
}

void XmlToCpp::EmitThemeIconToggle(std::ostringstream& code,
                                    const XmlNode& node,
                                    int indent,
                                    const std::string& variable) const {
    code << Indent(indent) << "Robo::ThemeIconToggle " << variable << ";\n";
    for (const auto& attribute : node.attributes) {
        if (attribute.first == "variant") {
            code << Indent(indent) << variable << ".SetVariant(Robo::ParseThemeIconToggleVariant(\"" << EscapeCpp(attribute.second) << "\"));\n";
        } else if (attribute.first == "size") {
            code << Indent(indent) << variable << ".SetSize(Robo::ParseThemeIconToggleSize(\"" << EscapeCpp(attribute.second) << "\"));\n";
        } else if (attribute.first == "disabled") {
            const bool disabled = attribute.second == "true" || attribute.second == "1";
            code << Indent(indent) << variable << ".SetDisabled(" << (disabled ? "true" : "false") << ");\n";
        }
    }
}

std::string XmlToCpp::NextShapeName() const {
    return "motionShape" + std::to_string(shapeSerial_++);
}

void XmlToCpp::EmitKeyframes(std::ostringstream& code, const XmlNode& node, int indent) const {
    std::string name = "motion";
    std::string duration = "1s";
    std::string easing = "linear";
    std::string delay = "0s";
    bool infinite = false;
    for (const auto& attribute : node.attributes) {
        if (attribute.first == "name") {
            name = attribute.second;
        } else if (attribute.first == "duration") {
            duration = attribute.second;
        } else if (attribute.first == "easing") {
            easing = attribute.second;
        } else if (attribute.first == "delay") {
            delay = attribute.second;
        } else if (attribute.first == "iteration" && attribute.second == "infinite") {
            infinite = true;
        }
    }
    code << Indent(indent) << "{\n";
    code << Indent(indent + 1) << "Robo::MotionClip clip;\n";
    code << Indent(indent + 1) << "clip.name = \"" << EscapeCpp(name) << "\";\n";
    code << Indent(indent + 1) << "clip.durationMs = Robo::ParseDurationMs(\"" << EscapeCpp(duration) << "\");\n";
    code << Indent(indent + 1) << "clip.delayMs = Robo::ParseDurationMs(\"" << EscapeCpp(delay) << "\");\n";
    code << Indent(indent + 1) << "clip.easing = Robo::ParseEasing(\"" << EscapeCpp(easing) << "\");\n";
    code << Indent(indent + 1) << "clip.infinite = " << (infinite ? "true" : "false") << ";\n";
    for (const XmlNode& child : node.children) {
        if (child.name != "Keyframe") {
            continue;
        }
        code << Indent(indent + 1) << "{\n";
        code << Indent(indent + 2) << "Robo::MotionKeyframe frame;\n";
        for (const auto& attribute : child.attributes) {
            if (attribute.first == "at") {
                std::string at = attribute.second;
                const bool percent = !at.empty() && at.back() == '%';
                if (percent) {
                    at.pop_back();
                }
                code << Indent(indent + 2) << "frame.at = " << FloatLiteral(at);
                if (percent) {
                    code << " / 100.0f";
                }
                code << ";\n";
            } else if (attribute.first == "x") {
                code << Indent(indent + 2) << "frame.x = " << FloatLiteral(attribute.second) << ";\n";
                code << Indent(indent + 2) << "frame.hasX = true;\n";
            } else if (attribute.first == "y") {
                code << Indent(indent + 2) << "frame.y = " << FloatLiteral(attribute.second) << ";\n";
                code << Indent(indent + 2) << "frame.hasY = true;\n";
            } else if (attribute.first == "opacity") {
                code << Indent(indent + 2) << "frame.opacity = " << FloatLiteral(attribute.second) << ";\n";
                code << Indent(indent + 2) << "frame.hasOpacity = true;\n";
            } else if (attribute.first == "scale") {
                code << Indent(indent + 2) << "frame.scale = " << FloatLiteral(attribute.second) << ";\n";
                code << Indent(indent + 2) << "frame.hasScale = true;\n";
            } else if (attribute.first == "rotate") {
                code << Indent(indent + 2) << "frame.rotate = " << FloatLiteral(attribute.second) << ";\n";
                code << Indent(indent + 2) << "frame.hasRotate = true;\n";
            }
        }
        code << Indent(indent + 2) << "clip.frames.push_back(frame);\n";
        code << Indent(indent + 1) << "}\n";
    }
    code << Indent(indent + 1) << "Robo::RegisterMotion(std::move(clip));\n";
    code << Indent(indent) << "}\n";
}

std::string CppFloat(float value) {
    std::ostringstream out;
    out.setf(std::ios::fixed);
    out.precision(4);
    out << value;
    return out.str() + "f";
}

void XmlToCpp::EmitShape(std::ostringstream& code, const XmlNode& node, int indent, const std::string& variable) const {
    code << Indent(indent) << "Robo::MotionShape " << variable << ";\n";
    auto flag = [](const std::string& value) {
        return value == "true" || value == "1" || value == "yes";
    };
    for (const auto& attribute : node.attributes) {
        const std::string& key = attribute.first;
        const std::string& value = attribute.second;
        if (key == "kind") {
            if (value == "ellipse" || value == "circle") {
                code << Indent(indent) << variable << ".SetKind(Robo::MotionShape::Kind::Ellipse);\n";
            } else if (value == "path") {
                code << Indent(indent) << variable << ".SetKind(Robo::MotionShape::Kind::Path);\n";
            } else {
                code << Indent(indent) << variable << ".SetKind(Robo::MotionShape::Kind::Rect);\n";
            }
        } else if (key == "x") {
            code << Indent(indent) << variable << ".SetX(" << FloatLiteral(value) << ");\n";
        } else if (key == "y") {
            code << Indent(indent) << variable << ".SetY(" << FloatLiteral(value) << ");\n";
        } else if (key == "darkX") {
            code << Indent(indent) << variable << ".SetDarkX(" << FloatLiteral(value) << ");\n";
        } else if (key == "darkY") {
            code << Indent(indent) << variable << ".SetDarkY(" << FloatLiteral(value) << ");\n";
        } else if (key == "bottom") {
            code << Indent(indent) << variable << ".SetBottom(" << FloatLiteral(value) << ");\n";
        } else if (key == "darkBottom") {
            code << Indent(indent) << variable << ".SetDarkBottom(" << FloatLiteral(value) << ");\n";
        } else if (key == "width") {
            code << Indent(indent) << variable << ".SetWidth(" << FloatLiteral(value) << ");\n";
        } else if (key == "height") {
            code << Indent(indent) << variable << ".SetHeight(" << FloatLiteral(value) << ");\n";
        } else if (key == "radius") {
            code << Indent(indent) << variable << ".SetRadius(" << FloatLiteral(value) << ");\n";
        } else if (key == "hole") {
            code << Indent(indent) << variable << ".SetHole(" << FloatLiteral(value) << ");\n";
        } else if (key == "fill" || key == "fillEnd" || key == "darkFill" || key == "darkFillEnd" || key == "hoverFill") {
            const char* setter = "SetFill";
            if (key == "fillEnd") {
                setter = "SetFillEnd";
            } else if (key == "darkFill") {
                setter = "SetDarkFill";
            } else if (key == "darkFillEnd") {
                setter = "SetDarkFillEnd";
            } else if (key == "hoverFill") {
                setter = "SetHoverFill";
            }
            code << Indent(indent) << variable << "." << setter << "(Robo::Color::FromHex(\"" << EscapeCpp(value) << "\"));\n";
        } else if (key == "opacity") {
            code << Indent(indent) << variable << ".SetOpacity(" << FloatLiteral(value) << ");\n";
        } else if (key == "darkOpacity") {
            code << Indent(indent) << variable << ".SetDarkOpacity(" << FloatLiteral(value) << ");\n";
        } else if (key == "scale") {
            code << Indent(indent) << variable << ".SetScale(" << FloatLiteral(value) << ");\n";
        } else if (key == "darkScale") {
            code << Indent(indent) << variable << ".SetDarkScale(" << FloatLiteral(value) << ");\n";
        } else if (key == "rotate") {
            code << Indent(indent) << variable << ".SetRotate(" << FloatLiteral(value) << ");\n";
        } else if (key == "darkRotate") {
            code << Indent(indent) << variable << ".SetDarkRotate(" << FloatLiteral(value) << ");\n";
        } else if (key == "hoverX") {
            code << Indent(indent) << variable << ".SetHoverX(" << FloatLiteral(value) << ");\n";
        } else if (key == "darkHoverX") {
            code << Indent(indent) << variable << ".SetDarkHoverX(" << FloatLiteral(value) << ");\n";
        } else if (key == "hoverY") {
            code << Indent(indent) << variable << ".SetHoverY(" << FloatLiteral(value) << ");\n";
        } else if (key == "hoverScale") {
            code << Indent(indent) << variable << ".SetHoverScale(" << FloatLiteral(value) << ");\n";
        } else if (key == "hoverRotate") {
            code << Indent(indent) << variable << ".SetHoverRotate(" << FloatLiteral(value) << ");\n";
        } else if (key == "hoverWithin") {
            code << Indent(indent) << variable << ".SetHoverWithin(" << (flag(value) ? "true" : "false") << ");\n";
        } else if (key == "clip") {
            code << Indent(indent) << variable << ".SetClip(" << (flag(value) ? "true" : "false") << ");\n";
        } else if (key == "toggle" && value == "theme") {
            code << Indent(indent) << variable << ".SetToggleTheme(true);\n";
        } else if (key == "gradient" && value == "horizontal") {
            code << Indent(indent) << variable << ".SetHorizontal(true);\n";
        } else if (key == "transition") {
            const std::size_t space = value.find(' ');
            const std::string duration = space == std::string::npos ? value : value.substr(0, space);
            const std::string easing = space == std::string::npos ? "linear" : value.substr(space + 1);
            code << Indent(indent) << variable << ".SetTransition(Robo::ParseDurationMs(\"" << EscapeCpp(duration)
                 << "\"), Robo::ParseEasing(\"" << EscapeCpp(easing) << "\"));\n";
        } else if (key == "animation") {
            std::string delay = "0s";
            const auto delayAttribute = node.attributes.find("animationDelay");
            if (delayAttribute != node.attributes.end()) {
                delay = delayAttribute->second;
            }
            code << Indent(indent) << variable << ".SetAnimation(\"" << EscapeCpp(value) << "\", Robo::ParseDurationMs(\""
                 << EscapeCpp(delay) << "\"));\n";
        } else if (key == "viewBox") {
            float box[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            int count = 0;
            std::string token;
            for (std::size_t index = 0; index <= value.size() && count < 4; ++index) {
                if (index == value.size() || value[index] == ' ' || value[index] == ',') {
                    if (!token.empty()) {
                        box[count++] = static_cast<float>(std::atof(token.c_str()));
                        token.clear();
                    }
                } else {
                    token.push_back(value[index]);
                }
            }
            code << Indent(indent) << variable << ".SetViewBox(" << CppFloat(box[0]) << ", " << CppFloat(box[1]) << ", "
                 << CppFloat(box[2]) << ", " << CppFloat(box[3]) << ");\n";
        } else if (key == "path") {
            const std::vector<CompiledCommand> commands = CompilePathData(value);
            for (const CompiledCommand& command : commands) {
                if (command.kind == 0) {
                    code << Indent(indent) << variable << ".AddCommand(Robo::SvgCommand::Move(" << CppFloat(command.x) << ", "
                         << CppFloat(command.y) << "));\n";
                } else if (command.kind == 1) {
                    code << Indent(indent) << variable << ".AddCommand(Robo::SvgCommand::Line(" << CppFloat(command.x) << ", "
                         << CppFloat(command.y) << "));\n";
                } else if (command.kind == 2) {
                    code << Indent(indent) << variable << ".AddCommand(Robo::SvgCommand::Cubic(" << CppFloat(command.c1x) << ", "
                         << CppFloat(command.c1y) << ", " << CppFloat(command.c2x) << ", " << CppFloat(command.c2y) << ", "
                         << CppFloat(command.x) << ", " << CppFloat(command.y) << "));\n";
                } else {
                    code << Indent(indent) << variable << ".AddCommand(Robo::SvgCommand::Close());\n";
                }
            }
        }
    }
    for (const XmlNode& child : node.children) {
        if (child.name == "Shape" || child.name == "MotionShape") {
            const std::string childName = NextShapeName();
            EmitShape(code, child, indent, childName);
            code << Indent(indent) << variable << ".AddChild(std::move(" << childName << "));\n";
        } else if (child.name == "Keyframes") {
            EmitKeyframes(code, child, indent);
        }
    }
}

void XmlToCpp::EmitSelectable(std::ostringstream& code,
                              int indent,
                              const std::string& styleName,
                              const XmlNode& node) const {
    for (const auto& attribute : node.attributes) {
        if (attribute.first == "selectable" || attribute.first == "select") {
            const std::string lhs = styleName + ".selectable";
            EmitEnum(code, indent, lhs.c_str(), attribute.second, MapSelectable, "ParseSelectable",
                     "Robo::SelectableMode::Inherit");
        }
    }
}

void XmlToCpp::EmitButtonFlags(std::ostringstream& code, int indent, const XmlNode& node) const {
    bool disabled = false;
    bool loading = false;
    std::string iconName;
    for (const auto& attribute : node.attributes) {
        if (attribute.first == "disabled" && attribute.second != "false") {
            disabled = true;
        } else if (attribute.first == "loading" && attribute.second != "false") {
            loading = true;
        } else if (attribute.first == "icon") {
            iconName = attribute.second;
        }
    }
    if (node.name == "IconButton") {
        code << Indent(indent) << "button.SetIconOnly(true);\n";
    }
    if (disabled) {
        code << Indent(indent) << "button.SetDisabled(true);\n";
    }
    if (loading) {
        code << Indent(indent) << "button.SetLoading(true);\n";
    }
    const auto icon = icons_.find(iconName);
    if (!iconName.empty() && icon != icons_.end()) {
        EmitPackedIcon(code, indent, "icon", iconName, 20.0f, "button.SetIcon(std::move(icon));");
    }
}

bool XmlToCpp::TryEmitGraphic(std::ostringstream& code,
                               const XmlNode& node,
                               int indent,
                               const std::string& parent,
                               bool withProps) const {
    const bool isSvg = node.name == "svg" || node.name == "Svg" || node.name == "SVG";
    const auto icon = icons_.find(node.name);
    const bool isIcon = icon != icons_.end();
    if (!isSvg && !isIcon) {
        return false;
    }

    const std::string props = withProps ? ", props" : "";
    if (!withProps) {
        code << Indent(indent - 1) << "{\n";
    }

    auto emitBox = [&](const std::string& styleName) {
        for (const auto& attribute : node.attributes) {
            if (attribute.first == "width") {
                if (attribute.second.find('%') != std::string::npos) {
                    code << Indent(indent) << styleName << ".widthSpec = \"" << EscapeCpp(attribute.second) << "\";\n";
                } else {
                    code << BindFloat(indent, styleName, "width", attribute.second, node.name);
                }
            } else if (attribute.first == "height") {
                if (attribute.second.find('%') != std::string::npos) {
                    code << Indent(indent) << styleName << ".heightSpec = \"" << EscapeCpp(attribute.second) << "\";\n";
                } else {
                    code << BindFloat(indent, styleName, "height", attribute.second, node.name);
                }
            } else if (attribute.first == "size") {
                code << Indent(indent) << styleName << ".width = " << FloatLiteral(attribute.second) << ";\n";
                code << Indent(indent) << styleName << ".height = " << FloatLiteral(attribute.second) << ";\n";
            } else if (attribute.first == "color") {
                code << BindColor(indent, styleName, "color", attribute.second, node.name);
            } else if (attribute.first == "opacity") {
                code << Indent(indent) << styleName << ".opacity = " << FloatLiteral(attribute.second) << ";\n";
            } else if (attribute.first == "align") {
                EmitEnum(code, indent, (styleName + ".align").c_str(), attribute.second, MapTextAlign, "ParseTextAlign",
                         "Robo::TextAlign::Left");
            }
        }
    };

    if (isIcon) {
        code << Indent(indent) << "Robo::Icon icon;\n";
        code << Indent(indent) << "if (Robo::RoboFillIconByName(icon, \"" << EscapeCpp(node.name) << "\")) {\n";
        code << Indent(indent + 1) << "Robo::SvgStyle packedStyle;\n";
        emitBox("packedStyle");
        code << Indent(indent + 1) << "if (packedStyle.width > 0.0f) icon.SetWidth(packedStyle.width);\n";
        code << Indent(indent + 1) << "if (packedStyle.height > 0.0f) icon.SetHeight(packedStyle.height);\n";
        code << Indent(indent + 1) << "if (!packedStyle.widthSpec.empty()) icon.SetWidthSpec(packedStyle.widthSpec);\n";
        code << Indent(indent + 1) << "if (!packedStyle.heightSpec.empty()) icon.SetHeightSpec(packedStyle.heightSpec);\n";
        code << Indent(indent + 1) << "for (const auto& token : packedStyle.tokens) icon.SetToken(token.first, token.second);\n";
        code << Indent(indent + 1) << "icon.SetColor(packedStyle.color);\n";
        code << Indent(indent + 1) << "icon.SetOpacity(packedStyle.opacity);\n";
        code << Indent(indent + 1) << "icon.SetAlign(packedStyle.align);\n";
        code << Indent(indent + 1) << parent << ".AddSvg(icon.Bake()" << props << ");\n";
        code << Indent(indent) << "}\n";
    } else {
        const CompiledSvg graphic = CompileSvgElement(node, false);
        code << Indent(indent) << "Robo::Svg svg;\n";
        code << Indent(indent) << "Robo::SvgStyle style;\n";
        code << Indent(indent) << "style.viewX = " << FloatLiteral(std::to_string(graphic.viewX)) << ";\n";
        code << Indent(indent) << "style.viewY = " << FloatLiteral(std::to_string(graphic.viewY)) << ";\n";
        code << Indent(indent) << "style.viewWidth = " << FloatLiteral(std::to_string(graphic.viewWidth)) << ";\n";
        code << Indent(indent) << "style.viewHeight = " << FloatLiteral(std::to_string(graphic.viewHeight)) << ";\n";
        emitBox("style");
        EmitSelectable(code, indent, "style", node);
        code << Indent(indent) << "svg.SetStyle(style);\n";
        code << EmitCompiledShapes(graphic.shapes, indent, "svg", false);
        code << Indent(indent) << parent << ".AddSvg(std::move(svg)" << props << ");\n";
    }

    if (!withProps) {
        code << Indent(indent - 1) << "}\n";
    }
    return true;
}

namespace {

std::string NodePlainText(const XmlNode& node) {
    std::string text;
    for (const XmlNode& child : node.children) {
        if (child.isText) {
            text += child.text;
        }
    }
    return text;
}

std::string BraceToken(const std::string& value) {
    if (value.size() >= 2 && value.front() == '{' && value.back() == '}') {
        return value.substr(1, value.size() - 2);
    }
    return {};
}

}  // namespace

void XmlToCpp::EmitPackedIcon(std::ostringstream& code, int indent, const std::string& iconVar, const std::string& iconName,
                               float size, const std::string& applyLine) const {
    code << Indent(indent) << "{\n";
    code << Indent(indent + 1) << "Robo::Svg " << iconVar << " = Robo::RoboBakeNamedIcon(\"" << EscapeCpp(iconName)
         << "\", " << FloatLiteral(std::to_string(static_cast<int>(size))) << ");\n";
    code << Indent(indent + 1) << "if (!" << iconVar << ".GetShapes().empty()) {\n";
    code << Indent(indent + 2) << applyLine << "\n";
    code << Indent(indent + 1) << "}\n";
    code << Indent(indent) << "}\n";
}

void XmlToCpp::EmitIconsPane(std::ostringstream& code, const XmlNode& pane, int indent) const {
    std::vector<std::string> galleryNames;
    std::vector<std::pair<std::string, std::string>> iconButtons;
    const XmlNode* wrapFlex = nullptr;
    const XmlNode* buttonFlex = nullptr;

    for (const XmlNode& child : pane.children) {
        if (child.isText || child.name != "Flexbox") {
            continue;
        }
        bool looksLikeGallery = false;
        for (const XmlNode& cell : child.children) {
            if (cell.isText || cell.name != "Flexbox") {
                continue;
            }
            const XmlNode* iconNode = nullptr;
            for (const XmlNode& part : cell.children) {
                if (!part.isText) {
                    iconNode = &part;
                    break;
                }
            }
            if (iconNode != nullptr && icons_.find(iconNode->name) != icons_.end()) {
                looksLikeGallery = true;
                break;
            }
        }
        if (looksLikeGallery && wrapFlex == nullptr) {
            wrapFlex = &child;
            for (const XmlNode& cell : child.children) {
                if (cell.isText || cell.name != "Flexbox") {
                    continue;
                }
                for (const XmlNode& part : cell.children) {
                    if (!part.isText && icons_.find(part.name) != icons_.end()) {
                        galleryNames.push_back(part.name);
                        break;
                    }
                }
            }
        } else if (buttonFlex == nullptr) {
            for (const XmlNode& part : child.children) {
                if (part.isText) {
                    continue;
                }
                if (part.name == "IconButton" && part.attributes.count("icon") != 0) {
                    const std::string iconName = part.attributes.at("icon");
                    std::string label = part.attributes.count("label") != 0 ? part.attributes.at("label") : iconName;
                    iconButtons.emplace_back(iconName, label);
                }
            }
            if (!iconButtons.empty()) {
                buttonFlex = &child;
            }
        }
    }

    code << Indent(indent) << "{\n";
    EmitContainer(code, pane, indent + 1, "scene", false);

    for (const XmlNode& child : pane.children) {
        if (child.isText) {
            continue;
        }
        if (wrapFlex != nullptr && &child == wrapFlex) {
            code << Indent(indent + 1) << "{\n";
            code << Indent(indent + 2) << "Robo::PositionItemProps props;\n";
            code << Indent(indent + 2) << "props.width = \"100%\";\n";
            code << Indent(indent + 2) << "Robo::Flexbox wrap;\n";
            code << Indent(indent + 2) << "Robo::FlexboxStyle style;\n";
            code << Indent(indent + 2) << "style.align = Robo::FlexAlign::FlexStart;\n";
            code << Indent(indent + 2) << "style.tokens[\"gap\"] = \"space.4\";\n";
            code << Indent(indent + 2) << "style.width = \"100%\";\n";
            code << Indent(indent + 2) << "style.widthLength = Robo::Length::Percent(100.0f);\n";
            code << Indent(indent + 2) << "style.hasWidthLength = true;\n";
            code << Indent(indent + 2) << "style.wrap = Robo::FlexWrap::Wrap;\n";
            code << Indent(indent + 2) << "wrap.SetStyle(style);\n";
            code << Indent(indent + 2) << "static const char* kRoboIconGallery[] = {\n";
            for (const std::string& name : galleryNames) {
                code << Indent(indent + 3) << "\"" << EscapeCpp(name) << "\",\n";
            }
            code << Indent(indent + 2) << "};\n";
            code << Indent(indent + 2)
                 << "static const int kRoboIconGalleryCount = "
                    "static_cast<int>(sizeof(kRoboIconGallery) / sizeof(kRoboIconGallery[0]));\n";
            code << Indent(indent + 2) << "scene.SetGalleryCount(kRoboIconGalleryCount);\n";
            code << Indent(indent + 2)
                 << "const int galleryLimit = scene.GalleryLimit() < kRoboIconGalleryCount ? scene.GalleryLimit() : "
                    "kRoboIconGalleryCount;\n";
            code << Indent(indent + 2) << "const int galleryFirst = scene.GalleryFirst();\n";
            code << Indent(indent + 2)
                 << "for (int i = galleryFirst; i < galleryFirst + galleryLimit && i < kRoboIconGalleryCount; ++i) {\n";
            code << Indent(indent + 3) << "const char* iconName = kRoboIconGallery[i];\n";
            code << Indent(indent + 3) << "Robo::FlexItemProps cellProps;\n";
            code << Indent(indent + 3) << "Robo::Flexbox cell;\n";
            code << Indent(indent + 3) << "Robo::FlexboxStyle cellStyle;\n";
            code << Indent(indent + 3) << "cellStyle.align = Robo::FlexAlign::Center;\n";
            code << Indent(indent + 3) << "cellStyle.direction = Robo::FlexDirection::Column;\n";
            code << Indent(indent + 3) << "cellStyle.tokens[\"gap\"] = \"space.2\";\n";
            code << Indent(indent + 3) << "cellStyle.tokens[\"width\"] = \"space.10\";\n";
            code << Indent(indent + 3) << "cell.SetStyle(cellStyle);\n";
            code << Indent(indent + 3) << "{\n";
            code << Indent(indent + 4) << "Robo::FlexItemProps props;\n";
            code << Indent(indent + 4) << "Robo::Icon icon;\n";
            code << Indent(indent + 4) << "if (Robo::RoboFillIconByName(icon, iconName)) {\n";
            code << Indent(indent + 5) << "icon.SetToken(\"width\", \"space.6\");\n";
            code << Indent(indent + 5) << "icon.SetToken(\"height\", \"space.6\");\n";
            code << Indent(indent + 5) << "icon.SetToken(\"color\", \"color.text.primary\");\n";
            code << Indent(indent + 5) << "cell.AddSvg(icon.Bake(), props);\n";
            code << Indent(indent + 4) << "}\n";
            code << Indent(indent + 3) << "}\n";
            code << Indent(indent + 3) << "{\n";
            code << Indent(indent + 4) << "Robo::FlexItemProps props;\n";
            code << Indent(indent + 4) << "Robo::Text text;\n";
            code << Indent(indent + 4) << "Robo::TextStyle style;\n";
            code << Indent(indent + 4) << "style.tokens[\"fontSize\"] = \"type.caption.size\";\n";
            code << Indent(indent + 4) << "style.tokens[\"color\"] = \"color.text.secondary\";\n";
            code << Indent(indent + 4) << "text.SetStyle(style);\n";
            code << Indent(indent + 4) << "text.AddPlain(iconName);\n";
            code << Indent(indent + 4) << "cell.AddText(std::move(text), props);\n";
            code << Indent(indent + 3) << "}\n";
            code << Indent(indent + 3) << "wrap.AddFlexbox(std::move(cell), cellProps);\n";
            code << Indent(indent + 2) << "}\n";
            code << Indent(indent + 2) << "container.GetContent().AddFlexbox(std::move(wrap), props);\n";
            code << Indent(indent + 1) << "}\n";
            continue;
        }
        if (buttonFlex != nullptr && &child == buttonFlex) {
            code << Indent(indent + 1) << "{\n";
            code << Indent(indent + 2) << "Robo::PositionItemProps props;\n";
            code << Indent(indent + 2) << "Robo::Flexbox row;\n";
            code << Indent(indent + 2) << "Robo::FlexboxStyle style;\n";
            code << Indent(indent + 2) << "style.align = Robo::FlexAlign::Center;\n";
            code << Indent(indent + 2) << "style.tokens[\"gap\"] = \"space.3\";\n";
            code << Indent(indent + 2) << "row.SetStyle(style);\n";
            code << Indent(indent + 2) << "struct RoboIconButtonRow { const char* icon; const char* label; };\n";
            code << Indent(indent + 2) << "static const RoboIconButtonRow kRoboIconButtons[] = {\n";
            for (const auto& button : iconButtons) {
                code << Indent(indent + 3) << "{\"" << EscapeCpp(button.first) << "\", \"" << EscapeCpp(button.second)
                     << "\"},\n";
            }
            code << Indent(indent + 2) << "};\n";
            code << Indent(indent + 2)
                 << "static const int kRoboIconButtonCount = "
                    "static_cast<int>(sizeof(kRoboIconButtons) / sizeof(kRoboIconButtons[0]));\n";
            code << Indent(indent + 2) << "for (int i = 0; i < kRoboIconButtonCount; ++i) {\n";
            code << Indent(indent + 3) << "Robo::FlexItemProps props;\n";
            code << Indent(indent + 3) << "Robo::Button button;\n";
            code << Indent(indent + 3) << "Robo::ButtonStyle style;\n";
            code << Indent(indent + 3) << "button.SetStyle(style);\n";
            code << Indent(indent + 3) << "button.SetLabel(kRoboIconButtons[i].label);\n";
            code << Indent(indent + 3) << "button.SetIconOnly(true);\n";
            code << Indent(indent + 3) << "button.SetIcon(Robo::RoboBakeNamedIcon(kRoboIconButtons[i].icon, 20.0f));\n";
            code << Indent(indent + 3) << "row.AddButton(std::move(button), props);\n";
            code << Indent(indent + 2) << "}\n";
            code << Indent(indent + 2) << "container.GetContent().AddFlexbox(std::move(row), props);\n";
            code << Indent(indent + 1) << "}\n";
            continue;
        }
        EmitPositionChild(code, child, indent + 1, "container.GetContent()");
    }

    code << Indent(indent + 1) << "scene.AddContainer(std::move(container));\n";
    code << Indent(indent) << "}\n";
}

void XmlToCpp::EmitProsePane(std::ostringstream& code, const XmlNode& pane, int indent) const {
    struct ProseRow {
        int headingLevel = 0;
        std::string text;
        bool selectable = false;
    };
    std::vector<ProseRow> blocks;
    std::vector<const XmlNode*> other;
    for (const XmlNode& child : pane.children) {
        if (child.isText) {
            continue;
        }
        int level = 0;
        if (child.name == "H1" || child.name == "h1") {
            level = 1;
        } else if (child.name == "H2" || child.name == "h2") {
            level = 2;
        } else if (child.name == "H3" || child.name == "h3") {
            level = 3;
        } else if (child.name == "Heading" && child.attributes.count("level") != 0) {
            level = std::atoi(child.attributes.at("level").c_str());
        }
        const bool isParagraph = child.name == "Paragraph" || child.name == "P" || child.name == "p";
        const bool isHeading = level > 0 || child.name == "Heading";
        if (!isParagraph && !isHeading) {
            other.push_back(&child);
            continue;
        }
        if (child.name == "Heading" && level <= 0) {
            level = 1;
        }
        ProseRow row;
        row.headingLevel = isHeading ? level : 0;
        for (const XmlNode& text : child.children) {
            if (text.isText) {
                row.text += text.text;
            }
        }
        if (child.attributes.count("selectable") != 0 && child.attributes.at("selectable") != "false" &&
            child.attributes.at("selectable") != "0") {
            row.selectable = true;
        }
        blocks.push_back(std::move(row));
    }
    code << Indent(indent) << "{\n";
    EmitContainer(code, pane, indent + 1, "scene", false);
    if (!blocks.empty()) {
        code << Indent(indent + 1) << "static const Robo::RoboTextBlock kRoboTextBlocks[] = {\n";
        for (const ProseRow& row : blocks) {
            code << Indent(indent + 2) << "{" << row.headingLevel << ", \"" << EscapeCpp(row.text) << "\", "
                 << (row.selectable ? "true" : "false") << "},\n";
        }
        code << Indent(indent + 1) << "};\n";
        code << Indent(indent + 1) << "Robo::RoboFillTextBlocks(container.GetContent(), kRoboTextBlocks, "
             << static_cast<int>(blocks.size()) << ");\n";
    }
    for (const XmlNode* node : other) {
        EmitPositionChild(code, *node, indent + 1, "container.GetContent()");
    }
    code << Indent(indent + 1) << "scene.AddContainer(std::move(container));\n";
    code << Indent(indent) << "}\n";
}

void XmlToCpp::EmitTypePane(std::ostringstream& code, const XmlNode& pane, int indent) const {
    std::vector<const XmlNode*> preface;
    std::vector<const XmlNode*> samples;
    std::vector<const XmlNode*> epilogue;
    bool sampling = false;
    bool finished = false;
    for (const XmlNode& child : pane.children) {
        if (child.isText) {
            continue;
        }
        const bool sample = child.name == "Text" && child.attributes.count("fontSize") != 0;
        if (!finished && sample) {
            sampling = true;
            samples.push_back(&child);
            continue;
        }
        if (sampling && !sample) {
            finished = true;
            sampling = false;
        }
        if (!finished) {
            preface.push_back(&child);
        } else {
            epilogue.push_back(&child);
        }
    }
    code << Indent(indent) << "{\n";
    EmitContainer(code, pane, indent + 1, "scene", false);
    for (const XmlNode* node : preface) {
        EmitPositionChild(code, *node, indent + 1, "container.GetContent()");
    }
    if (!samples.empty()) {
        code << Indent(indent + 1) << "static const Robo::RoboTypeSample kRoboTypeSamples[] = {\n";
        for (const XmlNode* node : samples) {
            const std::string size = ExtractToken(node->attributes.at("fontSize"));
            std::string weight;
            if (node->attributes.count("fontWeight") != 0) {
                weight = ExtractToken(node->attributes.at("fontWeight"));
            }
            std::string color = "color.text.primary";
            if (node->attributes.count("color") != 0) {
                const std::string token = ExtractToken(node->attributes.at("color"));
                if (!token.empty()) {
                    color = token;
                }
            }
            code << Indent(indent + 2) << "{\"" << EscapeCpp(size) << "\", \"" << EscapeCpp(weight) << "\", \""
                 << EscapeCpp(color) << "\", \"" << EscapeCpp(NodePlainText(*node)) << "\"},\n";
        }
        code << Indent(indent + 1) << "};\n";
        code << Indent(indent + 1) << "Robo::RoboFillTypeSamples(container.GetContent(), kRoboTypeSamples, "
             << static_cast<int>(samples.size()) << ");\n";
    }
    for (const XmlNode* node : epilogue) {
        EmitPositionChild(code, *node, indent + 1, "container.GetContent()");
    }
    code << Indent(indent + 1) << "scene.AddContainer(std::move(container));\n";
    code << Indent(indent) << "}\n";
}

void XmlToCpp::EmitSpacePane(std::ostringstream& code, const XmlNode& pane, int indent) const {
    std::vector<const XmlNode*> preface;
    std::vector<const XmlNode*> samples;
    std::vector<const XmlNode*> epilogue;
    bool sampling = false;
    bool finished = false;
    auto isSpaceRow = [](const XmlNode& child) {
        return child.name == "Flexbox" && child.attributes.count("gap") != 0;
    };
    for (const XmlNode& child : pane.children) {
        if (child.isText) {
            continue;
        }
        const bool sample = isSpaceRow(child);
        if (!finished && sample) {
            sampling = true;
            samples.push_back(&child);
            continue;
        }
        if (sampling && !sample) {
            finished = true;
        }
        if (!finished) {
            preface.push_back(&child);
        } else {
            epilogue.push_back(&child);
        }
    }
    code << Indent(indent) << "{\n";
    EmitContainer(code, pane, indent + 1, "scene", false);
    for (const XmlNode* node : preface) {
        EmitPositionChild(code, *node, indent + 1, "container.GetContent()");
    }
    if (!samples.empty()) {
        code << Indent(indent + 1) << "static const Robo::RoboSpaceRow kRoboSpaceRows[] = {\n";
        for (const XmlNode* node : samples) {
            std::string gap = ExtractToken(node->attributes.at("gap"));
            if (gap.empty()) {
                gap = node->attributes.at("gap");
            }
            std::string label;
            for (const XmlNode& child : node->children) {
                if (child.name == "Text") {
                    label = NodePlainText(child);
                }
            }
            code << Indent(indent + 2) << "{\"" << EscapeCpp(gap) << "\", \"" << EscapeCpp(label) << "\"},\n";
        }
        code << Indent(indent + 1) << "};\n";
        code << Indent(indent + 1) << "Robo::RoboFillSpaceRows(container.GetContent(), kRoboSpaceRows, "
             << static_cast<int>(samples.size()) << ");\n";
    }
    for (const XmlNode* node : epilogue) {
        EmitPositionChild(code, *node, indent + 1, "container.GetContent()");
    }
    code << Indent(indent + 1) << "scene.AddContainer(std::move(container));\n";
    code << Indent(indent) << "}\n";
}

void XmlToCpp::EmitRadiusPane(std::ostringstream& code, const XmlNode& pane, int indent) const {
    std::vector<const XmlNode*> preface;
    std::vector<const XmlNode*> samples;
    std::vector<const XmlNode*> epilogue;
    bool sawSamples = false;
    for (const XmlNode& child : pane.children) {
        if (child.isText) {
            continue;
        }
        bool radiusWrap = false;
        if (!sawSamples && child.name == "Flexbox") {
            int columns = 0;
            for (const XmlNode& column : child.children) {
                if (column.name == "Flexbox") {
                    ++columns;
                }
            }
            radiusWrap = columns > 0;
            if (radiusWrap) {
                for (const XmlNode& column : child.children) {
                    if (column.name == "Flexbox") {
                        samples.push_back(&column);
                    }
                }
                sawSamples = true;
                continue;
            }
        }
        if (!sawSamples) {
            preface.push_back(&child);
        } else {
            epilogue.push_back(&child);
        }
    }
    code << Indent(indent) << "{\n";
    EmitContainer(code, pane, indent + 1, "scene", false);
    for (const XmlNode* node : preface) {
        EmitPositionChild(code, *node, indent + 1, "container.GetContent()");
    }
    if (!samples.empty()) {
        code << Indent(indent + 1) << "static const Robo::RoboRadiusSample kRoboRadiusSamples[] = {\n";
        for (const XmlNode* column : samples) {
            std::string radius;
            std::string label;
            for (const XmlNode& child : column->children) {
                if (child.name == "Container" && child.attributes.count("cornerRadius") != 0) {
                    radius = ExtractToken(child.attributes.at("cornerRadius"));
                    if (radius.empty()) {
                        radius = child.attributes.at("cornerRadius");
                    }
                }
                if (child.name == "Text") {
                    label = NodePlainText(child);
                }
            }
            code << Indent(indent + 2) << "{\"" << EscapeCpp(radius) << "\", \"" << EscapeCpp(label) << "\"},\n";
        }
        code << Indent(indent + 1) << "};\n";
        code << Indent(indent + 1) << "Robo::RoboFillRadiusSamples(container.GetContent(), kRoboRadiusSamples, "
             << static_cast<int>(samples.size()) << ");\n";
    }
    for (const XmlNode* node : epilogue) {
        EmitPositionChild(code, *node, indent + 1, "container.GetContent()");
    }
    code << Indent(indent + 1) << "scene.AddContainer(std::move(container));\n";
    code << Indent(indent) << "}\n";
}

void XmlToCpp::EmitColorPane(std::ostringstream& code, const XmlNode& pane, int indent) const {
    struct ColorRow {
        std::string token;
        std::string label;
        std::string hex;
        std::string borderToken;
    };
    struct ColorSection {
        std::string heading;
        std::vector<ColorRow> rows;
        bool hasHeading = false;
    };
    std::vector<ColorSection> sections;
    ColorSection current;
    std::vector<const XmlNode*> preface;
    std::vector<const XmlNode*> epilogue;
    bool collecting = true;

    auto flushSection = [&]() {
        if (current.hasHeading || !current.rows.empty()) {
            sections.push_back(current);
            current = ColorSection{};
        }
    };

    for (const XmlNode& child : pane.children) {
        if (child.isText) {
            continue;
        }
        if (child.name == "H1" || (child.name == "Paragraph" && sections.empty() && current.rows.empty())) {
            if (collecting && sections.empty() && current.rows.empty()) {
                preface.push_back(&child);
            } else {
                flushSection();
                collecting = false;
                epilogue.push_back(&child);
            }
            continue;
        }
        if (child.name == "H2") {
            if (!collecting) {
                epilogue.push_back(&child);
                continue;
            }
            flushSection();
            current.heading = NodePlainText(child);
            current.hasHeading = true;
            continue;
        }
        if (child.name == "Flexbox" && collecting) {
            const XmlNode* swatch = nullptr;
            const XmlNode* labelNode = nullptr;
            const XmlNode* hexNode = nullptr;
            int partIndex = 0;
            for (const XmlNode& part : child.children) {
                if (part.isText) {
                    continue;
                }
                if (partIndex == 0) {
                    swatch = &part;
                } else if (partIndex == 1) {
                    labelNode = &part;
                } else if (partIndex == 2) {
                    hexNode = &part;
                }
                ++partIndex;
            }
            std::string bgToken;
            std::string borderToken = "color.border.subtle";
            if (swatch != nullptr && swatch->name == "Container") {
                if (swatch->attributes.count("background") != 0) {
                    bgToken = BraceToken(swatch->attributes.at("background"));
                }
                if (swatch->attributes.count("borderColor") != 0) {
                    const std::string border = BraceToken(swatch->attributes.at("borderColor"));
                    if (!border.empty()) {
                        borderToken = border;
                    }
                }
            }
            if (!bgToken.empty() && labelNode != nullptr && hexNode != nullptr) {
                ColorRow row;
                row.token = bgToken;
                row.label = NodePlainText(*labelNode);
                row.hex = NodePlainText(*hexNode);
                row.borderToken = borderToken;
                current.rows.push_back(std::move(row));
                continue;
            }
        }
        flushSection();
        collecting = false;
        epilogue.push_back(&child);
    }
    flushSection();

    code << Indent(indent) << "{\n";
    EmitContainer(code, pane, indent + 1, "scene", false);

    for (const XmlNode* node : preface) {
        EmitPositionChild(code, *node, indent + 1, "container.GetContent()");
    }

    for (std::size_t sectionIndex = 0; sectionIndex < sections.size(); ++sectionIndex) {
        const ColorSection& section = sections[sectionIndex];
        code << Indent(indent + 1) << "static const Robo::RoboColorSwatch kRoboColorRows" << sectionIndex << "[] = {\n";
        for (const ColorRow& row : section.rows) {
            code << Indent(indent + 2) << "{\"" << EscapeCpp(row.token) << "\", \"" << EscapeCpp(row.label)
                 << "\", \"" << EscapeCpp(row.hex) << "\", \"" << EscapeCpp(row.borderToken) << "\"},\n";
        }
        if (section.rows.empty()) {
            code << Indent(indent + 2) << "{nullptr, nullptr, nullptr, nullptr},\n";
        }
        code << Indent(indent + 1) << "};\n";
    }

    code << Indent(indent + 1) << "static const Robo::RoboColorSection kRoboColorSections[] = {\n";
    for (std::size_t sectionIndex = 0; sectionIndex < sections.size(); ++sectionIndex) {
        const ColorSection& section = sections[sectionIndex];
        const int count = section.rows.empty() ? 0 : static_cast<int>(section.rows.size());
        code << Indent(indent + 2) << "{\"" << EscapeCpp(section.heading) << "\", kRoboColorRows" << sectionIndex
             << ", " << count << "},\n";
    }
    code << Indent(indent + 1) << "};\n";
    code << Indent(indent + 1)
         << "static const int kRoboColorSectionCount = "
            "static_cast<int>(sizeof(kRoboColorSections) / sizeof(kRoboColorSections[0]));\n";

    code << Indent(indent + 1)
         << "Robo::RoboFillColorSections(container.GetContent(), kRoboColorSections, kRoboColorSectionCount);\n";

    for (const XmlNode* node : epilogue) {
        EmitPositionChild(code, *node, indent + 1, "container.GetContent()");
    }

    code << Indent(indent + 1) << "scene.AddContainer(std::move(container));\n";
    code << Indent(indent) << "}\n";
}

std::string XmlToCpp::Convert(const std::string& xml, const std::string& assetDirectory) const {
    icons_ = LoadIconPacks(assetDirectory);
    shapeSerial_ = 0;
    foundations_.clear();
    emittedFoundations_.clear();
    foundations_.insert("RoboFoundation");
    emittedFoundations_.insert("RoboFoundation");
    XmlParser parser;
    const XmlNode root = parser.Parse(xml);
    CollectFoundations(root);
    bool pageHasDesktop = false;
    bool pageHasCatalog = false;
    bool pageHasPanes = false;
    bool pageHasTitleBar = false;
    std::function<void(const XmlNode&)> scan = [&](const XmlNode& node) {
        if (node.isText) {
            return;
        }
        if (node.name == "Desktop") {
            pageHasDesktop = true;
        }
        if (node.name == "Sidebar" || node.name == "Pane") {
            pageHasCatalog = true;
        }
        if (node.name == "Container" && node.attributes.count("when") != 0) {
            pageHasPanes = true;
        }
        if (node.name == "TitleBar") {
            pageHasTitleBar = true;
        }
        for (const XmlNode& child : node.children) {
            scan(child);
        }
    };
    scan(root);

    std::ostringstream code;
    if (pageHasDesktop && !pageHasCatalog && !pageHasPanes) {
        code << "#include \"Robo/RoboScene.hpp\"\n";
        code << "#include \"Robo/RoboPageTables.hpp\"\n\n";
    } else if ((pageHasCatalog || pageHasPanes) && !pageHasDesktop) {
        // Pack-driven catalog/pane pages keep a thin include set. Pages that also
        // emit eager controls (inline Sidebar + gallery content) need full headers.
        bool needsFullHeaders = false;
        std::function<void(const XmlNode&)> probe = [&](const XmlNode& node) {
            if (node.name == "Heading" || node.name == "H1" || node.name == "H2" || node.name == "H3" ||
                node.name == "Paragraph" || node.name == "P" || node.name == "Button" || node.name == "Input" ||
                node.name == "Flexbox" || node.name == "Flex" || node.name == "Grid" || node.name == "Container" ||
                node.name == "BrandLogo" || node.name == "ThemeToggle" || node.name == "ThemeIconToggle" ||
                node.name == "Shape" || node.name == "MotionShape" || node.name == "Separator") {
                needsFullHeaders = true;
            }
            for (const XmlNode& child : node.children) {
                probe(child);
            }
        };
        probe(root);
        if (!needsFullHeaders) {
            code << "#include \"Robo/RoboScene.hpp\"\n";
            code << "#include \"Robo/TitleBar.hpp\"\n";
            code << "#include \"Robo/Sidebar.hpp\"\n";
            code << "#include \"Robo/RoboCatalog.hpp\"\n";
            if (pageHasPanes) {
                code << "#include \"Robo/RoboPanePack.hpp\"\n";
            }
            (void)pageHasTitleBar;
            code << "#include \"Robo/RoboPageTables.hpp\"\n\n";
        } else {
            goto full_headers;
        }
    } else {
full_headers:
        code << "#include \"Robo/Text.hpp\"\n";
        code << "#include \"Robo/Foundation.hpp\"\n";
        code << "#include \"Robo/Svg.hpp\"\n";
        code << "#include \"Robo/Icon.hpp\"\n";
        code << "#include \"Robo/BrandLogo.hpp\"\n";
        code << "#include \"Robo/TitleBar.hpp\"\n";
        code << "#include \"Robo/Sidebar.hpp\"\n";
        code << "#include \"Robo/Separator.hpp\"\n";
        code << "#include \"Robo/ThemeToggle.hpp\"\n";
        code << "#include \"Robo/ThemeIconToggle.hpp\"\n";
        code << "#include \"Robo/Motion.hpp\"\n";
        code << "#include \"Robo/MotionShape.hpp\"\n";
        code << "#include \"Robo/Heading.hpp\"\n";
        code << "#include \"Robo/Paragraph.hpp\"\n";
        code << "#include \"Robo/Button.hpp\"\n";
        code << "#include \"Robo/Input.hpp\"\n";
        code << "#include \"Robo/Scrollbar.hpp\"\n";
        code << "#include \"Robo/Container.hpp\"\n";
        code << "#include \"Robo/Flexbox.hpp\"\n";
        code << "#include \"Robo/Grid.hpp\"\n";
        code << "#include \"Robo/Position.hpp\"\n";
        code << "#include \"Robo/RoboScene.hpp\"\n";
        code << "#include \"Robo/Color.hpp\"\n";
        code << "#include \"Robo/FontStyle.hpp\"\n";
        code << "#include \"Robo/TextAlign.hpp\"\n";
        code << "#include \"Robo/Selectable.hpp\"\n";
        code << "#include \"Robo/RoboIconPack.hpp\"\n";
        code << "#include \"Robo/RoboCatalog.hpp\"\n";
        code << "#include \"Robo/RoboPanePack.hpp\"\n";
        code << "#include \"Robo/RoboPageTables.hpp\"\n\n";
    }
    code << ConvertNode(root, 0);
    return code.str();
}

}  // namespace Robo
