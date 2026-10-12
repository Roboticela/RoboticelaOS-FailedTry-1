#pragma once

#include <map>
#include <string>
#include <vector>

namespace Robo {

struct XmlNode {
    std::string name;
    std::map<std::string, std::string> attributes;
    std::string text;
    std::vector<XmlNode> children;
    bool isText = false;
};

}  // namespace Robo
