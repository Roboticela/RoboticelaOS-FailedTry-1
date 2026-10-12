#pragma once

struct RoboGPU;
struct RoboBootInfo;

bool RoboGPUVendorBind(RoboGPU* gpu, const RoboBootInfo* info);
void RoboGPUVendorFallback(void);
