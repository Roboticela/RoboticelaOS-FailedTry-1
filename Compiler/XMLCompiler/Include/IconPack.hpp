#pragma once

#include "SvgPath.hpp"

#include <map>
#include <string>

namespace Robo {

std::map<std::string, CompiledSvg> LoadIconPacks(const std::string& assetDirectory);

// Writes one runtime image (ROBOICON.PAK / ROBOCTRY.PAK) from a text pack file.
bool WriteRuntimeIconPack(const std::string& packPath, const std::string& outputPath);

// Writes ROBOCAT.PAK from a page XML that contains a Sidebar / Pane catalog.
bool WriteRuntimeCatalog(const std::string& xmlPath, const std::string& outputPath);

// Writes ROBOPANE.PAK from lazy <Container when="..."> trees in a page XML.
// assetDirectory supplies RoboIcons.pack / RoboCountry.pack so named icons are tagged.
bool WriteRuntimePanes(const std::string& xmlPath, const std::string& outputPath, const std::string& assetDirectory);

}  // namespace Robo
