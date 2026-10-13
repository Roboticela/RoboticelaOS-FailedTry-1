#pragma once

#include "Color.hpp"
#include "Svg.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace Robo {

class IPlatform;

// What a background is showing. A compiled app can switch this later by path.
enum class BackgroundKind {
    Color,
    Picture,
    Svg,
    Video,
    Gif
};

// Fills the desktop behind the shell. Color, a picture, an SVG, a GIF, or a
// video. SetPath accepts a new source at any time after the page is compiled:
//   #112233
//   {color.bg.primary}
//   color:#112233
//   picture:Wall.png
//   picture:Photo.jpg
//   svg:Mark.svg
//   gif:Idle.gif
//   video:Background.mp4
// A path with no prefix is chosen from the file itself.
// Pictures are BMP, TGA, PNG, and baseline JPEG. Video is RV1 raw frames or
// RJPG, a list of JPEG frames. An H.264 MP4 is played from the RJPG reel
// packed beside it for the OS.
class Background {
public:
    void SetColor(const Color& color, const std::string& token);
    void SetPath(const std::string& path);

    bool HasSource() const;
    BackgroundKind GetKind() const;
    const std::string& GetPath() const;
    const Color& GetColor() const;
    const std::string& GetColorToken() const;
    const std::string& GetStatus() const;
    bool IsAnimated() const;

    bool EnsureLoaded(const IPlatform& platform);
    bool Advance(double deltaMs);
    bool PaintsMedia() const;

    const std::uint8_t* Pixels() const;
    int PixelWidth() const;
    int PixelHeight() const;
    Svg* Graphic();

private:
    bool ApplyPath(std::vector<std::uint8_t>& bytes, const std::string& kindName);
    bool UseColor(const std::string& text);
    bool DecodePicture(const std::uint8_t* bytes, std::size_t size);
    bool DecodeGif(const std::uint8_t* bytes, std::size_t size);
    bool DecodeSvg(const std::uint8_t* bytes, std::size_t size);
    bool DecodeVideo(std::vector<std::uint8_t>& bytes);
    bool ShowVideoFrame(int index);
    void RememberFrame(std::vector<std::uint8_t> pixels, int delayMs);

    bool hasSource_ = false;
    bool pending_ = false;
    bool animated_ = false;
    BackgroundKind kind_ = BackgroundKind::Color;
    Color color_ = Color::FromHex("#202838");
    std::string token_;
    std::string path_;
    std::string status_;
    int width_ = 0;
    int height_ = 0;
    int frame_ = 0;
    double elapsedMs_ = 0.0;
    std::vector<int> delays_;
    std::vector<std::vector<std::uint8_t>> frames_;
    std::vector<std::uint8_t> video_;
    int videoFrames_ = 0;
    int videoDelayMs_ = 100;
    bool videoJpeg_ = false;
    Svg graphic_;
    bool hasGraphic_ = false;
};

}  // namespace Robo
