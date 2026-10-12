#pragma once

#include <stdint.h>

uint8_t RoboPortIn8(uint16_t port);
void RoboPortOut8(uint16_t port, uint8_t value);
uint32_t RoboPortIn32(uint16_t port);
void RoboPortOut32(uint16_t port, uint32_t value);
void RoboArchIrqClaim(uint8_t irq);
void RoboArchCli(void);
void RoboArchSti(void);
void RoboArchIdle(void);
void RoboArchInterruptsStart(void);
void RoboHalt(void);
