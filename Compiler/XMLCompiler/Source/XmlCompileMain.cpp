#include "IconPack.hpp"
#include "XmlToCpp.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

int main(int argc, char** argv) {
    if (argc == 4 && std::string(argv[1]) == "--pack") {
        if (!Robo::WriteRuntimeIconPack(argv[2], argv[3])) {
            std::cerr << "Failed to write pack image " << argv[3] << "\n";
            return 1;
        }
        return 0;
    }
    if (argc == 4 && std::string(argv[1]) == "--catalog") {
        if (!Robo::WriteRuntimeCatalog(argv[2], argv[3])) {
            std::cerr << "Failed to write catalog image " << argv[3] << "\n";
            return 1;
        }
        return 0;
    }
    if ((argc == 4 || argc == 5) && std::string(argv[1]) == "--panes") {
        const std::string assets = argc == 5 ? argv[4] : "Assets";
        if (!Robo::WriteRuntimePanes(argv[2], argv[3], assets)) {
            std::cerr << "Failed to write pane image " << argv[3] << "\n";
            return 1;
        }
        return 0;
    }
    if (argc != 3 && argc != 4) {
        std::cerr << "Usage: XmlCompile <input.xml> <output.cpp> [asset-directory]\n";
        std::cerr << "       XmlCompile --pack <pack.txt> <out.PAK>\n";
        std::cerr << "       XmlCompile --catalog <page.xml> <ROBOCAT.PAK>\n";
        std::cerr << "       XmlCompile --panes <page.xml> <ROBOPANE.PAK> [asset-directory]\n";
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
