#pragma once

#include "SvgPath.hpp"

#include <map>
#include <string>

namespace Robo {

std::map<std::string, CompiledSvg> LoadIconPacks(const std::string& assetDirectory);

// Writes one runtime image (ROBOICON.PAK / ROBOCTRY.PAK) from a text pack file.
bool WriteRuntimeIconPack(const std::string& packPath, const std::string& outputPath);

}  // namespace Robo
