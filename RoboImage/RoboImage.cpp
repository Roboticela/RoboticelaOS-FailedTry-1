#include "RoboPartition.hpp"
#include "RoboProgram.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

struct RoboNode {
    std::string name;
    bool directory;
    std::vector<std::uint8_t> data;
    std::vector<int> children;
    std::uint64_t block;
};

std::vector<RoboNode> gNodes;

void RoboStore32(std::uint8_t* dest, std::uint32_t value) {
    dest[0] = static_cast<std::uint8_t>(value);
    dest[1] = static_cast<std::uint8_t>(value >> 8);
    dest[2] = static_cast<std::uint8_t>(value >> 16);
    dest[3] = static_cast<std::uint8_t>(value >> 24);
}

void RoboStore64(std::uint8_t* dest, std::uint64_t value) {
    RoboStore32(dest, static_cast<std::uint32_t>(value));
    RoboStore32(dest + 4, static_cast<std::uint32_t>(value >> 32));
}

void RoboStoreName(std::uint8_t* dest, std::uint32_t capacity, const std::string& name) {
    for (std::uint32_t index = 0; index < capacity; ++index) {
        dest[index] = 0;
    }
    std::uint32_t count = static_cast<std::uint32_t>(name.size());
    if (count > capacity) {
        count = capacity;
    }
    for (std::uint32_t index = 0; index < count; ++index) {
        dest[index] = static_cast<std::uint8_t>(name[index]);
    }
}

int RoboAdd(int parent, const std::string& name, bool directory, std::vector<std::uint8_t> data) {
    if (parent >= 0) {
        for (int child : gNodes[static_cast<std::size_t>(parent)].children) {
            if (gNodes[static_cast<std::size_t>(child)].name == name) {
                return child;
            }
        }
    }
    RoboNode node;
    node.name = name;
    node.directory = directory;
    node.data = std::move(data);
    node.block = 0;
    int index = static_cast<int>(gNodes.size());
    gNodes.push_back(std::move(node));
    if (parent >= 0) {
        gNodes[static_cast<std::size_t>(parent)].children.push_back(index);
    }
    return index;
}

int RoboAddText(int parent, const char* name, const char* text) {
    std::vector<std::uint8_t> data(text, text + std::strlen(text));
    return RoboAdd(parent, name, false, std::move(data));
}

int RoboFolder(int parent, const char* path) {
    int current = parent;
    std::string name;
    for (std::size_t index = 0;; ++index) {
        char ch = path[index];
        if (ch == '/' || ch == 0) {
            if (!name.empty()) {
                current = RoboAdd(current, name, true, {});
                name.clear();
            }
            if (ch == 0) {
                break;
            }
        } else {
            name.push_back(ch);
        }
    }
    return current;
}

bool RoboLoad(const std::string& path, std::vector<std::uint8_t>* data) {
    FILE* file = std::fopen(path.c_str(), "rb");
    if (file == nullptr) {
        return false;
    }
    if (std::fseek(file, 0, SEEK_END) != 0) {
        std::fclose(file);
        return false;
    }
    long length = std::ftell(file);
    if (length < 0) {
        std::fclose(file);
        return false;
    }
    std::rewind(file);
    data->resize(static_cast<std::size_t>(length));
    if (length > 0 && std::fread(data->data(), 1, data->size(), file) != data->size()) {
        std::fclose(file);
        return false;
    }
    std::fclose(file);
    return true;
}

void RoboCopy(int folder, const std::string& esp, const char* hostName, const char* diskName) {
    std::vector<std::uint8_t> data;
    if (!RoboLoad(esp + "/" + hostName, &data)) {
        return;
    }
    RoboAdd(folder, diskName, false, std::move(data));
}

