#pragma once

#include <stdint.h>

bool RoboNetStart(void);
int RoboUdpSend(const uint8_t* packet, uint32_t bytes);
int RoboUdpReceive(uint8_t* packet, uint32_t capacity);
int RoboTcpSend(const uint8_t* packet, uint32_t bytes);
int RoboTcpReceive(uint8_t* packet, uint32_t capacity);
