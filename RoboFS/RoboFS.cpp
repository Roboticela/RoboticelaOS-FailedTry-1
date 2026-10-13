#include "RoboFS.hpp"

#include "RoboBootInfo.hpp"
#include "RoboDisk.hpp"
#include "RoboPartition.hpp"
#include "RoboPermit.hpp"

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
bool gRoboVolume = false;
uint32_t gRoboBase = 0;
uint64_t gRoboRoot = 1;
uint64_t gBlockCount = 0;
uint64_t gNextFree = 0;
uint64_t gFreeHead = 0;
bool gWritable = false;
uint8_t gDirectory[ROBO_DIRECTORY_BYTES];
uint8_t gSuper[ROBO_SECTOR_BYTES];
uint8_t gIoSector[ROBO_SECTOR_BYTES];

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

uint64_t RoboFsLoad64(const uint8_t* bytes) {
    return static_cast<uint64_t>(RoboFsLoad32(bytes))
        | (static_cast<uint64_t>(RoboFsLoad32(bytes + 4)) << 32);
}

bool RoboFsMagic(const uint8_t* bytes, const char magic[8]) {
    for (uint32_t index = 0; index < 8u; ++index) {
        if (bytes[index] != static_cast<uint8_t>(magic[index])) {
            return false;
        }
    }
    return true;
}

bool RoboNameEqual(const uint8_t* stored, const char* name, uint32_t length) {
    for (uint32_t index = 0; index < 40u; ++index) {
        uint8_t want = index < length ? static_cast<uint8_t>(name[index]) : 0;
        if (stored[index] != want) {
            return false;
        }
    }
    return true;
}

bool RoboDirectoryFind(const uint8_t* directory, uint32_t directoryBytes, const char* name, uint32_t length, uint32_t* kind, uint64_t* start, uint64_t* bytes) {
    if (directoryBytes > ROBO_DIRECTORY_BYTES) {
        directoryBytes = ROBO_DIRECTORY_BYTES;
    }
    for (uint32_t offset = 0; offset + ROBO_ENTRY_BYTES <= directoryBytes; offset += ROBO_ENTRY_BYTES) {
        const uint8_t* entry = directory + offset;
        uint32_t entryKind = RoboFsLoad32(entry + 40);
        if (entryKind == 0) {
            continue;
        }
        if (!RoboNameEqual(entry, name, length)) {
            continue;
        }
        *kind = entryKind;
        *start = RoboFsLoad64(entry + 48);
        *bytes = RoboFsLoad64(entry + 56);
        return true;
    }
    return false;
}

bool RoboReadDirectory(uint64_t block, uint64_t byteSize) {
    if (byteSize == 0 || byteSize > ROBO_DIRECTORY_BYTES) {
        byteSize = ROBO_DIRECTORY_BYTES;
    }
    uint32_t blocks = static_cast<uint32_t>((byteSize + ROBO_SECTOR_BYTES - 1u) / ROBO_SECTOR_BYTES);
    for (uint32_t index = 0; index < blocks; ++index) {
        if (block + index > 0xFFFFFFFFu || gRoboBase + static_cast<uint32_t>(block + index) < gRoboBase) {
            return false;
        }
        if (!RoboFsRead(gRoboBase + static_cast<uint32_t>(block + index), gDirectory + index * ROBO_SECTOR_BYTES)) {
            return false;
        }
    }
    return true;
}

bool RoboVolumeFind(const char* path, uint64_t* start, uint64_t* bytes) {
    uint64_t block = gRoboRoot;
    uint64_t directoryBytes = ROBO_DIRECTORY_BYTES;
    const char* cursor = path;
    if (!RoboReadDirectory(block, directoryBytes)) {
        return false;
    }
    while (cursor[0] != 0) {
        uint32_t length = 0;
        while (cursor[length] != 0 && cursor[length] != '/') {
            length += 1u;
            if (length > 40u) {
                return false;
            }
        }
        if (length == 0) {
            return false;
        }
        uint32_t kind = 0;
        uint64_t next = 0;
        uint64_t size = 0;
        if (!RoboDirectoryFind(gDirectory, static_cast<uint32_t>(directoryBytes), cursor, length, &kind, &next, &size)) {
            return false;
        }
        cursor += length;
        if (cursor[0] == 0) {
            if (kind != ROBO_ENTRY_FILE) {
                return false;
            }
            *start = next;
            *bytes = size;
            return true;
        }
        if (cursor[0] != '/' || kind != ROBO_ENTRY_DIRECTORY) {
            return false;
        }
        cursor += 1u;
        block = next;
        directoryBytes = size == 0 ? ROBO_DIRECTORY_BYTES : size;
        if (!RoboReadDirectory(block, directoryBytes)) {
            return false;
        }
    }
    return false;
}

