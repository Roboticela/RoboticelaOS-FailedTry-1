#include "RoboDisk.hpp"

#include "RoboAta.hpp"
#include "RoboPartition.hpp"

bool RoboDiskReadSectors(uint32_t lba, uint32_t count, uint8_t* dest);
uint32_t RoboVirtioBlkCount(void);
bool RoboVirtioBlkRead(uint32_t index, uint32_t lba, uint32_t count, uint8_t* dest);
bool RoboVirtioBlkWrite(uint32_t index, uint32_t lba, uint32_t count, const uint8_t* source);
uint32_t RoboAhciCount(void);
bool RoboAhciReadSectors(uint32_t index, uint32_t lba, uint32_t count, uint8_t* dest);
bool RoboAhciWriteSectors(uint32_t index, uint32_t lba, uint32_t count, const uint8_t* source);
uint32_t RoboNvmeCount(void);
bool RoboNvmeReadSectors(uint32_t index, uint32_t lba, uint32_t count, uint8_t* dest);
bool RoboNvmeWriteSectors(uint32_t index, uint32_t lba, uint32_t count, const uint8_t* source);

namespace {

constexpr int ROBO_DISK_NONE = 0;
constexpr int ROBO_DISK_VIRTIO = 1;
constexpr int ROBO_DISK_AHCI = 2;
constexpr int ROBO_DISK_ATA = 3;
constexpr int ROBO_DISK_NVME = 4;

int gKind = ROBO_DISK_NONE;
uint32_t gIndex = 0;
int gFallbackKind = ROBO_DISK_NONE;
uint32_t gFallbackIndex = 0;
bool gFallback = false;
int gProbeKind = 0;
uint32_t gProbeIndex = 0;
uint32_t gVirtioProbeCount = 0xFFFFFFFFu;
uint32_t gNvmeProbeCount = 0xFFFFFFFFu;
uint32_t gAhciProbeCount = 0xFFFFFFFFu;
bool gDiskFinished = false;
bool gDiskMounted = false;

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
        } else if (gKind == ROBO_DISK_NVME) {
            if (step > 8u) {
                step = 8u;
            }
            ok = RoboNvmeReadSectors(gIndex, lba + done, step, dest + done * ROBO_SECTOR_BYTES);
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

bool RoboDiskWriteSectors(uint32_t lba, uint32_t count, const uint8_t* source) {
    if (source == nullptr || count == 0) {
        return false;
    }
    uint32_t done = 0;
    while (done < count) {
        uint32_t step = count - done;
        bool ok = false;
        const uint8_t* part = source + done * ROBO_SECTOR_BYTES;
        if (gKind == ROBO_DISK_VIRTIO) {
            if (step > 16u) {
                step = 16u;
            }
            ok = RoboVirtioBlkWrite(gIndex, lba + done, step, part);
        } else if (gKind == ROBO_DISK_AHCI) {
            if (step > 32u) {
                step = 32u;
            }
            ok = RoboAhciWriteSectors(gIndex, lba + done, step, part);
        } else if (gKind == ROBO_DISK_NVME) {
            if (step > 8u) {
                step = 8u;
            }
            ok = RoboNvmeWriteSectors(gIndex, lba + done, step, part);
        } else if (gKind == ROBO_DISK_ATA) {
            return done == 0 && RoboAtaWriteSectors(lba, count, source);
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

bool RoboDiskWriteSector(uint32_t lba, const uint8_t* sector) {
    return RoboDiskWriteSectors(lba, 1, sector);
}

bool RoboDiskAdvance(void) {
    if (gDiskFinished) {
        return true;
    }
    if (gProbeKind == 0) {
        if (gVirtioProbeCount == 0xFFFFFFFFu) {
            gVirtioProbeCount = RoboVirtioBlkCount();
        }
        if (gProbeIndex < gVirtioProbeCount) {
            if (RoboDiskOffer(ROBO_DISK_VIRTIO, gProbeIndex)) {
                gDiskMounted = true;
                gDiskFinished = true;
                return true;
            }
            gProbeIndex += 1u;
            return false;
        }
        gProbeKind = 1;
        gProbeIndex = 0;
        return false;
    }
    if (gProbeKind == 1) {
        if (gNvmeProbeCount == 0xFFFFFFFFu) {
            gNvmeProbeCount = RoboNvmeCount();
        }
        if (gProbeIndex < gNvmeProbeCount) {
            if (RoboDiskOffer(ROBO_DISK_NVME, gProbeIndex)) {
                gDiskMounted = true;
                gDiskFinished = true;
                return true;
            }
            gProbeIndex += 1u;
            return false;
        }
        gProbeKind = 2;
        gProbeIndex = 0;
        return false;
    }
    if (gProbeKind == 2) {
        if (gAhciProbeCount == 0xFFFFFFFFu) {
            gAhciProbeCount = RoboAhciCount();
        }
        if (gProbeIndex < gAhciProbeCount) {
            if (RoboDiskOffer(ROBO_DISK_AHCI, gProbeIndex)) {
                gDiskMounted = true;
                gDiskFinished = true;
                return true;
            }
            gProbeIndex += 1u;
            return false;
        }
        gProbeKind = 3;
        return false;
    }
    if (RoboAtaStart() && RoboDiskOffer(ROBO_DISK_ATA, 0)) {
        gDiskMounted = true;
    } else if (gFallback) {
        gKind = gFallbackKind;
        gIndex = gFallbackIndex;
        gDiskMounted = true;
    } else {
        gKind = ROBO_DISK_NONE;
    }
    gDiskFinished = true;
    return true;
}

bool RoboDiskMounted(void) {
    return gDiskMounted && gKind != ROBO_DISK_NONE;
}

bool RoboDiskStart(void) {
    while (!RoboDiskAdvance()) {
    }
    return RoboDiskMounted();
}
