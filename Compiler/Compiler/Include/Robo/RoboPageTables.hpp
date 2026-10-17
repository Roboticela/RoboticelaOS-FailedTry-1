#pragma once

#include "Position.hpp"
#include "TopBarMenu.hpp"

namespace Robo {

struct RoboColorSwatch {
    const char* token = nullptr;
    const char* label = nullptr;
    const char* hex = nullptr;
    const char* borderToken = nullptr;
};

struct RoboColorSection {
    const char* heading = nullptr;
    const RoboColorSwatch* rows = nullptr;
    int count = 0;
};

struct RoboTypeSample {
    const char* fontSize = nullptr;
    const char* fontWeight = nullptr;
    const char* color = nullptr;
    const char* label = nullptr;
};

struct RoboSpaceRow {
    const char* gap = nullptr;
    const char* label = nullptr;
};

struct RoboRadiusSample {
    const char* radius = nullptr;
    const char* label = nullptr;
};

void RoboFillColorSections(Position& content, const RoboColorSection* sections, int count);
void RoboFillTypeSamples(Position& content, const RoboTypeSample* rows, int count);
void RoboFillSpaceRows(Position& content, const RoboSpaceRow* rows, int count);
void RoboFillRadiusSamples(Position& content, const RoboRadiusSample* rows, int count);
void RoboAddMenuLabels(TopBarMenu& menu, const char* const* labels, int count);

}  // namespace Robo
