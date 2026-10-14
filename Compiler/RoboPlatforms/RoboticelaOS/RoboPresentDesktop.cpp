#include "RoboticelaOSPlatform.hpp"

#include "Robo/Color.hpp"
#include "Robo/FontEngine.hpp"
#include "Robo/RoboScene.hpp"
#include "RoboDesktopImage.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

void BuildPage(Robo::RoboScene& scene);

namespace {

std::vector<std::uint8_t> ReadFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }
    return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

void Write32(std::ostream& output, std::uint32_t value) {
    const char bytes[4] = {
        static_cast<char>(value & 0xFFu),
        static_cast<char>((value >> 8) & 0xFFu),
        static_cast<char>((value >> 16) & 0xFFu),
        static_cast<char>((value >> 24) & 0xFFu),
    };
    output.write(bytes, 4);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "Usage: RoboPresentDesktop <width> <height> <output> <asset-root>\n";
        return 1;
    }
    const int width = std::stoi(argv[1]);
    const int height = std::stoi(argv[2]);
    const std::string outputPath = argv[3];
    const std::string assetRoot = argv[4];
    if (width <= 0 || height <= 0 || width > ROBO_DESKTOP_IMAGE_WIDTH || height > ROBO_DESKTOP_IMAGE_HEIGHT) {
        std::cerr << "Desktop size is outside the kernel image\n";
        return 1;
    }

    Robo::RoboticelaOSPlatform platform;
    platform.SetAssetRoot(assetRoot);
    platform.SetSurfaceSize(width, height);
    platform.SetPixelScale(1.0f);
    std::vector<std::uint8_t> scanout(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u, 0);
    platform.SetScanout(scanout.data(), width, height, width * 4, Robo::ROBOTICELA_PIXEL_BGRX);

    Robo::FontEngine fonts;
    const bool regular = fonts.LoadFace(Robo::FontFaceId::Regular, ReadFile(assetRoot + "/Fonts/Inter-Regular.ttf"));
    fonts.LoadFace(Robo::FontFaceId::Medium, ReadFile(assetRoot + "/Fonts/Inter-Medium.ttf"));
    fonts.LoadFace(Robo::FontFaceId::SemiBold, ReadFile(assetRoot + "/Fonts/Inter-SemiBold.ttf"));
    fonts.LoadFace(Robo::FontFaceId::Bold, ReadFile(assetRoot + "/Fonts/Inter-Bold.ttf"));
    fonts.LoadFace(Robo::FontFaceId::Italic, ReadFile(assetRoot + "/Fonts/Inter-Italic.ttf"));
    fonts.WarmChrome(1.0f);
    if (!regular) {
        std::cerr << "Inter-Regular.ttf was not loaded. The shell will draw without text.\n";
    }

    Robo::RoboScene scene;
    BuildPage(scene);
    scene.Render(platform, fonts, Robo::Color::FromHex("#202838"), 24.0f);

    std::ofstream output(outputPath, std::ios::binary);
    if (!output) {
        std::cerr << "Failed to write " << outputPath << "\n";
        return 1;
    }
    Write32(output, ROBO_DESKTOP_IMAGE_MAGIC);
    Write32(output, static_cast<std::uint32_t>(width));
    Write32(output, static_cast<std::uint32_t>(height));
    Write32(output, Robo::ROBOTICELA_PIXEL_BGRX);
    output.write(reinterpret_cast<const char*>(scanout.data()), static_cast<std::streamsize>(scanout.size()));
    if (!output) {
        std::cerr << "Failed to finish " << outputPath << "\n";
        return 1;
    }
    std::cout << "Wrote " << outputPath << " " << width << "x" << height << "\n";
    return 0;
}
