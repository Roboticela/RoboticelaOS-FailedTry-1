#pragma once

#include "Color.hpp"
#include "FontStyle.hpp"
#include "Selectable.hpp"
#include "TextAlign.hpp"
#include "TextRun.hpp"

#include <map>
#include <string>
#include <vector>

namespace Robo {

struct TextStyle {
    double fontSize = 16.0;
    Color color = Color::FromHex("#1A1A1A");
    FontWeight fontWeight = FontWeight::Regular;
    FontStyleKind fontStyle = FontStyleKind::Normal;
    TextDecoration textDecoration = TextDecoration::None;
    TextAlign align = TextAlign::Left;
    bool wrap = true;
    double lineHeight = 1.4;
    double letterSpacing = 0.0;
    double opacity = 1.0;
    SelectableMode selectable = SelectableMode::Inherit;
    std::string width = "100%";
    std::string maxWidth;
    // Empty token value means the attribute was set to a custom literal.
    std::map<std::string, std::string> tokens;
};

class Text {
public:
    Text();

    void SetStyle(const TextStyle& style);
    const TextStyle& GetStyle() const;

    void ClearRuns();
    void AddRun(const TextRun& run);
    void AddPlain(const std::string& content);
    const std::vector<TextRun>& GetRuns() const;

private:
    TextStyle style_;
    std::vector<TextRun> runs_;
};

}  // namespace Robo