void RoboWriteEntry(std::uint8_t* entry, const RoboNode& node) {
    for (std::uint32_t index = 0; index < ROBO_ENTRY_BYTES; ++index) {
        entry[index] = 0;
    }
    RoboStoreName(entry, 40, node.name);
    RoboStore32(entry + 40, node.directory ? ROBO_ENTRY_DIRECTORY : ROBO_ENTRY_FILE);
    RoboStore64(entry + 48, node.block);
    if (node.directory) {
        RoboStore64(entry + 56, ROBO_DIRECTORY_BYTES);
    } else {
        RoboStore64(entry + 56, node.data.size());
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "RoboImage image.img Build/Esp\n");
        return 1;
    }
    const std::string imagePath = argv[1];
    const std::string esp = argv[2];
    gNodes.clear();
    int root = RoboAdd(-1, "", true, {});
    int system = RoboFolder(root, "System");
    int fonts = RoboFolder(system, "Fonts");
    int applications = RoboFolder(root, "Applications");
    int desktop = RoboFolder(root, "Desktop");
    int documents = RoboFolder(root, "Documents");
    int pictures = RoboFolder(root, "Pictures");
    int music = RoboFolder(root, "Music");
    int downloads = RoboFolder(root, "Downloads");
    int videos = RoboFolder(root, "Videos");
    RoboAddText(system, "About",
        "System holds the operating system: the kernel, the fonts, and the volume name Roboticela.\n");
    RoboAddText(applications, "About", "Applications holds programs you can open.\n");
    RoboAddText(desktop, "About", "Desktop holds the screen you see, its picture, and its background.\n");
    RoboAddText(documents, "Notes", "Documents holds writing.\n");
    RoboAddText(pictures, "About", "Pictures holds images.\n");
    RoboAddText(music, "About", "Music holds sound.\n");
    RoboAddText(downloads, "About", "Downloads holds files that arrived.\n");
    RoboAddText(videos, "About", "Videos holds moving pictures.\n");
    RoboCopy(fonts, esp, "INTERR.TTF", "Inter-Regular.ttf");
    RoboCopy(fonts, esp, "INTERM.TTF", "Inter-Medium.ttf");
    RoboCopy(fonts, esp, "INTERS.TTF", "Inter-SemiBold.ttf");
    RoboCopy(fonts, esp, "INTERI.TTF", "Inter-Italic.ttf");
    RoboCopy(applications, esp, "RoboApp.bin", "RoboApp.bin");
    const std::uint8_t x64Code[] = {
        0x48, 0xB8, 0x00, 0x00, 0x03, 0x00, 0xFF, 0x7F, 0x00, 0x00,
        0xC7, 0x00, 0x58, 0x58, 0x58, 0x58,
        0x48, 0xC7, 0xC0, 0x02, 0x00, 0x00, 0x00,
        0x0F, 0x05,
    };
    const std::uint8_t armCode[] = {
        0x00, 0x00, 0x80, 0xD2,
        0x60, 0x00, 0xA0, 0xF2,
        0xE0, 0xFF, 0xCF, 0xF2,
        0x01, 0x0B, 0x8B, 0x52,
        0x01, 0x0B, 0xAB, 0x72,
        0x01, 0x00, 0x00, 0xB9,
        0x48, 0x00, 0x80, 0xD2,
        0x01, 0x00, 0x00, 0xD4,
    };
    auto program = [](std::uint32_t arch, const std::uint8_t* code, std::uint32_t codeBytes) {
        std::vector<std::uint8_t> image(ROBO_PROGRAM_HEADER + codeBytes, 0);
        for (std::uint32_t index = 0; index < 8u; ++index) {
            image[index] = static_cast<std::uint8_t>(ROBO_PROGRAM_MAGIC[index]);
        }
        RoboStore32(image.data() + 8, arch);
        RoboStore32(image.data() + 12, 0);
        RoboStore32(image.data() + 16, codeBytes);
        RoboStore32(image.data() + 20, ROBO_PROGRAM_PAGE);
        for (std::uint32_t index = 0; index < codeBytes; ++index) {
            image[ROBO_PROGRAM_HEADER + index] = code[index];
        }
        return image;
    };
    RoboAdd(applications, "X64.bin", false, program(ROBO_PROGRAM_X64, x64Code, sizeof(x64Code)));
    RoboAdd(applications, "ARM.bin", false, program(ROBO_PROGRAM_ARM64, armCode, sizeof(armCode)));
    RoboCopy(desktop, esp, "DESKTOP.BIN", "Desktop.bin");
    RoboCopy(desktop, esp, "BACKGROU.RJP", "Background.rjp");
    RoboCopy(desktop, esp, "BACKGROU.MP4", "Background.mp4");

    std::uint64_t used = 1;
    for (RoboNode& node : gNodes) {
        if (!node.directory) {
            continue;
        }
        node.block = used;
        used += ROBO_DIRECTORY_BYTES / ROBO_SECTOR_BYTES;
    }
    for (RoboNode& node : gNodes) {
        if (node.directory) {
            continue;
        }
        node.block = used;
        std::uint64_t blocks = (node.data.size() + ROBO_SECTOR_BYTES - 1u) / ROBO_SECTOR_BYTES;
        if (blocks == 0) {
            blocks = 1;
        }
        used += blocks;
    }
    std::uint64_t total = used + ROBO_FS_SPARE_BLOCKS;

    std::vector<std::uint8_t> volume(static_cast<std::size_t>(total * ROBO_SECTOR_BYTES), 0);
    volume[0] = static_cast<std::uint8_t>(ROBO_FS_MAGIC[0]);
    volume[1] = static_cast<std::uint8_t>(ROBO_FS_MAGIC[1]);
    volume[2] = static_cast<std::uint8_t>(ROBO_FS_MAGIC[2]);
    volume[3] = static_cast<std::uint8_t>(ROBO_FS_MAGIC[3]);
    volume[4] = static_cast<std::uint8_t>(ROBO_FS_MAGIC[4]);
    volume[5] = static_cast<std::uint8_t>(ROBO_FS_MAGIC[5]);
    volume[6] = static_cast<std::uint8_t>(ROBO_FS_MAGIC[6]);
    volume[7] = static_cast<std::uint8_t>(ROBO_FS_MAGIC[7]);
    RoboStore32(volume.data() + 8, ROBO_FS_VERSION);
    RoboStore32(volume.data() + 12, ROBO_SECTOR_BYTES);
    RoboStore64(volume.data() + 16, total);
    volume[ROBO_FS_ALLOC_AT] = static_cast<std::uint8_t>(ROBO_FS_ALLOC_MAGIC[0]);
    volume[ROBO_FS_ALLOC_AT + 1] = static_cast<std::uint8_t>(ROBO_FS_ALLOC_MAGIC[1]);
    volume[ROBO_FS_ALLOC_AT + 2] = static_cast<std::uint8_t>(ROBO_FS_ALLOC_MAGIC[2]);
    volume[ROBO_FS_ALLOC_AT + 3] = static_cast<std::uint8_t>(ROBO_FS_ALLOC_MAGIC[3]);
    RoboStore64(volume.data() + ROBO_FS_NEXT_FREE_AT, used);
    RoboStore64(volume.data() + ROBO_FS_FREE_HEAD_AT, 0);
    RoboStore64(volume.data() + 24, gNodes[static_cast<std::size_t>(root)].block);
    RoboStore32(volume.data() + 32, ROBO_ENTRY_BYTES);
    RoboStoreName(volume.data() + 40, 32, "Roboticela");

    for (const RoboNode& node : gNodes) {
        if (!node.directory) {
            continue;
        }
        std::uint8_t* directory = volume.data() + static_cast<std::size_t>(node.block * ROBO_SECTOR_BYTES);
        std::uint32_t slot = 0;
        for (int child : node.children) {
            if (slot >= ROBO_DIRECTORY_BYTES / ROBO_ENTRY_BYTES) {
                std::fprintf(stderr, "A folder has too many entries.\n");
                return 1;
            }
            RoboWriteEntry(directory + slot * ROBO_ENTRY_BYTES, gNodes[static_cast<std::size_t>(child)]);
            slot += 1;
        }
    }
    for (const RoboNode& node : gNodes) {
        if (node.directory || node.data.empty()) {
            continue;
        }
        std::uint8_t* dest = volume.data() + static_cast<std::size_t>(node.block * ROBO_SECTOR_BYTES);
        std::memcpy(dest, node.data.data(), node.data.size());
    }

    std::vector<std::uint8_t> image(static_cast<std::size_t>((ROBO_VOLUME_LBA + total) * ROBO_SECTOR_BYTES), 0);
    for (std::uint32_t index = 0; index < 8u; ++index) {
        image[index] = static_cast<std::uint8_t>(ROBO_PARTITION_MAGIC[index]);
    }
    RoboStore32(image.data() + 8, ROBO_PARTITION_VERSION);
    RoboStore32(image.data() + 12, 1);
    RoboStore32(image.data() + 16, ROBO_SECTOR_BYTES);
    RoboStoreName(image.data() + ROBO_PARTITION_NAME, 32, "Roboticela");
    std::uint8_t* partition = image.data() + ROBO_PARTITION_HEADER;
    RoboStoreName(partition, 32, "Roboticela");
    RoboStore32(partition + 32, ROBO_FS_KIND);
    RoboStore64(partition + 40, ROBO_VOLUME_LBA);
    RoboStore64(partition + 48, total);
    std::memcpy(image.data() + ROBO_VOLUME_LBA * ROBO_SECTOR_BYTES, volume.data(), volume.size());

    FILE* out = std::fopen(imagePath.c_str(), "wb");
    if (out == nullptr) {
        std::fprintf(stderr, "Could not write the disk image.\n");
        return 1;
    }
    if (std::fwrite(image.data(), 1, image.size(), out) != image.size()) {
        std::fclose(out);
        return 1;
    }
    std::fclose(out);
    return 0;
}