bool RoboVolumeRead(const char* path, uint8_t* dest, uint32_t capacity, uint32_t* size) {
    uint64_t start = 0;
    uint64_t fileSize = 0;
    if (!RoboVolumeFind(path, &start, &fileSize)) {
        return false;
    }
    uint32_t wanted = fileSize > capacity ? capacity : static_cast<uint32_t>(fileSize);
    uint32_t written = 0;
    while (written < wanted) {
        uint64_t block = start + written / ROBO_SECTOR_BYTES;
        if (block > 0xFFFFFFFFu || !RoboFsRead(gRoboBase + static_cast<uint32_t>(block), gSector)) {
            return false;
        }
        uint32_t chunk = ROBO_SECTOR_BYTES - (written % ROBO_SECTOR_BYTES);
        if (chunk > wanted - written) {
            chunk = wanted - written;
        }
        uint32_t offset = written % ROBO_SECTOR_BYTES;
        for (uint32_t index = 0; index < chunk; ++index) {
            dest[written + index] = gSector[offset + index];
        }
        written += chunk;
    }
    if (size != nullptr) {
        *size = static_cast<uint32_t>(fileSize > 0xFFFFFFFFu ? 0xFFFFFFFFu : fileSize);
    }
    return true;
}

struct RoboAlias {
    const char* from;
    const char* path;
};

const RoboAlias gAliases[] = {
    {"ROBOAPP.BIN", "Applications/RoboApp.bin"},
    {"DESKTOP.BIN", "Desktop/Desktop.bin"},
    {"BACKGROU.RJP", "Desktop/Background.rjp"},
    {"Background.rjp", "Desktop/Background.rjp"},
    {"BACKGROU.MP4", "Desktop/Background.mp4"},
    {"Background.mp4", "Desktop/Background.mp4"},
    {"INTERR.TTF", "System/Fonts/Inter-Regular.ttf"},
    {"Inter-Regular.ttf", "System/Fonts/Inter-Regular.ttf"},
    {"INTERM.TTF", "System/Fonts/Inter-Medium.ttf"},
    {"Inter-Medium.ttf", "System/Fonts/Inter-Medium.ttf"},
    {"INTERS.TTF", "System/Fonts/Inter-SemiBold.ttf"},
    {"Inter-SemiBold.ttf", "System/Fonts/Inter-SemiBold.ttf"},
    {"INTERI.TTF", "System/Fonts/Inter-Italic.ttf"},
    {"Inter-Italic.ttf", "System/Fonts/Inter-Italic.ttf"},
};

const char* RoboVolumeAlias(const char* name) {
    for (uint32_t index = 0; index < sizeof(gAliases) / sizeof(gAliases[0]); ++index) {
        const char* from = gAliases[index].from;
        bool same = true;
        for (uint32_t letter = 0; letter < 64u; ++letter) {
            if (from[letter] != name[letter]) {
                same = false;
                break;
            }
            if (from[letter] == 0) {
                break;
            }
        }
        if (same) {
            return gAliases[index].path;
        }
    }
    return nullptr;
}

