#include "SvgPath.hpp"

#include "Robo/Foundation.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cctype>

namespace Robo {

namespace {

struct Affine {
    float a = 1.0f;
    float b = 0.0f;
    float c = 0.0f;
    float d = 1.0f;
    float e = 0.0f;
    float f = 0.0f;
};

struct Paint {
    bool hasFill = true;
    bool currentFill = false;
    std::string fillHex = "#000000";
    std::string fillToken;
    bool hasStroke = false;
    bool currentStroke = false;
    std::string strokeHex = "#000000";
    std::string strokeToken;
    float strokeWidth = 1.0f;
    float opacity = 1.0f;
    bool evenOdd = false;
};

Affine Multiply(const Affine& parent, const Affine& local) {
    Affine out;
    out.a = parent.a * local.a + parent.c * local.b;
    out.b = parent.b * local.a + parent.d * local.b;
    out.c = parent.a * local.c + parent.c * local.d;
    out.d = parent.b * local.c + parent.d * local.d;
    out.e = parent.a * local.e + parent.c * local.f + parent.e;
    out.f = parent.b * local.e + parent.d * local.f + parent.f;
    return out;
}

void MapPoint(const Affine& matrix, float& x, float& y) {
    const float nextX = matrix.a * x + matrix.c * y + matrix.e;
    const float nextY = matrix.b * x + matrix.d * y + matrix.f;
    x = nextX;
    y = nextY;
}

bool EqualName(const std::string& value, const char* expected) {
    std::size_t i = 0;
    for (; expected[i] != '\0'; ++i) {
        if (i >= value.size()) {
            return false;
        }
        char left = value[i];
        char right = expected[i];
        if (left >= 'A' && left <= 'Z') {
            left = static_cast<char>(left - 'A' + 'a');
        }
        if (left != right) {
            return false;
        }
    }
    return i == value.size();
}

std::string Attribute(const XmlNode& node, const char* name, const std::string& fallback = {}) {
    const auto it = node.attributes.find(name);
    if (it == node.attributes.end()) {
        return fallback;
    }
    return it->second;
}

float NumberOf(const std::string& value, float fallback) {
    if (value.empty()) {
        return fallback;
    }
    return static_cast<float>(std::atof(value.c_str()));
}

class PathCursor {
public:
    explicit PathCursor(const std::string& text) : text_(text) {}

    void Skip() {
        while (index_ < text_.size()) {
            const char ch = text_[index_];
            if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == ',') {
                ++index_;
                continue;
            }
            break;
        }
    }

    bool More() {
        Skip();
        return index_ < text_.size();
    }

    bool NumberReady() {
        Skip();
        if (index_ >= text_.size()) {
            return false;
        }
        const char ch = text_[index_];
        return (ch >= '0' && ch <= '9') || ch == '-' || ch == '+' || ch == '.';
    }

    float Number() {
        Skip();
        const std::size_t start = index_;
        if (index_ < text_.size() && (text_[index_] == '+' || text_[index_] == '-')) {
            ++index_;
        }
        while (index_ < text_.size() && text_[index_] >= '0' && text_[index_] <= '9') {
            ++index_;
        }
        if (index_ < text_.size() && text_[index_] == '.') {
            ++index_;
            while (index_ < text_.size() && text_[index_] >= '0' && text_[index_] <= '9') {
                ++index_;
            }
        }
        if (index_ < text_.size() && (text_[index_] == 'e' || text_[index_] == 'E')) {
            ++index_;
            if (index_ < text_.size() && (text_[index_] == '+' || text_[index_] == '-')) {
                ++index_;
            }
            while (index_ < text_.size() && text_[index_] >= '0' && text_[index_] <= '9') {
                ++index_;
            }
        }
        return static_cast<float>(std::atof(text_.c_str() + start));
    }

    char Command() {
        Skip();
        if (index_ >= text_.size()) {
            return 0;
        }
        const char ch = text_[index_];
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
            ++index_;
            return ch;
        }
        return 0;
    }

private:
    const std::string& text_;
    std::size_t index_ = 0;
};

