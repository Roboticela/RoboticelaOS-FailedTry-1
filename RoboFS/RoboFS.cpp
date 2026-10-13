#include "RoboFS.hpp"

#include "RoboAta.hpp"
#include "RoboBootInfo.hpp"

namespace {

constexpr uint32_t ROBO_FS_SECTOR = 512;
constexpr uint32_t ROBO_FS_DIR_LIMIT = 512;
constexpr uint32_t ROBO_FS_CLUSTER_LIMIT = 65536;

RoboFsSectorRead gRead = nullptr;
uint32_t gVolumeLba = 0;
uint32_t gBytesPerSector = 0;
uint32_t gSectorsPerCluster = 0;
uint32_t gFatStart = 0;
uint32_t gRootStart = 0;
uint32_t gRootSectors = 0;
uint32_t gDataStart = 0;
uint32_t gRootCluster = 0;
uint8_t gFatKind = 0;
alignas(2) uint8_t gSector[ROBO_FS_SECTOR];
bool gMounted = false;

struct RoboHeldFile {
    uint8_t name[11];
    const uint8_t* data;
    uint32_t size;
};

RoboHeldFile gHeldFiles[ROBO_BOOT_FILE_MAX];
uint32_t gHeldCount = 0;

bool RoboHeldRead(const uint8_t wanted[11], uint8_t* dest, uint32_t capacity, uint32_t* size) {
    for (uint32_t file = 0; file < gHeldCount; ++file) {
        bool same = true;
        for (uint32_t index = 0; index < 11u; ++index) {
            if (gHeldFiles[file].name[index] != wanted[index]) {
                same = false;
                break;
            }
        }
        if (!same || gHeldFiles[file].data == nullptr || gHeldFiles[file].size == 0) {
            continue;
        }
        uint32_t copy = gHeldFiles[file].size < capacity ? gHeldFiles[file].size : capacity;
        const uint8_t* source = gHeldFiles[file].data;
        uint32_t index = 0;
        if ((reinterpret_cast<uintptr_t>(source) & 7u) == 0 && (reinterpret_cast<uintptr_t>(dest) & 7u) == 0) {
            while (index + 8u <= copy) {
                *reinterpret_cast<uint64_t*>(dest + index) = *reinterpret_cast<const uint64_t*>(source + index);
                index += 8u;
            }
        }
        while (index < copy) {
            dest[index] = source[index];
            index += 1u;
        }
        if (size != nullptr) {
            *size = gHeldFiles[file].size;
        }
        return true;
    }
    return false;
}

uint16_t RoboFsLoad16(const uint8_t* bytes) {
    return static_cast<uint16_t>(bytes[0] | (static_cast<uint16_t>(bytes[1]) << 8));
}

uint32_t RoboFsLoad32(const uint8_t* bytes) {
    return static_cast<uint32_t>(bytes[0])
        | (static_cast<uint32_t>(bytes[1]) << 8)
        | (static_cast<uint32_t>(bytes[2]) << 16)
        | (static_cast<uint32_t>(bytes[3]) << 24);
}

bool RoboFsRead(uint32_t lba, uint8_t* sector) {
    if (gRead == nullptr) {
        return false;
    }
    return gRead(lba, sector);
}

bool RoboFsMountVolume(uint32_t volumeLba) {
    if (!RoboFsRead(volumeLba, gSector)) {
        return false;
    }
    if (gSector[510] != 0x55u || gSector[511] != 0xAAu) {
        return false;
    }
    if (gSector[0] != 0xEBu && gSector[0] != 0xE9u) {
        return false;
    }
    uint32_t bytesPerSector = RoboFsLoad16(gSector + 11);
    uint32_t sectorsPerCluster = gSector[13];
    uint32_t reserved = RoboFsLoad16(gSector + 14);
    uint32_t fatCount = gSector[16];
    uint32_t rootEntries = RoboFsLoad16(gSector + 17);
    uint32_t total16 = RoboFsLoad16(gSector + 19);
    uint32_t fatSize16 = RoboFsLoad16(gSector + 22);
    uint32_t total32 = RoboFsLoad32(gSector + 32);
    uint32_t fatSize32 = RoboFsLoad32(gSector + 36);
    uint32_t rootCluster = RoboFsLoad32(gSector + 44);
    if (bytesPerSector != ROBO_FS_SECTOR || sectorsPerCluster == 0 || fatCount == 0 || reserved == 0) {
        return false;
    }
    uint32_t fatSectors = fatSize16 != 0 ? fatSize16 : fatSize32;
    uint32_t total = total16 != 0 ? total16 : total32;
    if (fatSectors == 0 || total <= reserved) {
        return false;
    }
    uint32_t rootSectors = ((rootEntries * 32u) + (ROBO_FS_SECTOR - 1u)) / ROBO_FS_SECTOR;
    uint32_t used = reserved + fatCount * fatSectors + rootSectors;
    if (total <= used) {
        return false;
    }
    uint32_t clusters = (total - used) / sectorsPerCluster;
    uint8_t kind = 32;
    if (fatSize16 != 0) {
        kind = clusters < 4085u ? 12u : 16u;
    }
    gVolumeLba = volumeLba;
    gBytesPerSector = bytesPerSector;
    gSectorsPerCluster = sectorsPerCluster;
    gFatStart = volumeLba + reserved;
    gRootStart = gFatStart + fatCount * fatSectors;
    gRootSectors = rootSectors;
    gDataStart = gRootStart + rootSectors;
    gRootCluster = kind == 32 ? rootCluster : 0;
    gFatKind = kind;
    gMounted = true;
    return true;
}

bool RoboFsNextCluster(uint32_t cluster, uint32_t* next) {
    uint32_t offset = 0;
    if (gFatKind == 12) {
        offset = cluster + (cluster / 2u);
    } else if (gFatKind == 16) {
        offset = cluster * 2u;
    } else {
        offset = cluster * 4u;
    }
    uint32_t sectorIndex = offset / ROBO_FS_SECTOR;
    uint32_t index = offset % ROBO_FS_SECTOR;
    if (!RoboFsRead(gFatStart + sectorIndex, gSector)) {
        return false;
    }
    uint32_t value = 0;
    if (gFatKind == 12) {
        uint32_t high = 0;
        if (index == ROBO_FS_SECTOR - 1u) {
            uint8_t first = gSector[index];
            if (!RoboFsRead(gFatStart + sectorIndex + 1u, gSector)) {
                return false;
            }
            high = gSector[0];
            value = first | (high << 8);
        } else {
            value = gSector[index] | (static_cast<uint32_t>(gSector[index + 1u]) << 8);
        }
        if ((cluster & 1u) != 0) {
            value >>= 4;
        } else {
            value &= 0x0FFFu;
        }
        if (value >= 0x0FF8u) {
            *next = 0;
            return true;
        }
    } else if (gFatKind == 16) {
        value = RoboFsLoad16(gSector + index);
        if (value >= 0xFFF8u) {
            *next = 0;
            return true;
        }
    } else {
        value = RoboFsLoad32(gSector + index) & 0x0FFFFFFFu;
        if (value >= 0x0FFFFFF8u) {
            *next = 0;
            return true;
        }
    }
    if (value < 2u) {
        return false;
    }
    *next = value;
    return true;
}

void RoboFsName83(const char* name, uint8_t out[11]) {
    for (uint32_t index = 0; index < 11u; ++index) {
        out[index] = ' ';
    }
    uint32_t slot = 0;
    bool extension = false;
    for (uint32_t index = 0; name[index] != 0 && index < 64u; ++index) {
        char ch = name[index];
        if (ch == '.') {
            extension = true;
            slot = 8;
            continue;
        }
        if (ch >= 'a' && ch <= 'z') {
            ch = static_cast<char>(ch - 'a' + 'A');
        }
        if (!extension && slot < 8u) {
            out[slot] = static_cast<uint8_t>(ch);
            slot += 1u;
        } else if (extension && slot < 11u) {
            out[slot] = static_cast<uint8_t>(ch);
            slot += 1u;
        }
    }
}

bool RoboFsNameMatches(const uint8_t* entry, const uint8_t wanted[11]) {
    if (entry[0] == 0x00u || entry[0] == 0xE5u || entry[11] == 0x0Fu || (entry[11] & 0x08u) != 0) {
        return false;
    }
    for (uint32_t index = 0; index < 11u; ++index) {
        if (entry[index] != wanted[index]) {
            return false;
        }
    }
    return true;
}

int RoboFsScanDirectory(const uint8_t* sector, const uint8_t wanted[11], uint32_t* cluster, uint32_t* size) {
    for (uint32_t offset = 0; offset + 32u <= ROBO_FS_SECTOR; offset += 32u) {
        const uint8_t* entry = sector + offset;
        if (entry[0] == 0x00u) {
            return -1;
        }
        if (!RoboFsNameMatches(entry, wanted)) {
            continue;
        }
        uint32_t low = RoboFsLoad16(entry + 26);
        uint32_t high = RoboFsLoad16(entry + 20);
        *cluster = low | (high << 16);
        *size = RoboFsLoad32(entry + 28);
        return 1;
    }
    return 0;
}

bool RoboFsFind(const uint8_t wanted[11], uint32_t* cluster, uint32_t* size) {
    if (gFatKind == 32) {
        uint32_t current = gRootCluster;
        for (uint32_t guard = 0; current >= 2u && guard < ROBO_FS_CLUSTER_LIMIT; ++guard) {
            for (uint32_t sector = 0; sector < gSectorsPerCluster; ++sector) {
                uint32_t lba = gDataStart + (current - 2u) * gSectorsPerCluster + sector;
                if (!RoboFsRead(lba, gSector)) {
                    return false;
                }
                int found = RoboFsScanDirectory(gSector, wanted, cluster, size);
                if (found != 0) {
                    return found > 0;
                }
            }
            uint32_t next = 0;
            if (!RoboFsNextCluster(current, &next)) {
                return false;
            }
            current = next;
        }
        return false;
    }
    for (uint32_t sector = 0; sector < gRootSectors && sector < ROBO_FS_DIR_LIMIT; ++sector) {
        if (!RoboFsRead(gRootStart + sector, gSector)) {
            return false;
        }
        int found = RoboFsScanDirectory(gSector, wanted, cluster, size);
        if (found != 0) {
            return found > 0;
        }
    }
    return false;
}

}  // namespace

