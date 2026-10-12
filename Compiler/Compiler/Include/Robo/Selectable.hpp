#pragma once

#include <string>

namespace Robo {

// Inherit follows the parent. Off and On are explicit. The default is Inherit,
// and the page root is Off, so nothing can be selected until a component opts in.
enum class SelectableMode {
    Inherit,
    Off,
    On
};

SelectableMode ParseSelectable(const std::string& value);

}  // namespace Robo
