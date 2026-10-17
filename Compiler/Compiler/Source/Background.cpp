#include "Robo/Background.hpp"

#include "Robo/IPlatform.hpp"

#include <cstdlib>
#include <cstring>
#ifdef ROBO_KERNEL
#include "RoboVideo.hpp"
#endif

namespace Robo {

bool RoboDecodePng(const std::uint8_t* bytes, std::size_t size, std::vector<std::uint8_t>& pixels, int& width, int& height);
bool RoboDecodeJpeg(const std::uint8_t* bytes, std::size_t size, std::vector<std::uint8_t>& pixels, int& width, int& height);

namespace {

constexpr int ROBO_BACKGROUND_BYTE_LIMIT = 24 * 1024 * 1024;
constexpr int ROBO_BACKGROUND_FRAME_LIMIT = 4 * 1024 * 1024;

int ReadU16(const std::uint8_t* bytes) {
    return static_cast<int>(bytes[0]) | (static_cast<int>(bytes[1]) << 8);
}

int ReadU32(const std::uint8_t* bytes) {
    return static_cast<int>(bytes[0]) | (static_cast<int>(bytes[1]) << 8) | (static_cast<int>(bytes[2]) << 16) |
           (static_cast<int>(bytes[3]) << 24);
}

std::string LowerCopy(const std::string& text) {
    std::string lower = text;
    for (char& ch : lower) {
        if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>(ch - 'A' + 'a');
        }
    }
    return lower;
}

std::string ExtensionOf(const std::string& path) {
    std::size_t slash = std::string::npos;
    for (std::size_t index = 0; index < path.size(); ++index) {
        if (path[index] == '/' || path[index] == '\\') {
            slash = index;
        }
    }
    const std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
    std::size_t dot = std::string::npos;
    for (std::size_t index = 0; index < name.size(); ++index) {
        if (name[index] == '.') {
            dot = index;
        }
    }
    if (dot == std::string::npos) {
        return {};
    }
    return LowerCopy(name.substr(dot));
}

std::string KindPrefix(const std::string& path, std::string& rest) {
    const std::string lower = LowerCopy(path);
    const char* names[] = {"color:", "picture:", "svg:", "video:", "gif:"};
    for (const char* name : names) {
        const std::size_t length = strlen(name);
        if (lower.compare(0, length, name) == 0) {
            rest = path.substr(length);
            const std::size_t first = rest.find_first_not_of(" \t");
            if (first != std::string::npos) {
                rest.erase(0, first);
            }
            return std::string(name, length - 1);
        }
    }
    rest = path;
    return {};
}

bool LooksLikeColor(const std::string& text) {
    if (text.empty()) {
        return false;
    }
    if (text.front() == '{' && text.back() == '}') {
        return true;
    }
    if (text.front() != '#') {
        return false;
    }
    if (text.size() != 4 && text.size() != 7 && text.size() != 9) {
        return false;
    }
    for (std::size_t index = 1; index < text.size(); ++index) {
        const char ch = text[index];
        const bool digit = (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
        if (!digit) {
            return false;
        }
    }
    return true;
}

void PutPixel(std::vector<std::uint8_t>& canvas, int width, int x, int y, std::uint8_t red, std::uint8_t green, std::uint8_t blue, std::uint8_t alpha) {
    if (x < 0 || y < 0 || x >= width) {
        return;
    }
    const std::size_t index = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4u;
    if (index + 3 >= canvas.size()) {
        return;
    }
    if (alpha == 0) {
        return;
    }
    canvas[index] = red;
    canvas[index + 1] = green;
    canvas[index + 2] = blue;
    canvas[index + 3] = alpha;
}

bool DecodeBmp(const std::uint8_t* bytes, std::size_t size, std::vector<std::uint8_t>& pixels, int& width, int& height) {
    if (size < 54 || bytes[0] != 'B' || bytes[1] != 'M') {
        return false;
    }
    const int offset = ReadU32(bytes + 10);
    const int header = ReadU32(bytes + 14);
    if (header < 40 || offset < 0 || static_cast<std::size_t>(offset) >= size) {
        return false;
    }
    width = ReadU32(bytes + 18);
    int storedHeight = ReadU32(bytes + 22);
    const bool topDown = storedHeight < 0;
    height = topDown ? -storedHeight : storedHeight;
    const int planes = ReadU16(bytes + 26);
    const int bpp = ReadU16(bytes + 28);
    const int compression = ReadU32(bytes + 30);
    if (planes != 1 || compression != 0 || width <= 0 || height <= 0 || width > 8192 || height > 8192) {
        return false;
    }
    if (bpp != 24 && bpp != 32) {
        return false;
    }
    const int rowStride = ((width * bpp + 31) / 32) * 4;
    if (static_cast<std::size_t>(offset) + static_cast<std::size_t>(rowStride) * static_cast<std::size_t>(height) > size) {
        return false;
    }
    pixels.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u, 255);
    for (int y = 0; y < height; ++y) {
        const int sourceY = topDown ? y : (height - 1 - y);
        const std::uint8_t* row = bytes + offset + static_cast<std::size_t>(sourceY) * static_cast<std::size_t>(rowStride);
        for (int x = 0; x < width; ++x) {
            const std::uint8_t* pixel = row + x * (bpp / 8);
            const std::size_t dest = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4u;
            pixels[dest] = pixel[2];
            pixels[dest + 1] = pixel[1];
            pixels[dest + 2] = pixel[0];
            pixels[dest + 3] = bpp == 32 ? pixel[3] : 255;
        }
    }
    return true;
}

bool DecodeTga(const std::uint8_t* bytes, std::size_t size, std::vector<std::uint8_t>& pixels, int& width, int& height) {
    if (size < 18) {
        return false;
    }
    const int imageType = bytes[2];
    const int colorMap = bytes[1];
    if (colorMap != 0 || (imageType != 2 && imageType != 10)) {
        return false;
    }
    width = ReadU16(bytes + 12);
    height = ReadU16(bytes + 14);
    const int bpp = bytes[16];
    const int descriptor = bytes[17];
    if ((bpp != 24 && bpp != 32) || width <= 0 || height <= 0 || width > 8192 || height > 8192) {
        return false;
    }
    std::size_t cursor = 18u + bytes[0];
    const int channels = bpp / 8;
    const bool top = (descriptor & 0x20) != 0;
    pixels.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u, 255);
    auto write = [&](int x, int y, const std::uint8_t* pixel) {
        const int destY = top ? y : (height - 1 - y);
        const std::size_t dest = (static_cast<std::size_t>(destY) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4u;
        pixels[dest] = pixel[2];
        pixels[dest + 1] = pixel[1];
        pixels[dest + 2] = pixel[0];
        pixels[dest + 3] = channels == 4 ? pixel[3] : 255;
    };
    if (imageType == 2) {
        if (cursor + static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * static_cast<std::size_t>(channels) > size) {
            return false;
        }
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                write(x, y, bytes + cursor);
                cursor += static_cast<std::size_t>(channels);
            }
        }
        return true;
    }
    int x = 0;
    int y = 0;
    while (y < height && cursor < size) {
        const int packet = bytes[cursor++];
        const int count = (packet & 0x7F) + 1;
        if ((packet & 0x80) != 0) {
            if (cursor + static_cast<std::size_t>(channels) > size) {
                return false;
            }
            const std::uint8_t* pixel = bytes + cursor;
            cursor += static_cast<std::size_t>(channels);
            for (int index = 0; index < count && y < height; ++index) {
                write(x, y, pixel);
                x += 1;
                if (x >= width) {
                    x = 0;
                    y += 1;
                }
            }
        } else {
            for (int index = 0; index < count && y < height; ++index) {
                if (cursor + static_cast<std::size_t>(channels) > size) {
                    return false;
                }
                write(x, y, bytes + cursor);
                cursor += static_cast<std::size_t>(channels);
                x += 1;
                if (x >= width) {
                    x = 0;
                    y += 1;
                }
            }
        }
    }
    return y >= height;
}