void AddMove(std::vector<CompiledCommand>& out, float x, float y) {
    CompiledCommand command;
    command.kind = 0;
    command.x = x;
    command.y = y;
    out.push_back(command);
}

void AddLine(std::vector<CompiledCommand>& out, float x, float y) {
    CompiledCommand command;
    command.kind = 1;
    command.x = x;
    command.y = y;
    out.push_back(command);
}

void AddCubic(std::vector<CompiledCommand>& out, float c1x, float c1y, float c2x, float c2y, float x, float y) {
    CompiledCommand command;
    command.kind = 2;
    command.c1x = c1x;
    command.c1y = c1y;
    command.c2x = c2x;
    command.c2y = c2y;
    command.x = x;
    command.y = y;
    out.push_back(command);
}

void AddClose(std::vector<CompiledCommand>& out) {
    CompiledCommand command;
    command.kind = 3;
    out.push_back(command);
}

void AddArc(std::vector<CompiledCommand>& commands,
            float x0,
            float y0,
            float rx,
            float ry,
            float angleDegrees,
            bool large,
            bool sweep,
            float x1,
            float y1) {
    if (rx == 0.0f || ry == 0.0f) {
        AddLine(commands, x1, y1);
        return;
    }
    rx = std::fabs(rx);
    ry = std::fabs(ry);
    const float angle = angleDegrees * 3.14159265f / 180.0f;
    const float cosA = std::cos(angle);
    const float sinA = std::sin(angle);
    const float dx = (x0 - x1) * 0.5f;
    const float dy = (y0 - y1) * 0.5f;
    const float x1p = cosA * dx + sinA * dy;
    const float y1p = -sinA * dx + cosA * dy;
    float rxSq = rx * rx;
    float rySq = ry * ry;
    const float x1pSq = x1p * x1p;
    const float y1pSq = y1p * y1p;
    float lambda = x1pSq / rxSq + y1pSq / rySq;
    if (lambda > 1.0f) {
        const float scale = std::sqrt(lambda);
        rx *= scale;
        ry *= scale;
        rxSq = rx * rx;
        rySq = ry * ry;
    }
    float factor = std::max(0.0f, (rxSq * rySq - rxSq * y1pSq - rySq * x1pSq) / (rxSq * y1pSq + rySq * x1pSq));
    factor = std::sqrt(factor);
    if (large == sweep) {
        factor = -factor;
    }
    const float cxp = factor * rx * y1p / ry;
    const float cyp = factor * -ry * x1p / rx;
    const float cx = cosA * cxp - sinA * cyp + (x0 + x1) * 0.5f;
    const float cy = sinA * cxp + cosA * cyp + (y0 + y1) * 0.5f;
    auto vectorAngle = [](float ux, float uy, float vx, float vy) {
        const float dot = ux * vx + uy * vy;
        const float len = std::sqrt(std::max(0.0f, (ux * ux + uy * uy) * (vx * vx + vy * vy)));
        float ang = len == 0.0f ? 0.0f : std::acos(std::max(-1.0f, std::min(1.0f, dot / len)));
        if (ux * vy - uy * vx < 0.0f) {
            ang = -ang;
        }
        return ang;
    };
    const float theta1 = vectorAngle(1.0f, 0.0f, (x1p - cxp) / rx, (y1p - cyp) / ry);
    float delta = vectorAngle((x1p - cxp) / rx, (y1p - cyp) / ry, (-x1p - cxp) / rx, (-y1p - cyp) / ry);
    if (!sweep && delta > 0.0f) {
        delta -= 6.2831853f;
    } else if (sweep && delta < 0.0f) {
        delta += 6.2831853f;
    }
    const int segments = std::max(1, static_cast<int>(std::ceil(std::fabs(delta) / 1.5707963f)));
    const float step = delta / static_cast<float>(segments);
    float start = theta1;
    for (int i = 0; i < segments; ++i) {
        const float end = start + step;
        const float alpha = std::sin(step) * (std::sqrt(4.0f + 3.0f * std::pow(std::tan(step * 0.5f), 2.0f)) - 1.0f) / 3.0f;
        const float cosStart = std::cos(start);
        const float sinStart = std::sin(start);
        const float cosEnd = std::cos(end);
        const float sinEnd = std::sin(end);
        float c1x = cx + rx * cosA * (cosStart - alpha * sinStart) - ry * sinA * (sinStart + alpha * cosStart);
        float c1y = cy + rx * sinA * (cosStart - alpha * sinStart) + ry * cosA * (sinStart + alpha * cosStart);
        float c2x = cx + rx * cosA * (cosEnd + alpha * sinEnd) - ry * sinA * (sinEnd - alpha * cosEnd);
        float c2y = cy + rx * sinA * (cosEnd + alpha * sinEnd) + ry * cosA * (sinEnd - alpha * cosEnd);
        float ex = cx + rx * cosA * cosEnd - ry * sinA * sinEnd;
        float ey = cy + rx * sinA * cosEnd + ry * cosA * sinEnd;
        AddCubic(commands, c1x, c1y, c2x, c2y, ex, ey);
        start = end;
    }
}