bool RoboVolumeMount(const uint8_t* table) {
    if (!RoboFsMagic(table, ROBO_PARTITION_MAGIC)) {
        return false;
    }
    uint32_t count = RoboFsLoad32(table + 12);
    if (count > ROBO_PARTITION_LIMIT) {
        count = ROBO_PARTITION_LIMIT;
    }
    for (uint32_t index = 0; index < count; ++index) {
        const uint8_t* entry = table + ROBO_PARTITION_HEADER + index * ROBO_ENTRY_BYTES;
        if (RoboFsLoad32(entry + 32) != ROBO_FS_KIND) {
            continue;
        }
        uint64_t start = RoboFsLoad64(entry + 40);
        if (start == 0 || start > 0xFFFFFFFFu) {
            continue;
        }
        if (!RoboFsRead(static_cast<uint32_t>(start), gSector)) {
            continue;
        }
        if (!RoboFsMagic(gSector, ROBO_FS_MAGIC)) {
            continue;
        }
        gRoboBase = static_cast<uint32_t>(start);
        gRoboRoot = RoboFsLoad64(gSector + 24);
        gBlockCount = RoboFsLoad64(gSector + 16);
        for (uint32_t byte = 0; byte < ROBO_SECTOR_BYTES; ++byte) {
            gSuper[byte] = gSector[byte];
        }
        gWritable = false;
        gNextFree = 0;
        gFreeHead = 0;
        if (gSuper[ROBO_FS_ALLOC_AT] == static_cast<uint8_t>(ROBO_FS_ALLOC_MAGIC[0])
            && gSuper[ROBO_FS_ALLOC_AT + 1u] == static_cast<uint8_t>(ROBO_FS_ALLOC_MAGIC[1])
            && gSuper[ROBO_FS_ALLOC_AT + 2u] == static_cast<uint8_t>(ROBO_FS_ALLOC_MAGIC[2])
            && gSuper[ROBO_FS_ALLOC_AT + 3u] == static_cast<uint8_t>(ROBO_FS_ALLOC_MAGIC[3])
            && gBlockCount > 1u && gBlockCount <= 0xFFFFFFFFu) {
            gNextFree = RoboFsLoad64(gSuper + ROBO_FS_NEXT_FREE_AT);
            gFreeHead = RoboFsLoad64(gSuper + ROBO_FS_FREE_HEAD_AT);
            gWritable = gNextFree > 0 && gNextFree <= gBlockCount;
        }
        gRoboVolume = true;
        gMounted = true;
        return true;
    }
    return false;
}

void RoboFsStore32(uint8_t* bytes, uint32_t value) {
    bytes[0] = static_cast<uint8_t>(value);
    bytes[1] = static_cast<uint8_t>(value >> 8);
    bytes[2] = static_cast<uint8_t>(value >> 16);
    bytes[3] = static_cast<uint8_t>(value >> 24);
}

void RoboFsStore64(uint8_t* bytes, uint64_t value) {
    RoboFsStore32(bytes, static_cast<uint32_t>(value));
    RoboFsStore32(bytes + 4, static_cast<uint32_t>(value >> 32));
}

void RoboFsZero(uint8_t* bytes, uint32_t count) {
    for (uint32_t index = 0; index < count; ++index) {
        bytes[index] = 0;
    }
}

bool RoboVolumeWriteBlock(uint64_t block, const uint8_t* sector) {
    if (!gRoboVolume || sector == nullptr || block > 0xFFFFFFFFu) {
        return false;
    }
    uint32_t lba = gRoboBase + static_cast<uint32_t>(block);
    if (lba < gRoboBase) {
        return false;
    }
    return RoboDiskWriteSector(lba, sector);
}

bool RoboVolumeReadBlock(uint64_t block) {
    if (!gRoboVolume || block > 0xFFFFFFFFu) {
        return false;
    }
    uint32_t lba = gRoboBase + static_cast<uint32_t>(block);
    if (lba < gRoboBase) {
        return false;
    }
    return RoboFsRead(lba, gIoSector);
}

bool RoboPersistAllocator(void) {
    RoboFsStore64(gSuper + ROBO_FS_NEXT_FREE_AT, gNextFree);
    RoboFsStore64(gSuper + ROBO_FS_FREE_HEAD_AT, gFreeHead);
    return RoboVolumeWriteBlock(0, gSuper);
}

bool RoboAlloc(uint32_t count, uint64_t* start) {
    if (!gWritable || count == 0 || start == nullptr) {
        return false;
    }
    uint64_t previous = 0;
    uint64_t block = gFreeHead;
    for (uint32_t guard = 0; block != 0 && guard < 128u; ++guard) {
        if (block >= gBlockCount || !RoboVolumeReadBlock(block)) {
            return false;
        }
        uint64_t next = RoboFsLoad64(gIoSector);
        uint32_t run = RoboFsLoad32(gIoSector + 8);
        if (run >= count && block + count >= block && block + count <= gBlockCount) {
            uint64_t link = next;
            if (run > count) {
                RoboFsZero(gIoSector, ROBO_SECTOR_BYTES);
                RoboFsStore64(gIoSector, next);
                RoboFsStore32(gIoSector + 8, run - count);
                if (!RoboVolumeWriteBlock(block + count, gIoSector)) {
                    return false;
                }
                link = block + count;
            }
            if (previous == 0) {
                gFreeHead = link;
            } else if (!RoboVolumeReadBlock(previous)) {
                return false;
            } else {
                RoboFsStore64(gIoSector, link);
                if (!RoboVolumeWriteBlock(previous, gIoSector)) {
                    return false;
                }
            }
            if (!RoboPersistAllocator()) {
                return false;
            }
            *start = block;
            return true;
        }
        previous = block;
        block = next;
    }
    if (gNextFree + count < gNextFree || gNextFree + count > gBlockCount) {
        return false;
    }
    uint64_t saved = gNextFree;
    gNextFree += count;
    if (!RoboPersistAllocator()) {
        gNextFree = saved;
        return false;
    }
    *start = saved;
    return true;
}

