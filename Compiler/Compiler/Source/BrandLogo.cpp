#include "Robo/BrandLogo.hpp"

#include <algorithm>

namespace Robo {

namespace {

const char* PRODUCT_NAME = "DevKit";
const char* COMPANY_NAME = "Roboticela";

void AddLine(std::vector<SvgCommand>& commands, float x, float y) {
    commands.push_back(SvgCommand::Line(x, y));
}

void AddRoundedRect(std::vector<SvgCommand>& commands, float x, float y, float width, float height, float radius) {
    radius = std::min(radius, std::min(width, height) * 0.5f);
    const float kx = radius * 0.55228475f;
    const float ky = radius * 0.55228475f;
    commands.push_back(SvgCommand::Move(x + radius, y));
    AddLine(commands, x + width - radius, y);
    commands.push_back(SvgCommand::Cubic(x + width - radius + kx, y, x + width, y + radius - ky, x + width, y + radius));
    AddLine(commands, x + width, y + height - radius);
    commands.push_back(SvgCommand::Cubic(x + width, y + height - radius + ky, x + width - radius + kx, y + height, x + width - radius, y + height));
    AddLine(commands, x + radius, y + height);
    commands.push_back(SvgCommand::Cubic(x + radius - kx, y + height, x, y + height - radius + ky, x, y + height - radius));
    AddLine(commands, x, y + radius);
    commands.push_back(SvgCommand::Cubic(x, y + radius - ky, x + radius - kx, y, x + radius, y));
    commands.push_back(SvgCommand::Close());
}

SvgShape Stroke(std::vector<SvgCommand> commands, const char* hex) {
    SvgShape shape;
    shape.commands = std::move(commands);
    shape.hasFill = false;
    shape.hasStroke = true;
    shape.stroke = Color::FromHex(hex);
    shape.strokeWidth = 2.0f;
    return shape;
}

}  // namespace

void BrandLogo::SetSize(float size) {
    size_ = size;
}

float BrandLogo::GetSize() const {
    return size_;
}

void BrandLogo::SetVariant(BrandLogoVariant variant) {
    variant_ = variant;
}

BrandLogoVariant BrandLogo::GetVariant() const {
    return variant_;
}

void BrandLogo::SetPrimary(const std::string& primary) {
    primary_ = primary;
}

void BrandLogo::SetSecondary(const std::string& secondary) {
    secondary_ = secondary;
}

const std::string& BrandLogo::Title() const {
    if (!primary_.empty()) {
        return primary_;
    }
    static const std::string company(COMPANY_NAME);
    static const std::string product(PRODUCT_NAME);
    return variant_ == BrandLogoVariant::Lockup ? company : product;
}

const std::string& BrandLogo::Subtitle() const {
    if (!secondary_.empty()) {
        return secondary_;
    }
    static const std::string product(PRODUCT_NAME);
    return product;
}

bool BrandLogo::ShowsLabel() const {
    return variant_ == BrandLogoVariant::Text || variant_ == BrandLogoVariant::Lockup;
}

Svg BrandLogo::Mark() const {
    Svg svg;
    SvgStyle style;
    style.viewX = 0.0f;
    style.viewY = 0.0f;
    style.viewWidth = 24.0f;
    style.viewHeight = 24.0f;
    style.width = size_;
    style.height = size_;
    svg.SetStyle(style);

    std::vector<SvgCommand> frame;
    AddRoundedRect(frame, 3.0f, 3.0f, 18.0f, 18.0f, 3.0f);
    svg.AddShape(Stroke(std::move(frame), "#8B5CF6"));

    std::vector<SvgCommand> left;
    left.push_back(SvgCommand::Move(10.0f, 8.0f));
    AddLine(left, 6.0f, 12.0f);
    AddLine(left, 10.0f, 16.0f);
    svg.AddShape(Stroke(std::move(left), "#10B981"));

    std::vector<SvgCommand> right;
    right.push_back(SvgCommand::Move(14.0f, 8.0f));
    AddLine(right, 18.0f, 12.0f);
    AddLine(right, 14.0f, 16.0f);
    svg.AddShape(Stroke(std::move(right), "#10B981"));
    return svg;
}

BrandLogoVariant ParseBrandLogoVariant(const std::string& value) {
    if (value == "icon") {
        return BrandLogoVariant::Icon;
    }
    if (value == "text") {
        return BrandLogoVariant::Text;
    }
    if (value == "lockup") {
        return BrandLogoVariant::Lockup;
    }
    return BrandLogoVariant::Mark;
}

}  // namespace Robo