void ParsePathData(const std::string& data, std::vector<CompiledCommand>& commands) {
    PathCursor cursor(data);
    char command = 0;
    float cx = 0.0f;
    float cy = 0.0f;
    float sx = 0.0f;
    float sy = 0.0f;
    float lastCx = 0.0f;
    float lastCy = 0.0f;
    bool hasControl = false;
    while (cursor.More()) {
        const char next = cursor.Command();
        if (next != 0) {
            command = next;
        } else if (command == 0) {
            break;
        }
        const bool relative = command >= 'a' && command <= 'z';
        const char kind = relative ? static_cast<char>(command - 'a' + 'A') : command;
        if (kind == 'M') {
            float x = cursor.Number();
            float y = cursor.Number();
            if (relative) {
                x += cx;
                y += cy;
            }
            AddMove(commands, x, y);
            cx = x;
            cy = y;
            sx = x;
            sy = y;
            hasControl = false;
            command = relative ? 'l' : 'L';
        } else if (kind == 'L') {
            float x = cursor.Number();
            float y = cursor.Number();
            if (relative) {
                x += cx;
                y += cy;
            }
            AddLine(commands, x, y);
            cx = x;
            cy = y;
            hasControl = false;
        } else if (kind == 'H') {
            float x = cursor.Number();
            if (relative) {
                x += cx;
            }
            AddLine(commands, x, cy);
            cx = x;
            hasControl = false;
        } else if (kind == 'V') {
            float y = cursor.Number();
            if (relative) {
                y += cy;
            }
            AddLine(commands, cx, y);
            cy = y;
            hasControl = false;
        } else if (kind == 'C') {
            float c1x = cursor.Number();
            float c1y = cursor.Number();
            float c2x = cursor.Number();
            float c2y = cursor.Number();
            float x = cursor.Number();
            float y = cursor.Number();
            if (relative) {
                c1x += cx;
                c1y += cy;
                c2x += cx;
                c2y += cy;
                x += cx;
                y += cy;
            }
            AddCubic(commands, c1x, c1y, c2x, c2y, x, y);
            lastCx = c2x;
            lastCy = c2y;
            hasControl = true;
            cx = x;
            cy = y;
        } else if (kind == 'S') {
            float c2x = cursor.Number();
            float c2y = cursor.Number();
            float x = cursor.Number();
            float y = cursor.Number();
            if (relative) {
                c2x += cx;
                c2y += cy;
                x += cx;
                y += cy;
            }
            const float c1x = hasControl ? 2.0f * cx - lastCx : cx;
            const float c1y = hasControl ? 2.0f * cy - lastCy : cy;
            AddCubic(commands, c1x, c1y, c2x, c2y, x, y);
            lastCx = c2x;
            lastCy = c2y;
            hasControl = true;
            cx = x;
            cy = y;
        } else if (kind == 'Q') {
            float qx = cursor.Number();
            float qy = cursor.Number();
            float x = cursor.Number();
            float y = cursor.Number();
            if (relative) {
                qx += cx;
                qy += cy;
                x += cx;
                y += cy;
            }
            AddCubic(commands, cx + 2.0f * (qx - cx) / 3.0f, cy + 2.0f * (qy - cy) / 3.0f, x + 2.0f * (qx - x) / 3.0f,
                     y + 2.0f * (qy - y) / 3.0f, x, y);
            lastCx = qx;
            lastCy = qy;
            hasControl = true;
            cx = x;
            cy = y;
        } else if (kind == 'T') {
            float x = cursor.Number();
            float y = cursor.Number();
            if (relative) {
                x += cx;
                y += cy;
            }
            const float qx = hasControl ? 2.0f * cx - lastCx : cx;
            const float qy = hasControl ? 2.0f * cy - lastCy : cy;
            AddCubic(commands, cx + 2.0f * (qx - cx) / 3.0f, cy + 2.0f * (qy - cy) / 3.0f, x + 2.0f * (qx - x) / 3.0f,
                     y + 2.0f * (qy - y) / 3.0f, x, y);
            lastCx = qx;
            lastCy = qy;
            hasControl = true;
            cx = x;
            cy = y;
        } else if (kind == 'A') {
            const float rx = cursor.Number();
            const float ry = cursor.Number();
            const float angle = cursor.Number();
            const float large = cursor.Number();
            const float sweep = cursor.Number();
            float x = cursor.Number();
            float y = cursor.Number();
            if (relative) {
                x += cx;
                y += cy;
            }
            AddArc(commands, cx, cy, rx, ry, angle, large != 0.0f, sweep != 0.0f, x, y);
            cx = x;
            cy = y;
            hasControl = false;
        } else if (kind == 'Z') {
            AddClose(commands);
            cx = sx;
            cy = sy;
            hasControl = false;
        } else {
            break;
        }
    }
}

