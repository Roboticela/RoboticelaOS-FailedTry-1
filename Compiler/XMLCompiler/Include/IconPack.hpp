#pragma once

#include "SvgPath.hpp"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace Robo {

std::map<std::string, CompiledSvg> LoadIconPacks(const std::string& assetDirectory);

// Full compiler library image from a text pack (stays under Compiler Assets).
bool WriteRuntimeIconPack(const std::string& packPath, const std::string& outputPath);

// App image: only icons from packPath that page XML (and chrome helpers) reference.
// Write beside generated page C++ in Build/; do not ship the full compiler pack.
bool WriteRuntimeIconPackUsed(const std::string& packPath,
                              const std::string& outputPath,
                              const std::vector<std::string>& pageXmlPaths);

// App catalog image (ROBOCAT.PAK) from page XML. Write beside generated page C++ in Build/.
bool WriteRuntimeCatalog(const std::string& xmlPath, const std::string& outputPath);

// App pane image (ROBOPANE.PAK) from lazy <Container when="..."> trees.
// Write beside generated page C++ in Build/. assetDirectory supplies compiler icon packs for names.
bool WriteRuntimePanes(const std::string& xmlPath, const std::string& outputPath, const std::string& assetDirectory);

}  // namespace Robo
