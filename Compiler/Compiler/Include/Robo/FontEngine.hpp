#pragma once

#include "Color.hpp"
#include "FontStyle.hpp"
#include "FrameBuffer.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Robo {

enum class FontFaceId {
    Regular,
    Medium,
    SemiBold,
    Italic,
    Bold
};

struct TextGlyphDraw {
    int faceIndex = 0;
    int glyph = 0;
    float x = 0.0f;
    float width = 0.0f;
    float baseline = 0.0f;
    float fontSize = 0.0f;
    float ascent = 0.0f;
    float lineHeight = 0.0f;
    Color color;
    float opacity = 1.0f;
    int index = -1;
};

class FontEngine {
public:
    FontEngine();
    ~FontEngine();

    bool LoadFace(FontFaceId faceId, const std::vector<std::uint8_t>& ttfBytes);
    bool IsReady() const;

    FontFaceId ResolveFace(FontWeight weight, FontStyleKind style) const;
    float MeasureWidth(const std::string& utf8, float fontSize, FontFaceId faceId, float letterSpacing = 0.0f) const;
    float GetAscent(float fontSize, FontFaceId faceId) const;
    float GetDescent(float fontSize, FontFaceId faceId) const;
    float GetLineGap(float fontSize, FontFaceId faceId) const;
    // Turns a CSS font-size into the pixel height this engine rasterizes.
    float PixelSizeForCss(float cssPixels, FontFaceId faceId) const;

    void DrawString(FrameBuffer& buffer,
                    float x,
                    float baselineY,
                    const std::string& utf8,
                    float fontSize,
                    FontFaceId faceId,
                    const Color& color,
                    float opacity,
                    float letterSpacing,
                    bool snap = false) const;

    // Builds the Latin glyphs for menu and bar sizes. The same sizes are used
    // on QEMU and on real hardware; pixelScale is the framebuffer scale.
    void WarmChrome(float pixelScale) const;

    // Layout one span into a line. The line is highlighted once, then blitted,
    // so adjacent words do not paint overlapping selection edges.
    void RecordString(std::vector<TextGlyphDraw>& out,
                      float x,
                      float baselineY,
                      const std::string& utf8,
                      float fontSize,
                      FontFaceId faceId,
                      const Color& color,
                      float opacity,
                      float letterSpacing) const;
    static void PaintSelectionBands(FrameBuffer& buffer, const std::vector<TextGlyphDraw>& draws);
    void PaintGlyphDraws(FrameBuffer& buffer, const std::vector<TextGlyphDraw>& draws) const;

private:
    struct FaceData;
    struct AtlasRow {
        int y = 0;
        int height = 0;
        int used = 0;
        std::uint32_t stamp = 0;
    };
    struct AtlasGlyph {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        int xOff = 0;
        int yOff = 0;
        int row = -1;
        std::uint32_t stamp = 0;
    };

    AtlasGlyph CachedGlyphFor(int faceIndex, int glyph, float fontSize, float shiftX, float shiftY) const;
    void EnsureAtlas() const;
    void ResetAtlas() const;
    void EvictOldestRow() const;
    int PlaceGlyph(int width, int height, int& outX, int& outY) const;

    static constexpr int ATLAS_WIDTH = 512;
    static constexpr int ATLAS_HEIGHT = 1024;

    std::unique_ptr<FaceData> faces_[5];
    mutable std::vector<std::uint8_t> atlas_;
    mutable std::vector<AtlasRow> atlasRows_;
    mutable int atlasPenY_ = 0;
    mutable std::uint32_t atlasStamp_ = 1;
    mutable int warmedScale_ = -1;
    mutable std::unordered_map<std::uint64_t, AtlasGlyph> glyphCache_;
};

}  // namespace Robo
