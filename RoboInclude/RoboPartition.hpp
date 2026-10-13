#pragma once

#include <stdint.h>

constexpr char ROBO_PARTITION_MAGIC[8] = {'R', 'O', 'B', 'O', 'P', 'R', 'T', '1'};
constexpr char ROBO_FS_MAGIC[8] = {'R', 'O', 'B', 'O', 'F', 'S', '0', '1'};
constexpr uint32_t ROBO_PARTITION_VERSION = 1;
constexpr uint32_t ROBO_FS_VERSION = 1;
constexpr uint32_t ROBO_FS_KIND = 1;
constexpr uint32_t ROBO_ENTRY_FILE = 1;
constexpr uint32_t ROBO_ENTRY_DIRECTORY = 2;
constexpr uint32_t ROBO_SECTOR_BYTES = 512;
constexpr uint32_t ROBO_ENTRY_BYTES = 64;
constexpr uint32_t ROBO_PARTITION_HEADER = 64;
constexpr uint32_t ROBO_PARTITION_NAME = 24;
constexpr uint32_t ROBO_PARTITION_LIMIT = 4;
constexpr uint32_t ROBO_VOLUME_LBA = 2048;
constexpr uint32_t ROBO_DIRECTORY_BYTES = 1024;
constexpr uint32_t ROBO_FS_ALLOC_AT = 72;
constexpr char ROBO_FS_ALLOC_MAGIC[4] = {'R', 'F', 'R', 'E'};
constexpr uint32_t ROBO_FS_NEXT_FREE_AT = 80;
constexpr uint32_t ROBO_FS_FREE_HEAD_AT = 88;
constexpr uint32_t ROBO_FS_SPARE_BLOCKS = 1024;
