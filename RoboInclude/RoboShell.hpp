#pragma once

#include <stdint.h>

struct RoboDesktop;
struct RoboBootInfo;

bool RoboShellStart(RoboDesktop* desktop);
#ifdef __cplusplus
extern "C"
#endif
void RoboDevKitBind(const RoboBootInfo* info);
void RoboShellPointerMove(int32_t x, int32_t y);
void RoboShellPointerDown(int32_t x, int32_t y, bool fromTouch);
void RoboShellPointerContext(int32_t x, int32_t y);
void RoboShellPointerUp(int32_t x, int32_t y);
void RoboShellKey(uint16_t code, bool pressed);
bool RoboShellFlush(RoboDesktop* desktop);
bool RoboShellTick(uint32_t deltaMs);
void RoboShellSetBackground(const char* path);
void RoboShellDamage(int32_t x, int32_t y, int32_t width, int32_t height);
