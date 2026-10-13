#include "RoboDisk.hpp"

#include "RoboAta.hpp"
#include "RoboPartition.hpp"

bool RoboDiskReadSectors(uint32_t lba, uint32_t count, uint8_t* dest);
uint32_t RoboVirtioBlkCount(void);
bool RoboVirtioBlkRead(uint32_t index, uint32_t lba, uint32_t count, uint8_t* dest);
uint32_t RoboAhciCount(void);
bool RoboAhciReadSectors(uint32_t index, uint32_t lba, uint32_t count, uint8_t* dest);

namespace {

constexpr int ROBO_DISK_NONE = 0;
constexpr int ROBO_DISK_VIRTIO = 1;
constexpr int ROBO_DISK_AHCI = 2;
constexpr int ROBO_DISK_ATA = 3;

int gKind = ROBO_DISK_NONE;
uint32_t gIndex = 0;
int gFallbackKind = ROBO_DISK_NONE;
uint32_t gFallbackIndex = 0;
bool gFallback = false;

bool RoboDiskMagic(const uint8_t* sector) {
    for (uint32_t index = 0; index < 8u; ++index) {
        if (sector[index] != static_cast<uint8_t>(ROBO_PARTITION_MAGIC[index])) {
            return false;
        }
    }
    return true;
}

bool RoboDiskOffer(int kind, uint32_t index) {
    uint8_t sector[ROBO_SECTOR_BYTES];
    gKind = kind;
    gIndex = index;
    if (!RoboDiskReadSectors(0, 1, sector)) {
        gKind = ROBO_DISK_NONE;
        return false;
    }
    if (RoboDiskMagic(sector)) {
        return true;
    }
    if (!gFallback) {
        gFallback = true;
        gFallbackKind = kind;
        gFallbackIndex = index;
    }
    return false;
}

}  // namespace

bool RoboDiskReadSectors(uint32_t lba, uint32_t count, uint8_t* dest) {
    if (dest == nullptr || count == 0) {
        return false;
    }
    uint32_t done = 0;
    while (done < count) {
        uint32_t step = count - done;
        bool ok = false;
        if (gKind == ROBO_DISK_VIRTIO) {
            if (step > 16u) {
                step = 16u;
            }
            ok = RoboVirtioBlkRead(gIndex, lba + done, step, dest + done * ROBO_SECTOR_BYTES);
        } else if (gKind == ROBO_DISK_AHCI) {
            if (step > 32u) {
                step = 32u;
            }
            ok = RoboAhciReadSectors(gIndex, lba + done, step, dest + done * ROBO_SECTOR_BYTES);
        } else if (gKind == ROBO_DISK_ATA) {
            return done == 0 && RoboAtaReadSectors(lba, count, dest);
        } else {
            return false;
        }
        if (!ok) {
            return false;
        }
        done += step;
    }
    return true;
}

bool RoboDiskReadSector(uint32_t lba, uint8_t* sector) {
    return RoboDiskReadSectors(lba, 1, sector);
}

bool RoboDiskStart(void) {
    gKind = ROBO_DISK_NONE;
    gFallback = false;
    uint32_t virtioCount = RoboVirtioBlkCount();
    for (uint32_t index = 0; index < virtioCount; ++index) {
        if (RoboDiskOffer(ROBO_DISK_VIRTIO, index)) {
            return true;
        }
    }
    uint32_t ahciCount = RoboAhciCount();
    for (uint32_t index = 0; index < ahciCount; ++index) {
        if (RoboDiskOffer(ROBO_DISK_AHCI, index)) {
            return true;
        }
    }
    if (RoboAtaStart() && RoboDiskOffer(ROBO_DISK_ATA, 0)) {
        return true;
    }
    if (!gFallback) {
        gKind = ROBO_DISK_NONE;
        return false;
    }
    gKind = gFallbackKind;
    gIndex = gFallbackIndex;
    return true;
}