struct GifReader {
    const std::uint8_t* bytes = nullptr;
    std::size_t size = 0;
    std::size_t cursor = 0;

    bool Take(std::uint8_t& value) {
        if (cursor >= size) {
            return false;
        }
        value = bytes[cursor++];
        return true;
    }
};

bool DecodeGif(const std::uint8_t* bytes, std::size_t size, std::vector<std::vector<std::uint8_t>>& frames, std::vector<int>& delays, int& width, int& height) {
    if (size < 13 || (memcmp(bytes, "GIF87a", 6) != 0 && memcmp(bytes, "GIF89a", 6) != 0)) {
        return false;
    }
    width = ReadU16(bytes + 6);
    height = ReadU16(bytes + 8);
    if (width <= 0 || height <= 0 || width > 4096 || height > 4096) {
        return false;
    }
    const int packed = bytes[10];
    const int background = bytes[11];
    std::size_t cursor = 13;
    std::uint8_t global[256 * 3] = {};
    int globalCount = 0;
    if ((packed & 0x80) != 0) {
        globalCount = 1 << ((packed & 0x07) + 1);
        if (cursor + static_cast<std::size_t>(globalCount) * 3u > size) {
            return false;
        }
        memcpy(global, bytes + cursor, static_cast<std::size_t>(globalCount) * 3u);
        cursor += static_cast<std::size_t>(globalCount) * 3u;
    }
    std::vector<std::uint8_t> canvas(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u, 0);
    std::vector<std::uint8_t> previous = canvas;
    int delayMs = 100;
    int transparent = -1;
    int disposal = 0;
    std::size_t storedBytes = 0;
    while (cursor < size) {
        const std::uint8_t block = bytes[cursor++];
        if (block == 0x3B) {
            break;
        }
        if (block == 0x21) {
            if (cursor >= size) {
                return false;
            }
            const std::uint8_t label = bytes[cursor++];
            if (label == 0xF9 && cursor < size) {
                const int length = bytes[cursor++];
                if (cursor + static_cast<std::size_t>(length) > size) {
                    return false;
                }
                if (length >= 4) {
                    disposal = (bytes[cursor] >> 2) & 0x07;
                    delayMs = ReadU16(bytes + cursor + 1) * 10;
                    if (delayMs < 20) {
                        delayMs = 100;
                    }
                    if ((bytes[cursor] & 0x01) != 0) {
                        transparent = bytes[cursor + 3];
                    } else {
                        transparent = -1;
                    }
                }
                cursor += static_cast<std::size_t>(length);
            }
            while (cursor < size) {
                const int length = bytes[cursor++];
                if (length == 0) {
                    break;
                }
                if (cursor + static_cast<std::size_t>(length) > size) {
                    return false;
                }
                cursor += static_cast<std::size_t>(length);
            }
            continue;
        }
        if (block != 0x2C) {
            return false;
        }
        if (cursor + 9 > size) {
            return false;
        }
        const int left = ReadU16(bytes + cursor);
        const int top = ReadU16(bytes + cursor + 2);
        const int frameWidth = ReadU16(bytes + cursor + 4);
        const int frameHeight = ReadU16(bytes + cursor + 6);
        const int imagePacked = bytes[cursor + 8];
        cursor += 9;
        std::uint8_t local[256 * 3];
        memcpy(local, global, sizeof(local));
        int colorCount = globalCount;
        if ((imagePacked & 0x80) != 0) {
            colorCount = 1 << ((imagePacked & 0x07) + 1);
            if (cursor + static_cast<std::size_t>(colorCount) * 3u > size) {
                return false;
            }
            memcpy(local, bytes + cursor, static_cast<std::size_t>(colorCount) * 3u);
            cursor += static_cast<std::size_t>(colorCount) * 3u;
        }
        if (cursor >= size) {
            return false;
        }
        const int minimum = bytes[cursor++];
        std::vector<std::uint8_t> compressed;
        while (cursor < size) {
            const int length = bytes[cursor++];
            if (length == 0) {
                break;
            }
            if (cursor + static_cast<std::size_t>(length) > size) {
                return false;
            }
            for (int index = 0; index < length; ++index) {
                compressed.push_back(bytes[cursor + static_cast<std::size_t>(index)]);
            }
            cursor += static_cast<std::size_t>(length);
        }
        if (minimum < 2 || minimum > 8 || frameWidth <= 0 || frameHeight <= 0) {
            return false;
        }
        previous = canvas;
        const int clearCode = 1 << minimum;
        const int endCode = clearCode + 1;
        int codeSize = minimum + 1;
        int nextCode = endCode + 1;
        std::vector<std::vector<std::uint8_t>> table(static_cast<std::size_t>(endCode + 1));
        for (int index = 0; index < clearCode; ++index) {
            table[static_cast<std::size_t>(index)] = {static_cast<std::uint8_t>(index)};
        }
        int bit = 0;
        auto readCode = [&](int& code) {
            if (bit + codeSize > static_cast<int>(compressed.size()) * 8) {
                return false;
            }
            int value = 0;
            for (int shift = 0; shift < codeSize; ++shift) {
                const int position = bit + shift;
                const int byte = compressed[static_cast<std::size_t>(position >> 3)];
                if ((byte & (1 << (position & 7))) != 0) {
                    value |= 1 << shift;
                }
            }
            bit += codeSize;
            code = value;
            return true;
        };
        std::vector<std::uint8_t> indices;
        indices.reserve(static_cast<std::size_t>(frameWidth) * static_cast<std::size_t>(frameHeight));
        int code = 0;
        if (!readCode(code) || code != clearCode) {
            return false;
        }
        std::vector<std::uint8_t> previousCode;
        while (readCode(code)) {
            if (code == clearCode) {
                codeSize = minimum + 1;
                nextCode = endCode + 1;
                table.resize(static_cast<std::size_t>(endCode + 1));
                previousCode.clear();
                continue;
            }
            if (code == endCode) {
                break;
            }
            std::vector<std::uint8_t> entry;
            if (code < nextCode && code < static_cast<int>(table.size())) {
                entry = table[static_cast<std::size_t>(code)];
            } else if (code == nextCode && !previousCode.empty()) {
                entry = previousCode;
                entry.push_back(previousCode.front());
            } else {
                return false;
            }
            for (std::uint8_t value : entry) {
                indices.push_back(value);
            }
            if (!previousCode.empty() && nextCode < 4096) {
                std::vector<std::uint8_t> added = previousCode;
                added.push_back(entry.front());
                if (nextCode == static_cast<int>(table.size())) {
                    table.push_back(added);
                }
                nextCode += 1;
                if (nextCode == (1 << codeSize) && codeSize < 12) {
                    codeSize += 1;
                }
            }
            previousCode = entry;
        }
        const bool interlace = (imagePacked & 0x40) != 0;
        int pass = 0;
        int passY = 0;
        const int start[] = {0, 4, 2, 1};
        const int step[] = {8, 8, 4, 2};
        for (std::size_t index = 0; index < indices.size(); ++index) {
            int localY = static_cast<int>(index) / frameWidth;
            const int localX = static_cast<int>(index) % frameWidth;
            if (interlace) {
                while (pass < 4 && passY >= frameHeight) {
                    pass += 1;
                    passY = pass < 4 ? start[pass] : frameHeight;
                }
                localY = passY;
                if ((static_cast<int>(index) + 1) % frameWidth == 0) {
                    passY += step[pass < 4 ? pass : 3];
                }
            }
            const int value = indices[index];
            if (value == transparent || value < 0 || value >= colorCount) {
                continue;
            }
            PutPixel(canvas, width, left + localX, top + localY, local[value * 3], local[value * 3 + 1], local[value * 3 + 2], 255);
        }
        if (storedBytes + canvas.size() <= static_cast<std::size_t>(ROBO_BACKGROUND_FRAME_LIMIT)) {
            frames.push_back(canvas);
            delays.push_back(delayMs);
            storedBytes += canvas.size();
        } else if (frames.empty()) {
            frames.push_back(canvas);
            delays.push_back(delayMs);
        }
        if (disposal == 2) {
            std::uint8_t red = 0;
            std::uint8_t green = 0;
            std::uint8_t blue = 0;
            if (background >= 0 && background < globalCount) {
                red = global[background * 3];
                green = global[background * 3 + 1];
                blue = global[background * 3 + 2];
            }
            for (int y = 0; y < frameHeight; ++y) {
                for (int x = 0; x < frameWidth; ++x) {
                    PutPixel(canvas, width, left + x, top + y, red, green, blue, 0);
                    const std::size_t pixel = (static_cast<std::size_t>(top + y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(left + x)) * 4u;
                    if (pixel + 3 < canvas.size()) {
                        canvas[pixel + 3] = 0;
                    }
                }
            }
        } else if (disposal == 3) {
            canvas = previous;
        }
        delayMs = 100;
        transparent = -1;
        disposal = 0;
    }
    return !frames.empty();
}

float NumberAt(const std::string& text, std::size_t& cursor) {
    while (cursor < text.size() && (text[cursor] == ' ' || text[cursor] == ',' || text[cursor] == '\n' || text[cursor] == '\r' || text[cursor] == '\t')) {
        cursor += 1;
    }
    const std::size_t start = cursor;
    if (cursor < text.size() && (text[cursor] == '+' || text[cursor] == '-')) {
        cursor += 1;
    }
    bool dot = false;
    bool digit = false;
    while (cursor < text.size()) {
        const char ch = text[cursor];
        if (ch >= '0' && ch <= '9') {
            digit = true;
            cursor += 1;
            continue;
        }
        if (ch == '.' && !dot) {
            dot = true;
            cursor += 1;
            continue;
        }
        break;
    }
    if (!digit) {
        cursor = start;
        return 0.0f;
    }
    return static_cast<float>(std::atof(text.substr(start, cursor - start).c_str()));
}

void AddCubic(SvgShape& shape, float c1x, float c1y, float c2x, float c2y, float x, float y) {
    shape.commands.push_back(SvgCommand::Cubic(c1x, c1y, c2x, c2y, x, y));
}

void AddPath(Svg& graphic, const std::string& data, const Color& fill, float opacity = 1.0f) {
    SvgShape shape;
    shape.fill = fill;
    shape.hasFill = true;
    shape.hasStroke = false;
    shape.opacity = opacity;
    float x = 0.0f;
    float y = 0.0f;
    float startX = 0.0f;
    float startY = 0.0f;
    float controlX = 0.0f;
    float controlY = 0.0f;
    char previous = 0;
    std::size_t cursor = 0;
    auto relative = [](char command) { return command >= 'a' && command <= 'z'; };
    while (cursor < data.size()) {
        while (cursor < data.size() && (data[cursor] == ' ' || data[cursor] == ',' || data[cursor] == '\n' || data[cursor] == '\r' || data[cursor] == '\t')) {
            cursor += 1;
        }
        if (cursor >= data.size()) {
            break;
        }
        char command = data[cursor];
        if ((command >= 'A' && command <= 'Z') || (command >= 'a' && command <= 'z')) {
            cursor += 1;
        } else {
            command = previous;
        }
        const bool rel = relative(command);
        const char absolute = rel ? static_cast<char>(command - 'a' + 'A') : command;
        if (absolute == 'M' || absolute == 'L' || absolute == 'T') {
            const float nx = NumberAt(data, cursor);
            const float ny = NumberAt(data, cursor);
            x = rel ? x + nx : nx;
            y = rel ? y + ny : ny;
            shape.commands.push_back(absolute == 'M' ? SvgCommand::Move(x, y) : SvgCommand::Line(x, y));
            if (absolute == 'M') {
                startX = x;
                startY = y;
            }
            controlX = x;
            controlY = y;
            previous = absolute == 'M' ? (rel ? 'l' : 'L') : command;
        } else if (absolute == 'H' || absolute == 'V') {
            const float value = NumberAt(data, cursor);
            if (absolute == 'H') {
                x = rel ? x + value : value;
            } else {
                y = rel ? y + value : value;
            }
            shape.commands.push_back(SvgCommand::Line(x, y));
            controlX = x;
            controlY = y;
            previous = command;
        } else if (absolute == 'C' || absolute == 'S' || absolute == 'Q') {
            float c1x = controlX;
            float c1y = controlY;
            float c2x = x;
            float c2y = y;
            float nx = x;
            float ny = y;
            if (absolute == 'C') {
                c1x = NumberAt(data, cursor);
                c1y = NumberAt(data, cursor);
                c2x = NumberAt(data, cursor);
                c2y = NumberAt(data, cursor);
                nx = NumberAt(data, cursor);
                ny = NumberAt(data, cursor);
                if (rel) {
                    c1x += x;
                    c1y += y;
                    c2x += x;
                    c2y += y;
                    nx += x;
                    ny += y;
                }
            } else if (absolute == 'S') {
                c1x = x + (x - controlX);
                c1y = y + (y - controlY);
                c2x = NumberAt(data, cursor);
                c2y = NumberAt(data, cursor);
                nx = NumberAt(data, cursor);
                ny = NumberAt(data, cursor);
                if (rel) {
                    c2x += x;
                    c2y += y;
                    nx += x;
                    ny += y;
                }
            } else {
                const float qx = NumberAt(data, cursor);
                const float qy = NumberAt(data, cursor);
                nx = NumberAt(data, cursor);
                ny = NumberAt(data, cursor);
                float ax = rel ? x + qx : qx;
                float ay = rel ? y + qy : qy;
                if (rel) {
                    nx += x;
                    ny += y;
                }
                c1x = x + (ax - x) * (2.0f / 3.0f);
                c1y = y + (ay - y) * (2.0f / 3.0f);
                c2x = nx + (ax - nx) * (2.0f / 3.0f);
                c2y = ny + (ay - ny) * (2.0f / 3.0f);
            }
            AddCubic(shape, c1x, c1y, c2x, c2y, nx, ny);
            controlX = c2x;
            controlY = c2y;
            x = nx;
            y = ny;
            previous = command;
        } else if (absolute == 'Z') {
            shape.commands.push_back(SvgCommand::Close());
            x = startX;
            y = startY;
            previous = command;
        } else {
            break;
        }
    }
    if (!shape.commands.empty()) {
        graphic.AddShape(std::move(shape));
    }
}

std::string Attribute(const std::string& tag, const char* name) {
    const std::string key = std::string(name) + "=\"";
    std::size_t found = 0;
    while (found < tag.size()) {
        found = tag.find(key, found);
        if (found == std::string::npos) {
            return {};
        }
        const bool boundary = found == 0 || tag[found - 1] == ' ' || tag[found - 1] == '\t' || tag[found - 1] == '\n' ||
                              tag[found - 1] == '\r';
        if (boundary) {
            break;
        }
        found += 1;
    }
    const std::size_t start = found + key.size();
    const std::size_t end = tag.find('"', start);
    if (end == std::string::npos) {
        return {};
    }
    return tag.substr(start, end - start);
}

struct GradientColor {
    std::string id;
    Color color;
};

Color AverageStops(const std::string& block) {
    int red = 0;
    int green = 0;
    int blue = 0;
    int count = 0;
    std::size_t at = 0;
    while (at < block.size()) {
        const std::size_t stop = block.find("stop-color=\"", at);
        if (stop == std::string::npos) {
            break;
        }
        const std::size_t start = stop + 12;
        const std::size_t end = block.find('"', start);
        if (end == std::string::npos) {
            break;
        }
        const Color color = Color::FromHex(block.substr(start, end - start));
        red += color.red;
        green += color.green;
        blue += color.blue;
        count += 1;
        at = end + 1;
    }
    if (count <= 0) {
        return Color::FromHex("#000000");
    }
    return Color::FromRgba(static_cast<std::uint8_t>(red / count), static_cast<std::uint8_t>(green / count),
                           static_cast<std::uint8_t>(blue / count));
}

void CollectGradients(const std::string& text, std::vector<GradientColor>& out) {
    std::size_t at = 0;
    while (at < text.size()) {
        const std::size_t found = text.find("<linearGradient", at);
        if (found == std::string::npos) {
            break;
        }
        const std::size_t tagEnd = text.find('>', found);
        const std::size_t close = text.find("</linearGradient>", found);
        if (tagEnd == std::string::npos || close == std::string::npos) {
            break;
        }
        const std::string id = Attribute(text.substr(found, tagEnd - found), "id");
        if (!id.empty()) {
            GradientColor entry;
            entry.id = id;
            entry.color = AverageStops(text.substr(found, close - found));
            out.push_back(std::move(entry));
        }
        at = close + 17;
    }
}

bool ResolvePaint(const std::string& fillText, const std::vector<GradientColor>& gradients, Color& color) {
    if (fillText == "none") {
        return false;
    }
    if (fillText == "white" || fillText == "White") {
        color = Color::FromHex("#FFFFFF");
        return true;
    }
    if (fillText.size() >= 6 && fillText.compare(0, 5, "url(#") == 0) {
        std::string id = fillText.substr(5);
        if (!id.empty() && id.back() == ')') {
            id.pop_back();
        }
        for (const GradientColor& entry : gradients) {
            if (entry.id == id) {
                color = entry.color;
                return true;
            }
        }
        return false;
    }
    color = fillText.empty() ? Color::FromHex("#000000") : Color::FromHex(fillText);
    return true;
}

bool DecodeSvg(const std::uint8_t* bytes, std::size_t size, Svg& graphic) {
    if (size < 5) {
        return false;
    }
    const std::string text(reinterpret_cast<const char*>(bytes), size);
    const std::string lower = LowerCopy(text);
    if (lower.find("<svg") == std::string::npos) {
        return false;
    }
    graphic = Svg();
    SvgStyle style;
    style.viewWidth = 24.0f;
    style.viewHeight = 24.0f;
    const std::size_t svgAt = lower.find("<svg");
    const std::size_t svgEnd = text.find('>', svgAt);
    if (svgEnd != std::string::npos) {
        const std::string tag = text.substr(svgAt, svgEnd - svgAt);
        const std::string viewBox = Attribute(tag, "viewBox");
        if (!viewBox.empty()) {
            std::size_t cursor = 0;
            style.viewX = NumberAt(viewBox, cursor);
            style.viewY = NumberAt(viewBox, cursor);
            style.viewWidth = NumberAt(viewBox, cursor);
            style.viewHeight = NumberAt(viewBox, cursor);
        }
    }
    if (style.viewWidth <= 0.0f) {
        style.viewWidth = 24.0f;
    }
    if (style.viewHeight <= 0.0f) {
        style.viewHeight = 24.0f;
    }
    graphic.SetStyle(style);
    std::vector<GradientColor> gradients;
    CollectGradients(text, gradients);
    std::size_t cursor = 0;
    while (cursor < lower.size()) {
        const std::size_t path = lower.find("<path", cursor);
        const std::size_t rect = lower.find("<rect", cursor);
        const std::size_t circle = lower.find("<circle", cursor);
        const std::size_t ellipse = lower.find("<ellipse", cursor);
        std::size_t next = std::string::npos;
        int which = 0;
        auto nearer = [&](std::size_t at, int kind) {
            if (at != std::string::npos && (next == std::string::npos || at < next)) {
                next = at;
                which = kind;
            }
        };
        nearer(path, 1);
        nearer(rect, 2);
        nearer(circle, 3);
        nearer(ellipse, 4);
        if (next == std::string::npos) {
            break;
        }
        const std::size_t tagEnd = text.find('>', next);
        if (tagEnd == std::string::npos) {
            break;
        }
        const std::string tag = text.substr(next, tagEnd - next);
        auto lastBefore = [&](const char* token) {
            const std::size_t count = strlen(token);
            if (count == 0 || lower.size() < count) {
                return std::string::npos;
            }
            std::size_t index = lower.size() - count;
            if (next < index) {
                index = next;
            }
            while (true) {
                if (memcmp(lower.data() + index, token, count) == 0) {
                    return index;
                }
                if (index == 0) {
                    break;
                }
                index -= 1;
            }
            return std::string::npos;
        };
        auto inside = [&](const char* openTag, const char* closeTag) {
            const std::size_t opened = lastBefore(openTag);
            if (opened == std::string::npos) {
                return false;
            }
            const std::size_t closed = lastBefore(closeTag);
            return closed == std::string::npos || closed < opened;
        };
        if (inside("<mask", "</mask>") || inside("<defs", "</defs>")) {
            cursor = tagEnd + 1;
            continue;
        }
        const std::string fillText = Attribute(tag, "fill");
        if (fillText.empty()) {
            const std::string styleText = Attribute(tag, "style");
            const bool blend = styleText.find("mix-blend-mode") != std::string::npos;
            const bool stroked = !Attribute(tag, "stroke").empty();
            if (blend || stroked) {
                cursor = tagEnd + 1;
                continue;
            }
        }
        Color fill;
        if (!ResolvePaint(fillText, gradients, fill)) {
            cursor = tagEnd + 1;
            continue;
        }
        float opacity = 1.0f;
        const std::string fade = Attribute(tag, "fill-opacity");
        if (!fade.empty()) {
            std::size_t fadeCursor = 0;
            opacity *= NumberAt(fade, fadeCursor);
        }
        const std::string whole = Attribute(tag, "opacity");
        if (!whole.empty()) {
            std::size_t wholeCursor = 0;
            opacity *= NumberAt(whole, wholeCursor);
        }
        if (opacity < 0.0f) {
            opacity = 0.0f;
        } else if (opacity > 1.0f) {
            opacity = 1.0f;
        }
        if (which == 1) {
            AddPath(graphic, Attribute(tag, "d"), fill, opacity);
        } else if (which == 2) {
            std::size_t unused = 0;
            const std::string xText = Attribute(tag, "x");
            const std::string yText = Attribute(tag, "y");
            const std::string wText = Attribute(tag, "width");
            const std::string hText = Attribute(tag, "height");
            unused = 0;
            const float x = NumberAt(xText, unused);
            unused = 0;
            const float y = NumberAt(yText, unused);
            unused = 0;
            const float w = NumberAt(wText, unused);
            unused = 0;
            const float h = NumberAt(hText, unused);
            unused = 0;
            float rx = NumberAt(Attribute(tag, "rx"), unused);
            unused = 0;
            float ry = NumberAt(Attribute(tag, "ry"), unused);
            if (rx <= 0.0f) {
                rx = ry;
            }
            if (ry <= 0.0f) {
                ry = rx;
            }
            if (rx > w * 0.5f) {
                rx = w * 0.5f;
            }
            if (ry > h * 0.5f) {
                ry = h * 0.5f;
            }
            std::string data;
            if (rx > 0.5f && ry > 0.5f) {
                const float k = 0.5522848f;
                auto n = [](float value) { return std::to_string(value); };
                data = "M " + n(x + rx) + " " + n(y);
                data += " H " + n(x + w - rx);
                data += " C " + n(x + w - rx + k * rx) + " " + n(y) + " " + n(x + w) + " " + n(y + ry - k * ry) + " " +
                        n(x + w) + " " + n(y + ry);
                data += " V " + n(y + h - ry);
                data += " C " + n(x + w) + " " + n(y + h - ry + k * ry) + " " + n(x + w - rx + k * rx) + " " + n(y + h) + " " +
                        n(x + w - rx) + " " + n(y + h);
                data += " H " + n(x + rx);
                data += " C " + n(x + rx - k * rx) + " " + n(y + h) + " " + n(x) + " " + n(y + h - ry + k * ry) + " " + n(x) +
                        " " + n(y + h - ry);
                data += " V " + n(y + ry);
                data += " C " + n(x) + " " + n(y + ry - k * ry) + " " + n(x + rx - k * rx) + " " + n(y) + " " + n(x + rx) + " " +
                        n(y) + " Z";
            } else {
                data = "M " + std::to_string(x) + " " + std::to_string(y) + " H " + std::to_string(x + w) + " V " +
                       std::to_string(y + h) + " H " + std::to_string(x) + " Z";
            }
            AddPath(graphic, data, fill, opacity);
        } else if (which == 3 || which == 4) {
            std::size_t unused = 0;
            const float cx = NumberAt(Attribute(tag, "cx"), unused);
            unused = 0;
            const float cy = NumberAt(Attribute(tag, "cy"), unused);
            unused = 0;
            const float rx = which == 3 ? NumberAt(Attribute(tag, "r"), unused) : NumberAt(Attribute(tag, "rx"), unused);
            unused = 0;
            const float ry = which == 3 ? rx : NumberAt(Attribute(tag, "ry"), unused);
            const float k = 0.552284f;
            std::string data = "M " + std::to_string(cx + rx) + " " + std::to_string(cy);
            data += " C " + std::to_string(cx + rx) + " " + std::to_string(cy + ry * k) + " " + std::to_string(cx + rx * k) + " " +
                    std::to_string(cy + ry) + " " + std::to_string(cx) + " " + std::to_string(cy + ry);
            data += " C " + std::to_string(cx - rx * k) + " " + std::to_string(cy + ry) + " " + std::to_string(cx - rx) + " " +
                    std::to_string(cy + ry * k) + " " + std::to_string(cx - rx) + " " + std::to_string(cy);
            data += " C " + std::to_string(cx - rx) + " " + std::to_string(cy - ry * k) + " " + std::to_string(cx - rx * k) + " " +
                    std::to_string(cy - ry) + " " + std::to_string(cx) + " " + std::to_string(cy - ry);
            data += " C " + std::to_string(cx + rx * k) + " " + std::to_string(cy - ry) + " " + std::to_string(cx + rx) + " " +
                    std::to_string(cy - ry * k) + " " + std::to_string(cx + rx) + " " + std::to_string(cy) + " Z";
            AddPath(graphic, data, fill, opacity);
        }
        cursor = tagEnd + 1;
    }
    return !graphic.GetShapes().empty();
}

}  // namespace

