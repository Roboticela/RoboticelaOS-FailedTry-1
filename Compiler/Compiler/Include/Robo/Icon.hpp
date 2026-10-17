#pragma once

#include "Svg.hpp"

#include <map>
#include <string>

namespace Robo {

// A pack icon. The page XML can resize it and change its color.
// Path data comes from the runtime pack image and is not editable in XML.
class Icon {
public:
    void SetWidth(float width);
    void SetHeight(float height);
    void SetWidthSpec(const std::string& widthSpec);
    void SetHeightSpec(const std::string& heightSpec);
    void SetColor(const Color& color);
    void SetOpacity(float opacity);
    void SetAlign(TextAlign align);
    void SetToken(const std::string& key, const std::string& token);
    void SetViewBox(float x, float y, float width, float height);

    // Used only by the compiler when it bakes a pack icon. XML cannot add shapes.
    void AddPackedShape(SvgShape shape);

    Svg Bake() const;

private:
    float width_ = 24.0f;
    float height_ = 0.0f;
    std::string widthSpec_;
    std::string heightSpec_;
    Color color_ = Color::FromHex("#1C1C1E");
    float opacity_ = 1.0f;
    TextAlign align_ = TextAlign::Left;
    float viewX_ = 0.0f;
    float viewY_ = 0.0f;
    float viewWidth_ = 24.0f;
    float viewHeight_ = 24.0f;
    std::map<std::string, std::string> tokens_;
    mutable std::vector<SvgShape> shapes_;
};

}  // namespace Robo