bool RoboFree(uint64_t start, uint32_t count) {
    if (!gWritable || count == 0 || start == 0 || start + count < start || start + count > gBlockCount) {
        return false;
    }
    RoboFsZero(gIoSector, ROBO_SECTOR_BYTES);
    RoboFsStore64(gIoSector, gFreeHead);
    RoboFsStore32(gIoSector + 8, count);
    if (!RoboVolumeWriteBlock(start, gIoSector)) {
        return false;
    }
    uint64_t saved = gFreeHead;
    gFreeHead = start;
    if (!RoboPersistAllocator()) {
        gFreeHead = saved;
        return false;
    }
    return true;
}

bool RoboWriteBytes(uint64_t start, const uint8_t* data, uint32_t size) {
    uint32_t blocks = size == 0 ? 1u : (size + ROBO_SECTOR_BYTES - 1u) / ROBO_SECTOR_BYTES;
    for (uint32_t index = 0; index < blocks; ++index) {
        RoboFsZero(gIoSector, ROBO_SECTOR_BYTES);
        uint32_t offset = index * ROBO_SECTOR_BYTES;
        if (data != nullptr && offset < size) {
            uint32_t chunk = size - offset;
            if (chunk > ROBO_SECTOR_BYTES) {
                chunk = ROBO_SECTOR_BYTES;
            }
            for (uint32_t byte = 0; byte < chunk; ++byte) {
                gIoSector[byte] = data[offset + byte];
            }
        }
        if (!RoboVolumeWriteBlock(start + index, gIoSector)) {
            return false;
        }
    }
    return true;
}

bool RoboWriteDirectory(uint64_t block, uint64_t byteSize) {
    if (byteSize == 0 || byteSize > ROBO_DIRECTORY_BYTES) {
        byteSize = ROBO_DIRECTORY_BYTES;
    }
    uint32_t blocks = static_cast<uint32_t>((byteSize + ROBO_SECTOR_BYTES - 1u) / ROBO_SECTOR_BYTES);
    for (uint32_t index = 0; index < blocks; ++index) {
        if (!RoboVolumeWriteBlock(block + index, gDirectory + index * ROBO_SECTOR_BYTES)) {
            return false;
        }
    }
    return true;
}

bool RoboDirectoryEmpty(uint64_t block, uint64_t byteSize) {
    if (byteSize == 0 || byteSize > ROBO_DIRECTORY_BYTES) {
        byteSize = ROBO_DIRECTORY_BYTES;
    }
    uint32_t blocks = static_cast<uint32_t>((byteSize + ROBO_SECTOR_BYTES - 1u) / ROBO_SECTOR_BYTES);
    for (uint32_t index = 0; index < blocks; ++index) {
        if (!RoboVolumeReadBlock(block + index)) {
            return false;
        }
        for (uint32_t offset = 0; offset + ROBO_ENTRY_BYTES <= ROBO_SECTOR_BYTES; offset += ROBO_ENTRY_BYTES) {
            if (RoboFsLoad32(gIoSector + offset + 40) != 0) {
                return false;
            }
        }
    }
    return true;
}

struct RoboPlace {
    uint64_t parent;
    uint64_t parentBytes;
    uint32_t slot;
    uint32_t kind;
    uint64_t start;
    uint64_t bytes;
    char name[40];
    uint32_t nameLength;
    bool found;
};

bool RoboBadName(const char* name, uint32_t length) {
    if (length == 1 && name[0] == '.') {
        return true;
    }
    return length == 2 && name[0] == '.' && name[1] == '.';
}