bool LoadSvgDocument(const std::uint8_t* bytes, std::size_t size, Svg& graphic) {
    return DecodeSvg(bytes, size, graphic);
}

void Background::SetColor(const Color& color, const std::string& token) {
    color_ = color;
    token_ = token;
    kind_ = BackgroundKind::Color;
    hasSource_ = true;
    pending_ = false;
    animated_ = false;
    hasGraphic_ = false;
    frames_.clear();
    video_.clear();
    status_.clear();
    path_.clear();
}

void Background::SetPath(const std::string& path) {
    path_ = path;
    pending_ = true;
    hasSource_ = true;
    status_.clear();
}

bool Background::HasSource() const {
    return hasSource_;
}

BackgroundKind Background::GetKind() const {
    return kind_;
}

const std::string& Background::GetPath() const {
    return path_;
}

const Color& Background::GetColor() const {
    return color_;
}

const std::string& Background::GetColorToken() const {
    return token_;
}

const std::string& Background::GetStatus() const {
    return status_;
}

bool Background::IsAnimated() const {
    return animated_;
}

bool Background::UseColor(const std::string& text) {
    if (text.size() >= 2 && text.front() == '{' && text.back() == '}') {
        token_ = text.substr(1, text.size() - 2);
        kind_ = BackgroundKind::Color;
        animated_ = false;
        hasGraphic_ = false;
        frames_.clear();
        video_.clear();
        videoJpeg_ = false;
        status_.clear();
        return true;
    }
    if (!LooksLikeColor(text)) {
        return false;
    }
    color_ = Color::FromHex(text);
    token_.clear();
    kind_ = BackgroundKind::Color;
    animated_ = false;
    hasGraphic_ = false;
    frames_.clear();
    video_.clear();
    videoJpeg_ = false;
    status_.clear();
    return true;
}