void TransformCommands(std::vector<CompiledCommand>& commands, const Affine& matrix) {
    for (CompiledCommand& command : commands) {
        if (command.kind == 3) {
            continue;
        }
        if (command.kind == 2) {
            MapPoint(matrix, command.c1x, command.c1y);
            MapPoint(matrix, command.c2x, command.c2y);
        }
        MapPoint(matrix, command.x, command.y);
    }
}

void AddEllipse(std::vector<CompiledCommand>& commands, float cx, float cy, float rx, float ry) {
    const float kx = rx * 0.55228475f;
    const float ky = ry * 0.55228475f;
    AddMove(commands, cx + rx, cy);
    AddCubic(commands, cx + rx, cy + ky, cx + kx, cy + ry, cx, cy + ry);
    AddCubic(commands, cx - kx, cy + ry, cx - rx, cy + ky, cx - rx, cy);
    AddCubic(commands, cx - rx, cy - ky, cx - kx, cy - ry, cx, cy - ry);
    AddCubic(commands, cx + kx, cy - ry, cx + rx, cy - ky, cx + rx, cy);
    AddClose(commands);
}

void AddRect(std::vector<CompiledCommand>& commands, float x, float y, float width, float height, float rx, float ry) {
    if (rx <= 0.0f && ry <= 0.0f) {
        AddMove(commands, x, y);
        AddLine(commands, x + width, y);
        AddLine(commands, x + width, y + height);
        AddLine(commands, x, y + height);
        AddClose(commands);
        return;
    }
    rx = std::min(rx, width * 0.5f);
    ry = std::min(ry <= 0.0f ? rx : ry, height * 0.5f);
    if (rx <= 0.0f) {
        rx = ry;
    }
    const float kx = rx * 0.55228475f;
    const float ky = ry * 0.55228475f;
    AddMove(commands, x + rx, y);
    AddLine(commands, x + width - rx, y);
    AddCubic(commands, x + width - rx + kx, y, x + width, y + ry - ky, x + width, y + ry);
    AddLine(commands, x + width, y + height - ry);
    AddCubic(commands, x + width, y + height - ry + ky, x + width - rx + kx, y + height, x + width - rx, y + height);
    AddLine(commands, x + rx, y + height);
    AddCubic(commands, x + rx - kx, y + height, x, y + height - ry + ky, x, y + height - ry);
    AddLine(commands, x, y + ry);
    AddCubic(commands, x, y + ry - ky, x + rx - kx, y, x + rx, y);
    AddClose(commands);
}

