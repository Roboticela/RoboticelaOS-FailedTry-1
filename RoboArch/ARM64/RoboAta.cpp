#include "RoboAta.hpp"

bool RoboAtaStart(void) {
    return false;
}

bool RoboAtaReadSector(uint32_t lba, uint8_t* sector) {
    (void)lba;
    (void)sector;
    return false;
}

bool RoboAtaReadSectors(uint32_t lba, uint32_t count, uint8_t* dest) {
    (void)lba;
    (void)count;
    (void)dest;
    return false;
}

bool RoboAtaWriteSectors(uint32_t lba, uint32_t count, const uint8_t* source) {
    (void)lba;
    (void)count;
    (void)source;
    return false;
}