void Background::RememberFrame(std::vector<std::uint8_t> pixels, int delayMs) {
    frames_.push_back(std::move(pixels));
    delays_.push_back(delayMs);
}

bool Background::DecodePicture(const std::uint8_t* bytes, std::size_t size) {
    std::vector<std::uint8_t> pixels;
    int width = 0;
    int height = 0;
    if (!DecodeBmp(bytes, size, pixels, width, height) && !DecodeTga(bytes, size, pixels, width, height) &&
        !RoboDecodePng(bytes, size, pixels, width, height) && !RoboDecodeJpeg(bytes, size, pixels, width, height)) {
        return false;
    }
    frames_.clear();
    delays_.clear();
    video_.clear();
    videoJpeg_ = false;
    hasGraphic_ = false;
    width_ = width;
    height_ = height;
    RememberFrame(std::move(pixels), 0);
    frame_ = 0;
    kind_ = BackgroundKind::Picture;
    animated_ = false;
    status_.clear();
    return true;
}

bool Background::DecodeGif(const std::uint8_t* bytes, std::size_t size) {
    std::vector<std::vector<std::uint8_t>> frames;
    std::vector<int> delays;
    int width = 0;
    int height = 0;
    if (!Robo::DecodeGif(bytes, size, frames, delays, width, height)) {
        return false;
    }
    frames_ = std::move(frames);
    delays_ = std::move(delays);
    video_.clear();
    videoJpeg_ = false;
    hasGraphic_ = false;
    width_ = width;
    height_ = height;
    frame_ = 0;
    elapsedMs_ = 0.0;
    kind_ = BackgroundKind::Gif;
    animated_ = frames_.size() > 1;
    status_.clear();
    return true;
}

