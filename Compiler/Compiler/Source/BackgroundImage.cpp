#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace Robo {

namespace {

int ReadU16(const std::uint8_t* bytes) {
    return static_cast<int>(bytes[0]) | (static_cast<int>(bytes[1]) << 8);
}

struct BitReader {
    const std::uint8_t* data = nullptr;
    std::size_t size = 0;
    std::size_t pos = 0;
    std::uint32_t acc = 0;
    int bits = 0;

    int Next() {
        if (bits == 0) {
            if (pos >= size) {
                return -1;
            }
            acc = data[pos++];
            bits = 8;
        }
        const int bit = static_cast<int>(acc & 1u);
        acc >>= 1;
        bits -= 1;
        return bit;
    }

    int Get(int count) {
        int value = 0;
        for (int index = 0; index < count; ++index) {
            const int bit = Next();
            if (bit < 0) {
                return -1;
            }
            value |= bit << index;
        }
        return value;
    }

    void Align() {
        acc = 0;
        bits = 0;
    }
};

struct InflateHuffman {
    int count[16] = {};
    int symbol[288] = {};
    int minCode[16] = {};
    int maxCode[16] = {};
    int index[16] = {};

    bool Build(const int* lengths, int total) {
        for (int length = 0; length < 16; ++length) {
            count[length] = 0;
        }
        for (int item = 0; item < total; ++item) {
            if (lengths[item] < 0 || lengths[item] > 15) {
                return false;
            }
            if (lengths[item] > 0) {
                count[lengths[item]] += 1;
            }
        }
        int code = 0;
        int cursor = 0;
        int next[16] = {};
        for (int length = 1; length <= 15; ++length) {
            index[length] = cursor;
            next[length] = cursor;
            minCode[length] = code;
            maxCode[length] = count[length] == 0 ? -1 : code + count[length] - 1;
            code = (code + count[length]) << 1;
            cursor += count[length];
        }
        if (cursor > 288) {
            return false;
        }
        for (int item = 0; item < total; ++item) {
            const int length = lengths[item];
            if (length > 0) {
                symbol[next[length]++] = item;
            }
        }
        return true;
    }