std::vector<float> ParseNumbers(const std::string& text) {
    PathCursor cursor(text);
    std::vector<float> numbers;
    while (cursor.NumberReady()) {
        numbers.push_back(cursor.Number());
    }
    return numbers;
}

bool IsNone(const std::string& value) {
    return value == "none" || value == "transparent";
}

bool IsCurrent(const std::string& value) {
    return value == "currentColor" || value == "currentcolor";
}

void ApplyPaint(const XmlNode& node, Paint& paint, bool defaultCurrentFill) {
    const std::string fill = Attribute(node, "fill");
    const std::string stroke = Attribute(node, "stroke");
    if (!fill.empty()) {
        if (IsNone(fill)) {
            paint.hasFill = false;
            paint.currentFill = false;
            paint.fillToken.clear();
        } else if (IsCurrent(fill)) {
            paint.hasFill = true;
            paint.currentFill = true;
            paint.fillToken.clear();
        } else if (fill.size() >= 2 && fill.front() == '{' && fill.back() == '}') {
            const std::string token = fill.substr(1, fill.size() - 2);
            if (!Foundation::IsKnownToken(token)) {
                throw std::runtime_error("Unknown foundation token {" + token + "}");
            }
            paint.hasFill = true;
            paint.currentFill = false;
            paint.fillToken = token;
        } else {
            if (!defaultCurrentFill) {
                std::cerr << "warning: custom value \"" << fill << "\" used for fill on <" << node.name
                          << ">; use a RoboFoundation token\n";
            }
            paint.hasFill = true;
            paint.currentFill = false;
            paint.fillHex = fill;
            paint.fillToken.clear();
        }
    } else if (defaultCurrentFill && paint.hasFill && paint.fillHex == "#000000" && !paint.currentFill) {
        paint.currentFill = true;
    }
    if (!stroke.empty()) {
        if (IsNone(stroke)) {
            paint.hasStroke = false;
            paint.currentStroke = false;
        } else if (IsCurrent(stroke)) {
            paint.hasStroke = true;
            paint.currentStroke = true;
            paint.strokeToken.clear();
        } else if (stroke.size() >= 2 && stroke.front() == '{' && stroke.back() == '}') {
            const std::string token = stroke.substr(1, stroke.size() - 2);
            if (!Foundation::IsKnownToken(token)) {
                throw std::runtime_error("Unknown foundation token {" + token + "}");
            }
            paint.hasStroke = true;
            paint.currentStroke = false;
            paint.strokeToken = token;
        } else {
            if (!defaultCurrentFill) {
                std::cerr << "warning: custom value \"" << stroke << "\" used for stroke on <" << node.name
                          << ">; use a RoboFoundation token\n";
            }
            paint.hasStroke = true;
            paint.currentStroke = false;
            paint.strokeHex = stroke;
            paint.strokeToken.clear();
        }
    }
    const std::string width = Attribute(node, "stroke-width", Attribute(node, "strokeWidth"));
    if (!width.empty()) {
        paint.strokeWidth = NumberOf(width, paint.strokeWidth);
    }
    const std::string opacity = Attribute(node, "opacity");
    if (!opacity.empty()) {
        paint.opacity *= NumberOf(opacity, 1.0f);
    }
    const std::string rule = Attribute(node, "fill-rule", Attribute(node, "fillRule"));
    if (rule == "evenodd") {
        paint.evenOdd = true;
    }
}