bool RoboFsStart(RoboFsSectorRead read) {
    gMounted = false;
    gRead = read;
    if (read == nullptr || !RoboFsRead(0, gSector)) {
        return false;
    }
    if ((gSector[0] == 0xEBu || gSector[0] == 0xE9u) && RoboFsMountVolume(0)) {
        return true;
    }
    if (gSector[510] != 0x55u || gSector[511] != 0xAAu) {
        return false;
    }
    uint8_t partitions[64];
    for (uint32_t index = 0; index < 64u; ++index) {
        partitions[index] = gSector[446u + index];
    }
    for (uint32_t index = 0; index < 4u; ++index) {
        const uint8_t* entry = partitions + index * 16u;
        uint32_t lba = RoboFsLoad32(entry + 8);
        if (entry[4] == 0 || lba == 0) {
            continue;
        }
        if (RoboFsMountVolume(lba)) {
            return true;
        }
    }
    return false;
}

void RoboFsUseBootFiles(const RoboBootInfo* info) {
    gHeldCount = 0;
    if (info == nullptr || info->fileCount == 0) {
        return;
    }
    uint32_t count = info->fileCount;
    if (count > ROBO_BOOT_FILE_MAX) {
        count = ROBO_BOOT_FILE_MAX;
    }
    for (uint32_t index = 0; index < count; ++index) {
        const RoboBootFile* file = &info->files[index];
        if (file->address < 0x100000ull || file->size == 0 || file->address >= 0x100000000ull) {
            continue;
        }
        if (file->size > 24u * 1024u * 1024u || file->address + file->size > 0x100000000ull) {
            continue;
        }
        bool named = false;
        for (uint32_t letter = 0; letter < ROBO_BOOT_FILE_NAME; ++letter) {
            if (file->name[letter] == 0) {
                named = letter > 0;
                break;
            }
        }
        if (!named) {
            continue;
        }
        RoboHeldFile* held = &gHeldFiles[gHeldCount];
        RoboFsName83(file->name, held->name);
        held->data = reinterpret_cast<const uint8_t*>(file->address);
        held->size = file->size;
        gHeldCount += 1;
    }
}

