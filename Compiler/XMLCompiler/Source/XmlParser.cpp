#include "XmlParser.hpp"

#include <stdexcept>

namespace Robo {

void XmlParser::SkipWhitespace(const std::string& xml, std::size_t& index) const {
    while (index < xml.size() && (xml[index] == ' ' || xml[index] == '\n' || xml[index] == '\r' || xml[index] == '\t')) {
        ++index;
    }
}

std::string XmlParser::ParseName(const std::string& xml, std::size_t& index) const {
    const std::size_t start = index;
    while (index < xml.size()) {
        const char ch = xml[index];
        const bool valid = (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') ||
                           ch == '_' || ch == '-' || ch == ':';
        if (!valid) {
            break;
        }
        ++index;
    }
    return xml.substr(start, index - start);
}

std::string XmlParser::ParseAttributeValue(const std::string& xml, std::size_t& index) const {
    if (index >= xml.size() || (xml[index] != '"' && xml[index] != '\'')) {
        throw std::runtime_error("Expected attribute value");
    }
    const char quote = xml[index++];
    const std::size_t start = index;
    while (index < xml.size() && xml[index] != quote) {
        ++index;
    }
    if (index >= xml.size()) {
        throw std::runtime_error("Unterminated attribute value");
    }
    const std::string value = xml.substr(start, index - start);
    ++index;
    return DecodeEntities(value);
}

std::string XmlParser::DecodeEntities(const std::string& value) const {
    std::string result;
    result.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '&') {
            if (value.compare(i, 4, "&lt;") == 0) {
                result.push_back('<');
                i += 3;
            } else if (value.compare(i, 4, "&gt;") == 0) {
                result.push_back('>');
                i += 3;
            } else if (value.compare(i, 5, "&amp;") == 0) {
                result.push_back('&');
                i += 4;
            } else if (value.compare(i, 6, "&quot;") == 0) {
                result.push_back('"');
                i += 5;
            } else if (value.compare(i, 6, "&apos;") == 0) {
                result.push_back('\'');
                i += 5;
            } else {
                result.push_back(value[i]);
            }
        } else {
            result.push_back(value[i]);
        }
    }
    return result;
}

XmlNode XmlParser::ParseElement(const std::string& xml, std::size_t& index) const {
    if (index >= xml.size() || xml[index] != '<') {
        throw std::runtime_error("Expected element");
    }
    ++index;

    if (index < xml.size() && xml[index] == '/') {
        throw std::runtime_error("Unexpected closing tag");
    }

    if (index + 2 < xml.size() && xml.compare(index, 3, "!--") == 0) {
        index += 3;
        while (index + 2 < xml.size() && xml.compare(index, 3, "-->") != 0) {
            ++index;
        }
        if (index + 2 >= xml.size()) {
            throw std::runtime_error("Unterminated comment");
        }
        index += 3;
        SkipWhitespace(xml, index);
        return ParseElement(xml, index);
    }

    if (index < xml.size() && xml[index] == '?') {
        ++index;
        while (index + 1 < xml.size() && !(xml[index] == '?' && xml[index + 1] == '>')) {
            ++index;
        }
        if (index + 1 >= xml.size()) {
            throw std::runtime_error("Unterminated processing instruction");
        }
        index += 2;
        SkipWhitespace(xml, index);
        return ParseElement(xml, index);
    }

    XmlNode node;
    node.name = ParseName(xml, index);
    if (node.name.empty()) {
        throw std::runtime_error("Missing element name");
    }

    SkipWhitespace(xml, index);
    while (index < xml.size() && xml[index] != '>' && xml[index] != '/') {
        const std::string attrName = ParseName(xml, index);
        SkipWhitespace(xml, index);
        if (index >= xml.size() || xml[index] != '=') {
            throw std::runtime_error("Expected '=' after attribute name");
        }
        ++index;
        SkipWhitespace(xml, index);
        node.attributes[attrName] = ParseAttributeValue(xml, index);
        SkipWhitespace(xml, index);
    }

    if (index < xml.size() && xml[index] == '/') {
        ++index;
        if (index >= xml.size() || xml[index] != '>') {
            throw std::runtime_error("Expected '/>'");
        }
        ++index;
        return node;
    }

    if (index >= xml.size() || xml[index] != '>') {
        throw std::runtime_error("Expected '>'");
    }
    ++index;

    ParseChildren(xml, index, node);

    if (index >= xml.size() || xml[index] != '<') {
        throw std::runtime_error("Expected closing tag");
    }
    ++index;
    if (index >= xml.size() || xml[index] != '/') {
        throw std::runtime_error("Expected closing tag slash");
    }
    ++index;
    const std::string closingName = ParseName(xml, index);
    if (closingName != node.name) {
        throw std::runtime_error("Mismatched closing tag for <" + node.name + ">");
    }
    SkipWhitespace(xml, index);
    if (index >= xml.size() || xml[index] != '>') {
        throw std::runtime_error("Expected '>' after closing tag");
    }
    ++index;
    return node;
}

void XmlParser::ParseChildren(const std::string& xml, std::size_t& index, XmlNode& parent) const {
    while (index < xml.size()) {
        if (xml[index] == '<') {
            if (index + 1 < xml.size() && xml[index + 1] == '/') {
                return;
            }
            if (index + 3 < xml.size() && xml.compare(index, 4, "<!--") == 0) {
                index += 4;
                while (index + 2 < xml.size() && xml.compare(index, 3, "-->") != 0) {
                    ++index;
                }
                if (index + 2 >= xml.size()) {
                    throw std::runtime_error("Unterminated comment");
                }
                index += 3;
                continue;
            }
            parent.children.push_back(ParseElement(xml, index));
            continue;
        }

        const std::size_t start = index;
        while (index < xml.size() && xml[index] != '<') {
            ++index;
        }
        XmlNode textNode;
        textNode.isText = true;
        textNode.text = DecodeEntities(xml.substr(start, index - start));
        if (!textNode.text.empty()) {
            parent.children.push_back(textNode);
        }
    }
}

XmlNode XmlParser::Parse(const std::string& xml) const {
    std::size_t index = 0;
    SkipWhitespace(xml, index);
    if (index >= xml.size()) {
        throw std::runtime_error("Empty XML");
    }
    XmlNode root = ParseElement(xml, index);
    SkipWhitespace(xml, index);
    if (index < xml.size()) {
        throw std::runtime_error("Trailing content after root element");
    }
    return root;
}

}  // namespace Robo