Affine ParseTransform(const std::string& source) {
    Affine result;
    std::size_t index = 0;
    auto skip = [&]() {
        while (index < source.size() && (source[index] == ' ' || source[index] == ',' || source[index] == '\t' || source[index] == '\n' || source[index] == '\r')) {
            ++index;
        }
    };
    while (index < source.size()) {
        skip();
        if (index >= source.size()) {
            break;
        }
        const std::size_t start = index;
        while (index < source.size() && ((source[index] >= 'A' && source[index] <= 'Z') || (source[index] >= 'a' && source[index] <= 'z'))) {
            ++index;
        }
        const std::string name = source.substr(start, index - start);
        skip();
        if (index < source.size() && source[index] == '(') {
            ++index;
        }
        std::vector<float> numbers;
        while (index < source.size() && source[index] != ')') {
            skip();
            if (index < source.size() && source[index] == ')') {
                break;
            }
            const std::size_t numberStart = index;
            if (index < source.size() && (source[index] == '+' || source[index] == '-')) {
                ++index;
            }
            while (index < source.size() && ((source[index] >= '0' && source[index] <= '9') || source[index] == '.' || source[index] == 'e' || source[index] == 'E' || source[index] == '+' || source[index] == '-')) {
                if ((source[index] == '+' || source[index] == '-') && index > numberStart && source[index - 1] != 'e' && source[index - 1] != 'E') {
                    break;
                }
                ++index;
            }
            if (index == numberStart) {
                break;
            }
            numbers.push_back(static_cast<float>(std::atof(source.c_str() + numberStart)));
            skip();
        }
        if (index < source.size() && source[index] == ')') {
            ++index;
        }
        Affine local;
        if (name == "translate") {
            local.e = numbers.empty() ? 0.0f : numbers[0];
            local.f = numbers.size() > 1 ? numbers[1] : 0.0f;
        } else if (name == "scale") {
            local.a = numbers.empty() ? 1.0f : numbers[0];
            local.d = numbers.size() > 1 ? numbers[1] : local.a;
        } else if (name == "rotate") {
            const float degrees = numbers.empty() ? 0.0f : numbers[0];
            const float radians = degrees * 3.14159265f / 180.0f;
            const float cosR = std::cos(radians);
            const float sinR = std::sin(radians);
            const float cx = numbers.size() > 1 ? numbers[1] : 0.0f;
            const float cy = numbers.size() > 2 ? numbers[2] : 0.0f;
            Affine toOrigin;
            toOrigin.e = -cx;
            toOrigin.f = -cy;
            Affine rotation;
            rotation.a = cosR;
            rotation.b = sinR;
            rotation.c = -sinR;
            rotation.d = cosR;
            Affine back;
            back.e = cx;
            back.f = cy;
            local = Multiply(back, Multiply(rotation, toOrigin));
        } else if (name == "matrix" && numbers.size() >= 6) {
            local.a = numbers[0];
            local.b = numbers[1];
            local.c = numbers[2];
            local.d = numbers[3];
            local.e = numbers[4];
            local.f = numbers[5];
        }
        result = Multiply(result, local);
    }
    return result;
}

void CompileNode(const XmlNode& node, const Affine& matrix, const Paint& inherited, bool defaultCurrentFill, std::vector<CompiledShape>& shapes);

void EmitLeaf(std::vector<CompiledCommand> commands, const Affine& matrix, const Paint& paint, std::vector<CompiledShape>& shapes) {
    if (commands.empty()) {
        return;
    }
    TransformCommands(commands, matrix);
    CompiledShape shape;
    shape.commands = std::move(commands);
    shape.hasFill = paint.hasFill;
    shape.hasStroke = paint.hasStroke;
    shape.currentFill = paint.currentFill;
    shape.currentStroke = paint.currentStroke;
    shape.fillHex = paint.fillHex;
    shape.strokeHex = paint.strokeHex;
    shape.fillToken = paint.fillToken;
    shape.strokeToken = paint.strokeToken;
    shape.strokeWidth = paint.strokeWidth;
    shape.opacity = paint.opacity;
    shape.evenOdd = paint.evenOdd;
    shapes.push_back(std::move(shape));
}

