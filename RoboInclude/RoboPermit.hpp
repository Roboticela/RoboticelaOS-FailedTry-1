#pragma once

#include <stdint.h>

constexpr uint32_t ROBO_PERMIT_READ = 1u;
constexpr uint32_t ROBO_PERMIT_WRITE = 2u;
constexpr uint32_t ROBO_PERMIT_CREATE = 4u;
constexpr uint32_t ROBO_PERMIT_DELETE = 8u;
constexpr uint32_t ROBO_PERMIT_ALL = 15u;

constexpr uint32_t ROBO_SCOPE_FILE = 1u;
constexpr uint32_t ROBO_SCOPE_FOLDER = 2u;
constexpr uint32_t ROBO_SCOPE_DISK = 3u;
constexpr uint32_t ROBO_SCOPE_STORAGE = 4u;
constexpr uint32_t ROBO_PERMIT_DISK_MOUNTED = 0u;

void RoboPermitStart(void);
int RoboPermitGrant(const char* app, uint32_t scope, const char* path, uint32_t disk, uint32_t rights);
int RoboPermitRevoke(const char* app, uint32_t scope, const char* path, uint32_t disk);
bool RoboPermitAllows(const char* app, uint32_t right, const char* path);
uint32_t RoboPermitRights(const char* app, const char* path);