bool Background::DecodeSvg(const std::uint8_t* bytes, std::size_t size) {
    Svg graphic;
    if (!Robo::DecodeSvg(bytes, size, graphic)) {
        return false;
    }
    graphic_ = std::move(graphic);
    hasGraphic_ = true;
    frames_.clear();
    delays_.clear();
    video_.clear();
    videoJpeg_ = false;
    kind_ = BackgroundKind::Svg;
    animated_ = false;
    status_.clear();
    return true;
}

bool Background::ShowVideoFrame(int index) {
    if (!videoJpeg_) {
        return true;
    }
    if (index < 0 || index >= videoFrames_) {
        return false;
    }
    const std::size_t table = 20u + static_cast<std::size_t>(index) * 4u;
    if (table + 8u > video_.size()) {
        return false;
    }
    const int start = ReadU32(video_.data() + table);
    const int end = ReadU32(video_.data() + table + 4);
    if (start < 0 || end < start || static_cast<std::size_t>(end) > video_.size()) {
        return false;
    }
    int width = 0;
    int height = 0;
    if (frames_.empty()) {
        frames_.push_back({});
    }
#ifdef ROBO_KERNEL
    if (!frames_[0].empty() && RoboVideoCollect(index, frames_[0].data(), static_cast<uint32_t>(frames_[0].size()))) {
        RoboVideoRequest(index + 1);
        return true;
    }
    // No AP workers: keep the last frame. BSP JPEG every tick freezes input after Present.
    if (RoboVideoOnline() == 0u) {
        if (!frames_[0].empty()) {
            RoboVideoArm(video_.data(), static_cast<uint32_t>(video_.size()), videoFrames_, width_, height_);
            RoboVideoRequest(index + 1);
            return true;
        }
    }
#endif
    if (!RoboDecodeJpeg(video_.data() + start, static_cast<std::size_t>(end - start), frames_[0], width, height)) {
        return false;
    }
    width_ = width;
    height_ = height;
#ifdef ROBO_KERNEL
    RoboVideoArm(video_.data(), static_cast<uint32_t>(video_.size()), videoFrames_, width_, height_);
    RoboVideoRequest(index + 1);
#endif
    return true;
}