    int Decode(BitReader& reader) const {
        int code = 0;
        for (int length = 1; length <= 15; ++length) {
            const int bit = reader.Next();
            if (bit < 0) {
                return -1;
            }
            code = (code << 1) | bit;
            if (maxCode[length] >= 0 && code <= maxCode[length]) {
                return symbol[index[length] + code - minCode[length]];
            }
        }
        return -1;
    }
};

const int kLengthBase[29] = {3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17,  19,  23, 27,
                             31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
const int kLengthExtra[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
const int kDistanceBase[30] = {1,   2,   3,   4,   5,   7,    9,    13,   17,   25,   33,   49,   65,    97,    129,
                               193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
const int kDistanceExtra[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

bool InflateBlock(BitReader& reader, std::vector<std::uint8_t>& out, const InflateHuffman& literal, const InflateHuffman& distance) {
    for (;;) {
        const int symbol = literal.Decode(reader);
        if (symbol < 0) {
            return false;
        }
        if (symbol < 256) {
            out.push_back(static_cast<std::uint8_t>(symbol));
            continue;
        }
        if (symbol == 256) {
            return true;
        }
        const int lengthIndex = symbol - 257;
        if (lengthIndex < 0 || lengthIndex > 28) {
            return false;
        }
        const int lengthExtra = reader.Get(kLengthExtra[lengthIndex]);
        const int distanceIndex = distance.Decode(reader);
        if (lengthExtra < 0 || distanceIndex < 0 || distanceIndex > 29) {
            return false;
        }
        const int distanceExtra = reader.Get(kDistanceExtra[distanceIndex]);
        if (distanceExtra < 0) {
            return false;
        }
        const int length = kLengthBase[lengthIndex] + lengthExtra;
        const int span = kDistanceBase[distanceIndex] + distanceExtra;
        if (span <= 0 || span > static_cast<int>(out.size())) {
            return false;
        }
        for (int index = 0; index < length; ++index) {
            out.push_back(out[out.size() - static_cast<std::size_t>(span)]);
        }
    }
}

bool Inflate(const std::uint8_t* data, std::size_t size, std::vector<std::uint8_t>& out) {
    BitReader reader;
    reader.data = data;
    reader.size = size;
    int lengths[320];
    const int order[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    for (;;) {
        const int header = reader.Get(3);
        if (header < 0) {
            return false;
        }
        const bool final = (header & 1) != 0;
        const int type = (header >> 1) & 3;
        if (type == 0) {
            reader.Align();
            if (reader.pos + 4 > reader.size) {
                return false;
            }
            const int length = ReadU16(reader.data + reader.pos);
            reader.pos += 4;
            if (reader.pos + static_cast<std::size_t>(length) > reader.size) {
                return false;
            }
            for (int index = 0; index < length; ++index) {
                out.push_back(reader.data[reader.pos++]);
            }
        } else if (type == 1 || type == 2) {
            InflateHuffman literal;
            InflateHuffman distance;
            if (type == 1) {
                for (int index = 0; index < 144; ++index) {
                    lengths[index] = 8;
                }
                for (int index = 144; index < 256; ++index) {
                    lengths[index] = 9;
                }
                for (int index = 256; index < 280; ++index) {
                    lengths[index] = 7;
                }
                for (int index = 280; index < 288; ++index) {
                    lengths[index] = 8;
                }
                if (!literal.Build(lengths, 288)) {
                    return false;
                }
                for (int index = 0; index < 32; ++index) {
                    lengths[index] = 5;
                }
                if (!distance.Build(lengths, 32)) {
                    return false;
                }
            } else {
                const int hlit = reader.Get(5);
                const int hdist = reader.Get(5);
                const int hclen = reader.Get(4);
                if (hlit < 0 || hdist < 0 || hclen < 0) {
                    return false;
                }
                int codeLengths[19] = {};
                for (int index = 0; index < hclen + 4; ++index) {
                    const int length = reader.Get(3);
                    if (length < 0) {
                        return false;
                    }
                    codeLengths[order[index]] = length;
                }
                InflateHuffman coder;
                if (!coder.Build(codeLengths, 19)) {
                    return false;
                }
                const int total = hlit + 257 + hdist + 1;
                int written = 0;
                while (written < total) {
                    const int symbol = coder.Decode(reader);
                    if (symbol < 0) {
                        return false;
                    }
                    if (symbol < 16) {
                        lengths[written++] = symbol;
                    } else if (symbol == 16) {
                        const int repeat = reader.Get(2);
                        if (repeat < 0 || written == 0) {
                            return false;
                        }
                        const int value = lengths[written - 1];
                        for (int index = 0; index < repeat + 3 && written < total; ++index) {
                            lengths[written++] = value;
                        }
                    } else if (symbol == 17 || symbol == 18) {
                        const int repeat = reader.Get(symbol == 17 ? 3 : 7);
                        if (repeat < 0) {
                            return false;
                        }
                        const int count = (symbol == 17 ? 3 : 11) + repeat;
                        for (int index = 0; index < count && written < total; ++index) {
                            lengths[written++] = 0;
                        }
                    } else {
                        return false;
                    }
                }
                if (!literal.Build(lengths, hlit + 257) || !distance.Build(lengths + hlit + 257, hdist + 1)) {
                    return false;
                }
            }
            if (!InflateBlock(reader, out, literal, distance)) {
                return false;
            }
        } else {
            return false;
        }
        if (final) {
            return !out.empty();
        }
    }
}

int Paeth(int left, int up, int corner) {
    const int estimate = left + up - corner;
    const int leftDistance = estimate > left ? estimate - left : left - estimate;
    const int upDistance = estimate > up ? estimate - up : up - estimate;
    const int cornerDistance = estimate > corner ? estimate - corner : corner - estimate;
    if (leftDistance <= upDistance && leftDistance <= cornerDistance) {
        return left;
    }
    return upDistance <= cornerDistance ? up : corner;
}

bool UnfilterPng(const std::uint8_t* raw, std::size_t rawSize, int width, int height, int channels, std::vector<std::uint8_t>& pixels) {
    const int stride = width * channels;
    if (rawSize < static_cast<std::size_t>(height) * static_cast<std::size_t>(stride + 1)) {
        return false;
    }
    pixels.assign(static_cast<std::size_t>(height) * static_cast<std::size_t>(stride), 0);
    std::size_t source = 0;
    for (int y = 0; y < height; ++y) {
        const int filter = raw[source++];
        std::uint8_t* row = pixels.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(stride);
        const std::uint8_t* previous = y == 0 ? nullptr : row - stride;
        for (int x = 0; x < stride; ++x) {
            const int value = raw[source++];
            const int left = x >= channels ? row[x - channels] : 0;
            const int up = previous != nullptr ? previous[x] : 0;
            const int corner = previous != nullptr && x >= channels ? previous[x - channels] : 0;
            int decoded = value;
            if (filter == 1) {
                decoded += left;
            } else if (filter == 2) {
                decoded += up;
            } else if (filter == 3) {
                decoded += (left + up) / 2;
            } else if (filter == 4) {
                decoded += Paeth(left, up, corner);
            } else if (filter != 0) {
                return false;
            }
            row[x] = static_cast<std::uint8_t>(decoded);
        }
    }
    return true;
}

struct JpegHuffman {
    int count[16] = {};
    int symbol[256] = {};
    int minCode[17] = {};
    int maxCode[17] = {};
    int index[17] = {};

    bool Build(const std::uint8_t* counts, const std::uint8_t* symbols) {
        int code = 0;
        int cursor = 0;
        int symbolCursor = 0;
        for (int length = 1; length <= 16; ++length) {
            count[length - 1] = counts[length - 1];
            index[length] = cursor;
            minCode[length] = code;
            maxCode[length] = counts[length - 1] == 0 ? -1 : code + counts[length - 1] - 1;
            for (int item = 0; item < counts[length - 1]; ++item) {
                if (cursor >= 256) {
                    return false;
                }
                symbol[cursor++] = symbols[symbolCursor++];
            }
            code = (code + counts[length - 1]) << 1;
        }
        return true;
    }

    int Decode(int (*next)(void*), void* cookie) const {
        int code = 0;
        for (int length = 1; length <= 16; ++length) {
            const int bit = next(cookie);
            if (bit < 0) {
                return -1;
            }
            code = (code << 1) | bit;
            if (maxCode[length] >= 0 && code <= maxCode[length]) {
                return symbol[index[length] + code - minCode[length]];
            }
        }
        return -1;
    }
};

struct JpegReader {
    const std::uint8_t* data = nullptr;
    std::size_t size = 0;
    std::size_t pos = 0;
    std::uint32_t acc = 0;
    int bits = 0;

    int NextBit() {
        if (bits == 0) {
            if (pos >= size) {
                return -1;
            }
            std::uint8_t byte = data[pos++];
            if (byte == 0xFF) {
                if (pos >= size) {
                    return -1;
                }
                if (data[pos] == 0x00) {
                    pos += 1;
                } else {
                    return -1;
                }
            }
            acc = byte;
            bits = 8;
        }
        bits -= 1;
        return static_cast<int>((acc >> bits) & 1u);
    }

    int Receive(int count) {
        int value = 0;
        for (int index = 0; index < count; ++index) {
            const int bit = NextBit();
            if (bit < 0) {
                return -1;
            }
            value = (value << 1) | bit;
        }
        return value;
    }
};

int JpegNextBit(void* cookie) {
    return static_cast<JpegReader*>(cookie)->NextBit();
}

void JpegIdct(float* block) {
    const float pi = 3.14159265f;
    float row[64];
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            float sum = 0.0f;
            for (int u = 0; u < 8; ++u) {
                const float cu = u == 0 ? 0.70710678f : 1.0f;
                sum += cu * block[y * 8 + u] * std::cos((2.0f * static_cast<float>(x) + 1.0f) * static_cast<float>(u) * pi / 16.0f);
            }
            row[y * 8 + x] = sum * 0.5f;
        }
    }
    for (int x = 0; x < 8; ++x) {
        for (int y = 0; y < 8; ++y) {
            float sum = 0.0f;
            for (int v = 0; v < 8; ++v) {
                const float cv = v == 0 ? 0.70710678f : 1.0f;
                sum += cv * row[v * 8 + x] * std::cos((2.0f * static_cast<float>(y) + 1.0f) * static_cast<float>(v) * pi / 16.0f);
            }
            block[y * 8 + x] = sum * 0.5f;
        }
    }
}

}  // namespace

bool RoboDecodePng(const std::uint8_t* bytes, std::size_t size, std::vector<std::uint8_t>& pixels, int& width, int& height) {
    static const std::uint8_t kSignature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    if (size < 8 || memcmp(bytes, kSignature, 8) != 0) {
        return false;
    }
    width = 0;
    height = 0;
    int bitDepth = 0;
    int colorType = 0;
    int interlace = 0;
    std::uint8_t palette[256 * 3] = {};
    int paletteSize = 0;
    std::vector<std::uint8_t> idat;
    std::size_t cursor = 8;
    while (cursor + 12 <= size) {
        const int length = (bytes[cursor] << 24) | (bytes[cursor + 1] << 16) | (bytes[cursor + 2] << 8) | bytes[cursor + 3];
        if (length < 0 || cursor + 12u + static_cast<std::size_t>(length) > size) {
            return false;
        }
        const std::uint8_t* type = bytes + cursor + 4;
        const std::uint8_t* data = bytes + cursor + 8;
        if (memcmp(type, "IHDR", 4) == 0 && length >= 13) {
            width = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
            height = (data[4] << 24) | (data[5] << 16) | (data[6] << 8) | data[7];
            bitDepth = data[8];
            colorType = data[9];
            interlace = data[12];
        } else if (memcmp(type, "PLTE", 4) == 0 && length <= 256 * 3) {
            memcpy(palette, data, static_cast<std::size_t>(length));
            paletteSize = length / 3;
        } else if (memcmp(type, "IDAT", 4) == 0) {
            for (int index = 0; index < length; ++index) {
                idat.push_back(data[index]);
            }
        } else if (memcmp(type, "IEND", 4) == 0) {
            break;
        }
        cursor += 12u + static_cast<std::size_t>(length);
    }
    if (width <= 0 || height <= 0 || width > 8192 || height > 8192 || bitDepth != 8 || interlace != 0) {
        return false;
    }
    if (static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u > 8u * 1024u * 1024u) {
        return false;
    }
    if (colorType != 0 && colorType != 2 && colorType != 3 && colorType != 6) {
        return false;
    }
    std::vector<std::uint8_t> inflated;
    bool inflatedOk = false;
    if (idat.size() >= 6) {
        inflatedOk = Inflate(idat.data() + 2, idat.size() - 6, inflated);
    }
    if (!inflatedOk) {
        inflated.clear();
        inflatedOk = Inflate(idat.data(), idat.size(), inflated);
    }
    if (!inflatedOk) {
        return false;
    }
    const int channels = colorType == 2 ? 3 : colorType == 6 ? 4 : 1;
    std::vector<std::uint8_t> raw;
    if (!UnfilterPng(inflated.data(), inflated.size(), width, height, channels, raw)) {
        return false;
    }
    pixels.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u, 255);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const std::uint8_t* source = raw.data() + (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * static_cast<std::size_t>(channels);
            std::uint8_t* dest = pixels.data() + (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4u;
            if (colorType == 0) {
                dest[0] = dest[1] = dest[2] = source[0];
            } else if (colorType == 2) {
                dest[0] = source[0];
                dest[1] = source[1];
                dest[2] = source[2];
            } else if (colorType == 3) {
                if (source[0] >= paletteSize) {
                    return false;
                }
                dest[0] = palette[source[0] * 3];
                dest[1] = palette[source[0] * 3 + 1];
                dest[2] = palette[source[0] * 3 + 2];
            } else {
                dest[0] = source[0];
                dest[1] = source[1];
                dest[2] = source[2];
                dest[3] = source[3];
            }
        }
    }
    return true;
}

bool RoboDecodeJpeg(const std::uint8_t* bytes, std::size_t size, std::vector<std::uint8_t>& pixels, int& width, int& height) {
    if (size < 4 || bytes[0] != 0xFF || bytes[1] != 0xD8) {
        return false;
    }
    JpegHuffman dc[2];
    JpegHuffman ac[2];
    bool hasDc[2] = {};
    bool hasAc[2] = {};
    std::uint8_t quant[2][64] = {};
    bool hasQuant[2] = {};
    int componentId[3] = {};
    int quantId[3] = {};
    int dcId[3] = {};
    int acId[3] = {};
    int sampleH[3] = {};
    int sampleV[3] = {};
    int componentCount = 0;
    width = 0;
    height = 0;
    bool haveFrame = false;
    const int zigzag[64] = {0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,  12, 19, 26, 33, 40, 48,
                            41, 34, 27, 20, 13, 6,  7,  14, 21, 28, 35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23,
                            30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};
    std::size_t cursor = 2;
    JpegReader reader;
    while (cursor + 4 <= size) {
        if (bytes[cursor] != 0xFF) {
            return false;
        }
        while (cursor < size && bytes[cursor] == 0xFF) {
            cursor += 1;
        }
        if (cursor >= size) {
            return false;
        }
        const int marker = bytes[cursor++];
        if (marker == 0xD9) {
            break;
        }
        if (marker == 0xDA) {
            if (!haveFrame || cursor + 2 > size) {
                return false;
            }
            const int length = (bytes[cursor] << 8) | bytes[cursor + 1];
            if (length < 6 || cursor + static_cast<std::size_t>(length) > size) {
                return false;
            }
            const int scanCount = bytes[cursor + 2];
            if (scanCount != componentCount || scanCount < 1 || scanCount > 3) {
                return false;
            }
            for (int index = 0; index < scanCount; ++index) {
                const int id = bytes[cursor + 3 + index * 2];
                const int tables = bytes[cursor + 4 + index * 2];
                int slot = -1;
                for (int component = 0; component < componentCount; ++component) {
                    if (componentId[component] == id) {
                        slot = component;
                    }
                }
                if (slot < 0) {
                    return false;
                }
                dcId[slot] = tables >> 4;
                acId[slot] = tables & 15;
            }
            reader.data = bytes + cursor + length;
            reader.size = size - (cursor + static_cast<std::size_t>(length));
            reader.pos = 0;
            reader.bits = 0;
            int hMax = 1;
            int vMax = 1;
            for (int component = 0; component < componentCount; ++component) {
                if (sampleH[component] > hMax) {
                    hMax = sampleH[component];
                }
                if (sampleV[component] > vMax) {
                    vMax = sampleV[component];
                }
            }
            pixels.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u, 255);
            int previousDc[3] = {};
            for (int my = 0; my < height; my += vMax * 8) {
                for (int mx = 0; mx < width; mx += hMax * 8) {
                    float blocks[3][4][64] = {};
                    int blockCount[3] = {};
                    for (int component = 0; component < componentCount; ++component) {
                        const int count = sampleH[component] * sampleV[component];
                        blockCount[component] = count;
                        if (dcId[component] > 1 || acId[component] > 1 || quantId[component] > 1 || !hasDc[dcId[component]] || !hasAc[acId[component]] || !hasQuant[quantId[component]]) {
                            return false;
                        }
                        for (int block = 0; block < count; ++block) {
                            const int dcSymbol = dc[dcId[component]].Decode(JpegNextBit, &reader);
                            if (dcSymbol < 0 || dcSymbol > 11) {
                                return false;
                            }
                            int diff = 0;
                            if (dcSymbol > 0) {
                                diff = reader.Receive(dcSymbol);
                                if (diff < 0) {
                                    return false;
                                }
                                if (diff < (1 << (dcSymbol - 1))) {
                                    diff -= (1 << dcSymbol) - 1;
                                }
                            }
                            previousDc[component] += diff;
                            float natural[64] = {};
                            natural[0] = static_cast<float>(previousDc[component] * quant[quantId[component]][0]);
                            int k = 1;
                            while (k < 64) {
                                const int acSymbol = ac[acId[component]].Decode(JpegNextBit, &reader);
                                if (acSymbol < 0) {
                                    return false;
                                }
                                if (acSymbol == 0) {
                                    break;
                                }
                                const int run = acSymbol >> 4;
                                const int magnitude = acSymbol & 15;
                                if (magnitude == 0) {
                                    if (run != 15) {
                                        return false;
                                    }
                                    k += 16;
                                    if (k > 64) {
                                        return false;
                                    }
                                    continue;
                                }
                                k += run;
                                if (k >= 64) {
                                    return false;
                                }
                                int value = reader.Receive(magnitude);
                                if (value < 0) {
                                    return false;
                                }
                                if (value < (1 << (magnitude - 1))) {
                                    value -= (1 << magnitude) - 1;
                                }
                                natural[zigzag[k]] = static_cast<float>(value * quant[quantId[component]][k]);
                                k += 1;
                            }
                            JpegIdct(natural);
                            for (int item = 0; item < 64; ++item) {
                                blocks[component][block][item] = natural[item];
                            }
                        }
                    }
                    for (int y = 0; y < vMax * 8 && my + y < height; ++y) {
                        for (int x = 0; x < hMax * 8 && mx + x < width; ++x) {
                            float channel[3] = {0, 0, 0};
                            for (int component = 0; component < componentCount; ++component) {
                                const int sx = sampleH[component] == 0 ? 0 : x * sampleH[component] / hMax;
                                const int sy = sampleV[component] == 0 ? 0 : y * sampleV[component] / vMax;
                                const int block = (sy / 8) * sampleH[component] + (sx / 8);
                                if (block < 0 || block >= blockCount[component]) {
                                    return false;
                                }
                                channel[component] = blocks[component][block][(sy & 7) * 8 + (sx & 7)];
                            }
                            int red = 0;
                            int green = 0;
                            int blue = 0;
                            if (componentCount == 1) {
                                red = green = blue = static_cast<int>(channel[0] + 128.0f);
                            } else {
                                const float yValue = channel[0];
                                const float cb = channel[1];
                                const float cr = channel[2];
                                red = static_cast<int>(yValue + 1.402f * cr + 128.0f);
                                green = static_cast<int>(yValue - 0.344136f * cb - 0.714136f * cr + 128.0f);
                                blue = static_cast<int>(yValue + 1.772f * cb + 128.0f);
                            }
                            auto clamp = [](int value) { return value < 0 ? 0 : value > 255 ? 255 : value; };
                            std::uint8_t* dest = pixels.data() + (static_cast<std::size_t>(my + y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(mx + x)) * 4u;
                            dest[0] = static_cast<std::uint8_t>(clamp(red));
                            dest[1] = static_cast<std::uint8_t>(clamp(green));
                            dest[2] = static_cast<std::uint8_t>(clamp(blue));
                            dest[3] = 255;
                        }
                    }
                }
            }
            return true;
        }
        if (cursor + 2 > size) {
            return false;
        }
        const int length = (bytes[cursor] << 8) | bytes[cursor + 1];
        if (length < 2 || cursor + static_cast<std::size_t>(length) > size) {
            return false;
        }
        const std::uint8_t* data = bytes + cursor + 2;
        const int payload = length - 2;
        if (marker == 0xC0 && payload >= 8) {
            if (data[0] != 8) {
                return false;
            }
            height = (data[1] << 8) | data[2];
            width = (data[3] << 8) | data[4];
            componentCount = data[5];
            if (componentCount < 1 || componentCount > 3 || payload < 6 + componentCount * 3 || width <= 0 || height <= 0 || width > 8192 || height > 8192) {
                return false;
            }
            if (static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u > 8u * 1024u * 1024u) {
                return false;
            }
            for (int index = 0; index < componentCount; ++index) {
                componentId[index] = data[6 + index * 3];
                sampleH[index] = data[7 + index * 3] >> 4;
                sampleV[index] = data[7 + index * 3] & 15;
                quantId[index] = data[8 + index * 3];
                if (sampleH[index] < 1 || sampleV[index] < 1 || sampleH[index] > 2 || sampleV[index] > 2) {
                    return false;
                }
            }
            haveFrame = true;
        } else if (marker == 0xC4 && payload >= 17) {
            int offset = 0;
            while (offset + 17 <= payload) {
                const int klass = data[offset] >> 4;
                const int id = data[offset] & 15;
                if ((klass != 0 && klass != 1) || id > 1) {
                    return false;
                }
                const std::uint8_t* counts = data + offset + 1;
                int total = 0;
                for (int index = 0; index < 16; ++index) {
                    total += counts[index];
                }
                if (offset + 17 + total > payload || total > 256) {
                    return false;
                }
                JpegHuffman* table = klass == 0 ? &dc[id] : &ac[id];
                if (!table->Build(counts, data + offset + 17)) {
                    return false;
                }
                if (klass == 0) {
                    hasDc[id] = true;
                } else {
                    hasAc[id] = true;
                }
                offset += 17 + total;
            }
        } else if (marker == 0xDB && payload >= 65) {
            int offset = 0;
            while (offset + 65 <= payload) {
                const int id = data[offset] & 15;
                if (id > 1 || (data[offset] >> 4) != 0) {
                    return false;
                }
                memcpy(quant[id], data + offset + 1, 64);
                hasQuant[id] = true;
                offset += 65;
            }
        } else if (marker == 0xC2 || marker == 0xC1) {
            return false;
        }
        cursor += static_cast<std::size_t>(length);
    }
    return false;
}

}  // namespace Robo
