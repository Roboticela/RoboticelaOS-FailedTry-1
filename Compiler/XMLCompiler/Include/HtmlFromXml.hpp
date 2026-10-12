#pragma once

#include "XmlNode.hpp"

#include <string>

namespace Robo {

class HtmlFromXml {
public:
    std::string Convert(const std::string& xml) const;
    std::string ConvertNode(const XmlNode& node) const;

private:
    std::string EscapeHtml(const std::string& value) const;
    std::string BuildTextHtml(const XmlNode& node, const std::string& tagName) const;
    std::string BuildSpanHtml(const XmlNode& node) const;
    std::string BuildButtonHtml(const XmlNode& node) const;
    std::string BuildInputHtml(const XmlNode& node) const;
    std::string BuildScrollbarHtml(const XmlNode& node) const;
    std::string BuildContainerHtml(const XmlNode& node) const;
    std::string BuildFlexboxHtml(const XmlNode& node) const;
    std::string BuildGridHtml(const XmlNode& node) const;
    std::string BuildPositionHtml(const XmlNode& node) const;
    std::string StyleFromAttributes(const XmlNode& node, bool isBlock) const;
    int ParseHeadingLevel(const XmlNode& node) const;
};

}  // namespace Robo
