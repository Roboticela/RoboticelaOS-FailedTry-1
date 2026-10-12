#pragma once

#include <stdint.h>

uint8_t RoboPortIn8(uint16_t port);
void RoboPortOut8(uint16_t port, uint8_t value);
void RoboArchCli(void);
void RoboArchSti(void);
void RoboArchIdle(void);
void RoboArchInterruptsStart(void);
void RoboHalt(void);
