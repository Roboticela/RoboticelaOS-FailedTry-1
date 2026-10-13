#pragma once

#include <stdint.h>

bool RoboAtaStart(void);
bool RoboAtaReadSector(uint32_t lba, uint8_t* sector);
bool RoboAtaReadSectors(uint32_t lba, uint32_t count, uint8_t* dest);
bool RoboAtaWriteSectors(uint32_t lba, uint32_t count, const uint8_t* source);