bool Background::DecodeVideo(std::vector<std::uint8_t>& bytes) {
    const std::uint8_t* data = bytes.data();
    const std::size_t size = bytes.size();
    if (size >= 24 && data != nullptr && memcmp(data, "RJPG", 4) == 0) {
        const int frameCount = ReadU32(data + 4);
        const int frameMs = ReadU32(data + 8);
        const int width = ReadU32(data + 12);
        const int height = ReadU32(data + 16);
        if (frameCount <= 0 || frameCount > 512 || width <= 0 || height <= 0 || width > 4096 || height > 4096) {
            return false;
        }
        const std::size_t tableBytes = static_cast<std::size_t>(frameCount + 1) * 4u;
        if (20u + tableBytes > size) {
            return false;
        }
        video_ = std::move(bytes);
        frames_.clear();
        delays_.clear();
        hasGraphic_ = false;
        videoJpeg_ = true;
        videoFrames_ = frameCount;
        videoDelayMs_ = frameMs > 0 ? frameMs : 100;
        width_ = width;
        height_ = height;
        frame_ = 0;
        elapsedMs_ = 0.0;
        if (!ShowVideoFrame(0)) {
            video_.clear();
            videoJpeg_ = false;
            return false;
        }
        kind_ = BackgroundKind::Video;
        animated_ = frameCount > 1;
        status_.clear();
        return true;
    }
    if (size < 20 || data == nullptr || memcmp(data, "RV1", 3) != 0) {
        return false;
    }
    const int width = ReadU32(data + 4);
    const int height = ReadU32(data + 8);
    const int frameCount = ReadU32(data + 12);
    const int frameMs = ReadU32(data + 16);
    if (width <= 0 || height <= 0 || frameCount <= 0 || width > 4096 || height > 4096) {
        return false;
    }
    const std::size_t frameBytes = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u;
    if (20u + frameBytes * static_cast<std::size_t>(frameCount) > size) {
        return false;
    }
    video_ = std::move(bytes);
    frames_.clear();
    delays_.clear();
    hasGraphic_ = false;
    videoJpeg_ = false;
    width_ = width;
    height_ = height;
    videoFrames_ = frameCount;
    videoDelayMs_ = frameMs > 0 ? frameMs : 100;
    frame_ = 0;
    elapsedMs_ = 0.0;
    kind_ = BackgroundKind::Video;
    animated_ = frameCount > 1;
    status_.clear();
    return true;
}

