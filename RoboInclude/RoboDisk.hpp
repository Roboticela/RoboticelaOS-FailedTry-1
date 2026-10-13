#pragma once

#include <stdint.h>

bool RoboDiskStart(void);
bool RoboDiskReadSector(uint32_t lba, uint8_t* sector);
bool RoboDiskReadSectors(uint32_t lba, uint32_t count, uint8_t* dest);
