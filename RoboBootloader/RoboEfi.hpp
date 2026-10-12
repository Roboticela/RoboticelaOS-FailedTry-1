#pragma once

#include <stdint.h>

typedef uint64_t EfiStatus;
typedef void* EfiHandle;

#define ROBO_EFI_SUCCESS 0ull
#define ROBO_EFI_BUFFER_TOO_SMALL 0x8000000000000005ull

typedef struct EfiTableHeader {
    uint64_t signature;
    uint32_t revision;
    uint32_t headerSize;
    uint32_t crc32;
    uint32_t reserved;
} EfiTableHeader;

typedef struct EfiGuid {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t data4[8];
} EfiGuid;

typedef struct EfiSimpleTextOutput {
    void* reset;
    EfiStatus (*outputString)(struct EfiSimpleTextOutput* self, uint16_t* string);
} EfiSimpleTextOutput;

typedef struct EfiBootServices EfiBootServices;
typedef struct EfiSystemTable EfiSystemTable;

struct EfiSystemTable {
    EfiTableHeader hdr;
    uint16_t* firmwareVendor;
    uint32_t firmwareRevision;
    EfiHandle consoleInHandle;
    void* conIn;
    EfiHandle consoleOutHandle;
    EfiSimpleTextOutput* conOut;
    EfiHandle standardErrorHandle;
    EfiSimpleTextOutput* stdErr;
    void* runtimeServices;
    EfiBootServices* bootServices;
    uint64_t numberOfTableEntries;
    void* configurationTable;
};

typedef struct EfiGraphicsOutputModeInformation {
    uint32_t version;
    uint32_t horizontalResolution;
    uint32_t verticalResolution;
    uint32_t pixelFormat;
    uint32_t redMask;
    uint32_t greenMask;
    uint32_t blueMask;
    uint32_t reservedMask;
    uint32_t pixelsPerScanLine;
} EfiGraphicsOutputModeInformation;

typedef struct EfiGraphicsOutputProtocolMode {
    uint32_t maxMode;
    uint32_t mode;
    EfiGraphicsOutputModeInformation* info;
    uint64_t sizeOfInfo;
    uint64_t frameBufferBase;
    uint64_t frameBufferSize;
} EfiGraphicsOutputProtocolMode;

typedef struct EfiGraphicsOutputProtocol EfiGraphicsOutputProtocol;
struct EfiGraphicsOutputProtocol {
    EfiStatus (*queryMode)(EfiGraphicsOutputProtocol* self, uint32_t modeNumber, uint64_t* sizeOfInfo, EfiGraphicsOutputModeInformation** info);
    EfiStatus (*setMode)(EfiGraphicsOutputProtocol* self, uint32_t modeNumber);
    void* blt;
    EfiGraphicsOutputProtocolMode* mode;
};

typedef struct EfiFileProtocol EfiFileProtocol;
struct EfiFileProtocol {
    uint64_t revision;
    EfiStatus (*open)(EfiFileProtocol* self, EfiFileProtocol** newHandle, uint16_t* fileName, uint64_t openMode, uint64_t attributes);
    EfiStatus (*close)(EfiFileProtocol* self);
    void* deleteFile;
    EfiStatus (*read)(EfiFileProtocol* self, uint64_t* bufferSize, void* buffer);
    void* write;
    void* getPosition;
    void* setPosition;
    EfiStatus (*getInfo)(EfiFileProtocol* self, EfiGuid* informationType, uint64_t* bufferSize, void* buffer);
    void* setInfo;
    void* flush;
};

typedef struct EfiSimpleFileSystemProtocol {
    uint64_t revision;
    EfiStatus (*openVolume)(struct EfiSimpleFileSystemProtocol* self, EfiFileProtocol** root);
} EfiSimpleFileSystemProtocol;

typedef struct EfiLoadedImageProtocol {
    uint32_t revision;
    EfiHandle parentHandle;
    EfiSystemTable* systemTable;
    EfiHandle deviceHandle;
    void* filePath;
    void* reserved;
    uint32_t loadOptionsSize;
    void* loadOptions;
    void* imageBase;
    uint64_t imageSize;
    uint32_t imageCodeType;
    uint32_t imageDataType;
    void* unload;
} EfiLoadedImageProtocol;

typedef struct EfiTime {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t pad1;
    uint32_t nanosecond;
    int16_t timeZone;
    uint8_t daylight;
    uint8_t pad2;
} EfiTime;

typedef struct EfiFileInfo {
    uint64_t size;
    uint64_t fileSize;
    uint64_t physicalSize;
    EfiTime createTime;
    EfiTime lastAccessTime;
    EfiTime modificationTime;
    uint64_t attribute;
    uint16_t fileName[1];
} EfiFileInfo;

struct EfiBootServices {
    EfiTableHeader hdr;
    void* raiseTPL;
    void* restoreTPL;
    EfiStatus (*allocatePages)(uint32_t type, uint32_t memoryType, uint64_t pages, uint64_t* memory);
    EfiStatus (*freePages)(uint64_t memory, uint64_t pages);
    EfiStatus (*getMemoryMap)(uint64_t* memoryMapSize, void* memoryMap, uint64_t* mapKey, uint64_t* descriptorSize, uint32_t* descriptorVersion);
    EfiStatus (*allocatePool)(uint32_t poolType, uint64_t size, void** buffer);
    EfiStatus (*freePool)(void* buffer);
    void* createEvent;
    void* setTimer;
    void* waitForEvent;
    void* signalEvent;
    void* closeEvent;
    void* checkEvent;
    void* installProtocolInterface;
    void* reinstallProtocolInterface;
    void* uninstallProtocolInterface;
    EfiStatus (*handleProtocol)(EfiHandle handle, EfiGuid* protocol, void** interface);
    void* reserved;
    void* registerProtocolNotify;
    void* locateHandle;
    void* locateDevicePath;
    void* installConfigurationTable;
    void* loadImage;
    void* startImage;
    void* exit;
    void* unloadImage;
    EfiStatus (*exitBootServices)(EfiHandle imageHandle, uint64_t mapKey);
    void* getNextMonotonicCount;
    void* stall;
    EfiStatus (*setWatchdogTimer)(uint64_t timeout, uint64_t watchdogCode, uint64_t dataSize, uint16_t* watchdogData);
    void* connectController;
    void* disconnectController;
    void* openProtocol;
    void* closeProtocol;
    void* openProtocolInformation;
    void* protocolsPerHandle;
    void* locateHandleBuffer;
    EfiStatus (*locateProtocol)(EfiGuid* protocol, void* registration, void** interface);
};

typedef struct Elf64Ehdr {
    uint8_t ident[16];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint64_t entry;
    uint64_t phOff;
    uint64_t shOff;
    uint32_t flags;
    uint16_t ehSize;
    uint16_t phEntSize;
    uint16_t phNum;
    uint16_t shEntSize;
    uint16_t shNum;
    uint16_t shStrNdx;
} Elf64Ehdr;

typedef struct Elf64Phdr {
    uint32_t type;
    uint32_t flags;
    uint64_t offset;
    uint64_t vAddr;
    uint64_t pAddr;
    uint64_t fileSize;
    uint64_t memSize;
    uint64_t align;
} Elf64Phdr;
