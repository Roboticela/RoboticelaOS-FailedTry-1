#pragma once

#include "ContextMenu.hpp"
#include "Position.hpp"
#include "RoboScene.hpp"
#include "SelectionArea.hpp"
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

// headingLevel 0 = paragraph, 1..6 = heading.
struct RoboTextBlock {
    int headingLevel = 0;
    const char* text = nullptr;
    bool selectable = false;
};

enum RoboMenuFlag : unsigned {
    RoboMenuSeparator = 1u,
    RoboMenuCheckable = 2u,
    RoboMenuChecked = 4u,
    RoboMenuDisabled = 8u,
    RoboMenuSubmenu = 16u
};

struct RoboMenuEntry {
    const char* label = nullptr;
    const char* icon = nullptr;
    const char* shortcut = nullptr;
    int parent = -1;
    unsigned flags = 0;
};

void RoboFillColorSections(Position& content, const RoboColorSection* sections, int count);
void RoboFillTypeSamples(Position& content, const RoboTypeSample* rows, int count);
void RoboFillSpaceRows(Position& content, const RoboSpaceRow* rows, int count);
void RoboFillRadiusSamples(Position& content, const RoboRadiusSample* rows, int count);
void RoboFillTextBlocks(Position& content, const RoboTextBlock* rows, int count);
void RoboAddMenuLabels(TopBarMenu& menu, const char* const* labels, int count);
void RoboFillContextMenu(ContextMenu& menu, const RoboMenuEntry* rows, int count);

// Desktop page chrome: foundation register + padding, selection-label defaults.
void RoboApplyDesktopChrome(RoboScene& scene);
void RoboApplySelectionLabelDefaults(SelectionArea& area);

}  // namespace Robo
