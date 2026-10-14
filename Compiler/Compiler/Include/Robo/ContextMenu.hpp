#pragma once

#include "Svg.hpp"

#include <string>
#include <vector>

namespace Robo {

class FontEngine;

struct ContextMenuBounds {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    bool Contains(float pointX, float pointY) const;
};

// One piece of a keyboard shortcut: a pack icon, or a letter.
struct ContextMenuGlyph {
    bool text = false;
    std::string label;
    Svg graphic;
};

// One row in a context menu. A separator is a rule. Children make a submenu.
class ContextMenuItem {
public:
    void SetLabel(const std::string& label);
    const std::string& GetLabel() const;

    void SetDisabled(bool disabled);
    bool IsDisabled() const;

    void SetSeparator(bool separator);
    bool IsSeparator() const;

    void SetChecked(bool checked);
    bool IsChecked() const;

    void SetCheckable(bool checkable);
    bool IsCheckable() const;

    void SetIcon(Svg icon);
    bool HasIcon() const;
    Svg& Icon();
    const Svg& Icon() const;

    void SetChevron(Svg graphic);
    bool HasChevron() const;
    Svg& Chevron();

    void SetCheckGraphic(Svg graphic);
    bool HasCheckGraphic() const;
    Svg& CheckGraphic();

    void AddShortcutGlyph(Svg graphic);
    void AddShortcutText(const std::string& text);
    const std::vector<ContextMenuGlyph>& Shortcut() const;
    float ShortcutWidth(const FontEngine& fonts, float fontSize, float glyphSize) const;

    void AddItem(ContextMenuItem item);
    std::vector<ContextMenuItem>& Items();
    const std::vector<ContextMenuItem>& Items() const;
    bool HasSubmenu() const;

    void SetBounds(const ContextMenuBounds& bounds);
    const ContextMenuBounds& GetBounds() const;

private:
    std::string label_;
    bool disabled_ = false;
    bool separator_ = false;
    bool checked_ = false;
    bool checkable_ = false;
    bool hasIcon_ = false;
    bool hasChevron_ = false;
    bool hasCheck_ = false;
    Svg icon_;
    Svg chevron_;
    Svg check_;
    std::vector<ContextMenuGlyph> shortcut_;
    std::vector<ContextMenuItem> items_;
    ContextMenuBounds bounds_{};
};

// A floating desktop menu. Light, dark, and transparency match the top bar modes.
class ContextMenu {
public:
    void SetAppearanceDark(bool dark);
    void ApplyTheme(bool dark);
    bool IsDark() const;

    void SetTransparency(float amount);
    float GetTransparency() const;

    void AddItem(ContextMenuItem item);
    std::vector<ContextMenuItem>& Items();
    const std::vector<ContextMenuItem>& Items() const;

    void Open(float x, float y);
    void Close();
    bool IsVisible() const;
    float OpenAmount() const;

    void Layout(const FontEngine& fonts, float pixelScale, float viewWidth, float viewHeight, float topInset, float bottomInset);
    bool Contains(float x, float y) const;
    int Hit(float x, float y, int& child) const;

    bool PointerMove(float x, float y);
    bool PointerDown(float x, float y);
    bool PointerUp(float x, float y);
    bool KeyDown(const std::string& key);
    bool Tick(float deltaMs);

    const ContextMenuBounds& Panel() const;
    const ContextMenuBounds& SubmenuPanel() const;
    int Submenu() const;
    int Hover() const;
    int HoverChild() const;
    int Focus() const;
    int FocusChild() const;
    int Pressed() const;
    int PressedChild() const;
    ContextMenuBounds DamageBounds() const;
    std::string ConsumeAction();

private:
    int NextRow(const std::vector<ContextMenuItem>& rows, int index, int direction) const;
    void OpenSubmenu(int index);
    void Activate(int item, int child);

    bool dark_ = false;
    bool appearanceLocked_ = false;
    float transparency_ = 0.0f;
    std::vector<ContextMenuItem> items_;
    bool visible_ = false;
    bool opening_ = false;
    bool closing_ = false;
    float amount_ = 0.0f;
    float anchorX_ = 0.0f;
    float anchorY_ = 0.0f;
    ContextMenuBounds panel_{};
    ContextMenuBounds submenuPanel_{};
    int submenu_ = -1;
    int hover_ = -1;
    int hoverChild_ = -1;
    int focus_ = -1;
    int focusChild_ = -1;
    int pressed_ = -1;
    int pressedChild_ = -1;
    float hoverMs_ = 0.0f;
    std::string action_;
};

}  // namespace Robo
