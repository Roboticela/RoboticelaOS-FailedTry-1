#pragma once

#include <stdint.h>

struct RoboDesktop;

bool RoboShellStart(RoboDesktop* desktop);
void RoboShellPointerMove(int32_t x, int32_t y);
void RoboShellPointerDown(int32_t x, int32_t y, bool fromTouch);
void RoboShellPointerUp(int32_t x, int32_t y);
void RoboShellKey(uint16_t code, bool pressed);
bool RoboShellFlush(RoboDesktop* desktop);
