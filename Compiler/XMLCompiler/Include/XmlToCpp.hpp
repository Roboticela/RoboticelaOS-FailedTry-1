#pragma once

#include "SvgPath.hpp"
#include "XmlNode.hpp"

#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace Robo {

class XmlToCpp {
public:
    std::string Convert(const std::string& xml, const std::string& assetDirectory) const;

private:
    std::string ConvertNode(const XmlNode& node, int indent) const;
    void EmitScrollbar(std::ostringstream& code, const XmlNode& node, int indent, const std::string& targetVar) const;
    void EmitContainer(std::ostringstream& code, const XmlNode& node, int indent, const std::string& targetVar,
                       bool emitChildren = true) const;
    void EmitPackedIcon(std::ostringstream& code, int indent, const std::string& iconVar, const std::string& iconName,
                        float size, const std::string& applyLine) const;
    void EmitIconsPane(std::ostringstream& code, const XmlNode& pane, int indent) const;
    void EmitColorPane(std::ostringstream& code, const XmlNode& pane, int indent) const;
    void EmitProsePane(std::ostringstream& code, const XmlNode& pane, int indent) const;
    void EmitTypePane(std::ostringstream& code, const XmlNode& pane, int indent) const;
    void EmitSpacePane(std::ostringstream& code, const XmlNode& pane, int indent) const;
    void EmitRadiusPane(std::ostringstream& code, const XmlNode& pane, int indent) const;
    void EmitBool(std::ostringstream& code, int indent, const char* lhs, const std::string& value, bool fallback,
                  bool defaultValue) const;
    void EmitEnum(std::ostringstream& code, int indent, const char* lhs, const std::string& value,
                  const char* (*map)(const std::string&), const char* parseName, const char* defaultExpr) const;
    void EmitFlexbox(std::ostringstream& code, const XmlNode& node, int indent, const std::string& targetVar) const;
    void EmitFlexChild(std::ostringstream& code, const XmlNode& child, int indent, const std::string& parentVar) const;
    void EmitGrid(std::ostringstream& code, const XmlNode& node, int indent, const std::string& targetVar) const;
    void EmitGridChild(std::ostringstream& code, const XmlNode& child, int indent, const std::string& parentVar) const;
    void EmitPosition(std::ostringstream& code, const XmlNode& node, int indent, const std::string& targetVar) const;
    void EmitPositionChild(std::ostringstream& code, const XmlNode& child, int indent, const std::string& parentVar) const;
    void EmitBrandLogo(std::ostringstream& code, const XmlNode& node, int indent, const std::string& variable) const;
    void EmitTitleBar(std::ostringstream& code, const XmlNode& node, int indent, const std::string& variable) const;
    void EmitDesktop(std::ostringstream& code, const XmlNode& node, int indent) const;
    void EmitSelectionArea(std::ostringstream& code, const XmlNode& node, int indent) const;
    void EmitSelectionLabelStyle(std::ostringstream& code, const XmlNode& node, int indent, const std::string& variable, const std::string& seed = {}) const;
    void EmitDockItem(std::ostringstream& code, const XmlNode& node, int indent, const std::string& variable) const;
    void EmitTopBarMenu(std::ostringstream& code, const XmlNode& node, int indent) const;
    void EmitContextMenu(std::ostringstream& code, const XmlNode& node, int indent) const;
    void EmitContextMenuItem(std::ostringstream& code, const XmlNode& node, int indent, const std::string& variable, int& index) const;
    void EmitSidebar(std::ostringstream& code, const XmlNode& node, int indent, const std::string& variable) const;
    void EmitSidebarItem(std::ostringstream& code, const XmlNode& node, int indent, const std::string& variable,
                         int nestDepth = 0) const;
    void EmitSeparator(std::ostringstream& code, const XmlNode& node, int indent, const std::string& variable) const;
    void EmitThemeToggle(std::ostringstream& code, const XmlNode& node, int indent, const std::string& variable) const;
    void EmitThemeIconToggle(std::ostringstream& code, const XmlNode& node, int indent, const std::string& variable) const;
    void EmitKeyframes(std::ostringstream& code, const XmlNode& node, int indent) const;
    void EmitShape(std::ostringstream& code, const XmlNode& node, int indent, const std::string& variable) const;
    std::string NextShapeName() const;
    // Normalises a numeric XML attribute into a valid C++ float literal.
    void EmitSelectable(std::ostringstream& code, int indent, const std::string& styleName, const XmlNode& node) const;
    void EmitButtonFlags(std::ostringstream& code, int indent, const XmlNode& node) const;
    bool TryEmitGraphic(std::ostringstream& code, const XmlNode& node, int indent, const std::string& parent, bool withProps) const;
    std::string FloatLiteral(const std::string& value) const;
    std::string ExtractToken(const std::string& value) const;
    void WarnLiteral(const std::string& element, const std::string& attribute, const std::string& value) const;
    bool IsFreeLength(const std::string& value) const;
    std::string BindColor(int indent, const std::string& object, const std::string& field, const std::string& value, const std::string& element) const;
    std::string BindFloat(int indent, const std::string& object, const std::string& field, const std::string& value, const std::string& element) const;
    std::string BindLength(int indent, const std::string& object, const std::string& field, const std::string& value, const std::string& element) const;
    // Item props (Flex/Grid/Position) have no token map — emit Length only.
    std::string BindPropsLength(int indent, const std::string& object, const std::string& field, const std::string& value) const;
    std::string BindWeight(int indent, const std::string& object, const std::string& value, const std::string& element) const;
    std::string BindSides(int indent, const std::string& object, const std::string& kind, const std::string& value, const std::string& element) const;
    void CollectFoundations(const XmlNode& node) const;
    std::string EmitFoundation(const XmlNode& node, int indent) const;
    std::string EmitFoundationUse(const XmlNode& node, int indent) const;
    std::string Indent(int level) const;
    std::string EscapeCpp(const std::string& value) const;

    mutable std::map<std::string, CompiledSvg> icons_;
    mutable std::set<std::string> foundations_;
    mutable std::set<std::string> emittedFoundations_;
    mutable int shapeSerial_ = 0;
};

}  // namespace Robo
