#include "Robo/Icon.hpp"

namespace Robo {

void Icon::SetWidth(float width) {
    width_ = width;
}

void Icon::SetHeight(float height) {
    height_ = height;
}

void Icon::SetWidthSpec(const std::string& widthSpec) {
    widthSpec_ = widthSpec;
}

void Icon::SetHeightSpec(const std::string& heightSpec) {
    heightSpec_ = heightSpec;
}

void Icon::SetColor(const Color& color) {
    color_ = color;
}

void Icon::SetOpacity(float opacity) {
    opacity_ = opacity;
}

void Icon::SetAlign(TextAlign align) {
    align_ = align;
}

void Icon::SetToken(const std::string& key, const std::string& token) {
    tokens_[key] = token;
}

void Icon::SetViewBox(float x, float y, float width, float height) {
    viewX_ = x;
    viewY_ = y;
    viewWidth_ = width;
    viewHeight_ = height;
}

void Icon::AddPackedShape(SvgShape shape) {
    shapes_.push_back(std::move(shape));
}

Svg Icon::Bake() const {
    Svg svg;
    SvgStyle style;
    style.width = width_;
    style.height = height_;
    style.widthSpec = widthSpec_;
    style.heightSpec = heightSpec_;
    style.viewX = viewX_;
    style.viewY = viewY_;
    style.viewWidth = viewWidth_;
    style.viewHeight = viewHeight_;
    style.color = color_;
    style.opacity = opacity_;
    style.align = align_;
    style.tokens = tokens_;
    svg.SetStyle(style);
    for (const SvgShape& shape : shapes_) {
        svg.AddShape(shape);
    }
    return svg;
}

}  // namespace Robo
