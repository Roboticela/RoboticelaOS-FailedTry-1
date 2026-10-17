#pragma once

#include "Icon.hpp"
#include "IPlatform.hpp"
#include "Svg.hpp"

#include <cstdint>

namespace Robo {

// Copies one runtime pack image (ROBOICON.PAK or ROBOCTRY.PAK) into the name table.
void RoboIconPackAdd(const std::uint8_t* data, std::uint32_t size);

// Loads both pack images through the platform asset path. Safe to call more than once.
void RoboIconPackLoad(const IPlatform& platform);

bool RoboIconPackContains(const char* name);

// Fills an icon from the loaded packs. The page names the icon; the shapes stay in the pack file.
bool RoboFillIconByName(Icon& icon, const char* name);

// One bake for every page. The same name and size reuse the cached picture.
Svg RoboBakeNamedIcon(const char* name, float size);

}  // namespace Robo