bool Background::ApplyPath(std::vector<std::uint8_t>& bytes, const std::string& kindName) {
    const std::uint8_t* data = bytes.data();
    const std::size_t size = bytes.size();
    if (kindName == "color") {
        return false;
    }
    if ((kindName.empty() || kindName == "gif") && DecodeGif(data, size)) {
        return true;
    }
    if ((kindName.empty() || kindName == "picture") && DecodePicture(data, size)) {
        return true;
    }
    if ((kindName.empty() || kindName == "svg") && DecodeSvg(data, size)) {
        return true;
    }
    if ((kindName.empty() || kindName == "video") && DecodeVideo(bytes)) {
        return true;
    }
    if (kindName == "gif") {
        status_ = "GIF could not be read";
    } else if (kindName == "svg") {
        status_ = "SVG could not be read";
    } else if (kindName == "video") {
        status_ = "Video must be RJPG or RV1 frames";
    } else {
        status_ = "Picture must be BMP, TGA, PNG, or JPEG";
    }
    return false;
}

bool Background::EnsureLoaded(const IPlatform& platform) {
    if (!pending_) {
        return hasSource_;
    }
    pending_ = false;
    std::string rest;
    const std::string kindName = KindPrefix(path_, rest);
    if (rest.empty()) {
        rest = path_;
    }
    if (kindName.empty() || kindName == "color") {
        if (UseColor(rest)) {
            return true;
        }
        if (kindName == "color") {
            status_ = "Color must be a hex value or a foundation token";
            return false;
        }
    }
    std::vector<std::uint8_t> bytes = platform.LoadAsset(rest);
    if (bytes.empty() || static_cast<int>(bytes.size()) > ROBO_BACKGROUND_BYTE_LIMIT) {
        status_ = "Background file was not found";
        return false;
    }
    std::string forced = kindName;
    if (forced.empty()) {
        const std::string extension = ExtensionOf(rest);
        if (extension == ".gif") {
            forced = "gif";
        } else if (extension == ".svg") {
            forced = "svg";
        } else if (extension == ".rv1" || extension == ".rjp" || extension == ".mp4" || extension == ".webm" || extension == ".avi" || extension == ".mov") {
            forced = "video";
        } else if (extension == ".bmp" || extension == ".tga" || extension == ".png" || extension == ".jpg" || extension == ".jpeg") {
            forced = "picture";
        }
    }
    if (ApplyPath(bytes, forced)) {
        return true;
    }
    if (forced.empty() && ApplyPath(bytes, {})) {
        return true;
    }
    return false;
}

