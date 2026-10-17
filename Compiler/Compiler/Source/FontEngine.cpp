#include "Robo/FontEngine.hpp"
#include "Robo/TextSelection.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "StbTrueType.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace Robo {

namespace {

int DecodeUtf8(const char*& cursor, const char* end) {
    if (cursor >= end) {
        return -1;
    }
    const unsigned char lead = static_cast<unsigned char>(*cursor++);
    if (lead < 0x80) {
        return lead;
    }
    if ((lead >> 5) == 0x6 && cursor < end) {
        const unsigned char b1 = static_cast<unsigned char>(*cursor++);
        return ((lead & 0x1F) << 6) | (b1 & 0x3F);
    }
    if ((lead >> 4) == 0xE && cursor + 1 < end) {
        const unsigned char b1 = static_cast<unsigned char>(*cursor++);
        const unsigned char b2 = static_cast<unsigned char>(*cursor++);
        return ((lead & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
    }
    if ((lead >> 3) == 0x1E && cursor + 2 < end) {
        const unsigned char b1 = static_cast<unsigned char>(*cursor++);
        const unsigned char b2 = static_cast<unsigned char>(*cursor++);
        const unsigned char b3 = static_cast<unsigned char>(*cursor++);
        return ((lead & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
    }
    return -1;
}

int FaceIndex(FontFaceId faceId) {
    return static_cast<int>(faceId);
}

bool IsSkippableCodepoint(int codepoint) {
    return codepoint < 32 || codepoint == 0x7F || codepoint == 0xFFFD;
}

}  // namespace

struct FontEngine::FaceData {
    std::vector<std::uint8_t> bytes;
    stbtt_fontinfo info{};
    bool ready = false;
};

FontEngine::FontEngine() {
    for (int i = 0; i < 5; ++i) {
        faces_[i] = std::make_unique<FaceData>();
    }
}

FontEngine::~FontEngine() = default;

bool FontEngine::LoadFace(FontFaceId faceId, const std::vector<std::uint8_t>& ttfBytes) {
    FaceData& face = *faces_[FaceIndex(faceId)];
    face.bytes = ttfBytes;
    face.ready = false;
    if (face.bytes.empty()) {
        return false;
    }
    if (!stbtt_InitFont(&face.info, face.bytes.data(), stbtt_GetFontOffsetForIndex(face.bytes.data(), 0))) {
        return false;
    }
    face.ready = true;
    return true;
}

bool FontEngine::IsReady() const {
    return faces_[FaceIndex(FontFaceId::Regular)]->ready;
}

FontFaceId FontEngine::ResolveFace(FontWeight weight, FontStyleKind style) const {
    if (style == FontStyleKind::Italic && faces_[FaceIndex(FontFaceId::Italic)]->ready) {
        return FontFaceId::Italic;
    }
    if (weight == FontWeight::Bold && faces_[FaceIndex(FontFaceId::Bold)]->ready) {
        return FontFaceId::Bold;
    }
    if (weight == FontWeight::Bold || weight == FontWeight::SemiBold) {
        if (faces_[FaceIndex(FontFaceId::SemiBold)]->ready) {
            return FontFaceId::SemiBold;
        }
    }
    if (weight == FontWeight::Medium && faces_[FaceIndex(FontFaceId::Medium)]->ready) {
        return FontFaceId::Medium;
    }
    return FontFaceId::Regular;
}

float FontEngine::MeasureWidth(const std::string& utf8, float fontSize, FontFaceId faceId, float letterSpacing) const {
    const FaceData& face = *faces_[FaceIndex(faceId)];
    if (!face.ready || fontSize <= 0.0f) {
        return 0.0f;
    }

    const float scale = stbtt_ScaleForPixelHeight(&face.info, fontSize);
    const char* cursor = utf8.data();
    const char* end = utf8.data() + utf8.size();
    float width = 0.0f;
    int previousGlyph = -1;
    int glyphCount = 0;

    while (cursor < end) {
        const int codepoint = DecodeUtf8(cursor, end);
        if (codepoint < 0) {
            break;
        }
        if (IsSkippableCodepoint(codepoint)) {
            previousGlyph = -1;
            continue;
        }

        const int glyph = stbtt_FindGlyphIndex(&face.info, codepoint);
        if (glyph == 0) {
            width += fontSize * 0.5f;
            previousGlyph = -1;
            ++glyphCount;
            continue;
        }

        int advance = 0;
        int leftSideBearing = 0;
        stbtt_GetGlyphHMetrics(&face.info, glyph, &advance, &leftSideBearing);
        if (previousGlyph >= 0) {
            width += scale * stbtt_GetGlyphKernAdvance(&face.info, previousGlyph, glyph);
        }
        width += scale * advance;
        previousGlyph = glyph;
        ++glyphCount;
    }

    if (glyphCount > 1) {
        width += letterSpacing * static_cast<float>(glyphCount - 1);
    }
    return width;
}

float FontEngine::GetAscent(float fontSize, FontFaceId faceId) const {
    const FaceData& face = *faces_[FaceIndex(faceId)];
    if (!face.ready) {
        return fontSize * 0.8f;
    }
    int ascent = 0;
    int descent = 0;
    int lineGap = 0;
    stbtt_GetFontVMetrics(&face.info, &ascent, &descent, &lineGap);
    return stbtt_ScaleForPixelHeight(&face.info, fontSize) * ascent;
}

float FontEngine::GetDescent(float fontSize, FontFaceId faceId) const {
    const FaceData& face = *faces_[FaceIndex(faceId)];
    if (!face.ready) {
        return fontSize * 0.2f;
    }
    int ascent = 0;
    int descent = 0;
    int lineGap = 0;
    stbtt_GetFontVMetrics(&face.info, &ascent, &descent, &lineGap);
    return stbtt_ScaleForPixelHeight(&face.info, fontSize) * static_cast<float>(-descent);
}

float FontEngine::PixelSizeForCss(float cssPixels, FontFaceId faceId) const {
    const FaceData& face = *faces_[FaceIndex(faceId)];
    if (!face.ready || cssPixels <= 0.0f) {
        return cssPixels;
    }
    const float emScale = stbtt_ScaleForMappingEmToPixels(&face.info, 1.0f);
    const float heightScale = stbtt_ScaleForPixelHeight(&face.info, 1.0f);
    if (emScale <= 0.0f || heightScale <= 0.0f) {
        return cssPixels;
    }
    return cssPixels * emScale / heightScale;
}

float FontEngine::GetLineGap(float fontSize, FontFaceId faceId) const {
    const FaceData& face = *faces_[FaceIndex(faceId)];
    if (!face.ready) {
        return 0.0f;
    }
    int ascent = 0;
    int descent = 0;
    int lineGap = 0;
    stbtt_GetFontVMetrics(&face.info, &ascent, &descent, &lineGap);
    return stbtt_ScaleForPixelHeight(&face.info, fontSize) * lineGap;
}

void StrengthenStems(std::vector<std::uint8_t>& coverage, int width, int height) {
    if (width < 2 || height < 3) {
        return;
    }
    for (int col = 0; col < width; ++col) {
        int peak = 0;
        int peakRow = 0;
        int run = 0;
        for (int row = 0; row < height; ++row) {
            const int value = coverage[static_cast<std::size_t>(row * width + col)];
            if (value > peak) {
                peak = value;
                peakRow = row;
            }
            if (value > 40) {
                ++run;
            }
        }
        if (peak < 80 || peak > 220 || run > height / 2) {
            continue;
        }
        const int lifted = peak + (255 - peak) * 2 / 3;
        coverage[static_cast<std::size_t>(peakRow * width + col)] =
            static_cast<std::uint8_t>(lifted > 255 ? 255 : lifted);
    }
}

float DarkenCoverage(float coverage) {
    if (coverage <= 0.0f) {
        return 0.0f;
    }
    if (coverage >= 1.0f) {
        return 1.0f;
    }
    return std::pow(coverage, 1.0f / 1.8f);
}

void FontEngine::EnsureAtlas() const {
    if (atlas_.size() == static_cast<std::size_t>(ATLAS_WIDTH * ATLAS_HEIGHT)) {
        return;
    }
    atlas_.assign(static_cast<std::size_t>(ATLAS_WIDTH * ATLAS_HEIGHT), 0);
    atlasRows_.clear();
    atlasPenY_ = 0;
}

void FontEngine::ResetAtlas() const {
    EnsureAtlas();
    memset(atlas_.data(), 0, atlas_.size());
    atlasRows_.clear();
    atlasPenY_ = 0;
    glyphCache_.clear();
}

void FontEngine::EvictOldestRow() const {
    if (atlasRows_.empty()) {
        ResetAtlas();
        return;
    }
    int oldest = 0;
    for (int index = 1; index < static_cast<int>(atlasRows_.size()); ++index) {
        if (atlasRows_[static_cast<std::size_t>(index)].stamp < atlasRows_[static_cast<std::size_t>(oldest)].stamp) {
            oldest = index;
        }
    }
    for (auto entry = glyphCache_.begin(); entry != glyphCache_.end();) {
        if (entry->second.row == oldest) {
            entry = glyphCache_.erase(entry);
        } else {
            ++entry;
        }
    }
    AtlasRow& row = atlasRows_[static_cast<std::size_t>(oldest)];
    if (row.height > 0) {
        memset(atlas_.data() + static_cast<std::size_t>(row.y * ATLAS_WIDTH), 0, static_cast<std::size_t>(row.height * ATLAS_WIDTH));
    }
    row.used = 0;
    row.stamp = atlasStamp_;
}

int FontEngine::PlaceGlyph(int width, int height, int& outX, int& outY) const {
    EnsureAtlas();
    if (width <= 0 || height <= 0 || width > ATLAS_WIDTH || height > ATLAS_HEIGHT) {
        return -1;
    }
    const int padded = width + 1;
    int best = -1;
    int bestSlack = ATLAS_HEIGHT;
    for (int index = 0; index < static_cast<int>(atlasRows_.size()); ++index) {
        const AtlasRow& row = atlasRows_[static_cast<std::size_t>(index)];
        if (row.height >= height && row.used + padded <= ATLAS_WIDTH) {
            const int slack = row.height - height;
            if (slack < bestSlack) {
                best = index;
                bestSlack = slack;
            }
        }
    }
    if (best >= 0) {
        AtlasRow& row = atlasRows_[static_cast<std::size_t>(best)];
        outX = row.used;
        outY = row.y;
        row.used += padded;
        row.stamp = atlasStamp_;
        return best;
    }
    if (atlasPenY_ + height <= ATLAS_HEIGHT) {
        AtlasRow row;
        row.y = atlasPenY_;
        row.height = height;
        row.used = padded;
        row.stamp = atlasStamp_;
        outX = 0;
        outY = row.y;
        atlasPenY_ += height;
        atlasRows_.push_back(row);
        return static_cast<int>(atlasRows_.size()) - 1;
    }
    return -1;
}

FontEngine::AtlasGlyph FontEngine::CachedGlyphFor(int faceIndex,
                                                             int glyph,
                                                             float fontSize,
                                                             float shiftX,
                                                             float shiftY) const {
    constexpr int OVERSAMPLE = 2;
    const int subX = std::max(0, std::min(3, static_cast<int>(shiftX * 4.0f)));
    const int subY = std::max(0, std::min(3, static_cast<int>(shiftY * 4.0f)));
    const int sizeQ = std::max(1, static_cast<int>(std::round(fontSize * 4.0f)));
    const std::uint64_t key = (static_cast<std::uint64_t>(faceIndex) & 0xF)
                              | ((static_cast<std::uint64_t>(glyph) & 0xFFFFF) << 4)
                              | ((static_cast<std::uint64_t>(sizeQ) & 0xFFFF) << 24)
                              | (static_cast<std::uint64_t>(subX) << 40)
                              | (static_cast<std::uint64_t>(subY) << 44);
    const auto found = glyphCache_.find(key);
    if (found != glyphCache_.end()) {
        found->second.stamp = ++atlasStamp_;
        if (found->second.row >= 0 && found->second.row < static_cast<int>(atlasRows_.size())) {
            atlasRows_[static_cast<std::size_t>(found->second.row)].stamp = found->second.stamp;
        }
        return found->second;
    }

    AtlasGlyph cached;
    const FaceData& face = *faces_[faceIndex];
    const float snappedSize = static_cast<float>(sizeQ) / 4.0f;
    const float scale = stbtt_ScaleForPixelHeight(&face.info, snappedSize);
    const float snappedShiftX = static_cast<float>(subX) * 0.25f;
    const float snappedShiftY = static_cast<float>(subY) * 0.25f;
    const float renderScale = scale * static_cast<float>(OVERSAMPLE);
    int ix0 = 0;
    int iy0 = 0;
    int ix1 = 0;
    int iy1 = 0;
    stbtt_GetGlyphBitmapBoxSubpixel(&face.info,
                                    glyph,
                                    renderScale,
                                    renderScale,
                                    snappedShiftX * static_cast<float>(OVERSAMPLE),
                                    snappedShiftY * static_cast<float>(OVERSAMPLE),
                                    &ix0,
                                    &iy0,
                                    &ix1,
                                    &iy1);
    const int glyphWidth = ix1 - ix0;
    const int glyphHeight = iy1 - iy0;
    if (glyphWidth <= 0 || glyphHeight <= 0) {
        return glyphCache_.emplace(key, std::move(cached)).first->second;
    }

    std::vector<unsigned char> bitmap(static_cast<std::size_t>(glyphWidth * glyphHeight));
    stbtt_MakeGlyphBitmapSubpixel(&face.info,
                                  bitmap.data(),
                                  glyphWidth,
                                  glyphHeight,
                                  glyphWidth,
                                  renderScale,
                                  renderScale,
                                  snappedShiftX * static_cast<float>(OVERSAMPLE),
                                  snappedShiftY * static_cast<float>(OVERSAMPLE),
                                  glyph);

    auto FloorDiv = [](int value, int divisor) {
        return value >= 0 ? value / divisor : -((-value + divisor - 1) / divisor);
    };
    const int outW = FloorDiv(ix1 - 1, OVERSAMPLE) - FloorDiv(ix0, OVERSAMPLE) + 1;
    const int outH = FloorDiv(iy1 - 1, OVERSAMPLE) - FloorDiv(iy0, OVERSAMPLE) + 1;
    cached.width = std::max(0, outW);
    cached.height = std::max(0, outH);
    cached.xOff = FloorDiv(ix0, OVERSAMPLE);
    cached.yOff = FloorDiv(iy0, OVERSAMPLE);
    std::vector<std::uint8_t> coverage(static_cast<std::size_t>(cached.width * cached.height), 0);
    const int originSampleX = FloorDiv(ix0, OVERSAMPLE) * OVERSAMPLE;
    const int originSampleY = FloorDiv(iy0, OVERSAMPLE) * OVERSAMPLE;
    const float sampleCount = static_cast<float>(OVERSAMPLE * OVERSAMPLE);
    for (int row = 0; row < cached.height; ++row) {
        for (int col = 0; col < cached.width; ++col) {
            unsigned int sum = 0;
            const int sampleX0 = originSampleX + col * OVERSAMPLE;
            const int sampleY0 = originSampleY + row * OVERSAMPLE;
            for (int sampleY = 0; sampleY < OVERSAMPLE; ++sampleY) {
                for (int sampleX = 0; sampleX < OVERSAMPLE; ++sampleX) {
                    const int sourceX = sampleX0 + sampleX - ix0;
                    const int sourceY = sampleY0 + sampleY - iy0;
                    if (sourceX < 0 || sourceY < 0 || sourceX >= glyphWidth || sourceY >= glyphHeight) {
                        continue;
                    }
                    sum += bitmap[static_cast<std::size_t>(sourceY * glyphWidth + sourceX)];
                }
            }
            const float linear = static_cast<float>(sum) / sampleCount / 255.0f;
            coverage[static_cast<std::size_t>(row * cached.width + col)] =
                static_cast<std::uint8_t>(std::min(255.0f, DarkenCoverage(linear) * 255.0f + 0.5f));
        }
    }
    if (snappedSize <= 22.0f) {
        StrengthenStems(coverage, cached.width, cached.height);
    }

    int placeX = 0;
    int placeY = 0;
    int rowIndex = PlaceGlyph(cached.width, cached.height, placeX, placeY);
    if (rowIndex < 0) {
        EvictOldestRow();
        rowIndex = PlaceGlyph(cached.width, cached.height, placeX, placeY);
    }
    if (rowIndex < 0) {
        ResetAtlas();
        rowIndex = PlaceGlyph(cached.width, cached.height, placeX, placeY);
    }
    if (rowIndex < 0) {
        return cached;
    }
    cached.x = placeX;
    cached.y = placeY;
    cached.row = rowIndex;
    cached.stamp = ++atlasStamp_;
    for (int row = 0; row < cached.height; ++row) {
        std::uint8_t* dest = atlas_.data() + static_cast<std::size_t>((cached.y + row) * ATLAS_WIDTH + cached.x);
        memcpy(dest, coverage.data() + static_cast<std::size_t>(row * cached.width), static_cast<std::size_t>(cached.width));
    }
    return glyphCache_.emplace(key, cached).first->second;
}

void FontEngine::RecordString(std::vector<TextGlyphDraw>& out,
                              float x,
                              float baselineY,
                              const std::string& utf8,
                              float fontSize,
                              FontFaceId faceId,
                              const Color& color,
                              float opacity,
                              float letterSpacing) const {
    const FaceData& face = *faces_[FaceIndex(faceId)];
    if (!face.ready || utf8.empty() || fontSize <= 0.0f) {
        return;
    }

    const float scale = stbtt_ScaleForPixelHeight(&face.info, fontSize);
    const float ascent = GetAscent(fontSize, faceId);
    const float glyphHeight = std::max(1.0f, ascent + GetDescent(fontSize, faceId));
    const char* cursor = utf8.data();
    const char* end = utf8.data() + utf8.size();
    float penX = x;
    int previousGlyph = -1;
    bool drewGlyph = false;
    const int faceIndex = FaceIndex(faceId);

    while (cursor < end) {
        const int codepoint = DecodeUtf8(cursor, end);
        if (codepoint < 0) {
            break;
        }
        if (IsSkippableCodepoint(codepoint)) {
            previousGlyph = -1;
            continue;
        }

        const int glyph = stbtt_FindGlyphIndex(&face.info, codepoint);
        if (glyph == 0) {
            if (drewGlyph) {
                penX += letterSpacing;
            }
            const float box = std::max(1.0f, fontSize * 0.5f);
            TextGlyphDraw item;
            item.faceIndex = faceIndex;
            item.glyph = -1;
            item.x = penX;
            item.width = box;
            item.baseline = baselineY;
            item.fontSize = fontSize;
            item.ascent = ascent;
            item.lineHeight = glyphHeight;
            item.color = color;
            item.opacity = opacity;
            out.push_back(item);
            penX += box;
            previousGlyph = -1;
            drewGlyph = true;
            continue;
        }

        if (drewGlyph) {
            penX += letterSpacing;
        }
        if (previousGlyph >= 0) {
            penX += scale * stbtt_GetGlyphKernAdvance(&face.info, previousGlyph, glyph);
        }

        int advance = 0;
        int lsb = 0;
        stbtt_GetGlyphHMetrics(&face.info, glyph, &advance, &lsb);
        const float glyphWidth = std::max(0.0f, scale * static_cast<float>(advance));
        TextGlyphDraw item;
        item.faceIndex = faceIndex;
        item.glyph = glyph;
        item.x = penX;
        item.width = glyphWidth;
        item.baseline = baselineY;
        item.fontSize = fontSize;
        item.ascent = ascent;
        item.lineHeight = glyphHeight;
        item.color = color;
        item.opacity = opacity;
        if (TextSelection::IsOpen()) {
            item.index = TextSelection::AddCodepoint(static_cast<char32_t>(codepoint),
                                                     penX,
                                                     baselineY - ascent,
                                                     std::max(1.0f, glyphWidth),
                                                     glyphHeight);
        }
        out.push_back(item);

        penX += glyphWidth;
        drewGlyph = true;
        previousGlyph = glyph;
    }
}

void FontEngine::PaintSelectionBands(FrameBuffer& buffer, const std::vector<TextGlyphDraw>& draws) {
    std::size_t run = 0;
    while (run < draws.size()) {
        if (draws[run].index < 0 || !TextSelection::Covers(draws[run].index)) {
            ++run;
            continue;
        }
        const float left = draws[run].x;
        float right = draws[run].x + draws[run].width;
        float top = draws[run].baseline - draws[run].ascent;
        float bottom = top + draws[run].lineHeight;
        std::size_t next = run + 1;
        while (next < draws.size() && draws[next].index >= 0 && TextSelection::Covers(draws[next].index)) {
            right = std::max(right, draws[next].x + draws[next].width);
            const float nextTop = draws[next].baseline - draws[next].ascent;
            top = std::min(top, nextTop);
            bottom = std::max(bottom, nextTop + draws[next].lineHeight);
            ++next;
        }
        const int x0 = static_cast<int>(std::floor(left));
        const int x1 = static_cast<int>(std::ceil(right));
        const int y0 = static_cast<int>(std::floor(top));
        const int y1 = static_cast<int>(std::ceil(bottom));
        const int bandWidth = std::max(1, x1 - x0);
        const int bandHeight = std::max(1, y1 - y0);
        if (!buffer.MissesClip(x0, y0, bandWidth, bandHeight)) {
            buffer.FillRect(x0, y0, bandWidth, bandHeight, Color::FromRgba(0, 122, 255, 255), 0.28f);
        }
        run = next;
    }
}

void FontEngine::PaintGlyphDraws(FrameBuffer& buffer, const std::vector<TextGlyphDraw>& draws) const {
    int painted = 0;
    for (const TextGlyphDraw& item : draws) {
        if ((painted & 7) == 0) {
            buffer.Pump();
        }
        painted += 1;
        if (item.glyph < 0) {
            const int boxX = static_cast<int>(std::floor(item.x));
            const int boxY = static_cast<int>(std::floor(item.baseline - item.fontSize * 0.8f));
            const int boxW = std::max(1, static_cast<int>(item.width) - 1);
            const int boxH = std::max(1, static_cast<int>(item.fontSize * 0.8f));
            if (!buffer.MissesClip(boxX, boxY, boxW, boxH)) {
                buffer.FillRect(boxX, boxY, boxW, 1, item.color, item.opacity);
                buffer.FillRect(boxX, boxY + boxH - 1, boxW, 1, item.color, item.opacity);
                buffer.FillRect(boxX, boxY, 1, boxH, item.color, item.opacity);
                buffer.FillRect(boxX + boxW - 1, boxY, 1, boxH, item.color, item.opacity);
            }
            continue;
        }
        const bool onScreen = item.baseline > -item.fontSize * 2.0f &&
                              item.baseline < static_cast<float>(buffer.GetHeight()) + item.fontSize * 2.0f;
        if (onScreen) {
            const float shiftX = item.x - std::floor(item.x);
            const float shiftY = item.baseline - std::floor(item.baseline);
            const AtlasGlyph cached = CachedGlyphFor(item.faceIndex, item.glyph, item.fontSize, shiftX, shiftY);
            const int destX = static_cast<int>(std::floor(item.x)) + cached.xOff;
            const int destY = static_cast<int>(std::floor(item.baseline)) + cached.yOff;
            if (cached.width > 0 && cached.height > 0 && !buffer.MissesClip(destX, destY, cached.width, cached.height)) {
                for (int row = 0; row < cached.height; ++row) {
                    const int py = destY + row;
                    if (py < 0 || py >= buffer.GetHeight()) {
                        continue;
                    }
                    const std::uint8_t* src = atlas_.data() + static_cast<std::size_t>((cached.y + row) * ATLAS_WIDTH + cached.x);
                    for (int col = 0; col < cached.width; ++col) {
                        const unsigned char coverage = src[col];
                        if (coverage == 0) {
                            continue;
                        }
                        buffer.BlendPixel(destX + col, py, item.color, (coverage / 255.0f) * item.opacity);
                    }
                }
            }
        }

        if (item.index >= 0 && (TextSelection::IsCaretBefore(item.index) || TextSelection::IsCaretAfter(item.index))) {
            const float caretOpacity = TextSelection::CaretOpacity();
            if (caretOpacity > 0.004f) {
                const float boundary = TextSelection::IsCaretAfter(item.index) ? item.x + item.width : item.x;
                const float caretWidth = std::max(2.0f, item.fontSize * 0.085f);
                const float inset = std::min(item.lineHeight * 0.06f, caretWidth);
                const float caretX = boundary - caretWidth * 0.5f;
                const float caretY = item.baseline - item.ascent + inset;
                const float caretHeight = std::max(caretWidth, item.lineHeight - inset * 2.0f);
                buffer.FillRoundedRect(caretX, caretY, caretWidth, caretHeight, caretWidth * 0.5f,
                                       Color::FromHex("#007AFF"), caretOpacity);
            }
        }
    }
}

void FontEngine::DrawString(FrameBuffer& buffer,
                            float x,
                            float baselineY,
                            const std::string& utf8,
                            float fontSize,
                            FontFaceId faceId,
                            const Color& color,
                            float opacity,
                            float letterSpacing,
                            bool snap) const {
    if (snap) {
        x = std::round(x);
        baselineY = std::round(baselineY);
    }
    std::vector<TextGlyphDraw> draws;
    RecordString(draws, x, baselineY, utf8, fontSize, faceId, color, opacity, letterSpacing);
    PaintSelectionBands(buffer, draws);
    PaintGlyphDraws(buffer, draws);
}

void FontEngine::WarmChrome(float pixelScale) const {
    const float scale = std::max(0.5f, pixelScale);
    const int scaleKey = std::max(1, static_cast<int>(std::round(scale * 4.0f)));
    if (warmedScale_ == scaleKey) {
        return;
    }
    warmedScale_ = scaleKey;
    const float sizes[] = {12.0f * scale, 13.0f * scale, 16.0f * scale};
    const FontFaceId faces[] = {FontFaceId::Regular, FontFaceId::Medium, FontFaceId::SemiBold, FontFaceId::Bold};
    for (FontFaceId faceId : faces) {
        const FaceData& face = *faces_[FaceIndex(faceId)];
        if (!face.ready) {
            continue;
        }
        for (float fontSize : sizes) {
            for (int codepoint = 32; codepoint <= 126; ++codepoint) {
                const int glyph = stbtt_FindGlyphIndex(&face.info, codepoint);
                if (glyph == 0) {
                    continue;
                }
                CachedGlyphFor(FaceIndex(faceId), glyph, fontSize, 0.0f, 0.0f);
            }
        }
    }
}

}  // namespace Robo
