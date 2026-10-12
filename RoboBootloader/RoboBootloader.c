#include "RoboEfi.hpp"
#include "RoboBootInfo.hpp"

#define ROBO_EFI_LOADER_DATA 1u
#define ROBO_EFI_ALLOCATE_ADDRESS 2u
#define ROBO_ELF_PT_LOAD 1u
#define ROBO_ELF_MACHINE_X86_64 62u
#define ROBO_GOP_RGBX 0u
#define ROBO_GOP_BGRX 1u
#define ROBO_GOP_BITMASK 2u
#define ROBO_GOP_BLT_ONLY 3u

static EfiGuid gLoadedImageGuid = {0x5B1B31A1u, 0x9562u, 0x11d2u, {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};
static EfiGuid gSimpleFileGuid = {0x964E5B22u, 0x6459u, 0x11d2u, {0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};
static EfiGuid gFileInfoGuid = {0x09576E92u, 0x6d3fu, 0x11d2u, {0x8E, 0x39, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};
static EfiGuid gGopGuid = {0x9042A9DEu, 0x23DCu, 0x4A38u, {0x96, 0xFB, 0x7A, 0xDE, 0xD0, 0x80, 0x51, 0x6A}};

static void RoboPrint(EfiSystemTable* system, uint16_t* text) {
    if (system != 0 && system->conOut != 0 && system->conOut->outputString != 0) {
        system->conOut->outputString(system->conOut, text);
    }
}

static void RoboFail(EfiSystemTable* system, uint16_t* text) {
    RoboPrint(system, text);
    for (;;) {
        __asm__ volatile("hlt");
    }
}

static int RoboPixelFormatFromInfo(const EfiGraphicsOutputModeInformation* info, uint32_t* format) {
    if (info == 0 || info->pixelFormat == ROBO_GOP_BLT_ONLY) {
        return 0;
    }
    if (info->pixelFormat == ROBO_GOP_BGRX) {
        *format = ROBO_PIXEL_BGRX;
        return 1;
    }
    if (info->pixelFormat == ROBO_GOP_RGBX) {
        *format = ROBO_PIXEL_RGBX;
        return 1;
    }
    if (info->pixelFormat == ROBO_GOP_BITMASK) {
        if (info->redMask == 0x00FF0000u && info->blueMask == 0x000000FFu) {
            *format = ROBO_PIXEL_RGBX;
            return 1;
        }
        if (info->redMask == 0x000000FFu && info->blueMask == 0x00FF0000u) {
            *format = ROBO_PIXEL_BGRX;
            return 1;
        }
    }
    return 0;
}

static int RoboCaptureGop(EfiBootServices* boot, RoboBootInfo* info) {
    EfiGraphicsOutputProtocol* gop = 0;
    if (boot->locateProtocol(&gGopGuid, 0, (void**)&gop) != ROBO_EFI_SUCCESS || gop == 0 || gop->mode == 0 || gop->mode->info == 0) {
        return 0;
    }
    uint32_t format = 0;
    if (!RoboPixelFormatFromInfo(gop->mode->info, &format) && gop->queryMode != 0 && gop->setMode != 0) {
        uint32_t mode = 0;
        for (mode = 0; mode < gop->mode->maxMode; mode += 1) {
            uint64_t sizeOfInfo = 0;
            EfiGraphicsOutputModeInformation* modeInfo = 0;
            if (gop->queryMode(gop, mode, &sizeOfInfo, &modeInfo) != ROBO_EFI_SUCCESS) {
                continue;
            }
            if (!RoboPixelFormatFromInfo(modeInfo, &format)) {
                continue;
            }
            if (gop->setMode(gop, mode) == ROBO_EFI_SUCCESS) {
                break;
            }
            format = 0;
        }
    }
    if (gop->mode == 0 || gop->mode->info == 0 || gop->mode->frameBufferBase == 0) {
        return 0;
    }
    if (!RoboPixelFormatFromInfo(gop->mode->info, &format)) {
        return 0;
    }
    if (gop->mode->info->horizontalResolution == 0 || gop->mode->info->verticalResolution == 0 || gop->mode->info->pixelsPerScanLine == 0) {
        return 0;
    }
    info->frameBufferBase = gop->mode->frameBufferBase;
    info->width = gop->mode->info->horizontalResolution;
    info->height = gop->mode->info->verticalResolution;
    info->stride = gop->mode->info->pixelsPerScanLine * 4u;
    info->pixelFormat = format;
    return 1;
}

static uint8_t* RoboLoadFile(EfiBootServices* boot, EfiHandle imageHandle, uint64_t* fileSize) {
    EfiLoadedImageProtocol* loaded = 0;
    if (boot->handleProtocol(imageHandle, &gLoadedImageGuid, (void**)&loaded) != ROBO_EFI_SUCCESS || loaded == 0) {
        return 0;
    }
    EfiSimpleFileSystemProtocol* volume = 0;
    if (boot->handleProtocol(loaded->deviceHandle, &gSimpleFileGuid, (void**)&volume) != ROBO_EFI_SUCCESS || volume == 0) {
        return 0;
    }
    EfiFileProtocol* root = 0;
    if (volume->openVolume(volume, &root) != ROBO_EFI_SUCCESS || root == 0) {
        return 0;
    }
    static uint16_t kernelPath[] = u"\\RoboKernel.elf";
    EfiFileProtocol* file = 0;
    if (root->open(root, &file, kernelPath, 1, 0) != ROBO_EFI_SUCCESS || file == 0) {
        root->close(root);
        return 0;
    }
    uint8_t infoBytes[256];
    uint64_t infoSize = sizeof(infoBytes);
    if (file->getInfo(file, &gFileInfoGuid, &infoSize, infoBytes) != ROBO_EFI_SUCCESS) {
        file->close(file);
        root->close(root);
        return 0;
    }
    EfiFileInfo* fileInfo = (EfiFileInfo*)infoBytes;
    *fileSize = fileInfo->fileSize;
    void* buffer = 0;
    if (*fileSize == 0 || boot->allocatePool(ROBO_EFI_LOADER_DATA, *fileSize, &buffer) != ROBO_EFI_SUCCESS) {
        file->close(file);
        root->close(root);
        return 0;
    }
    uint64_t readSize = *fileSize;
    if (file->read(file, &readSize, buffer) != ROBO_EFI_SUCCESS || readSize != *fileSize) {
        boot->freePool(buffer);
        file->close(file);
        root->close(root);
        return 0;
    }
    file->close(file);
    root->close(root);
    return (uint8_t*)buffer;
}

static uint64_t RoboLoadKernel(EfiBootServices* boot, uint8_t* image, uint64_t imageSize) {
    if (imageSize < sizeof(Elf64Ehdr)) {
        return 0;
    }
    Elf64Ehdr* header = (Elf64Ehdr*)image;
    if (header->ident[0] != 0x7F || header->ident[1] != 'E' || header->ident[2] != 'L' || header->ident[3] != 'F') {
        return 0;
    }
    if (header->ident[4] != 2 || header->machine != ROBO_ELF_MACHINE_X86_64) {
        return 0;
    }
    if (header->phOff + (uint64_t)header->phNum * header->phEntSize > imageSize || header->phEntSize < sizeof(Elf64Phdr)) {
        return 0;
    }
    uint16_t index = 0;
    for (index = 0; index < header->phNum; index += 1) {
        Elf64Phdr* program = (Elf64Phdr*)(image + header->phOff + (uint64_t)index * header->phEntSize);
        if (program->type != ROBO_ELF_PT_LOAD || program->memSize == 0) {
            continue;
        }
        if (program->offset + program->fileSize > imageSize || program->fileSize > program->memSize) {
            return 0;
        }
        uint64_t address = program->vAddr & ~0xFFFull;
        uint64_t end = program->vAddr + program->memSize;
        uint64_t pages = (end - address + 0xFFFu) >> 12;
        if (boot->allocatePages(ROBO_EFI_ALLOCATE_ADDRESS, ROBO_EFI_LOADER_DATA, pages, &address) != ROBO_EFI_SUCCESS) {
            return 0;
        }
        uint8_t* dest = (uint8_t*)(uintptr_t)program->vAddr;
        uint64_t byte = 0;
        for (byte = 0; byte < program->fileSize; byte += 1) {
            dest[byte] = image[program->offset + byte];
        }
        for (byte = program->fileSize; byte < program->memSize; byte += 1) {
            dest[byte] = 0;
        }
    }
    return header->entry;
}

static int RoboExitBoot(EfiBootServices* boot, EfiHandle imageHandle) {
    uint8_t probe[8];
    uint64_t mapSize = sizeof(probe);
    uint64_t mapKey = 0;
    uint64_t descSize = 0;
    uint32_t descVersion = 0;
    EfiStatus status = boot->getMemoryMap(&mapSize, probe, &mapKey, &descSize, &descVersion);
    if (status != ROBO_EFI_BUFFER_TOO_SMALL || mapSize == 0) {
        return 0;
    }
    mapSize += 4096;
    void* map = 0;
    if (boot->allocatePool(ROBO_EFI_LOADER_DATA, mapSize, &map) != ROBO_EFI_SUCCESS) {
        return 0;
    }
    status = boot->getMemoryMap(&mapSize, map, &mapKey, &descSize, &descVersion);
    if (status != ROBO_EFI_SUCCESS) {
        return 0;
    }
    status = boot->exitBootServices(imageHandle, mapKey);
    if (status != ROBO_EFI_SUCCESS) {
        status = boot->getMemoryMap(&mapSize, map, &mapKey, &descSize, &descVersion);
        if (status != ROBO_EFI_SUCCESS) {
            return 0;
        }
        status = boot->exitBootServices(imageHandle, mapKey);
    }
    return status == ROBO_EFI_SUCCESS;
}

typedef void (__attribute__((sysv_abi)) *RoboKernelEntry)(RoboBootInfo* info);

void __chkstk(void) {
}

EfiStatus EfiMain(EfiHandle imageHandle, EfiSystemTable* system) {
    static uint16_t banner[] = u"RoboBootloader\r\n";
    static uint16_t noGop[] = u"RoboGPU framebuffer was not provided by firmware.\r\n";
    static uint16_t noKernel[] = u"RoboKernel.elf is missing from the system partition.\r\n";
    static uint16_t badKernel[] = u"RoboKernel.elf could not be loaded.\r\n";
    static uint16_t noExit[] = u"ExitBootServices failed.\r\n";

    EfiBootServices* boot = system->bootServices;
    boot->setWatchdogTimer(0, 0, 0, 0);
    RoboPrint(system, banner);

    RoboBootInfo info;
    info.frameBufferBase = 0;
    info.width = 0;
    info.height = 0;
    info.stride = 0;
    info.pixelFormat = 0;
    if (!RoboCaptureGop(boot, &info)) {
        RoboFail(system, noGop);
    }

    uint64_t fileSize = 0;
    uint8_t* image = RoboLoadFile(boot, imageHandle, &fileSize);
    if (image == 0) {
        RoboFail(system, noKernel);
    }
    uint64_t entry = RoboLoadKernel(boot, image, fileSize);
    boot->freePool(image);
    if (entry == 0) {
        RoboFail(system, badKernel);
    }
    if (!RoboExitBoot(boot, imageHandle)) {
        RoboFail(system, noExit);
    }

    RoboKernelEntry kernel = (RoboKernelEntry)(uintptr_t)entry;
    kernel(&info);
    for (;;) {
        __asm__ volatile("hlt");
    }
}
