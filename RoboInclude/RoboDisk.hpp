#pragma once

#include <stdint.h>

bool RoboDiskStart(void);
bool RoboDiskReadSector(uint32_t lba, uint8_t* sector);
bool RoboDiskReadSectors(uint32_t lba, uint32_t count, uint8_t* dest);
bool RoboDiskWriteSector(uint32_t lba, const uint8_t* sector);
bool RoboDiskWriteSectors(uint32_t lba, uint32_t count, const uint8_t* source);
