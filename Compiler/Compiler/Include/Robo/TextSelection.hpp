#pragma once

#include "Selectable.hpp"

#include <cstdint>
#include <string>

namespace Robo {

// Glyph map rebuilt every frame for components that opted into selection.
// Anchor and focus are character boundaries inside one selectable component.
// A caret is both boundaries on the same spot. A range highlights the glyphs
// between them, in either direction.
class TextSelection {
public:
    static void BeginFrame();
    static void EndFrame();

    static void Push(const void* host, SelectableMode mode);
    static void Pop();
    static bool IsOpen();

    static int AddCodepoint(char32_t codepoint, float x, float y, float width, float height);

    static bool Covers(int index);
    static bool IsCaretBefore(int index);
    static bool IsCaretAfter(int index);

    static int Hit(float x, float y);
    static void BeginClick(float x, float y, int clicks);
    static void LongPress(float x, float y);
    static bool BeginHandleDrag(float x, float y);
    static void MovePointer(float x, float y);
    static void EndDrag();
    static void Clear();
    static bool IsDragging();
    static bool ShowsHandles();

    // 0 left, 1 right, 2 up, 3 down, 4 line start, 5 line end,
    // 6 word left, 7 word right, 8 document start, 9 document end.
    static bool Move(int command, bool extend);

    static bool SelectAll();
    static std::string SelectedText();
    static bool HasSelection();
    static bool HasCaret();
    static bool Tick(double deltaMs);
    static bool CaretIsVisible();
    static bool NeedsBlink();
    static float CaretOpacity();
    static void RestartCaret();

    static bool HandleGeometry(float& startX, float& startY, float& endX, float& endY, float& lineHeight);
    // Device-pixel box of the caret, the highlight, and the touch handles.
    static bool VisualBounds(float& x, float& y, float& width, float& height);
};

struct SelectableScope {
    SelectableScope(const void* host, SelectableMode mode) { TextSelection::Push(host, mode); }
    ~SelectableScope() { TextSelection::Pop(); }
};

}  // namespace Robo
