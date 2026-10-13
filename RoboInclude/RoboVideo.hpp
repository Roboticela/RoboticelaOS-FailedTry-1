#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void RoboVideoArm(const uint8_t* file, uint32_t fileBytes, int frameCount, int width, int height);
void RoboVideoRequest(int index);
bool RoboVideoCollect(int index, uint8_t* dest, uint32_t destBytes);
void RoboVideoPoll(uint32_t worker);

#ifdef __cplusplus
}
#endif