int RoboScan(bool wantExisting, const char* name, uint32_t length, RoboPlace* place) {
    bool sawHole = false;
    uint32_t hole = 0;
    uint32_t limit = place->parentBytes > ROBO_DIRECTORY_BYTES ? ROBO_DIRECTORY_BYTES : static_cast<uint32_t>(place->parentBytes);
    for (uint32_t offset = 0; offset + ROBO_ENTRY_BYTES <= limit; offset += ROBO_ENTRY_BYTES) {
        uint32_t kind = RoboFsLoad32(gDirectory + offset + 40);
        if (kind == 0) {
            if (!sawHole) {
                sawHole = true;
                hole = offset;
            }
            continue;
        }
        if (!RoboNameEqual(gDirectory + offset, name, length)) {
            continue;
        }
        place->found = true;
        place->slot = offset;
        place->kind = kind;
        place->start = RoboFsLoad64(gDirectory + offset + 48);
        place->bytes = RoboFsLoad64(gDirectory + offset + 56);
        return wantExisting ? ROBO_FS_OK : ROBO_FS_EXISTS;
    }
    if (wantExisting) {
        return ROBO_FS_MISSING;
    }
    if (!sawHole) {
        return ROBO_FS_FULL;
    }
    place->found = false;
    place->slot = hole;
    place->nameLength = length;
    RoboFsZero(reinterpret_cast<uint8_t*>(place->name), 40);
    for (uint32_t index = 0; index < length; ++index) {
        place->name[index] = name[index];
    }
    return ROBO_FS_OK;
}

int RoboWalk(const char* path, bool wantExisting, RoboPlace* place) {
    if (path == nullptr || path[0] == 0 || path[0] == '/') {
        return ROBO_FS_IO;
    }
    uint32_t pathLength = 0;
    while (path[pathLength] != 0) {
        pathLength += 1u;
        if (pathLength > 95u) {
            return ROBO_FS_IO;
        }
    }
    uint64_t block = gRoboRoot;
    uint64_t bytes = ROBO_DIRECTORY_BYTES;
    const char* cursor = path;
    if (!RoboReadDirectory(block, bytes)) {
        return ROBO_FS_IO;
    }
    while (cursor[0] != 0) {
        uint32_t length = 0;
        while (cursor[length] != 0 && cursor[length] != '/') {
            if (cursor[length] == '\\') {
                return ROBO_FS_IO;
            }
            length += 1u;
            if (length > 40u) {
                return ROBO_FS_IO;
            }
        }
        if (length == 0 || RoboBadName(cursor, length)) {
            return ROBO_FS_IO;
        }
        bool last = cursor[length] == 0;
        place->parent = block;
        place->parentBytes = bytes;
        int status = RoboScan(last ? wantExisting : true, cursor, length, place);
        if (!last) {
            if (status != ROBO_FS_OK || place->kind != ROBO_ENTRY_DIRECTORY) {
                return ROBO_FS_MISSING;
            }
            cursor += length + 1u;
            if (cursor[0] == 0) {
                return ROBO_FS_IO;
            }
            block = place->start;
            bytes = place->bytes == 0 ? ROBO_DIRECTORY_BYTES : place->bytes;
            if (!RoboReadDirectory(block, bytes)) {
                return ROBO_FS_IO;
            }
            continue;
        }
        return status;
    }
    return ROBO_FS_IO;
}

void RoboFillEntry(uint8_t* entry, const RoboPlace* place, uint32_t kind, uint64_t start, uint64_t bytes) {
    RoboFsZero(entry, ROBO_ENTRY_BYTES);
    for (uint32_t index = 0; index < place->nameLength && index < 40u; ++index) {
        entry[index] = static_cast<uint8_t>(place->name[index]);
    }
    RoboFsStore32(entry + 40, kind);
    RoboFsStore64(entry + 48, start);
    RoboFsStore64(entry + 56, bytes);
}

