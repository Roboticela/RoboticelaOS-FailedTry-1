#include "Robo/TextSelection.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace Robo {

namespace {

enum class Unit {
    Character,
    Word,
    Line,
    Document
};

enum class Cluster {
    Word,
    Space,
    Other
};

struct Glyph {
    const void* root = nullptr;
    char32_t codepoint = 0;
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

struct Scope {
    const void* root = nullptr;
};

std::vector<Glyph> glyphs;
std::vector<Scope> scopes;
const void* selectionRoot = nullptr;
int anchor = 0;
int focus = 0;
bool dragging = false;
bool handleDragging = false;
bool showHandles = false;
double caretMs = 0.0;
Unit dragUnit = Unit::Character;
int dragOrigin = 0;
int dragStart = 0;
int dragEnd = 0;
float affinityX = 0.0f;

void AppendUtf8(std::string& out, char32_t codepoint) {
    const auto value = static_cast<std::uint32_t>(codepoint);
    if (value < 0x80) {
        out.push_back(static_cast<char>(value));
    } else if (value < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (value >> 6)));
        out.push_back(static_cast<char>(0x80 | (value & 0x3F)));
    } else if (value < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (value >> 12)));
        out.push_back(static_cast<char>(0x80 | ((value >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (value & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (value >> 18)));
        out.push_back(static_cast<char>(0x80 | ((value >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((value >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (value & 0x3F)));
    }
}

int RootCount(const void* root) {
    int count = 0;
    for (const Glyph& glyph : glyphs) {
        if (glyph.root == root) {
            ++count;
        }
    }
    return count;
}

int GlobalAt(const void* root, int ordinal) {
    int seen = 0;
    for (int i = 0; i < static_cast<int>(glyphs.size()); ++i) {
        if (glyphs[static_cast<std::size_t>(i)].root != root) {
            continue;
        }
        if (seen == ordinal) {
            return i;
        }
        ++seen;
    }
    return -1;
}

int OrdinalOf(int global) {
    if (global < 0 || global >= static_cast<int>(glyphs.size())) {
        return 0;
    }
    const void* root = glyphs[static_cast<std::size_t>(global)].root;
    int ordinal = 0;
    for (int i = 0; i < global; ++i) {
        if (glyphs[static_cast<std::size_t>(i)].root == root) {
            ++ordinal;
        }
    }
    return ordinal;
}

const Glyph* GlyphAt(const void* root, int ordinal) {
    const int global = GlobalAt(root, ordinal);
    if (global < 0) {
        return nullptr;
    }
    return &glyphs[static_cast<std::size_t>(global)];
}

bool SameLine(const Glyph& a, const Glyph& b) {
    const float band = std::max(a.height, b.height) * 0.5f;
    return std::fabs(a.y - b.y) <= band;
}

Cluster ClassOf(char32_t codepoint) {
    if (codepoint == ' ' || codepoint == '\t' || codepoint == '\n' || codepoint == '\r' || codepoint == 0x00A0) {
        return Cluster::Space;
    }
    if (codepoint == '\'' || codepoint == 0x2019) {
        return Cluster::Word;
    }
    if ((codepoint >= '0' && codepoint <= '9') || (codepoint >= 'A' && codepoint <= 'Z') ||
        (codepoint >= 'a' && codepoint <= 'z') || codepoint > 0xA0) {
        return Cluster::Word;
    }
    return Cluster::Other;
}

int ClampBoundary(int boundary) {
    const int count = RootCount(selectionRoot);
    return std::max(0, std::min(boundary, count));
}

void UnitBounds(int ordinal, Unit unit, int& start, int& end) {
    const int count = RootCount(selectionRoot);
    ordinal = std::max(0, std::min(ordinal, std::max(0, count - 1)));
    if (unit == Unit::Document || count <= 0) {
        start = 0;
        end = count;
        return;
    }
    const Glyph* seed = GlyphAt(selectionRoot, ordinal);
    if (!seed) {
        start = 0;
        end = count;
        return;
    }
    if (unit == Unit::Character) {
        start = ordinal;
        end = std::min(count, ordinal + 1);
        return;
    }
    const Cluster kind = ClassOf(seed->codepoint);
    start = ordinal;
    end = ordinal + 1;
    if (unit == Unit::Word) {
        while (start > 0) {
            const Glyph* previous = GlyphAt(selectionRoot, start - 1);
            if (!previous || ClassOf(previous->codepoint) != kind) {
                break;
            }
            if (kind != Cluster::Word && !SameLine(*previous, *seed)) {
                break;
            }
            --start;
        }
        while (end < count) {
            const Glyph* next = GlyphAt(selectionRoot, end);
            if (!next || ClassOf(next->codepoint) != kind) {
                break;
            }
            if (kind != Cluster::Word && !SameLine(*seed, *next)) {
                break;
            }
            ++end;
        }
        return;
    }
    while (start > 0) {
        const Glyph* previous = GlyphAt(selectionRoot, start - 1);
        if (!previous || !SameLine(*previous, *seed)) {
            break;
        }
        --start;
    }
    while (end < count) {
        const Glyph* next = GlyphAt(selectionRoot, end);
        if (!next || !SameLine(*seed, *next)) {
            break;
        }
        ++end;
    }
}

float BoundaryX(int boundary) {
    const int count = RootCount(selectionRoot);
    boundary = ClampBoundary(boundary);
    if (count <= 0) {
        return 0.0f;
    }
    if (boundary >= count) {
        const Glyph* last = GlyphAt(selectionRoot, count - 1);
        return last ? last->x + last->width : 0.0f;
    }
    const Glyph* glyph = GlyphAt(selectionRoot, boundary);
    return glyph ? glyph->x : 0.0f;
}

float BoundaryY(int boundary) {
    const int count = RootCount(selectionRoot);
    if (count <= 0) {
        return 0.0f;
    }
    const int ordinal = std::min(ClampBoundary(boundary), count - 1);
    const Glyph* glyph = GlyphAt(selectionRoot, ordinal);
    return glyph ? glyph->y : 0.0f;
}

int BoundaryAt(float x, float y, const void* onlyRoot) {
    int found = -1;
    for (int i = 0; i < static_cast<int>(glyphs.size()); ++i) {
        const Glyph& glyph = glyphs[static_cast<std::size_t>(i)];
        if (onlyRoot && glyph.root != onlyRoot) {
            continue;
        }
        if (x >= glyph.x && x < glyph.x + glyph.width && y >= glyph.y && y < glyph.y + glyph.height) {
            found = i;
        }
    }
    if (found < 0) {
        return -1;
    }
    const Glyph& glyph = glyphs[static_cast<std::size_t>(found)];
    const int ordinal = OrdinalOf(found);
    return x >= glyph.x + glyph.width * 0.5f ? ordinal + 1 : ordinal;
}

int NearestBoundary(float x, float y) {
    if (!selectionRoot) {
        return -1;
    }
    const int direct = BoundaryAt(x, y, selectionRoot);
    if (direct >= 0) {
        return direct;
    }
    const int count = RootCount(selectionRoot);
    if (count <= 0) {
        return 0;
    }
    int bestOrdinal = 0;
    float bestLine = 1.0e9f;
    const Glyph* first = GlyphAt(selectionRoot, 0);
    float lineY = first ? first->y : 0.0f;
    for (int i = 0; i < count; ++i) {
        const Glyph* glyph = GlyphAt(selectionRoot, i);
        if (!glyph) {
            continue;
        }
        if (i == 0 || !SameLine(*glyph, *GlyphAt(selectionRoot, bestOrdinal))) {
            const float distance = std::fabs((glyph->y + glyph->height * 0.5f) - y);
            if (distance < bestLine) {
                bestLine = distance;
                bestOrdinal = i;
                lineY = glyph->y;
            }
        }
    }
    int bestBoundary = 0;
    float bestX = 1.0e9f;
    for (int i = 0; i <= count; ++i) {
        const Glyph* glyph = GlyphAt(selectionRoot, std::min(i, count - 1));
        if (!glyph || std::fabs(glyph->y - lineY) > glyph->height * 0.5f) {
            continue;
        }
        const float edge = i >= count ? glyph->x + glyph->width : GlyphAt(selectionRoot, i)->x;
        const float distance = std::fabs(edge - x);
        if (distance < bestX) {
            bestX = distance;
            bestBoundary = i;
        }
    }
    return bestBoundary;
}

void ApplyDrag(int boundary) {
    boundary = ClampBoundary(boundary);
    if (dragUnit == Unit::Character) {
        focus = boundary;
        return;
    }
    const int count = RootCount(selectionRoot);
    if (count <= 0) {
        return;
    }
    if (boundary > dragOrigin) {
        int start = 0;
        int end = 0;
        UnitBounds(std::min(boundary, count - 1), dragUnit, start, end);
        anchor = dragStart;
        focus = end;
    } else if (boundary < dragOrigin) {
        int start = 0;
        int end = 0;
        UnitBounds(std::max(0, boundary - 1), dragUnit, start, end);
        anchor = dragEnd;
        focus = start;
    } else {
        anchor = dragStart;
        focus = dragEnd;
    }
}

int StepHorizontal(int boundary, int direction, bool word) {
    const int count = RootCount(selectionRoot);
    if (word) {
        if (direction < 0) {
            if (boundary <= 0) {
                return 0;
            }
            int index = boundary - 1;
            while (index > 0) {
                const Glyph* glyph = GlyphAt(selectionRoot, index);
                if (!glyph || ClassOf(glyph->codepoint) != Cluster::Space) {
                    break;
                }
                --index;
            }
            int start = 0;
            int end = 0;
            UnitBounds(index, Unit::Word, start, end);
            if (boundary > start) {
                return start;
            }
            if (start <= 0) {
                return 0;
            }
            index = start - 1;
            while (index > 0) {
                const Glyph* glyph = GlyphAt(selectionRoot, index);
                if (!glyph || ClassOf(glyph->codepoint) != Cluster::Space) {
                    break;
                }
                --index;
            }
            UnitBounds(index, Unit::Word, start, end);
            return start;
        }
        int index = boundary;
        while (index < count) {
            const Glyph* glyph = GlyphAt(selectionRoot, index);
            if (!glyph || ClassOf(glyph->codepoint) != Cluster::Space) {
                break;
            }
            ++index;
        }
        if (index >= count) {
            return count;
        }
        int start = 0;
        int end = 0;
        UnitBounds(index, Unit::Word, start, end);
        return end;
    }
    return ClampBoundary(boundary + direction);
}

int LineEdge(int boundary, bool start) {
    const int count = RootCount(selectionRoot);
    if (count <= 0) {
        return 0;
    }
    const int ordinal = std::min(std::max(0, boundary == count ? count - 1 : boundary), count - 1);
    int unitStart = 0;
    int unitEnd = 0;
    UnitBounds(ordinal, Unit::Line, unitStart, unitEnd);
    return start ? unitStart : unitEnd;
}

int VerticalBoundary(int boundary, int direction) {
    const int count = RootCount(selectionRoot);
    if (count <= 0) {
        return 0;
    }
    const float x = affinityX;
    const int ordinal = std::min(std::max(0, boundary == count ? count - 1 : boundary), count - 1);
    const Glyph* current = GlyphAt(selectionRoot, ordinal);
    if (!current) {
        return boundary;
    }
    float targetY = direction < 0 ? -1.0e9f : 1.0e9f;
    bool found = false;
    for (int i = 0; i < count; ++i) {
        const Glyph* glyph = GlyphAt(selectionRoot, i);
        if (!glyph || SameLine(*glyph, *current)) {
            continue;
        }
        if (direction < 0 && glyph->y < current->y && glyph->y > targetY) {
            targetY = glyph->y;
            found = true;
        }
        if (direction > 0 && glyph->y > current->y && glyph->y < targetY) {
            targetY = glyph->y;
            found = true;
        }
    }
    if (!found) {
        return direction < 0 ? 0 : count;
    }
    int best = 0;
    float bestDistance = 1.0e9f;
    for (int i = 0; i <= count; ++i) {
        const Glyph* glyph = GlyphAt(selectionRoot, std::min(i, count - 1));
        if (!glyph || std::fabs(glyph->y - targetY) > glyph->height * 0.5f) {
            continue;
        }
        const float edge = BoundaryX(i);
        const float distance = std::fabs(edge - x);
        if (distance < bestDistance) {
            bestDistance = distance;
            best = i;
        }
    }
    return best;
}

void RememberAffinity(int boundary) {
    affinityX = BoundaryX(boundary);
}

}  // namespace

SelectableMode ParseSelectable(const std::string& value) {
    if (value == "true" || value == "1" || value == "yes" || value == "on") {
        return SelectableMode::On;
    }
    if (value == "false" || value == "0" || value == "no" || value == "off") {
        return SelectableMode::Off;
    }
    return SelectableMode::Inherit;
}

void TextSelection::BeginFrame() {
    glyphs.clear();
    scopes.clear();
}

void TextSelection::EndFrame() {
    if (!selectionRoot || RootCount(selectionRoot) <= 0) {
        if (glyphs.empty()) {
            selectionRoot = nullptr;
            anchor = 0;
            focus = 0;
            dragging = false;
        }
        return;
    }
    const int count = RootCount(selectionRoot);
    anchor = std::max(0, std::min(anchor, count));
    focus = std::max(0, std::min(focus, count));
}

void TextSelection::Push(const void* host, SelectableMode mode) {
    const void* parent = scopes.empty() ? nullptr : scopes.back().root;
    const void* root = parent;
    if (mode == SelectableMode::On) {
        root = host;
    } else if (mode == SelectableMode::Off) {
        root = nullptr;
    }
    scopes.push_back(Scope{root});
}

void TextSelection::Pop() {
    if (!scopes.empty()) {
        scopes.pop_back();
    }
}

bool TextSelection::IsOpen() {
    return !scopes.empty() && scopes.back().root != nullptr;
}

int TextSelection::AddCodepoint(char32_t codepoint, float x, float y, float width, float height) {
    if (!IsOpen() || width <= 0.0f || height <= 0.0f) {
        return -1;
    }
    Glyph glyph;
    glyph.root = scopes.back().root;
    glyph.codepoint = codepoint;
    glyph.x = x;
    glyph.y = y;
    glyph.width = std::max(1.0f, width);
    glyph.height = height;
    glyphs.push_back(glyph);
    return static_cast<int>(glyphs.size()) - 1;
}

bool TextSelection::Covers(int index) {
    if (!selectionRoot || anchor == focus || index < 0 || index >= static_cast<int>(glyphs.size())) {
        return false;
    }
    if (glyphs[static_cast<std::size_t>(index)].root != selectionRoot) {
        return false;
    }
    const int ordinal = OrdinalOf(index);
    return ordinal >= std::min(anchor, focus) && ordinal < std::max(anchor, focus);
}

bool TextSelection::IsCaretBefore(int index) {
    if (!selectionRoot || anchor != focus || index < 0 || index >= static_cast<int>(glyphs.size())) {
        return false;
    }
    if (glyphs[static_cast<std::size_t>(index)].root != selectionRoot) {
        return false;
    }
    return OrdinalOf(index) == anchor && anchor < RootCount(selectionRoot);
}

bool TextSelection::IsCaretAfter(int index) {
    if (!selectionRoot || anchor != focus || index < 0 || index >= static_cast<int>(glyphs.size())) {
        return false;
    }
    const Glyph& glyph = glyphs[static_cast<std::size_t>(index)];
    if (glyph.root != selectionRoot) {
        return false;
    }
    const int count = RootCount(selectionRoot);
    return anchor == count && OrdinalOf(index) == count - 1;
}

int TextSelection::Hit(float x, float y) {
    int found = -1;
    for (int i = 0; i < static_cast<int>(glyphs.size()); ++i) {
        const Glyph& glyph = glyphs[static_cast<std::size_t>(i)];
        if (x >= glyph.x && x < glyph.x + glyph.width && y >= glyph.y && y < glyph.y + glyph.height) {
            found = i;
        }
    }
    return found;
}

void TextSelection::BeginClick(float x, float y, int clicks) {
    const int global = Hit(x, y);
    if (global < 0) {
        return;
    }
    showHandles = false;
    handleDragging = false;
    selectionRoot = glyphs[static_cast<std::size_t>(global)].root;
    const int boundary = BoundaryAt(x, y, selectionRoot);
    const int ordinal = OrdinalOf(global);
    const int count = std::max(1, clicks);
    dragUnit = Unit::Character;
    if (count >= 4) {
        dragUnit = Unit::Document;
    } else if (count == 3) {
        dragUnit = Unit::Line;
    } else if (count == 2) {
        dragUnit = Unit::Word;
    }
    int start = boundary;
    int end = boundary;
    if (dragUnit == Unit::Character) {
        anchor = boundary;
        focus = boundary;
    } else {
        UnitBounds(ordinal, dragUnit, start, end);
        anchor = start;
        focus = end;
    }
    dragOrigin = boundary;
    dragStart = anchor;
    dragEnd = focus;
    dragging = true;
    RememberAffinity(focus);
    TextSelection::RestartCaret();
}

void TextSelection::LongPress(float x, float y) {
    const int global = Hit(x, y);
    if (global < 0) {
        return;
    }
    selectionRoot = glyphs[static_cast<std::size_t>(global)].root;
    int start = 0;
    int end = 0;
    UnitBounds(OrdinalOf(global), Unit::Word, start, end);
    anchor = start;
    focus = end;
    dragUnit = Unit::Word;
    dragOrigin = BoundaryAt(x, y, selectionRoot);
    if (dragOrigin < 0) {
        dragOrigin = start;
    }
    dragStart = start;
    dragEnd = end;
    dragging = true;
    handleDragging = false;
    showHandles = true;
    RememberAffinity(focus);
    TextSelection::RestartCaret();
}

bool TextSelection::BeginHandleDrag(float x, float y) {
    if (!showHandles || !HasSelection()) {
        return false;
    }
    float startX = 0.0f;
    float startY = 0.0f;
    float endX = 0.0f;
    float endY = 0.0f;
    float height = 0.0f;
    if (!HandleGeometry(startX, startY, endX, endY, height)) {
        return false;
    }
    const float radius = std::max(14.0f, height * 0.7f);
    const auto nearHandle = [&](float handleX, float handleY) {
        const float dx = x - handleX;
        const float dy = y - (handleY + height);
        return dx * dx + dy * dy <= radius * radius;
    };
    const bool onStart = nearHandle(startX, startY);
    const bool onEnd = nearHandle(endX, endY);
    if (!onStart && !onEnd) {
        return false;
    }
    const bool grabStart = onStart && (!onEnd || std::fabs(x - startX) <= std::fabs(x - endX));
    anchor = grabStart ? std::max(anchor, focus) : std::min(anchor, focus);
    focus = NearestBoundary(x, y);
    if (focus < 0) {
        focus = anchor;
    }
    dragUnit = Unit::Character;
    dragging = true;
    handleDragging = true;
    return true;
}

void TextSelection::MovePointer(float x, float y) {
    if (!dragging || !selectionRoot) {
        return;
    }
    int boundary = BoundaryAt(x, y, selectionRoot);
    if (boundary < 0) {
        boundary = NearestBoundary(x, y);
    }
    if (boundary < 0) {
        return;
    }
    if (handleDragging) {
        focus = ClampBoundary(boundary);
        RememberAffinity(focus);
        return;
    }
    ApplyDrag(boundary);
    RememberAffinity(focus);
}

void TextSelection::EndDrag() {
    dragging = false;
    handleDragging = false;
}

void TextSelection::Clear() {
    selectionRoot = nullptr;
    anchor = 0;
    focus = 0;
    dragging = false;
    handleDragging = false;
    showHandles = false;
    dragUnit = Unit::Character;
}

bool TextSelection::IsDragging() {
    return dragging;
}

bool TextSelection::ShowsHandles() {
    return showHandles && HasSelection();
}

bool TextSelection::Move(int command, bool extend) {
    if (!selectionRoot || RootCount(selectionRoot) <= 0) {
        return false;
    }
    showHandles = false;
    if (!extend && anchor != focus) {
        if (command == 0) {
            anchor = focus = std::min(anchor, focus);
            RememberAffinity(focus);
            return true;
        }
        if (command == 1) {
            anchor = focus = std::max(anchor, focus);
            RememberAffinity(focus);
            return true;
        }
        const bool towardStart = command == 2 || command == 4 || command == 6 || command == 8;
        focus = towardStart ? std::min(anchor, focus) : std::max(anchor, focus);
        anchor = focus;
    }
    int next = focus;
    switch (command) {
        case 0:
            next = StepHorizontal(focus, -1, false);
            break;
        case 1:
            next = StepHorizontal(focus, 1, false);
            break;
        case 2:
            next = VerticalBoundary(focus, -1);
            break;
        case 3:
            next = VerticalBoundary(focus, 1);
            break;
        case 4:
            next = LineEdge(focus, true);
            break;
        case 5:
            next = LineEdge(focus, false);
            break;
        case 6:
            next = StepHorizontal(focus, -1, true);
            break;
        case 7:
            next = StepHorizontal(focus, 1, true);
            break;
        case 8:
            next = 0;
            break;
        case 9:
            next = RootCount(selectionRoot);
            break;
        default:
            return false;
    }
    focus = ClampBoundary(next);
    if (!extend) {
        anchor = focus;
    }
    if (command != 2 && command != 3) {
        RememberAffinity(focus);
    }
    RestartCaret();
    return true;
}

bool TextSelection::SelectAll() {
    if (!selectionRoot) {
        if (glyphs.empty()) {
            return false;
        }
        selectionRoot = glyphs.front().root;
    }
    const int count = RootCount(selectionRoot);
    if (count <= 0) {
        return false;
    }
    anchor = 0;
    focus = count;
    dragging = false;
    showHandles = false;
    RememberAffinity(focus);
    return true;
}

std::string TextSelection::SelectedText() {
    if (!HasSelection()) {
        return {};
    }
    std::string text;
    float previousY = -100000.0f;
    float previousHeight = 0.0f;
    const int first = std::min(anchor, focus);
    const int last = std::max(anchor, focus);
    for (int ordinal = first; ordinal < last; ++ordinal) {
        const Glyph* glyph = GlyphAt(selectionRoot, ordinal);
        if (!glyph) {
            continue;
        }
        if (!text.empty() && glyph->y > previousY + previousHeight * 0.5f) {
            text.push_back('\n');
        }
        AppendUtf8(text, glyph->codepoint);
        previousY = glyph->y;
        previousHeight = glyph->height;
    }
    return text;
}

bool TextSelection::HasSelection() {
    return selectionRoot != nullptr && anchor != focus && RootCount(selectionRoot) > 0;
}

bool TextSelection::HasCaret() {
    return selectionRoot != nullptr && RootCount(selectionRoot) > 0 && anchor >= 0;
}

bool TextSelection::CaretIsVisible() {
    return selectionRoot != nullptr && anchor == focus && !dragging && RootCount(selectionRoot) > 0;
}

void TextSelection::RestartCaret() {
    caretMs = 0.0;
}

bool TextSelection::Tick(double deltaMs) {
    if (!CaretIsVisible()) {
        return false;
    }
    const float before = CaretOpacity();
    caretMs += std::max(0.0, deltaMs);
    constexpr double kPeriod = 1060.0;
    if (caretMs >= kPeriod) {
        caretMs = std::fmod(caretMs, kPeriod);
    }
    return std::fabs(CaretOpacity() - before) > 0.02f;
}

bool TextSelection::NeedsBlink() {
    return CaretIsVisible();
}

float TextSelection::CaretOpacity() {
    if (!CaretIsVisible()) {
        return 0.0f;
    }
    // Solid, then a short fade out, a rest, and a short fade in. Same rhythm as the macOS caret.
    constexpr double kSolid = 460.0;
    constexpr double kFade = 90.0;
    constexpr double kHidden = 420.0;
    const double t = caretMs;
    if (t < kSolid) {
        return 1.0f;
    }
    if (t < kSolid + kFade) {
        const float u = static_cast<float>((t - kSolid) / kFade);
        return 1.0f - u * u * (3.0f - 2.0f * u);
    }
    if (t < kSolid + kFade + kHidden) {
        return 0.0f;
    }
    const float u = static_cast<float>((t - (kSolid + kFade + kHidden)) / kFade);
    const float s = std::max(0.0f, std::min(1.0f, u));
    return s * s * (3.0f - 2.0f * s);
}

bool TextSelection::HandleGeometry(float& startX, float& startY, float& endX, float& endY, float& lineHeight) {
    if (!HasSelection()) {
        return false;
    }
    const int count = RootCount(selectionRoot);
    const int first = std::min(anchor, focus);
    const int last = std::max(anchor, focus);
    const Glyph* startGlyph = GlyphAt(selectionRoot, std::min(first, count - 1));
    const Glyph* endGlyph = GlyphAt(selectionRoot, std::min(std::max(0, last - 1), count - 1));
    if (!startGlyph || !endGlyph) {
        return false;
    }
    startX = BoundaryX(first);
    startY = startGlyph->y;
    endX = BoundaryX(last);
    endY = endGlyph->y;
    lineHeight = std::max(startGlyph->height, endGlyph->height);
    return true;
}

bool TextSelection::VisualBounds(float& x, float& y, float& width, float& height) {
    if (!selectionRoot) {
        return false;
    }
    const int count = RootCount(selectionRoot);
    if (count <= 0 || anchor < 0) {
        return false;
    }

    const int first = std::max(0, std::min(anchor, focus));
    const int last = std::max(anchor, focus);
    float left = 0.0f;
    float top = 0.0f;
    float right = 0.0f;
    float bottom = 0.0f;
    bool any = false;
    auto include = [&](float boxX, float boxY, float boxWidth, float boxHeight) {
        if (boxWidth <= 0.0f || boxHeight <= 0.0f) {
            return;
        }
        const float boxRight = boxX + boxWidth;
        const float boxBottom = boxY + boxHeight;
        if (!any) {
            left = boxX;
            top = boxY;
            right = boxRight;
            bottom = boxBottom;
            any = true;
            return;
        }
        left = std::min(left, boxX);
        top = std::min(top, boxY);
        right = std::max(right, boxRight);
        bottom = std::max(bottom, boxBottom);
    };

    if (first == last) {
        const Glyph* glyph = GlyphAt(selectionRoot, std::min(first, count - 1));
        const float boxHeight = glyph ? glyph->height : 16.0f;
        include(BoundaryX(first) - 3.0f, BoundaryY(first), 6.0f, boxHeight);
    } else {
        for (int i = first; i < last && i < count; ++i) {
            const Glyph* glyph = GlyphAt(selectionRoot, i);
            if (!glyph) {
                continue;
            }
            float boxWidth = glyph->width;
            if (i + 1 < last) {
                const Glyph* next = GlyphAt(selectionRoot, i + 1);
                if (next && SameLine(*glyph, *next)) {
                    boxWidth = std::max(boxWidth, next->x - glyph->x);
                }
            }
            include(glyph->x, glyph->y, boxWidth, glyph->height);
        }
    }

    if (ShowsHandles()) {
        float startX = 0.0f;
        float startY = 0.0f;
        float endX = 0.0f;
        float endY = 0.0f;
        float lineHeight = 0.0f;
        if (HandleGeometry(startX, startY, endX, endY, lineHeight)) {
            const float radius = std::max(5.0f, lineHeight * 0.22f);
            include(startX - radius, startY + lineHeight - radius, radius * 2.0f, radius * 2.0f);
            include(endX - radius, endY + lineHeight - radius, radius * 2.0f, radius * 2.0f);
        }
    }

    if (!any) {
        return false;
    }
    x = left;
    y = top;
    width = std::max(1.0f, right - left);
    height = std::max(1.0f, bottom - top);
    return true;
}

}  // namespace Robo
