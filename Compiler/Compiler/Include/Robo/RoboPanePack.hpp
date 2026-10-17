#pragma once

#include "IPlatform.hpp"
#include "RoboScene.hpp"

#include <cstdint>

namespace Robo {

// Loads the app pane pack (ROBOPANE.PAK) from the platform asset path / ESP. Safe to call more than once.
void RoboPaneLoad(const IPlatform& platform);

// Copies one pane pack image into the runtime table.
void RoboPaneAdd(const std::uint8_t* data, std::uint32_t size);

// Builds one lazy pane container into the scene from the loaded pack.
bool RoboPaneBuild(RoboScene& scene, const char* paneId);

bool RoboPaneContains(const char* paneId);

}  // namespace Robo