int RoboFsCreateBlocks(const char* path, uint32_t kind, const uint8_t* data, uint32_t size) {
    if (!gRoboVolume || !gWritable) {
        return ROBO_FS_IO;
    }
    RoboPlace place{};
    int status = RoboWalk(path, false, &place);
    if (status != ROBO_FS_OK) {
        return status;
    }
    uint32_t blocks = kind == ROBO_ENTRY_DIRECTORY ? 2u : (size + ROBO_SECTOR_BYTES - 1u) / ROBO_SECTOR_BYTES;
    if (blocks == 0) {
        blocks = 1;
    }
    if (place.slot + ROBO_ENTRY_BYTES > ROBO_DIRECTORY_BYTES) {
        return ROBO_FS_FULL;
    }
    uint64_t start = 0;
    if (!RoboAlloc(blocks, &start)) {
        return ROBO_FS_FULL;
    }
    bool stored = true;
    if (kind == ROBO_ENTRY_DIRECTORY) {
        RoboFsZero(gIoSector, ROBO_SECTOR_BYTES);
        stored = RoboVolumeWriteBlock(start, gIoSector) && RoboVolumeWriteBlock(start + 1u, gIoSector);
    } else {
        stored = RoboWriteBytes(start, data, size);
    }
    if (!stored) {
        RoboFree(start, blocks);
        return ROBO_FS_IO;
    }
    RoboFillEntry(gDirectory + place.slot, &place, kind, start, kind == ROBO_ENTRY_DIRECTORY ? ROBO_DIRECTORY_BYTES : size);
    if (!RoboWriteDirectory(place.parent, place.parentBytes)) {
        RoboFree(start, blocks);
        return ROBO_FS_IO;
    }
    return ROBO_FS_OK;
}

uint32_t RoboBlockCount(uint64_t bytes) {
    if (bytes == 0) {
        return 1;
    }
    uint64_t blocks = (bytes + ROBO_SECTOR_BYTES - 1u) / ROBO_SECTOR_BYTES;
    if (blocks == 0 || blocks > 0xFFFFFFFFu) {
        return 1;
    }
    return static_cast<uint32_t>(blocks);
}

int RoboFsReplace(const char* path, const uint8_t* data, uint32_t size) {
    if (!gRoboVolume || !gWritable) {
        return ROBO_FS_IO;
    }
    RoboPlace place{};
    int status = RoboWalk(path, true, &place);
    if (status != ROBO_FS_OK) {
        return status;
    }
    if (place.kind != ROBO_ENTRY_FILE) {
        return ROBO_FS_IO;
    }
    uint32_t newBlocks = size == 0 ? 1u : (size + ROBO_SECTOR_BYTES - 1u) / ROBO_SECTOR_BYTES;
    uint32_t oldBlocks = RoboBlockCount(place.bytes);
    if (newBlocks <= oldBlocks) {
        if (!RoboWriteBytes(place.start, data, size)) {
            return ROBO_FS_IO;
        }
        RoboFsStore64(gDirectory + place.slot + 56, size);
        if (!RoboWriteDirectory(place.parent, place.parentBytes)) {
            return ROBO_FS_IO;
        }
        if (newBlocks < oldBlocks && !RoboFree(place.start + newBlocks, oldBlocks - newBlocks)) {
            return ROBO_FS_IO;
        }
        return ROBO_FS_OK;
    }
    uint64_t start = 0;
    if (!RoboAlloc(newBlocks, &start)) {
        return ROBO_FS_FULL;
    }
    if (!RoboWriteBytes(start, data, size)) {
        RoboFree(start, newBlocks);
        return ROBO_FS_IO;
    }
    RoboFsStore64(gDirectory + place.slot + 48, start);
    RoboFsStore64(gDirectory + place.slot + 56, size);
    if (!RoboWriteDirectory(place.parent, place.parentBytes)) {
        RoboFree(start, newBlocks);
        return ROBO_FS_IO;
    }
    RoboFree(place.start, oldBlocks);
    return ROBO_FS_OK;
}

int RoboFsDeleteRaw(const char* path) {
    if (!gRoboVolume || !gWritable) {
        return ROBO_FS_IO;
    }
    RoboPlace place{};
    int status = RoboWalk(path, true, &place);
    if (status != ROBO_FS_OK) {
        return status;
    }
    uint32_t blocks = place.kind == ROBO_ENTRY_DIRECTORY ? 2u : RoboBlockCount(place.bytes);
    if (place.kind == ROBO_ENTRY_DIRECTORY) {
        if (place.bytes != 0) {
            blocks = RoboBlockCount(place.bytes);
        }
        if (!RoboDirectoryEmpty(place.start, place.bytes)) {
            return ROBO_FS_BUSY;
        }
    }
    RoboFsZero(gDirectory + place.slot, ROBO_ENTRY_BYTES);
    if (!RoboWriteDirectory(place.parent, place.parentBytes)) {
        return ROBO_FS_IO;
    }
    if (!RoboFree(place.start, blocks)) {
        return ROBO_FS_IO;
    }
    return ROBO_FS_OK;
}

}  // namespace

