#include "RoboAta.hpp"

#include "RoboArch.hpp"

namespace {

constexpr uint16_t ROBO_ATA_DATA = 0x1F0;
constexpr uint16_t ROBO_ATA_SECTOR_COUNT = 0x1F2;
constexpr uint16_t ROBO_ATA_LBA_LOW = 0x1F3;
constexpr uint16_t ROBO_ATA_LBA_MID = 0x1F4;
constexpr uint16_t ROBO_ATA_LBA_HIGH = 0x1F5;
constexpr uint16_t ROBO_ATA_DRIVE = 0x1F6;
constexpr uint16_t ROBO_ATA_STATUS = 0x1F7;
constexpr uint16_t ROBO_ATA_CONTROL = 0x3F6;
constexpr uint8_t ROBO_ATA_STATUS_ERR = 0x01;
constexpr uint8_t ROBO_ATA_STATUS_DRQ = 0x08;
constexpr uint8_t ROBO_ATA_STATUS_RDY = 0x40;
constexpr uint8_t ROBO_ATA_STATUS_BSY = 0x80;
constexpr uint8_t ROBO_ATA_CMD_READ = 0x20;
constexpr uint8_t ROBO_ATA_CMD_IDENTIFY = 0xEC;

bool gAtaReady = false;

void RoboAtaDelay(void) {
    RoboPortIn8(ROBO_ATA_CONTROL);
    RoboPortIn8(ROBO_ATA_CONTROL);
    RoboPortIn8(ROBO_ATA_CONTROL);
    RoboPortIn8(ROBO_ATA_CONTROL);
}

bool RoboAtaWait(uint8_t mask, uint8_t value) {
    for (uint32_t spin = 0; spin < 200000u; ++spin) {
        uint8_t status = RoboPortIn8(ROBO_ATA_STATUS);
        if (status == 0xFFu) {
            return false;
        }
        if ((status & ROBO_ATA_STATUS_BSY) != 0) {
            continue;
        }
        if ((status & ROBO_ATA_STATUS_ERR) != 0) {
            return false;
        }
        if ((status & mask) == value) {
            return true;
        }
        __asm__ volatile("pause");
    }
    return false;
}

bool RoboAtaReadWords(uint8_t* sector) {
    if (!RoboAtaWait(ROBO_ATA_STATUS_DRQ, ROBO_ATA_STATUS_DRQ)) {
        return false;
    }
    for (uint32_t index = 0; index < 256u; ++index) {
        uint16_t word = RoboPortIn16(ROBO_ATA_DATA);
        sector[index * 2u] = static_cast<uint8_t>(word);
        sector[index * 2u + 1u] = static_cast<uint8_t>(word >> 8);
    }
    return true;
}

}  // namespace

bool RoboAtaStart(void) {
    gAtaReady = false;
    uint8_t status = RoboPortIn8(ROBO_ATA_STATUS);
    if (status == 0x00u || status == 0xFFu) {
        return false;
    }
    RoboPortOut8(ROBO_ATA_DRIVE, 0xE0);
    RoboAtaDelay();
    if (!RoboAtaWait(ROBO_ATA_STATUS_RDY, ROBO_ATA_STATUS_RDY)) {
        return false;
    }
    RoboPortOut8(ROBO_ATA_SECTOR_COUNT, 0);
    RoboPortOut8(ROBO_ATA_LBA_LOW, 0);
    RoboPortOut8(ROBO_ATA_LBA_MID, 0);
    RoboPortOut8(ROBO_ATA_LBA_HIGH, 0);
    RoboPortOut8(ROBO_ATA_STATUS, ROBO_ATA_CMD_IDENTIFY);
    RoboAtaDelay();
    status = RoboPortIn8(ROBO_ATA_STATUS);
    if (status == 0x00u || status == 0xFFu) {
        return false;
    }
    alignas(2) uint8_t discard[512];
    if (!RoboAtaWait(ROBO_ATA_STATUS_DRQ, ROBO_ATA_STATUS_DRQ) || !RoboAtaReadWords(discard)) {
        return false;
    }
    gAtaReady = true;
    return true;
}

bool RoboAtaReadSectors(uint32_t lba, uint32_t count, uint8_t* dest) {
    if (!gAtaReady || dest == nullptr || count == 0 || lba > 0x0FFFFFFFu) {
        return false;
    }
    while (count > 0) {
        uint32_t chunk = count > 128u ? 128u : count;
        if (lba + chunk - 1u > 0x0FFFFFFFu) {
            return false;
        }
        if (!RoboAtaWait(ROBO_ATA_STATUS_RDY, ROBO_ATA_STATUS_RDY)) {
            return false;
        }
        RoboPortOut8(ROBO_ATA_DRIVE, static_cast<uint8_t>(0xE0u | ((lba >> 24) & 0x0Fu)));
        RoboAtaDelay();
        RoboPortOut8(ROBO_ATA_SECTOR_COUNT, static_cast<uint8_t>(chunk));
        RoboPortOut8(ROBO_ATA_LBA_LOW, static_cast<uint8_t>(lba));
        RoboPortOut8(ROBO_ATA_LBA_MID, static_cast<uint8_t>(lba >> 8));
        RoboPortOut8(ROBO_ATA_LBA_HIGH, static_cast<uint8_t>(lba >> 16));
        RoboPortOut8(ROBO_ATA_STATUS, ROBO_ATA_CMD_READ);
        RoboAtaDelay();
        for (uint32_t index = 0; index < chunk; ++index) {
            if (!RoboAtaReadWords(dest)) {
                return false;
            }
            dest += 512u;
        }
        lba += chunk;
        count -= chunk;
    }
    return true;
}

bool RoboAtaReadSector(uint32_t lba, uint8_t* sector) {
    return RoboAtaReadSectors(lba, 1, sector);
}