void CompileNode(const XmlNode& node, const Affine& matrix, const Paint& inherited, bool defaultCurrentFill, std::vector<CompiledShape>& shapes) {
    if (node.isText) {
        return;
    }
    Paint paint = inherited;
    ApplyPaint(node, paint, defaultCurrentFill);
    Affine local = matrix;
    const std::string transform = Attribute(node, "transform");
    if (!transform.empty()) {
        local = Multiply(matrix, ParseTransform(transform));
    }
    if (EqualName(node.name, "g") || EqualName(node.name, "svg")) {
        for (const XmlNode& child : node.children) {
            CompileNode(child, local, paint, defaultCurrentFill, shapes);
        }
        return;
    }
    std::vector<CompiledCommand> commands;
    if (EqualName(node.name, "path")) {
        ParsePathData(Attribute(node, "d"), commands);
    } else if (EqualName(node.name, "rect")) {
        const float x = NumberOf(Attribute(node, "x"), 0.0f);
        const float y = NumberOf(Attribute(node, "y"), 0.0f);
        const float width = NumberOf(Attribute(node, "width"), 0.0f);
        const float height = NumberOf(Attribute(node, "height"), 0.0f);
        float rx = NumberOf(Attribute(node, "rx"), 0.0f);
        float ry = NumberOf(Attribute(node, "ry"), 0.0f);
        AddRect(commands, x, y, width, height, rx, ry);
    } else if (EqualName(node.name, "circle")) {
        AddEllipse(commands, NumberOf(Attribute(node, "cx"), 0.0f), NumberOf(Attribute(node, "cy"), 0.0f),
                   NumberOf(Attribute(node, "r"), 0.0f), NumberOf(Attribute(node, "r"), 0.0f));
    } else if (EqualName(node.name, "ellipse")) {
        AddEllipse(commands, NumberOf(Attribute(node, "cx"), 0.0f), NumberOf(Attribute(node, "cy"), 0.0f),
                   NumberOf(Attribute(node, "rx"), 0.0f), NumberOf(Attribute(node, "ry"), 0.0f));
    } else if (EqualName(node.name, "line")) {
        AddMove(commands, NumberOf(Attribute(node, "x1"), 0.0f), NumberOf(Attribute(node, "y1"), 0.0f));
        AddLine(commands, NumberOf(Attribute(node, "x2"), 0.0f), NumberOf(Attribute(node, "y2"), 0.0f));
        paint.hasFill = false;
        if (!paint.hasStroke) {
            paint.hasStroke = true;
            paint.currentStroke = paint.currentFill || defaultCurrentFill;
        }
    } else if (EqualName(node.name, "polyline") || EqualName(node.name, "polygon")) {
        const std::vector<float> numbers = ParseNumbers(Attribute(node, "points"));
        for (std::size_t i = 0; i + 1 < numbers.size(); i += 2) {
            if (i == 0) {
                AddMove(commands, numbers[i], numbers[i + 1]);
            } else {
                AddLine(commands, numbers[i], numbers[i + 1]);
            }
        }
        if (EqualName(node.name, "polygon")) {
            AddClose(commands);
        } else {
            paint.hasFill = false;
        }
    } else {
        return;
    }
    EmitLeaf(std::move(commands), local, paint, shapes);
}

std::string FloatCpp(float value) {
    std::ostringstream stream;
    stream.setf(std::ios::fmtflags(0), std::ios::floatfield);
    stream.precision(6);
    stream << value;
    std::string text = stream.str();
    if (text.find('.') == std::string::npos && text.find('e') == std::string::npos && text.find('E') == std::string::npos) {
        text += ".0";
    }
    return text + "f";
}

std::string Indent(int level) {
    return std::string(static_cast<std::size_t>(level) * 4, ' ');
}

}  // namespace

std::vector<CompiledCommand> CompilePathData(const std::string& data) {
    std::vector<CompiledCommand> commands;
    ParsePathData(data, commands);
    return commands;
}