bool RoboFsStart(RoboFsSectorRead read) {
    gMounted = false;
    gRoboVolume = false;
    gRead = read;
    if (read == nullptr || !RoboFsRead(0, gSector)) {
        return false;
    }
    uint8_t table[ROBO_SECTOR_BYTES];
    for (uint32_t index = 0; index < ROBO_SECTOR_BYTES; ++index) {
        table[index] = gSector[index];
    }
    if (RoboVolumeMount(table)) {
        RoboPermitStart();
        return true;
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
    bool nested = false;
    for (uint32_t index = 0; name[index] != 0 && index < 160u; ++index) {
        if (name[index] == '/') {
            nested = true;
            break;
        }
    }
    if (nested) {
        return gRoboVolume && RoboVolumeRead(name, dest, capacity, size);
    }
    uint8_t wanted[11];
    RoboFsName83(name, wanted);
    if (RoboHeldRead(wanted, dest, capacity, size)) {
        return true;
    }
    if (gRoboVolume) {
        const char* alias = RoboVolumeAlias(name);
        if (alias != nullptr) {
            return RoboVolumeRead(alias, dest, capacity, size);
        }
        return RoboVolumeRead(name, dest, capacity, size);
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
        if (copy == byteCount && (reinterpret_cast<uint64_t>(dest + written) & 1u) == 0 && gRead == RoboDiskReadSector) {
            if (!RoboDiskReadSectors(lba, sectorCount, dest + written)) {
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

bool RoboFsProtected(const char* path) {
    const char* ledger = "System/Permissions";
    if (path == nullptr) {
        return false;
    }
    for (uint32_t index = 0; index < 32u; ++index) {
        if (ledger[index] != path[index]) {
            return false;
        }
        if (ledger[index] == 0) {
            return true;
        }
    }
    return false;
}

int RoboFsReadApp(const char* app, const char* path, uint8_t* dest, uint32_t capacity, uint32_t* size) {
    if (dest == nullptr || capacity == 0 || RoboFsProtected(path) || !RoboPermitAllows(app, ROBO_PERMIT_READ, path)) {
        return ROBO_FS_DENIED;
    }
    uint32_t fileSize = 0;
    if (!RoboFsReadFile(path, dest, capacity, &fileSize)) {
        return ROBO_FS_MISSING;
    }
    if (size != nullptr) {
        *size = fileSize;
    }
    return ROBO_FS_OK;
}

int RoboFsCreateFile(const char* app, const char* path, const uint8_t* data, uint32_t size) {
    if (RoboFsProtected(path) || !RoboPermitAllows(app, ROBO_PERMIT_CREATE, path)) {
        return ROBO_FS_DENIED;
    }
    if (size > 0 && data == nullptr) {
        return ROBO_FS_IO;
    }
    return RoboFsCreateBlocks(path, ROBO_ENTRY_FILE, data, size);
}

int RoboFsWriteFile(const char* app, const char* path, const uint8_t* data, uint32_t size) {
    if (RoboFsProtected(path) || !RoboPermitAllows(app, ROBO_PERMIT_WRITE, path)) {
        return ROBO_FS_DENIED;
    }
    if (size > 0 && data == nullptr) {
        return ROBO_FS_IO;
    }
    return RoboFsReplace(path, data, size);
}

int RoboFsCreateDirectory(const char* app, const char* path) {
    if (RoboFsProtected(path) || !RoboPermitAllows(app, ROBO_PERMIT_CREATE, path)) {
        return ROBO_FS_DENIED;
    }
    return RoboFsCreateBlocks(path, ROBO_ENTRY_DIRECTORY, nullptr, 0);
}

int RoboFsDeleteFile(const char* app, const char* path) {
    if (RoboFsProtected(path) || !RoboPermitAllows(app, ROBO_PERMIT_DELETE, path)) {
        return ROBO_FS_DENIED;
    }
    return RoboFsDeleteRaw(path);
}

int RoboFsCommit(const char* path, const uint8_t* data, uint32_t size) {
    if (path == nullptr || (size > 0 && data == nullptr)) {
        return ROBO_FS_IO;
    }
    int status = RoboFsReplace(path, data, size);
    if (status == ROBO_FS_MISSING) {
        return RoboFsCreateBlocks(path, ROBO_ENTRY_FILE, data, size);
    }
    return status;
}

int RoboFsRemove(const char* path) {
    return RoboFsDeleteRaw(path);
}
