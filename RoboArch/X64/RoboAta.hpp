#pragma once

#include <stdint.h>

bool RoboAtaStart(void);
bool RoboAtaReadSector(uint32_t lba, uint8_t* sector);