bool RoboFsReadFile(const char* name, uint8_t* dest, uint32_t capacity, uint32_t* size) {
    if (name == nullptr || dest == nullptr || capacity == 0) {
        return false;
    }
    uint8_t wanted[11];
    RoboFsName83(name, wanted);
    if (RoboHeldRead(wanted, dest, capacity, size)) {
        return true;
    }
    if (!gMounted) {
        return false;
    }
    uint32_t cluster = 0;
    uint32_t fileSize = 0;
    if (!RoboFsFind(wanted, &cluster, &fileSize)) {
        return false;
    }
    uint32_t wantedBytes = fileSize < capacity ? fileSize : capacity;
    uint32_t written = 0;
    uint32_t guard = 0;
    while (written < wantedBytes && cluster >= 2u && guard < ROBO_FS_CLUSTER_LIMIT) {
        uint32_t run = 1;
        uint32_t last = cluster;
        uint32_t clusterBytes = gSectorsPerCluster * ROBO_FS_SECTOR;
        while (run < 64u && guard + run < ROBO_FS_CLUSTER_LIMIT) {
            if (run * clusterBytes >= wantedBytes - written) {
                break;
            }
            uint32_t next = 0;
            if (!RoboFsNextCluster(last, &next) || next != last + 1u) {
                break;
            }
            last = next;
            run += 1;
        }
        uint32_t lba = gDataStart + (cluster - 2u) * gSectorsPerCluster;
        uint32_t sectorCount = run * gSectorsPerCluster;
        uint32_t byteCount = sectorCount * ROBO_FS_SECTOR;
        uint32_t copy = byteCount > wantedBytes - written ? wantedBytes - written : byteCount;
        if (copy == byteCount && (reinterpret_cast<uint64_t>(dest + written) & 1u) == 0 && gRead == RoboAtaReadSector) {
            if (!RoboAtaReadSectors(lba, sectorCount, dest + written)) {
                return false;
            }
        } else {
            for (uint32_t sector = 0; sector < sectorCount && written < wantedBytes; ++sector) {
                if (!RoboFsRead(lba + sector, gSector)) {
                    return false;
                }
                uint32_t chunk = ROBO_FS_SECTOR;
                if (chunk > wantedBytes - written) {
                    chunk = wantedBytes - written;
                }
                for (uint32_t index = 0; index < chunk; ++index) {
                    dest[written + index] = gSector[index];
                }
                written += chunk;
            }
            copy = 0;
        }
        written += copy;
        uint32_t next = 0;
        if (!RoboFsNextCluster(last, &next)) {
            return false;
        }
        cluster = next;
        guard += run;
    }
    if (written < wantedBytes) {
        return false;
    }
    if (size != nullptr) {
        *size = fileSize;
    }
    return true;
}
