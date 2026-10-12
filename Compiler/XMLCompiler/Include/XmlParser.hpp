#pragma once

#include "XmlNode.hpp"

#include <string>

namespace Robo {

class XmlParser {
public:
    XmlNode Parse(const std::string& xml) const;

private:
    void SkipWhitespace(const std::string& xml, std::size_t& index) const;
    std::string ParseName(const std::string& xml, std::size_t& index) const;
    std::string ParseAttributeValue(const std::string& xml, std::size_t& index) const;
    std::string DecodeEntities(const std::string& value) const;
    XmlNode ParseElement(const std::string& xml, std::size_t& index) const;
    void ParseChildren(const std::string& xml, std::size_t& index, XmlNode& parent) const;
};

}  // namespace Robo
