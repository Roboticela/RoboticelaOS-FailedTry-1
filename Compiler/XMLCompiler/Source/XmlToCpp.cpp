#include "XmlToCpp.hpp"
#include "IconPack.hpp"
#include "XmlParser.hpp"

#include "Robo/Foundation.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace Robo {

namespace {

bool IsPaneItem(const std::string& name) {
    return name == "SidebarItem" || name == "PaneItem" || name == "PaneItems";
}

}  // namespace

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
    return Indent(indent) + object + ".tokens[\"" + field + "\"] = \"\";\n" + Indent(indent) + object + "." + field +
           " = \"" + EscapeCpp(value) + "\";\n";
}

std::string XmlToCpp::BindWeight(int indent, const std::string& object, const std::string& value, const std::string& element) const {
    const std::string token = ExtractToken(value);
    if (!token.empty()) {
        return Indent(indent) + object + ".tokens[\"fontWeight\"] = \"" + EscapeCpp(token) + "\";\n";
    }
    WarnLiteral(element, "fontWeight", value);
    return Indent(indent) + object + ".tokens[\"fontWeight\"] = \"\";\n" + Indent(indent) + object +
           ".fontWeight = Robo::ParseFontWeight(\"" + EscapeCpp(value) + "\");\n";
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
        std::ostringstream parts;
        std::ostringstream calls;
        int part = 0;
        for (const XmlNode& child : node.children) {
            if (child.name == "Foundation" || child.name == "FoundationUse" || child.name == "Keyframes" || child.isText) {
                continue;
            }
            if (child.name == "Sidebar" || child.name == "Pane") {
                int itemIndex = 0;
                std::ostringstream body;
                body << Indent(indent + 1) << "Robo::Sidebar sidebar;\n";
                if (child.attributes.count("variant") != 0) {
                    body << Indent(indent + 1) << "sidebar.SetVariant(Robo::ParseSidebarVariant(\""
                         << EscapeCpp(child.attributes.at("variant")) << "\"));\n";
                }
                for (const XmlNode& item : child.children) {
                    if (!IsPaneItem(item.name)) {
                        continue;
                    }
                    const std::string function = "BuildSidebarItem" + std::to_string(part) + "_" + std::to_string(itemIndex++);
                    parts << "static Robo::SidebarItem " << function << "() {\n";
                    EmitSidebarItem(parts, item, 1, "item");
                    parts << "    return item;\n}\n\n";
                    body << Indent(indent + 1) << "sidebar.AddItem(" << function << "());\n";
                }
                body << Indent(indent + 1) << "scene.AddSidebar(std::move(sidebar));\n";
                parts << "static void BuildPagePart" << part << "(Robo::RoboScene& scene) {\n";
                parts << body.str();
                parts << "}\n\n";
            } else {
                parts << "static void BuildPagePart" << part << "(Robo::RoboScene& scene) {\n";
                parts << ConvertNode(child, indent + 1);
                parts << "}\n\n";
            }
            calls << Indent(indent + 1) << "BuildPagePart" << part << "(scene);\n";
            ++part;
        }
        code << parts.str();
        code << Indent(indent) << "// Generated from Robo XML\n";
        code << Indent(indent) << "void BuildPage(Robo::RoboScene& scene) {\n";
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
        for (const XmlNode& child : node.children) {
            if (child.name == "Keyframes") {
                code << ConvertNode(child, indent + 1);
            }
        }
        code << calls.str();
        code << Indent(indent) << "}\n";
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
                code << Indent(indent + 1) << "style.align = Robo::ParseTextAlign(\"" << EscapeCpp(attribute.second)
                     << "\");\n";
            } else if (attribute.first == "wrap") {
                const bool wrap = !(attribute.second == "false" || attribute.second == "0" || attribute.second == "no");
                code << Indent(indent + 1) << "style.wrap = " << (wrap ? "true" : "false") << ";\n";
            } else if (attribute.first == "fontWeight") {
                code << BindWeight(indent + 1, "style", attribute.second, node.name);
            } else if (attribute.first == "fontStyle") {
                code << Indent(indent + 1) << "style.fontStyle = Robo::ParseFontStyle(\"" << EscapeCpp(attribute.second)
                     << "\");\n";
            } else if (attribute.first == "textDecoration") {
                code << Indent(indent + 1) << "style.textDecoration = Robo::ParseTextDecoration(\""
                     << EscapeCpp(attribute.second) << "\");\n";
            } else if (attribute.first == "lineHeight") {
                code << BindFloat(indent + 1, "style", "lineHeight", attribute.second, node.name);
            } else if (attribute.first == "letterSpacing") {
                code << BindFloat(indent + 1, "style", "letterSpacing", attribute.second, node.name);
            } else if (attribute.first == "opacity") {
                code << Indent(indent + 1) << "style.opacity = " << attribute.second << ";\n";
            } else if (attribute.first == "width") {
                code << BindLength(indent + 1, "style", "width", attribute.second, node.name);
            } else if (attribute.first == "maxWidth") {
                code << Indent(indent + 1) << "style.maxWidth = \"" << EscapeCpp(attribute.second) << "\";\n";
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
                        code << Indent(indent + 2) << "run.fontWeight = Robo::ParseFontWeight(\""
                             << EscapeCpp(attribute.second) << "\");\n";
                        code << Indent(indent + 2) << "run.hasFontWeight = true;\n";
                    } else if (attribute.first == "fontStyle") {
                        code << Indent(indent + 2) << "run.fontStyle = Robo::ParseFontStyle(\""
                             << EscapeCpp(attribute.second) << "\");\n";
                        code << Indent(indent + 2) << "run.hasFontStyle = true;\n";
                    } else if (attribute.first == "textDecoration") {
                        code << Indent(indent + 2) << "run.textDecoration = Robo::ParseTextDecoration(\""
                             << EscapeCpp(attribute.second) << "\");\n";
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
        code << Indent(indent + 1) << "Robo::TextStyle style = text.GetStyle();\n";

        for (const auto& attribute : node.attributes) {
            if (attribute.first == "fontSize") {
                code << BindFloat(indent + 1, "style", "fontSize", attribute.second, node.name);
            } else if (attribute.first == "color") {
                code << BindColor(indent + 1, "style", "color", attribute.second, node.name);
            } else if (attribute.first == "align") {
                code << Indent(indent + 1) << "style.align = Robo::ParseTextAlign(\"" << EscapeCpp(attribute.second)
                     << "\");\n";
            } else if (attribute.first == "wrap") {
                const bool wrap = !(attribute.second == "false" || attribute.second == "0" || attribute.second == "no");
                code << Indent(indent + 1) << "style.wrap = " << (wrap ? "true" : "false") << ";\n";
            } else if (attribute.first == "fontWeight") {
                code << BindWeight(indent + 1, "style", attribute.second, node.name);
            } else if (attribute.first == "lineHeight") {
                code << BindFloat(indent + 1, "style", "lineHeight", attribute.second, node.name);
            } else if (attribute.first == "width") {
                code << BindLength(indent + 1, "style", "width", attribute.second, node.name);
            } else if (attribute.first == "maxWidth") {
                code << Indent(indent + 1) << "style.maxWidth = \"" << EscapeCpp(attribute.second) << "\";\n";
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
                code << Indent(indent + 1) << "style.variant = Robo::ParseButtonVariant(\"" << EscapeCpp(attribute.second)
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
            } else if (attribute.first == "paddingX") {
                code << BindFloat(indent + 1, "style", "paddingX", attribute.second, node.name);
            } else if (attribute.first == "paddingY") {
                code << BindFloat(indent + 1, "style", "paddingY", attribute.second, node.name);
            } else if (attribute.first == "align") {
                code << Indent(indent + 1) << "style.align = Robo::ParseTextAlign(\"" << EscapeCpp(attribute.second)
                     << "\");\n";
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
                code << Indent(indent + 1) << "style.align = Robo::ParseTextAlign(\"" << EscapeCpp(attribute.second)
                     << "\");\n";
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
            code << Indent(indent) << "style.showArrows = Robo::ParseScrollbarFlag(\""
                 << EscapeCpp(attribute.second) << "\", false);\n";
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
            code << Indent(indent) << "style.autoHide = Robo::ParseScrollbarFlag(\""
                 << EscapeCpp(attribute.second) << "\", false);\n";
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
                             const std::string& targetVar) const {
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
            code << Indent(indent) << "style.clipToBounds = Robo::ParseBool(\"" << EscapeCpp(attribute.second)
                 << "\", true);\n";
        } else if (attribute.first == "showScrollbar" || attribute.first == "scrollbar") {
            code << Indent(indent) << "style.showScrollbar = Robo::ParseBool(\"" << EscapeCpp(attribute.second)
                 << "\", true);\n";
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
            code << Indent(indent) << "style.scrollbarStyle.showArrows = Robo::ParseBool(\""
                 << EscapeCpp(attribute.second) << "\", false);\n";
        } else if (attribute.first == "arrowColor") {
            code << BindColor(indent, "style.scrollbarStyle", "arrowColor", attribute.second, node.name);
            code << Indent(indent) << "style.scrollbarStyle.hasCustomArrowColor = true;\n";
        } else if (attribute.first == "autoHide" || attribute.first == "autohide") {
            code << Indent(indent) << "style.scrollbarStyle.autoHide = Robo::ParseBool(\""
                 << EscapeCpp(attribute.second) << "\", false);\n";
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

    for (const auto& attribute : node.attributes) {
        if (attribute.first == "value" || attribute.first == "scroll" || attribute.first == "scrollValue") {
            code << Indent(indent) << containerVar << ".SetScrollValue(" << attribute.second << ");\n";
        }
    }

    if (targetVar == "scene") {
        code << Indent(indent) << "scene.AddContainer(std::move(" << containerVar << "));\n";
    }
}

void XmlToCpp::EmitFlexbox(std::ostringstream& code, const XmlNode& node, int indent, const std::string& targetVar) const {
    const std::string flexVar = (targetVar == "scene") ? "flexbox" : targetVar;
    code << Indent(indent) << "Robo::Flexbox " << flexVar << ";\n";
    code << Indent(indent) << "Robo::FlexboxStyle style;\n";

    for (const auto& attribute : node.attributes) {
        if (attribute.first == "direction" || attribute.first == "flexDirection") {
            code << Indent(indent) << "style.direction = Robo::ParseFlexDirection(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "justify" || attribute.first == "justifyContent") {
            code << Indent(indent) << "style.justify = Robo::ParseFlexJustify(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "align" || attribute.first == "alignItems") {
            code << Indent(indent) << "style.align = Robo::ParseFlexAlign(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "wrap" || attribute.first == "flexWrap") {
            code << Indent(indent) << "style.wrap = Robo::ParseFlexWrap(\""
                 << EscapeCpp(attribute.second) << "\");\n";
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
            code << Indent(indent + 1) << "props.alignSelf = Robo::ParseFlexAlign(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "width") {
            if (ExtractToken(attribute.second).empty()) {
                if (!IsFreeLength(attribute.second)) {
                    WarnLiteral(child.name, "width", attribute.second);
                }
                code << Indent(indent + 1) << "props.width = \"" << EscapeCpp(attribute.second) << "\";\n";
            }
        } else if (attribute.first == "height") {
            if (ExtractToken(attribute.second).empty()) {
                if (!IsFreeLength(attribute.second)) {
                    WarnLiteral(child.name, "height", attribute.second);
                }
                code << Indent(indent + 1) << "props.height = \"" << EscapeCpp(attribute.second) << "\";\n";
            }
        } else if (attribute.first == "minWidth") {
            code << Indent(indent + 1) << "props.minWidth = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "maxWidth") {
            code << Indent(indent + 1) << "props.maxWidth = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "minHeight") {
            code << Indent(indent + 1) << "props.minHeight = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "maxHeight") {
            code << Indent(indent + 1) << "props.maxHeight = \"" << EscapeCpp(attribute.second) << "\";\n";
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
                code << Indent(indent + 1) << "style.variant = Robo::ParseButtonVariant(\"" << EscapeCpp(attribute.second) << "\");\n";
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
            code << Indent(indent) << "style.autoFlow = Robo::ParseGridAutoFlow(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "justifyItems" || attribute.first == "justify") {
            code << Indent(indent) << "style.justifyItems = Robo::ParseGridAlignment(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "alignItems" || attribute.first == "align") {
            code << Indent(indent) << "style.alignItems = Robo::ParseGridAlignment(\""
                 << EscapeCpp(attribute.second) << "\");\n";
        } else if (attribute.first == "justifyContent") {
            code << Indent(indent) << "style.justifyContent = Robo::ParseGridJustifyContent(\""
                 << EscapeCpp(attribute.second) << "\");\n";
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
                code << Indent(indent + 1) << "props.width = \"" << EscapeCpp(attribute.second) << "\";\n";
            }
        } else if (attribute.first == "height") {
            if (ExtractToken(attribute.second).empty()) {
                if (!IsFreeLength(attribute.second)) {
                    WarnLiteral(child.name, "height", attribute.second);
                }
                code << Indent(indent + 1) << "props.height = \"" << EscapeCpp(attribute.second) << "\";\n";
            }
        } else if (attribute.first == "minWidth") {
            code << Indent(indent + 1) << "props.minWidth = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "maxWidth") {
            code << Indent(indent + 1) << "props.maxWidth = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "minHeight") {
            code << Indent(indent + 1) << "props.minHeight = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "maxHeight") {
            code << Indent(indent + 1) << "props.maxHeight = \"" << EscapeCpp(attribute.second) << "\";\n";
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
                code << Indent(indent + 1) << "style.variant = Robo::ParseButtonVariant(\"" << EscapeCpp(attribute.second) << "\");\n";
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
                code << Indent(indent + 1) << "props.width = \"" << EscapeCpp(attribute.second) << "\";\n";
            }
        } else if (attribute.first == "height") {
            if (ExtractToken(attribute.second).empty()) {
                if (!IsFreeLength(attribute.second)) {
                    WarnLiteral(child.name, "height", attribute.second);
                }
                code << Indent(indent + 1) << "props.height = \"" << EscapeCpp(attribute.second) << "\";\n";
            }
        } else if (attribute.first == "minWidth") {
            code << Indent(indent + 1) << "props.minWidth = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "maxWidth") {
            code << Indent(indent + 1) << "props.maxWidth = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "minHeight") {
            code << Indent(indent + 1) << "props.minHeight = \"" << EscapeCpp(attribute.second) << "\";\n";
        } else if (attribute.first == "maxHeight") {
            code << Indent(indent + 1) << "props.maxHeight = \"" << EscapeCpp(attribute.second) << "\";\n";
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
                code << Indent(indent + 1) << "style.variant = Robo::ParseButtonVariant(\"" << EscapeCpp(attribute.second) << "\");\n";
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
                                 const std::string& variable) const {
    code << Indent(indent) << "Robo::SidebarItem " << variable << ";\n";
    std::string label;
    std::string iconName;
    if (node.attributes.count("label") != 0) {
        label = node.attributes.at("label");
    }
    if (node.attributes.count("icon") != 0) {
        iconName = node.attributes.at("icon");
    }
    for (const XmlNode& child : node.children) {
        if (child.isText) {
            label += child.text;
        }
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
    if (!iconName.empty() && icon != icons_.end()) {
        code << Indent(indent) << "{\n";
        code << Indent(indent + 1) << "Robo::Icon icon;\n";
        code << Indent(indent + 1) << "icon.SetViewBox(" << FloatLiteral(std::to_string(icon->second.viewX)) << ", "
             << FloatLiteral(std::to_string(icon->second.viewY)) << ", "
             << FloatLiteral(std::to_string(icon->second.viewWidth)) << ", "
             << FloatLiteral(std::to_string(icon->second.viewHeight)) << ");\n";
        code << Indent(indent + 1) << "icon.SetWidth(20.0f);\n";
        code << Indent(indent + 1) << "icon.SetHeight(20.0f);\n";
        code << EmitCompiledShapes(icon->second.shapes, indent + 1, "icon", true);
        code << Indent(indent + 1) << variable << ".SetIcon(icon.Bake());\n";
        code << Indent(indent) << "}\n";
    }
    int childIndex = 0;
    for (const XmlNode& child : node.children) {
        if (!IsPaneItem(child.name)) {
            continue;
        }
        const std::string childName = variable + "Child" + std::to_string(childIndex++);
        code << Indent(indent) << "{\n";
        EmitSidebarItem(code, child, indent + 1, childName);
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
        code << Indent(indent) << variable << ".SetVariant(Robo::ParseSidebarVariant(\"" << EscapeCpp(node.attributes.at("variant")) << "\"));\n";
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
    for (const auto& attribute : node.attributes) {
        if (attribute.first == "variant") {
            code << Indent(indent) << variable << ".SetVariant(Robo::ParseSeparatorVariant(\"" << EscapeCpp(attribute.second) << "\"));\n";
        } else if (attribute.first == "size") {
            code << Indent(indent) << variable << ".SetSize(Robo::ParseSeparatorSize(\"" << EscapeCpp(attribute.second) << "\"));\n";
        } else if (attribute.first == "tone") {
            code << Indent(indent) << variable << ".SetTone(Robo::ParseSeparatorTone(\"" << EscapeCpp(attribute.second) << "\"));\n";
        } else if (attribute.first == "align") {
            code << Indent(indent) << variable << ".SetAlign(Robo::ParseSeparatorAlign(\"" << EscapeCpp(attribute.second) << "\"));\n";
        } else if (attribute.first == "orientation") {
            code << Indent(indent) << variable << ".SetOrientation(Robo::ParseSeparatorOrientation(\"" << EscapeCpp(attribute.second) << "\"));\n";
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
        code << Indent(indent) << "{\n";
        code << Indent(indent + 1) << "Robo::Icon icon;\n";
        code << Indent(indent + 1) << "icon.SetViewBox(" << FloatLiteral(std::to_string(icon->second.viewX)) << ", "
             << FloatLiteral(std::to_string(icon->second.viewY)) << ", "
             << FloatLiteral(std::to_string(icon->second.viewWidth)) << ", "
             << FloatLiteral(std::to_string(icon->second.viewHeight)) << ");\n";
        code << Indent(indent + 1) << "icon.SetWidth(22.0f);\n";
        code << Indent(indent + 1) << "icon.SetHeight(22.0f);\n";
        code << EmitCompiledShapes(icon->second.shapes, indent + 1, "icon", true);
        code << Indent(indent + 1) << variable << ".SetIcon(icon.Bake());\n";
        code << Indent(indent) << "}\n";
    }
}

void XmlToCpp::EmitDesktop(std::ostringstream& code, const XmlNode& node, int indent) const {
    code << Indent(indent) << "scene.EnableDesktop();\n";
    int dockIndex = 0;
    for (const XmlNode& child : node.children) {
        if (child.name == "Background") {
            std::string color = "{color.bg.primary}";
            if (child.attributes.count("color") != 0) {
                color = child.attributes.at("color");
            } else if (child.attributes.count("fill") != 0) {
                color = child.attributes.at("fill");
            }
            const std::string token = ExtractToken(color);
            if (!token.empty()) {
                code << Indent(indent) << "scene.SetDesktopBackground(Robo::Color::FromHex(\"#202838\"), \""
                     << EscapeCpp(token) << "\");\n";
            } else {
                WarnLiteral("Background", "color", color);
                code << Indent(indent) << "scene.SetDesktopBackground(Robo::Color::FromHex(\"" << EscapeCpp(color)
                     << "\"), \"\");\n";
            }
            std::string source;
            if (child.attributes.count("src") != 0) {
                source = child.attributes.at("src");
            } else if (child.attributes.count("path") != 0) {
                source = child.attributes.at("path");
            }
            if (child.attributes.count("kind") != 0 && !source.empty()) {
                source = child.attributes.at("kind") + ":" + source;
            }
            if (!source.empty()) {
                code << Indent(indent) << "scene.SetDesktopBackgroundPath(\"" << EscapeCpp(source) << "\");\n";
            }
        } else if (child.name == "AppShell") {
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
    std::string app = "Finder";
    if (node.attributes.count("app") != 0) {
        app = node.attributes.at("app");
    } else if (node.attributes.count("title") != 0) {
        app = node.attributes.at("title");
    }
    code << Indent(indent + 1) << "topBarMenu.SetApp(\"" << EscapeCpp(app) << "\");\n";
    if (node.attributes.count("clock") != 0) {
        code << Indent(indent + 1) << "topBarMenu.SetClock(\"" << EscapeCpp(node.attributes.at("clock")) << "\");\n";
    }
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
        if (item.attributes.count("selected") != 0 && item.attributes.at("selected") != "false" &&
            item.attributes.at("selected") != "0") {
            code << Indent(indent + 1) << itemName << ".SetSelected(true);\n";
        }
        code << Indent(indent + 1) << "topBarMenu.AddItem(std::move(" << itemName << "));\n";
    }
    code << Indent(indent + 1) << "scene.SetTopBarMenu(std::move(topBarMenu));\n";
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

    const auto shown = icons_.find("Sidebar");
    const auto hidden = icons_.find("SidebarHidden");
    if (shown != icons_.end() && hidden != icons_.end()) {
        auto emitIcon = [&](const CompiledSvg& graphic, const std::string& name) {
            code << Indent(indent + 1) << "Robo::Icon " << name << ";\n";
            code << Indent(indent + 1) << name << ".SetViewBox(" << FloatLiteral(std::to_string(graphic.viewX)) << ", "
                 << FloatLiteral(std::to_string(graphic.viewY)) << ", "
                 << FloatLiteral(std::to_string(graphic.viewWidth)) << ", "
                 << FloatLiteral(std::to_string(graphic.viewHeight)) << ");\n";
            code << Indent(indent + 1) << name << ".SetWidth(20.0f);\n";
            code << Indent(indent + 1) << name << ".SetHeight(20.0f);\n";
            code << EmitCompiledShapes(graphic.shapes, indent + 1, name, true);
        };
        code << Indent(indent) << "{\n";
        emitIcon(shown->second, "shown");
        emitIcon(hidden->second, "hidden");
        code << Indent(indent + 1) << variable << ".SetSidebarIcons(shown.Bake(), hidden.Bake());\n";
        code << Indent(indent) << "}\n";
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
            code << Indent(indent) << styleName << ".selectable = Robo::ParseSelectable(\""
                 << EscapeCpp(attribute.second) << "\");\n";
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
        code << Indent(indent) << "{\n";
        code << Indent(indent + 1) << "Robo::Icon icon;\n";
        code << Indent(indent + 1) << "icon.SetViewBox(" << FloatLiteral(std::to_string(icon->second.viewX)) << ", "
             << FloatLiteral(std::to_string(icon->second.viewY)) << ", "
             << FloatLiteral(std::to_string(icon->second.viewWidth)) << ", "
             << FloatLiteral(std::to_string(icon->second.viewHeight)) << ");\n";
        code << Indent(indent + 1) << "icon.SetWidth(20.0f);\n";
        code << Indent(indent + 1) << "icon.SetHeight(20.0f);\n";
        code << EmitCompiledShapes(icon->second.shapes, indent + 1, "icon", true);
        code << Indent(indent + 1) << "button.SetIcon(icon.Bake());\n";
        code << Indent(indent) << "}\n";
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
                code << Indent(indent) << styleName << ".align = Robo::ParseTextAlign(\"" << EscapeCpp(attribute.second) << "\");\n";
            }
        }
    };

    if (isIcon) {
        code << Indent(indent) << "Robo::Icon icon;\n";
        code << Indent(indent) << "icon.SetViewBox(" << FloatLiteral(std::to_string(icon->second.viewX)) << ", "
             << FloatLiteral(std::to_string(icon->second.viewY)) << ", "
             << FloatLiteral(std::to_string(icon->second.viewWidth)) << ", "
             << FloatLiteral(std::to_string(icon->second.viewHeight)) << ");\n";
        code << Indent(indent) << "{\n";
        code << Indent(indent + 1) << "Robo::SvgStyle packedStyle;\n";
        emitBox("packedStyle");
        code << Indent(indent) << "if (packedStyle.width > 0.0f) icon.SetWidth(packedStyle.width);\n";
        code << Indent(indent) << "if (packedStyle.height > 0.0f) icon.SetHeight(packedStyle.height);\n";
        code << Indent(indent) << "if (!packedStyle.widthSpec.empty()) icon.SetWidthSpec(packedStyle.widthSpec);\n";
        code << Indent(indent) << "if (!packedStyle.heightSpec.empty()) icon.SetHeightSpec(packedStyle.heightSpec);\n";
        code << Indent(indent) << "for (const auto& token : packedStyle.tokens) icon.SetToken(token.first, token.second);\n";
        code << Indent(indent) << "icon.SetColor(packedStyle.color);\n";
        code << Indent(indent) << "icon.SetOpacity(packedStyle.opacity);\n";
        code << Indent(indent) << "icon.SetAlign(packedStyle.align);\n";
        code << Indent(indent) << "}\n";
        code << EmitCompiledShapes(icon->second.shapes, indent, "icon", true);
        code << Indent(indent) << parent << ".AddSvg(icon.Bake()" << props << ");\n";
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
    std::ostringstream code;
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
    code << "#include \"Robo/Selectable.hpp\"\n\n";
    code << ConvertNode(root, 0);
    return code.str();
}

}  // namespace Robo
