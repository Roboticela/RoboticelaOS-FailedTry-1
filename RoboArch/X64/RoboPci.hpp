#pragma once

#include <stdint.h>

struct RoboPciDevice {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint8_t irq;
    uint64_t bar;
    uint8_t found;
};

uint32_t RoboPciRead32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
void RoboPciWrite32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);
bool RoboPciFindXhci(RoboPciDevice* device);
