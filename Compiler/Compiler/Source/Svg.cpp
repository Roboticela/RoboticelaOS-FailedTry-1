#include "Robo/Svg.hpp"

namespace Robo {

SvgCommand SvgCommand::Move(float x, float y) {
    SvgCommand command;
    command.kind = SvgCommandKind::Move;
    command.x = x;
    command.y = y;
    return command;
}

SvgCommand SvgCommand::Line(float x, float y) {
    SvgCommand command;
    command.kind = SvgCommandKind::Line;
    command.x = x;
    command.y = y;
    return command;
}

SvgCommand SvgCommand::Cubic(float c1x, float c1y, float c2x, float c2y, float x, float y) {
    SvgCommand command;
    command.kind = SvgCommandKind::Cubic;
    command.c1x = c1x;
    command.c1y = c1y;
    command.c2x = c2x;
    command.c2y = c2y;
    command.x = x;
    command.y = y;
    return command;
}

SvgCommand SvgCommand::Close() {
    SvgCommand command;
    command.kind = SvgCommandKind::Close;
    return command;
}

bool SvgBounds::Contains(float pointX, float pointY) const {
    return pointX >= x && pointY >= y && pointX < x + width && pointY < y + height;
}

void Svg::SetStyle(const SvgStyle& style) {
    style_ = style;
}

const SvgStyle& Svg::GetStyle() const {
    return style_;
}

SvgStyle& Svg::GetStyle() {
    return style_;
}

void Svg::AddShape(SvgShape shape) {
    shapes_.push_back(std::move(shape));
}

const std::vector<SvgShape>& Svg::GetShapes() const {
    return shapes_;
}

void AppendSvgCommandsFromBlob(SvgShape& shape, const float* data, std::size_t count) {
    if (data == nullptr || count == 0) {
        return;
    }
    shape.commands.reserve(shape.commands.size() + count / 7u);
    for (std::size_t i = 0; i + 6 < count; i += 7) {
        const int kind = static_cast<int>(data[i]);
        if (kind == 0) {
            shape.commands.push_back(SvgCommand::Move(data[i + 1], data[i + 2]));
        } else if (kind == 1) {
            shape.commands.push_back(SvgCommand::Line(data[i + 1], data[i + 2]));
        } else if (kind == 2) {
            shape.commands.push_back(SvgCommand::Cubic(data[i + 3], data[i + 4], data[i + 5], data[i + 6],
                                                      data[i + 1], data[i + 2]));
        } else {
            shape.commands.push_back(SvgCommand::Close());
        }
    }
}

void Svg::SetBounds(const SvgBounds& bounds) {
    bounds_ = bounds;
}

const SvgBounds& Svg::GetBounds() const {
    return bounds_;
}

}  // namespace Robo
