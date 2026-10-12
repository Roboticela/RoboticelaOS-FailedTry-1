#include "XmlToCpp.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

int main(int argc, char** argv) {
    if (argc != 3 && argc != 4) {
        std::cerr << "Usage: XmlCompile <input.xml> <output.cpp> [asset-directory]\n";
        return 1;
    }

    std::ifstream input(argv[1], std::ios::binary);
    if (!input) {
        std::cerr << "Failed to read " << argv[1] << "\n";
        return 1;
    }
    const std::string xml((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

    try {
        std::string assetDirectory = "Assets";
        const std::string inputPath = argv[1];
        const std::size_t slash = inputPath.find_last_of("/\\");
        if (argc == 4 && argv[3][0] != '\0') {
            assetDirectory = argv[3];
        } else if (slash != std::string::npos) {
            assetDirectory = inputPath.substr(0, slash) + "/Assets";
        }
        auto packExists = [](const std::string& directory) {
            std::ifstream pack(directory + "/RoboIcons.pack", std::ios::binary);
            return static_cast<bool>(pack);
        };
        if (argc != 4 && !packExists(assetDirectory) && slash != std::string::npos) {
            const std::string parent = inputPath.substr(0, slash);
            const std::size_t parentSlash = parent.find_last_of("/\\");
            if (parentSlash != std::string::npos) {
                const std::string fallback = parent.substr(0, parentSlash) + "/Assets";
                if (packExists(fallback)) {
                    assetDirectory = fallback;
                }
            }
        }
        Robo::XmlToCpp compiler;
        const std::string cpp = compiler.Convert(xml, assetDirectory);
        std::ofstream output(argv[2], std::ios::binary);
        if (!output) {
            std::cerr << "Failed to write " << argv[2] << "\n";
            return 1;
        }
        output << cpp;
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << "\n";
        return 1;
    }
    return 0;
}
