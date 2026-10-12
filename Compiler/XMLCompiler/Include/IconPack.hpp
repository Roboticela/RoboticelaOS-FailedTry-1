#pragma once

#include "SvgPath.hpp"

#include <map>
#include <string>

namespace Robo {

std::map<std::string, CompiledSvg> LoadIconPacks(const std::string& assetDirectory);

}  // namespace Robo
