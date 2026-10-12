#include "Robo/Foundation.hpp"

#include "Robo/Button.hpp"
#include "Robo/Container.hpp"
#include "Robo/Flexbox.hpp"
#include "Robo/Grid.hpp"
#include "Robo/Input.hpp"
#include "Robo/Position.hpp"
#include "Robo/Scrollbar.hpp"
#include "Robo/Svg.hpp"
#include "Robo/Text.hpp"
#include "Robo/TextRun.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

namespace Robo {

namespace {

std::map<std::string, Foundation>& Registry() {
    static std::map<std::string, Foundation> registry;
    return registry;
}

struct ScopeFrame {
    const Foundation* foundation = nullptr;
    Theme theme = Theme::Light;
    float themeMix = 0.0f;
};

Color LerpThemeColor(const Color& light, const Color& dark, float mix) {
    mix = std::max(0.0f, std::min(1.0f, mix));
    auto channel = [mix](std::uint8_t from, std::uint8_t to) {
        return static_cast<std::uint8_t>(std::lround(static_cast<float>(from) + (static_cast<float>(to) - static_cast<float>(from)) * mix));
    };
    return Color::FromRgba(channel(light.red, dark.red), channel(light.green, dark.green), channel(light.blue, dark.blue),
                           channel(light.alpha, dark.alpha));
}

std::vector<ScopeFrame>& ScopeStack() {
    static std::vector<ScopeFrame> stack;
    return stack;
}

bool StartsWith(const std::string& value, const char* prefix) {
    const std::size_t length = std::char_traits<char>::length(prefix);
    return value.size() >= length && value.compare(0, length, prefix) == 0;
}

std::string StripBraces(const std::string& token) {
    if (token.size() >= 2 && token.front() == '{' && token.back() == '}') {
        return token.substr(1, token.size() - 2);
    }
    return token;
}

void ApplyFloat(float& field, const std::map<std::string, std::string>& tokens, const char* key) {
    const std::string token = TokenOf(tokens, key);
    if (token.empty()) {
        return;
    }
    field = ResolveFloat(field, token);
}

void ApplyDouble(double& field, const std::map<std::string, std::string>& tokens, const char* key) {
    const std::string token = TokenOf(tokens, key);
    if (token.empty()) {
        return;
    }
    field = static_cast<double>(ResolveFloat(static_cast<float>(field), token));
}

void ApplyColor(Color& color, const std::map<std::string, std::string>& tokens, const char* key) {
    const std::string token = TokenOf(tokens, key);
    if (token.empty()) {
        return;
    }
    color = ResolveColor(color, token);
}

void ApplyLength(std::string& length, const std::map<std::string, std::string>& tokens, const char* key) {
    const std::string token = TokenOf(tokens, key);
    if (token.empty()) {
        return;
    }
    float value = 0.0f;
    if (!CurrentFoundation().LookupFloat(token, value)) {
        return;
    }
    length = std::to_string(static_cast<double>(value));
}

void ApplyTracking(double& letterSpacing, double fontSize, const std::map<std::string, std::string>& tokens) {
    const std::string token = TokenOf(tokens, "letterSpacing");
    if (token.empty()) {
        return;
    }
    const float value = ResolveFloat(static_cast<float>(letterSpacing), token);
    if (token.find(".tracking") != std::string::npos) {
        letterSpacing = static_cast<double>(value) * fontSize;
        return;
    }
    letterSpacing = static_cast<double>(value);
}

void ApplyWeight(FontWeight& weight, const std::map<std::string, std::string>& tokens) {
    const std::string token = TokenOf(tokens, "fontWeight");
    if (token.empty()) {
        return;
    }
    weight = ResolveWeight(weight, token);
}

}  // namespace

void Foundation::SetName(const std::string& name) {
    name_ = name;
}

const std::string& Foundation::GetName() const {
    return name_;
}

void Foundation::Seal() {
    sealed_ = true;
}

bool Foundation::IsSealed() const {
    return sealed_;
}

void Foundation::SetColor(const std::string& name, const Color& light, const Color& dark) {
    if (sealed_) {
        return;
    }
    colors_[name].light = light;
    colors_[name].dark = dark;
    explicitColors_[name] = true;
}

void Foundation::SetSpace(const std::string& name, float value) {
    if (sealed_) {
        return;
    }
    space_[name] = value;
}

void Foundation::SetRadius(const std::string& name, float value) {
    if (sealed_) {
        return;
    }
    radius_[name] = value;
}

void Foundation::SetLayout(const std::string& name, float value) {
    if (sealed_) {
        return;
    }
    layout_[name] = value;
}

void Foundation::SetType(const std::string& role, float size, float mobile, int weight, float tracking, float leading) {
    if (sealed_) {
        return;
    }
    TypeRole& type = type_[role];
    type.size = size;
    type.mobile = mobile;
    type.weight = weight;
    type.tracking = tracking;
    type.leading = leading;
}

void Foundation::CopyColorsFrom(const Foundation& source) {
    if (sealed_) {
        return;
    }
    colors_ = source.colors_;
    explicitColors_ = source.explicitColors_;
}

void Foundation::CopyTypographyFrom(const Foundation& source) {
    if (sealed_) {
        return;
    }
    type_ = source.type_;
}

void Foundation::CopySpacingFrom(const Foundation& source) {
    if (sealed_) {
        return;
    }
    space_ = source.space_;
}

void Foundation::CopyRadiusFrom(const Foundation& source) {
    if (sealed_) {
        return;
    }
    radius_ = source.radius_;
}

void Foundation::CopyLayoutFrom(const Foundation& source) {
    if (sealed_) {
        return;
    }
    layout_ = source.layout_;
}

bool Foundation::LookupColor(const std::string& name, Theme theme, Color& out) const {
    std::string key = StripBraces(name);
    if (StartsWith(key, "color.")) {
        key = key.substr(6);
    }
    const auto explicitColor = explicitColors_.find(key);
    const bool isExplicit = explicitColor != explicitColors_.end() && explicitColor->second;
    if (!isExplicit && (key == "text.link" || key == "border.focus")) {
        key = "brand.primary";
    }
    const auto found = colors_.find(key);
    if (found == colors_.end()) {
        return false;
    }
    out = theme == Theme::Dark ? found->second.dark : found->second.light;
    return true;
}

bool Foundation::LookupFloat(const std::string& token, float& out) const {
    const std::string path = StripBraces(token);
    if (StartsWith(path, "space.")) {
        const auto found = space_.find(path.substr(6));
        if (found == space_.end()) {
            return false;
        }
        out = found->second;
        return true;
    }
    if (StartsWith(path, "radius.")) {
        std::string key = path.substr(7);
        if (key == "control") {
            key = "md";
        } else if (key == "card") {
            key = "lg";
        } else if (key == "panel") {
            key = "2xl";
        }
        const auto found = radius_.find(key);
        if (found == radius_.end()) {
            return false;
        }
        out = found->second;
        return true;
    }
    if (StartsWith(path, "layout.")) {
        const auto found = layout_.find(path.substr(7));
        if (found == layout_.end()) {
            return false;
        }
        out = found->second;
        return true;
    }
    std::string rest;
    if (StartsWith(path, "type.")) {
        rest = path.substr(5);
    } else if (StartsWith(path, "typography.")) {
        rest = path.substr(12);
    } else {
        return false;
    }
    const std::size_t dot = rest.find('.');
    const std::string role = dot == std::string::npos ? rest : rest.substr(0, dot);
    const std::string field = dot == std::string::npos ? "size" : rest.substr(dot + 1);
    const auto found = type_.find(role);
    if (found == type_.end()) {
        return false;
    }
    const TypeRole& type = found->second;
    if (field == "size") {
        out = type.size;
    } else if (field == "mobile") {
        out = type.mobile;
    } else if (field == "weight") {
        out = static_cast<float>(type.weight);
    } else if (field == "tracking") {
        out = type.tracking;
    } else if (field == "leading") {
        out = type.leading;
    } else {
        return false;
    }
    return true;
}

bool Foundation::IsKnownToken(const std::string& token) {
    const Foundation& foundation = RoboFoundation();
    Color color;
    float value = 0.0f;
    return foundation.LookupColor(token, Theme::Light, color) || foundation.LookupFloat(token, value);
}

Foundation Foundation::CopyOf(const std::string& name) {
    const Foundation* source = Find(name);
    if (source == nullptr) {
        source = &RoboFoundation();
    }
    Foundation copy = *source;
    copy.sealed_ = false;
    copy.name_.clear();
    return copy;
}

const Foundation* Foundation::Find(const std::string& name) {
    const auto found = Registry().find(name);
    if (found == Registry().end()) {
        return nullptr;
    }
    return &found->second;
}

void Foundation::Register(Foundation foundation) {
    if (foundation.name_.empty()) {
        return;
    }
    if (foundation.name_ == "RoboFoundation" && Registry().count("RoboFoundation") != 0) {
        return;
    }
    const std::string name = foundation.name_;
    Registry()[name] = std::move(foundation);
}

FoundationScope::FoundationScope(const Foundation& foundation, Theme theme, float themeMix) {
    if (themeMix < 0.0f) {
        themeMix = theme == Theme::Dark ? 1.0f : 0.0f;
    }
    ScopeStack().push_back(ScopeFrame{&foundation, theme, themeMix});
}

FoundationScope::~FoundationScope() {
    if (!ScopeStack().empty()) {
        ScopeStack().pop_back();
    }
}

const Foundation& CurrentFoundation() {
    if (ScopeStack().empty() || ScopeStack().back().foundation == nullptr) {
        return Foundation::RoboFoundation();
    }
    return *ScopeStack().back().foundation;
}

Theme CurrentTheme() {
    if (ScopeStack().empty()) {
        return Theme::Light;
    }
    return ScopeStack().back().theme;
}

float CurrentThemeMix() {
    if (ScopeStack().empty()) {
        return 0.0f;
    }
    return ScopeStack().back().themeMix;
}

Color ResolveColor(const Color& literal, const std::string& token) {
    if (token.empty()) {
        return literal;
    }
    Color light = literal;
    Color dark = literal;
    const bool hasLight = CurrentFoundation().LookupColor(token, Theme::Light, light);
    const bool hasDark = CurrentFoundation().LookupColor(token, Theme::Dark, dark);
    if (!hasLight && !hasDark) {
        return literal;
    }
    if (!hasLight) {
        light = dark;
    }
    if (!hasDark) {
        dark = light;
    }
    return LerpThemeColor(light, dark, CurrentThemeMix());
}

float ResolveFloat(float literal, const std::string& token) {
    if (token.empty()) {
        return literal;
    }
    float resolved = literal;
    if (!CurrentFoundation().LookupFloat(token, resolved)) {
        return literal;
    }
    return resolved;
}

FontWeight ResolveWeight(FontWeight literal, const std::string& token) {
    if (token.empty()) {
        return literal;
    }
    float weight = 400.0f;
    if (!CurrentFoundation().LookupFloat(token, weight)) {
        return literal;
    }
    if (weight >= 700.0f) {
        return FontWeight::Bold;
    }
    if (weight >= 600.0f) {
        return FontWeight::SemiBold;
    }
    if (weight >= 500.0f) {
        return FontWeight::Medium;
    }
    return FontWeight::Regular;
}

std::string TokenOf(const std::map<std::string, std::string>& tokens, const std::string& key) {
    const auto found = tokens.find(key);
    if (found == tokens.end()) {
        return {};
    }
    return found->second;
}

void Resolve(TextStyle& style) {
    ApplyDouble(style.fontSize, style.tokens, "fontSize");
    ApplyDouble(style.lineHeight, style.tokens, "lineHeight");
    ApplyTracking(style.letterSpacing, style.fontSize, style.tokens);
    ApplyWeight(style.fontWeight, style.tokens);
    ApplyColor(style.color, style.tokens, "color");
    ApplyLength(style.width, style.tokens, "width");
    ApplyLength(style.maxWidth, style.tokens, "maxWidth");
}

void Resolve(TextRun& run) {
    ApplyDouble(run.fontSize, run.tokens, "fontSize");
    ApplyTracking(run.letterSpacing, run.fontSize, run.tokens);
    ApplyWeight(run.fontWeight, run.tokens);
    ApplyColor(run.color, run.tokens, "color");
}

void Resolve(ButtonStyle& style) {
    ApplyDouble(style.fontSize, style.tokens, "fontSize");
    ApplyDouble(style.cornerRadius, style.tokens, "cornerRadius");
    ApplyDouble(style.paddingX, style.tokens, "paddingX");
    ApplyDouble(style.paddingY, style.tokens, "paddingY");
    ApplyTracking(style.letterSpacing, style.fontSize, style.tokens);
    ApplyWeight(style.fontWeight, style.tokens);
    ApplyColor(style.textColor, style.tokens, "textColor");
    ApplyColor(style.backgroundColor, style.tokens, "backgroundColor");
}

void Resolve(InputStyle& style) {
    ApplyDouble(style.fontSize, style.tokens, "fontSize");
    ApplyDouble(style.cornerRadius, style.tokens, "cornerRadius");
    ApplyDouble(style.paddingX, style.tokens, "paddingX");
    ApplyDouble(style.paddingY, style.tokens, "paddingY");
    ApplyTracking(style.letterSpacing, style.fontSize, style.tokens);
    ApplyWeight(style.fontWeight, style.tokens);
    ApplyColor(style.textColor, style.tokens, "textColor");
    ApplyColor(style.placeholderColor, style.tokens, "placeholderColor");
    ApplyColor(style.backgroundColor, style.tokens, "backgroundColor");
    ApplyColor(style.borderColor, style.tokens, "borderColor");
    ApplyColor(style.focusBorderColor, style.tokens, "focusBorderColor");
    ApplyColor(style.caretColor, style.tokens, "caretColor");
    ApplyLength(style.width, style.tokens, "width");
}

void Resolve(FlexboxStyle& style) {
    ApplyFloat(style.gap, style.tokens, "gap");
    ApplyFloat(style.rowGap, style.tokens, "rowGap");
    ApplyFloat(style.columnGap, style.tokens, "columnGap");
    ApplyFloat(style.paddingTop, style.tokens, "paddingTop");
    ApplyFloat(style.paddingBottom, style.tokens, "paddingBottom");
    ApplyFloat(style.paddingLeft, style.tokens, "paddingLeft");
    ApplyFloat(style.paddingRight, style.tokens, "paddingRight");
    ApplyFloat(style.marginTop, style.tokens, "marginTop");
    ApplyFloat(style.marginBottom, style.tokens, "marginBottom");
    ApplyFloat(style.marginLeft, style.tokens, "marginLeft");
    ApplyFloat(style.marginRight, style.tokens, "marginRight");
    ApplyFloat(style.cornerRadius, style.tokens, "cornerRadius");
    ApplyColor(style.backgroundColor, style.tokens, "backgroundColor");
    ApplyColor(style.borderColor, style.tokens, "borderColor");
    ApplyLength(style.width, style.tokens, "width");
    ApplyLength(style.height, style.tokens, "height");
}

void Resolve(GridStyle& style) {
    ApplyFloat(style.gap, style.tokens, "gap");
    ApplyFloat(style.rowGap, style.tokens, "rowGap");
    ApplyFloat(style.columnGap, style.tokens, "columnGap");
    ApplyFloat(style.paddingTop, style.tokens, "paddingTop");
    ApplyFloat(style.paddingBottom, style.tokens, "paddingBottom");
    ApplyFloat(style.paddingLeft, style.tokens, "paddingLeft");
    ApplyFloat(style.paddingRight, style.tokens, "paddingRight");
    ApplyFloat(style.marginTop, style.tokens, "marginTop");
    ApplyFloat(style.marginBottom, style.tokens, "marginBottom");
    ApplyFloat(style.marginLeft, style.tokens, "marginLeft");
    ApplyFloat(style.marginRight, style.tokens, "marginRight");
    ApplyFloat(style.cornerRadius, style.tokens, "cornerRadius");
    ApplyColor(style.backgroundColor, style.tokens, "backgroundColor");
    ApplyColor(style.borderColor, style.tokens, "borderColor");
    ApplyLength(style.width, style.tokens, "width");
    ApplyLength(style.height, style.tokens, "height");
}

void Resolve(PositionStyle& style) {
    ApplyFloat(style.paddingTop, style.tokens, "paddingTop");
    ApplyFloat(style.paddingBottom, style.tokens, "paddingBottom");
    ApplyFloat(style.paddingLeft, style.tokens, "paddingLeft");
    ApplyFloat(style.paddingRight, style.tokens, "paddingRight");
    ApplyFloat(style.cornerRadius, style.tokens, "cornerRadius");
    ApplyColor(style.backgroundColor, style.tokens, "backgroundColor");
    ApplyColor(style.borderColor, style.tokens, "borderColor");
    ApplyLength(style.width, style.tokens, "width");
    ApplyLength(style.height, style.tokens, "height");
}

void Resolve(ContainerStyle& style) {
    ApplyFloat(style.gap, style.tokens, "gap");
    ApplyFloat(style.paddingTop, style.tokens, "paddingTop");
    ApplyFloat(style.paddingBottom, style.tokens, "paddingBottom");
    ApplyFloat(style.paddingLeft, style.tokens, "paddingLeft");
    ApplyFloat(style.paddingRight, style.tokens, "paddingRight");
    ApplyFloat(style.cornerRadius, style.tokens, "cornerRadius");
    ApplyColor(style.backgroundColor, style.tokens, "backgroundColor");
    ApplyColor(style.borderColor, style.tokens, "borderColor");
    ApplyLength(style.width, style.tokens, "width");
    ApplyLength(style.height, style.tokens, "height");
}

void Resolve(ScrollbarStyle& style) {
    ApplyDouble(style.cornerRadius, style.tokens, "cornerRadius");
    ApplyColor(style.trackColor, style.tokens, "trackColor");
    ApplyColor(style.thumbColor, style.tokens, "thumbColor");
    ApplyColor(style.thumbHoverColor, style.tokens, "thumbHoverColor");
    ApplyColor(style.thumbActiveColor, style.tokens, "thumbActiveColor");
    ApplyColor(style.arrowColor, style.tokens, "arrowColor");
    ApplyColor(style.arrowHoverColor, style.tokens, "arrowHoverColor");
    ApplyColor(style.arrowActiveColor, style.tokens, "arrowActiveColor");
}

void Resolve(SvgStyle& style) {
    if (!TokenOf(style.tokens, "width").empty()) {
        ApplyFloat(style.width, style.tokens, "width");
        style.widthSpec.clear();
    }
    if (!TokenOf(style.tokens, "height").empty()) {
        ApplyFloat(style.height, style.tokens, "height");
        style.heightSpec.clear();
    }
    ApplyColor(style.color, style.tokens, "color");
}

}  // namespace Robo
