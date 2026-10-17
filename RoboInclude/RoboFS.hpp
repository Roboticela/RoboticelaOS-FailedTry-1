#pragma once

#include <stdint.h>

typedef bool (*RoboFsSectorRead)(uint32_t lba, uint8_t* sector);

struct RoboBootInfo;

constexpr int ROBO_FS_OK = 0;
constexpr int ROBO_FS_DENIED = 1;
constexpr int ROBO_FS_MISSING = 2;
constexpr int ROBO_FS_EXISTS = 3;
constexpr int ROBO_FS_FULL = 4;
constexpr int ROBO_FS_IO = 5;
constexpr int ROBO_FS_BUSY = 6;

bool RoboFsStart(RoboFsSectorRead read);
void RoboFsUseBootFiles(const RoboBootInfo* info);
bool RoboFsReadFile(const char* name, uint8_t* dest, uint32_t capacity, uint32_t* size);
int RoboFsList(const char* directory, char* names, int nameStride, int capacity);
int RoboFsReadApp(const char* app, const char* path, uint8_t* dest, uint32_t capacity, uint32_t* size);
int RoboFsCreateFile(const char* app, const char* path, const uint8_t* data, uint32_t size);
int RoboFsWriteFile(const char* app, const char* path, const uint8_t* data, uint32_t size);
int RoboFsCreateDirectory(const char* app, const char* path);
int RoboFsDeleteFile(const char* app, const char* path);
int RoboFsCommit(const char* path, const uint8_t* data, uint32_t size);
int RoboFsRemove(const char* path);