CompiledSvg CompileSvgElement(const XmlNode& svgNode, bool defaultCurrentFill) {
    CompiledSvg graphic;
    const std::string viewBox = Attribute(svgNode, "viewBox");
    if (!viewBox.empty()) {
        const std::vector<float> numbers = ParseNumbers(viewBox);
        if (numbers.size() >= 4) {
            graphic.viewX = numbers[0];
            graphic.viewY = numbers[1];
            graphic.viewWidth = numbers[2];
            graphic.viewHeight = numbers[3];
            graphic.hasViewBox = true;
        }
    }
    Paint paint;
    if (defaultCurrentFill) {
        paint.currentFill = true;
    }
    ApplyPaint(svgNode, paint, defaultCurrentFill);
    for (const XmlNode& child : svgNode.children) {
        CompileNode(child, Affine{}, paint, defaultCurrentFill, graphic.shapes);
    }
    return graphic;
}

std::string EmitCompiledShapes(const std::vector<CompiledShape>& shapes, int indent, const std::string& target, bool packed) {
    std::ostringstream code;
    const std::string add = packed ? "AddPackedShape" : "AddShape";
    int index = 0;
    for (const CompiledShape& shape : shapes) {
        const std::string name = "shape" + std::to_string(index++);
        code << Indent(indent) << "{\n";
        code << Indent(indent + 1) << "Robo::SvgShape " << name << ";\n";
        code << Indent(indent + 1) << name << ".hasFill = " << (shape.hasFill ? "true" : "false") << ";\n";
        code << Indent(indent + 1) << name << ".hasStroke = " << (shape.hasStroke ? "true" : "false") << ";\n";
        code << Indent(indent + 1) << name << ".currentFill = " << (shape.currentFill ? "true" : "false") << ";\n";
        code << Indent(indent + 1) << name << ".currentStroke = " << (shape.currentStroke ? "true" : "false") << ";\n";
        if (!shape.fillToken.empty()) {
            code << Indent(indent + 1) << name << ".fillToken = \"" << shape.fillToken << "\";\n";
        } else if (!shape.currentFill) {
            code << Indent(indent + 1) << name << ".fill = Robo::Color::FromHex(\"" << shape.fillHex << "\");\n";
        }
        if (!shape.strokeToken.empty()) {
            code << Indent(indent + 1) << name << ".strokeToken = \"" << shape.strokeToken << "\";\n";
        } else if (!shape.currentStroke) {
            code << Indent(indent + 1) << name << ".stroke = Robo::Color::FromHex(\"" << shape.strokeHex << "\");\n";
        }
        code << Indent(indent + 1) << name << ".strokeWidth = " << FloatCpp(shape.strokeWidth) << ";\n";
        code << Indent(indent + 1) << name << ".opacity = " << FloatCpp(shape.opacity) << ";\n";
        code << Indent(indent + 1) << name << ".fillRule = "
             << (shape.evenOdd ? "Robo::SvgFillRule::EvenOdd" : "Robo::SvgFillRule::NonZero") << ";\n";
        for (const CompiledCommand& command : shape.commands) {
            if (command.kind == 0) {
                code << Indent(indent + 1) << name << ".commands.push_back(Robo::SvgCommand::Move(" << FloatCpp(command.x)
                     << ", " << FloatCpp(command.y) << "));\n";
            } else if (command.kind == 1) {
                code << Indent(indent + 1) << name << ".commands.push_back(Robo::SvgCommand::Line(" << FloatCpp(command.x)
                     << ", " << FloatCpp(command.y) << "));\n";
            } else if (command.kind == 2) {
                code << Indent(indent + 1) << name << ".commands.push_back(Robo::SvgCommand::Cubic(" << FloatCpp(command.c1x)
                     << ", " << FloatCpp(command.c1y) << ", " << FloatCpp(command.c2x) << ", " << FloatCpp(command.c2y)
                     << ", " << FloatCpp(command.x) << ", " << FloatCpp(command.y) << "));\n";
            } else {
                code << Indent(indent + 1) << name << ".commands.push_back(Robo::SvgCommand::Close());\n";
            }
        }
        code << Indent(indent + 1) << target << "." << add << "(std::move(" << name << "));\n";
        code << Indent(indent) << "}\n";
    }
    return code.str();
}

}  // namespace Robo
