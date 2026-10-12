#pragma once

#include <stdint.h>

bool RoboPS2MouseStart(void);
void RoboPS2MouseArm(void);
void RoboPS2MouseInterrupt(void);
void RoboPS2MouseDrain(int32_t* dx, int32_t* dy, uint8_t* leftPressed, uint8_t* leftChanged);
bool RoboPS2MouseReadPacket(int32_t* dx, int32_t* dy, uint8_t* leftPressed, uint8_t* leftChanged);