bool Background::Advance(double deltaMs) {
    if (!animated_ || deltaMs <= 0.0) {
        return false;
    }
    elapsedMs_ += deltaMs;
    const bool video = kind_ == BackgroundKind::Video;
    const int delay = video ? videoDelayMs_ : (frame_ >= 0 && frame_ < static_cast<int>(delays_.size()) ? delays_[static_cast<std::size_t>(frame_)] : 100);
    if (delay <= 0 || elapsedMs_ < static_cast<double>(delay)) {
        return false;
    }
    const int count = video ? videoFrames_ : static_cast<int>(frames_.size());
    if (count <= 1) {
        return false;
    }
    int steps = 1;
    if (video) {
        steps = static_cast<int>(elapsedMs_ / static_cast<double>(delay));
        if (steps < 1) {
            steps = 1;
        }
    }
    elapsedMs_ -= static_cast<double>(steps) * static_cast<double>(delay);
    if (elapsedMs_ < 0.0) {
        elapsedMs_ = 0.0;
    }
    const int advance = steps % count;
    if (advance == 0) {
        return false;
    }
    frame_ = (frame_ + advance) % count;
    if (video && videoJpeg_ && !ShowVideoFrame(frame_)) {
        status_ = "Video frame could not be read";
        return false;
    }
    return true;
}

bool Background::PaintsMedia() const {
    return kind_ == BackgroundKind::Picture || kind_ == BackgroundKind::Svg || kind_ == BackgroundKind::Video || kind_ == BackgroundKind::Gif;
}

const std::uint8_t* Background::Pixels() const {
    if (kind_ == BackgroundKind::Video) {
        if (videoJpeg_) {
            if (frames_.empty() || frames_[0].empty()) {
                return nullptr;
            }
            return frames_[0].data();
        }
        if (video_.size() < 20 || frame_ < 0 || frame_ >= videoFrames_) {
            return nullptr;
        }
        const std::size_t frameBytes = static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_) * 4u;
        const std::size_t offset = 20u + frameBytes * static_cast<std::size_t>(frame_);
        if (offset + frameBytes > video_.size()) {
            return nullptr;
        }
        return video_.data() + offset;
    }
    if (frame_ < 0 || frame_ >= static_cast<int>(frames_.size()) || frames_[static_cast<std::size_t>(frame_)].empty()) {
        return nullptr;
    }
    return frames_[static_cast<std::size_t>(frame_)].data();
}

int Background::PixelWidth() const {
    return width_;
}

int Background::PixelHeight() const {
    return height_;
}

Svg* Background::Graphic() {
    return hasGraphic_ ? &graphic_ : nullptr;
}

}  // namespace Robo
