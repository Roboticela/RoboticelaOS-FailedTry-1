#pragma once

#include "Color.hpp"
#include "FontStyle.hpp"

#include <map>
#include <string>
#include <vector>

namespace Robo {

enum class Theme {
    Light,
    Dark
};

// One named design system. RoboFoundation is sealed. A page copies it,
// edits the copy, and can make that copy the active foundation.
class Foundation {
public:
    void SetName(const std::string& name);
    const std::string& GetName() const;
    void Seal();
    bool IsSealed() const;

    void SetColor(const std::string& name, const Color& light, const Color& dark);
    void SetSpace(const std::string& name, float value);
    void SetRadius(const std::string& name, float value);
    void SetLayout(const std::string& name, float value);
    void SetType(const std::string& role, float size, float mobile, int weight, float tracking, float leading);

    void CopyColorsFrom(const Foundation& source);
    void CopyTypographyFrom(const Foundation& source);
    void CopySpacingFrom(const Foundation& source);
    void CopyRadiusFrom(const Foundation& source);
    void CopyLayoutFrom(const Foundation& source);

    bool LookupColor(const std::string& name, Theme theme, Color& out) const;
    bool LookupFloat(const std::string& token, float& out) const;

    static bool IsKnownToken(const std::string& token);
    static Foundation CopyOf(const std::string& name);
    static const Foundation* Find(const std::string& name);
    static const Foundation& RoboFoundation();
    static void Register(Foundation foundation);

private:
    struct ColorPair {
        Color light;
        Color dark;
    };
    struct TypeRole {
        float size = 16.0f;
        float mobile = 16.0f;
        int weight = 400;
        float tracking = 0.0f;
        float leading = 1.4f;
    };

    std::string name_;
    bool sealed_ = false;
    std::map<std::string, ColorPair> colors_;
    std::map<std::string, bool> explicitColors_;
    std::map<std::string, TypeRole> type_;
    std::map<std::string, float> space_;
    std::map<std::string, float> radius_;
    std::map<std::string, float> layout_;
};

class FoundationScope {
public:
    // themeMix blends light (0) to dark (1). A negative value snaps to the theme.
    FoundationScope(const Foundation& foundation, Theme theme, float themeMix = -1.0f);
    ~FoundationScope();

    FoundationScope(const FoundationScope&) = delete;
    FoundationScope& operator=(const FoundationScope&) = delete;
};

const Foundation& CurrentFoundation();
Theme CurrentTheme();
float CurrentThemeMix();

Color ResolveColor(const Color& literal, const std::string& token);
float ResolveFloat(float literal, const std::string& token);
FontWeight ResolveWeight(FontWeight literal, const std::string& token);

std::string TokenOf(const std::map<std::string, std::string>& tokens, const std::string& key);

struct TextStyle;
struct TextRun;
struct ButtonStyle;
struct InputStyle;
struct FlexboxStyle;
struct GridStyle;
struct PositionStyle;
struct ContainerStyle;
struct SvgStyle;
struct ScrollbarStyle;

void Resolve(TextStyle& style);
void Resolve(TextRun& run);
void Resolve(ButtonStyle& style);
void Resolve(InputStyle& style);
void Resolve(FlexboxStyle& style);
void Resolve(GridStyle& style);
void Resolve(PositionStyle& style);
void Resolve(ContainerStyle& style);
void Resolve(SvgStyle& style);
void Resolve(ScrollbarStyle& style);

}  // namespace Robo
