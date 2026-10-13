#include "RoboGPUVendor.hpp"

#include "RoboArch.hpp"
#include "RoboBootInfo.hpp"
#include "RoboDesktop.hpp"
#include "RoboGPU.hpp"
#include "RoboMemory.hpp"

namespace {

constexpr uint16_t ROBO_VBE_INDEX = 0x1CE;
constexpr uint16_t ROBO_VBE_DATA = 0x1CF;
constexpr uint16_t ROBO_VBE_ID = 0;
constexpr uint16_t ROBO_VBE_XRES = 1;
constexpr uint16_t ROBO_VBE_YRES = 2;
constexpr uint16_t ROBO_VBE_BPP = 3;
constexpr uint16_t ROBO_VBE_ENABLE = 4;
constexpr uint16_t ROBO_VBE_VIRT_WIDTH = 6;
constexpr uint16_t ROBO_VBE_LFB = 0x40;
constexpr uint16_t ROBO_VBE_ENABLED = 0x01;
constexpr uint16_t ROBO_PCI_VENDOR_QEMU = 0x1234;
constexpr uint16_t ROBO_PCI_DEVICE_VGA = 0x1111;
constexpr uint8_t ROBO_PCI_CLASS_DISPLAY = 0x03;

uint32_t RoboPciRead32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = 0x80000000u
        | (static_cast<uint32_t>(bus) << 16)
        | (static_cast<uint32_t>(device) << 11)
        | (static_cast<uint32_t>(function) << 8)
        | (offset & 0xFCu);
    RoboPortOut32(0xCF8, address);
    return RoboPortIn32(0xCFC);
}

void RoboPciWrite32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    uint32_t address = 0x80000000u
        | (static_cast<uint32_t>(bus) << 16)
        | (static_cast<uint32_t>(device) << 11)
        | (static_cast<uint32_t>(function) << 8)
        | (offset & 0xFCu);
    RoboPortOut32(0xCF8, address);
    RoboPortOut32(0xCFC, value);
}

uint16_t RoboVbeRead(uint16_t index) {
    RoboPortOut16(ROBO_VBE_INDEX, index);
    return RoboPortIn16(ROBO_VBE_DATA);
}

void RoboVbeWrite(uint16_t index, uint16_t value) {
    RoboPortOut16(ROBO_VBE_INDEX, index);
    RoboPortOut16(ROBO_VBE_DATA, value);
}

bool RoboVbeReady(void) {
    uint16_t id = RoboVbeRead(ROBO_VBE_ID);
    if ((id & 0xFFF0u) != 0xB0C0u) {
        return false;
    }
    if (RoboVbeRead(ROBO_VBE_BPP) != 32) {
        return false;
    }
    uint16_t enable = RoboVbeRead(ROBO_VBE_ENABLE);
    if ((enable & ROBO_VBE_ENABLED) == 0 || (enable & ROBO_VBE_LFB) == 0) {
        RoboVbeWrite(ROBO_VBE_ENABLE, static_cast<uint16_t>(enable | ROBO_VBE_ENABLED | ROBO_VBE_LFB));
    }
    return RoboVbeRead(ROBO_VBE_XRES) != 0 && RoboVbeRead(ROBO_VBE_YRES) != 0;
}

uint64_t RoboQemuVgaBar(void) {
    for (uint8_t bus = 0; bus < 8u; ++bus) {
        for (uint8_t slot = 0; slot < 32u; ++slot) {
            uint32_t identity = RoboPciRead32(bus, slot, 0, 0);
            if ((identity & 0xFFFFu) == 0xFFFFu) {
                continue;
            }
            uint8_t header = static_cast<uint8_t>(RoboPciRead32(bus, slot, 0, 0x0C) >> 16);
            uint8_t functions = (header & 0x80u) != 0 ? 8u : 1u;
            for (uint8_t function = 0; function < functions; ++function) {
                identity = RoboPciRead32(bus, slot, function, 0);
                uint16_t vendor = static_cast<uint16_t>(identity);
                uint16_t device = static_cast<uint16_t>(identity >> 16);
                if (vendor != ROBO_PCI_VENDOR_QEMU || device != ROBO_PCI_DEVICE_VGA) {
                    continue;
                }
                uint32_t classRegister = RoboPciRead32(bus, slot, function, 0x08);
                if (static_cast<uint8_t>(classRegister >> 24) != ROBO_PCI_CLASS_DISPLAY) {
                    continue;
                }
                uint32_t bar0 = RoboPciRead32(bus, slot, function, 0x10);
                if ((bar0 & 1u) != 0) {
                    continue;
                }
                uint64_t bar = bar0 & ~0xFull;
                if (bar == 0 || bar >= 0x100000000ull) {
                    continue;
                }
                uint32_t command = RoboPciRead32(bus, slot, function, 0x04);
                RoboPciWrite32(bus, slot, function, 0x04, command | 0x06u);
                return bar;
            }
        }
    }
    return 0;
}

bool RoboDisplayPresent(void) {
    for (uint32_t bus = 0; bus < 256u; ++bus) {
        for (uint32_t device = 0; device < 32u; ++device) {
            for (uint32_t function = 0; function < 8u; ++function) {
                uint32_t vendor = RoboPciRead32(bus, device, function, 0x00);
                if (vendor == 0xFFFFFFFFu || (vendor & 0xFFFFu) == 0xFFFFu) {
                    if (function == 0) {
                        break;
                    }
                    continue;
                }
                uint32_t classCode = RoboPciRead32(bus, device, function, 0x08);
                if (((classCode >> 24) & 0xFFu) == 0x03u) {
                    uint32_t command = RoboPciRead32(bus, device, function, 0x04);
                    RoboPciWrite32(bus, device, function, 0x04, command | 0x06u);
                    return true;
                }
                if (function == 0) {
                    uint32_t header = RoboPciRead32(bus, device, function, 0x0C);
                    if ((header & 0x00800000u) == 0) {
                        break;
                    }
                }
            }
        }
    }
    return false;
}

bool RoboGPUScanout(RoboGPU* gpu, const RoboBootInfo* info) {
    if (info->frameBufferBase == 0 || info->width == 0 || info->height == 0 || info->stride < info->width * 4u) {
        return false;
    }
    if (!RoboDisplayPresent()) {
        return false;
    }
    gpu->base = reinterpret_cast<uint8_t*>(info->frameBufferBase);
    gpu->width = static_cast<int32_t>(info->width);
    gpu->height = static_cast<int32_t>(info->height);
    gpu->stride = static_cast<int32_t>(info->stride);
    gpu->pixelFormat = info->pixelFormat;
    RoboMemoryMarkWriteCombine(info->frameBufferBase, static_cast<uint64_t>(info->height) * info->stride);
    return true;
}

}  // namespace

bool RoboGPUVendorBind(RoboGPU* gpu, const RoboBootInfo* info) {
    if (gpu == nullptr || info == nullptr) {
        return false;
    }
    uint64_t bar = RoboQemuVgaBar();
    if (bar == 0 || !RoboVbeReady()) {
        return RoboGPUScanout(gpu, info);
    }
    uint32_t width = RoboVbeRead(ROBO_VBE_XRES);
    uint32_t height = RoboVbeRead(ROBO_VBE_YRES);
    uint32_t virtWidth = RoboVbeRead(ROBO_VBE_VIRT_WIDTH);
    if (virtWidth < width) {
        virtWidth = width;
    }
    if (bar == info->frameBufferBase) {
        width = info->width;
        height = info->height;
        virtWidth = info->stride / 4u;
    }
    if (width == 0 || height == 0 || virtWidth < width) {
        return false;
    }
    gpu->base = reinterpret_cast<uint8_t*>(bar);
    gpu->width = static_cast<int32_t>(width);
    gpu->height = static_cast<int32_t>(height);
    gpu->stride = static_cast<int32_t>(virtWidth * 4u);
    gpu->pixelFormat = info->pixelFormat;
    RoboMemoryMarkWriteCombine(bar, static_cast<uint64_t>(height) * static_cast<uint64_t>(virtWidth) * 4ull);
    return true;
}

void RoboGPUVendorFallback(void) {
}

constexpr uint32_t ROBO_GPU_CMD_CREATE = 0x0101;
constexpr uint32_t ROBO_GPU_CMD_SCANOUT = 0x0103;
constexpr uint32_t ROBO_GPU_CMD_FLUSH = 0x0104;
constexpr uint32_t ROBO_GPU_CMD_TRANSFER = 0x0105;
constexpr uint32_t ROBO_GPU_CMD_ATTACH = 0x0106;
constexpr uint32_t ROBO_GPU_RESP_OK = 0x1100;
constexpr uint32_t ROBO_GPU_FORMAT_BGRX = 2;
constexpr uint32_t ROBO_GPU_FORMAT_RGBX = 134;
constexpr int ROBO_GPU_MAX_WIDTH = 1920;
constexpr int ROBO_GPU_MAX_HEIGHT = 1080;

struct RoboVirtioCap {
    uint8_t kind;
    uint8_t bar;
    uint32_t offset;
    uint32_t length;
    uint32_t notifyMultiply;
};

struct RoboVqDesc {
    uint64_t address;
    uint32_t length;
    uint16_t flags;
    uint16_t next;
};

struct RoboVqAvail {
    uint16_t flags;
    uint16_t index;
    uint16_t ring[256];
};

struct RoboVqUsed {
    uint16_t flags;
    uint16_t index;
    struct {
        uint32_t id;
        uint32_t length;
    } ring[256];
};

struct RoboGpuHeader {
    uint32_t type;
    uint32_t flags;
    uint64_t fence;
    uint32_t context;
    uint32_t padding;
};

alignas(4096) uint8_t gCard[ROBO_GPU_MAX_WIDTH * ROBO_GPU_MAX_HEIGHT * 4];
alignas(16) RoboVqDesc gDesc[8];
alignas(2) RoboVqAvail gAvail;
alignas(4) RoboVqUsed gUsed;
alignas(8) uint8_t gCommand[64];
alignas(8) uint8_t gResponse[32];
uint8_t* gCommon = nullptr;
uint8_t* gNotify = nullptr;
uint32_t gNotifyMultiply = 0;
uint16_t gQueueSize = 0;
uint16_t gNotifyOff = 0;
int gCardWidth = 0;
int gCardHeight = 0;
uint32_t gCardFormat = 0;
bool gVirtioReady = false;
bool gVirtioFailed = false;
uint16_t gUsedSeen = 0;

uint8_t* RoboVirtioBar(uint8_t bus, uint8_t device, uint8_t function, uint8_t barIndex, uint32_t offset) {
    uint32_t bar = RoboPciRead32(bus, device, function, static_cast<uint8_t>(0x10 + barIndex * 4));
    if ((bar & 1u) != 0 || bar == 0) {
        return nullptr;
    }
    uint64_t address = bar & ~0xFull;
    if ((bar & 4u) != 0) {
        uint32_t high = RoboPciRead32(bus, device, function, static_cast<uint8_t>(0x14 + barIndex * 4));
        address |= static_cast<uint64_t>(high) << 32;
    }
    if (address == 0 || address >= 0x100000000ull) {
        return nullptr;
    }
    RoboMemoryMarkUncached(address, 4096);
    return reinterpret_cast<uint8_t*>(address + offset);
}

bool RoboVirtioFind(uint8_t* bus, uint8_t* device, uint8_t* function, RoboVirtioCap* common, RoboVirtioCap* notify) {
    for (uint32_t busIndex = 0; busIndex < 256u; ++busIndex) {
        for (uint32_t deviceIndex = 0; deviceIndex < 32u; ++deviceIndex) {
            uint32_t vendor = RoboPciRead32(static_cast<uint8_t>(busIndex), static_cast<uint8_t>(deviceIndex), 0, 0x00);
            if ((vendor & 0xFFFFu) != 0x1AF4u) {
                continue;
            }
            uint32_t classCode = RoboPciRead32(static_cast<uint8_t>(busIndex), static_cast<uint8_t>(deviceIndex), 0, 0x08);
            if (((classCode >> 24) & 0xFFu) != ROBO_PCI_CLASS_DISPLAY) {
                continue;
            }
            *bus = static_cast<uint8_t>(busIndex);
            *device = static_cast<uint8_t>(deviceIndex);
            *function = 0;
            uint8_t next = static_cast<uint8_t>(RoboPciRead32(*bus, *device, 0, 0x34) & 0xFFu);
            while (next >= 0x40u) {
                uint32_t first = RoboPciRead32(*bus, *device, 0, next);
                uint32_t second = RoboPciRead32(*bus, *device, 0, static_cast<uint8_t>(next + 4));
                uint32_t third = RoboPciRead32(*bus, *device, 0, static_cast<uint8_t>(next + 8));
                uint8_t vendorCap = static_cast<uint8_t>(first);
                uint8_t capNext = static_cast<uint8_t>(first >> 8);
                uint8_t capKind = static_cast<uint8_t>(first >> 24);
                if (vendorCap == 0x09u && (capKind == 1u || capKind == 2u)) {
                    RoboVirtioCap* cap = capKind == 1u ? common : notify;
                    cap->kind = capKind;
                    cap->bar = static_cast<uint8_t>(second);
                    cap->offset = third;
                    cap->length = RoboPciRead32(*bus, *device, 0, static_cast<uint8_t>(next + 12));
                    if (capKind == 2u) {
                        cap->notifyMultiply = RoboPciRead32(*bus, *device, 0, static_cast<uint8_t>(next + 16));
                    }
                }
                next = capNext;
            }
            return common->bar != 0xFFu && notify->bar != 0xFFu;
        }
    }
    return false;
}

uint8_t RoboVirtioStatus(void) {
    return gCommon[20];
}

void RoboVirtioSetStatus(uint8_t status) {
    gCommon[20] = status;
}

bool RoboVirtioCommand(uint32_t type, const void* extra, uint32_t extraBytes) {
    if (extraBytes > 32u) {
        return false;
    }
    RoboGpuHeader* header = reinterpret_cast<RoboGpuHeader*>(gCommand);
    header->type = type;
    header->flags = 0;
    header->fence = 0;
    header->context = 0;
    header->padding = 0;
    const uint8_t* bytes = static_cast<const uint8_t*>(extra);
    for (uint32_t index = 0; index < extraBytes; ++index) {
        gCommand[sizeof(RoboGpuHeader) + index] = bytes[index];
    }
    gDesc[0].address = reinterpret_cast<uint64_t>(gCommand);
    gDesc[0].length = static_cast<uint32_t>(sizeof(RoboGpuHeader) + extraBytes);
    gDesc[0].flags = 1;
    gDesc[0].next = 1;
    gDesc[1].address = reinterpret_cast<uint64_t>(gResponse);
    gDesc[1].length = sizeof(gResponse);
    gDesc[1].flags = 2;
    gDesc[1].next = 0;
    uint16_t slot = static_cast<uint16_t>(gAvail.index % gQueueSize);
    gAvail.ring[slot] = 0;
    __asm__ volatile("" ::: "memory");
    gAvail.index = static_cast<uint16_t>(gAvail.index + 1u);
    __asm__ volatile("" ::: "memory");
    *reinterpret_cast<volatile uint16_t*>(gNotify + static_cast<uint32_t>(gNotifyOff) * gNotifyMultiply) = 0;
    for (uint32_t spin = 0; spin < 1000000u; ++spin) {
        if (gUsed.index != gUsedSeen) {
            gUsedSeen = gUsed.index;
            RoboGpuHeader* response = reinterpret_cast<RoboGpuHeader*>(gResponse);
            return response->type == ROBO_GPU_RESP_OK;
        }
        __asm__ volatile("pause");
    }
    return false;
}

bool RoboVirtioStart(int width, int height, uint32_t pixelFormat) {
    if (width <= 0 || height <= 0 || width > ROBO_GPU_MAX_WIDTH || height > ROBO_GPU_MAX_HEIGHT) {
        return false;
    }
    uint8_t bus = 0;
    uint8_t device = 0;
    uint8_t function = 0;
    RoboVirtioCap common{};
    RoboVirtioCap notify{};
    common.bar = 0xFF;
    notify.bar = 0xFF;
    if (!RoboVirtioFind(&bus, &device, &function, &common, &notify)) {
        return false;
    }
    uint32_t command = RoboPciRead32(bus, device, function, 0x04);
    RoboPciWrite32(bus, device, function, 0x04, command | 0x06u);
    gCommon = RoboVirtioBar(bus, device, function, common.bar, common.offset);
    gNotify = RoboVirtioBar(bus, device, function, notify.bar, notify.offset);
    gNotifyMultiply = notify.notifyMultiply;
    if (gCommon == nullptr || gNotify == nullptr) {
        return false;
    }
    RoboVirtioSetStatus(0);
    for (uint32_t spin = 0; spin < 100000u && RoboVirtioStatus() != 0; ++spin) {
        __asm__ volatile("pause");
    }
    if (RoboVirtioStatus() != 0) {
        return false;
    }
    RoboVirtioSetStatus(1);
    RoboVirtioSetStatus(3);
    *reinterpret_cast<volatile uint32_t*>(gCommon) = 1;
    uint32_t features = *reinterpret_cast<volatile uint32_t*>(gCommon + 4);
    if ((features & 1u) == 0) {
        return false;
    }
    *reinterpret_cast<volatile uint32_t*>(gCommon + 8) = 1;
    *reinterpret_cast<volatile uint32_t*>(gCommon + 12) = 1u;
    RoboVirtioSetStatus(11);
    if ((RoboVirtioStatus() & 8u) == 0) {
        return false;
    }
    *reinterpret_cast<volatile uint16_t*>(gCommon + 22) = 0;
    gQueueSize = *reinterpret_cast<volatile uint16_t*>(gCommon + 24);
    if (gQueueSize < 2u || gQueueSize > 256u) {
        return false;
    }
    gNotifyOff = *reinterpret_cast<volatile uint16_t*>(gCommon + 30);
    uint64_t descAddress = reinterpret_cast<uint64_t>(gDesc);
    uint64_t availAddress = reinterpret_cast<uint64_t>(&gAvail);
    uint64_t usedAddress = reinterpret_cast<uint64_t>(&gUsed);
    *reinterpret_cast<volatile uint32_t*>(gCommon + 32) = static_cast<uint32_t>(descAddress);
    *reinterpret_cast<volatile uint32_t*>(gCommon + 36) = static_cast<uint32_t>(descAddress >> 32);
    *reinterpret_cast<volatile uint32_t*>(gCommon + 40) = static_cast<uint32_t>(availAddress);
    *reinterpret_cast<volatile uint32_t*>(gCommon + 44) = static_cast<uint32_t>(availAddress >> 32);
    *reinterpret_cast<volatile uint32_t*>(gCommon + 48) = static_cast<uint32_t>(usedAddress);
    *reinterpret_cast<volatile uint32_t*>(gCommon + 52) = static_cast<uint32_t>(usedAddress >> 32);
    *reinterpret_cast<volatile uint16_t*>(gCommon + 28) = 1;
    RoboVirtioSetStatus(15);
    gCardWidth = width;
    gCardHeight = height;
    gCardFormat = pixelFormat == ROBO_PIXEL_RGBX ? ROBO_GPU_FORMAT_BGRX : ROBO_GPU_FORMAT_RGBX;
    uint32_t create[4] = {1u, gCardFormat, static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    if (!RoboVirtioCommand(ROBO_GPU_CMD_CREATE, create, sizeof(create))) {
        return false;
    }
    struct RoboBacking {
        uint32_t resource;
        uint32_t entries;
        uint64_t address;
        uint32_t length;
        uint32_t padding;
    } backing{};
    backing.resource = 1;
    backing.entries = 1;
    backing.address = reinterpret_cast<uint64_t>(gCard);
    backing.length = static_cast<uint32_t>(width) * static_cast<uint32_t>(height) * 4u;
    if (!RoboVirtioCommand(ROBO_GPU_CMD_ATTACH, &backing, sizeof(backing))) {
        return false;
    }
    struct RoboScanout {
        uint32_t x;
        uint32_t y;
        uint32_t width;
        uint32_t height;
        uint32_t scanout;
        uint32_t resource;
    } scanout{};
    scanout.width = static_cast<uint32_t>(width);
    scanout.height = static_cast<uint32_t>(height);
    scanout.resource = 1;
    if (!RoboVirtioCommand(ROBO_GPU_CMD_SCANOUT, &scanout, sizeof(scanout))) {
        return false;
    }
    gVirtioReady = true;
    return true;
}

void RoboCardStore(const uint8_t* pixels, int pixelStride, int x, int y, int width, int height, uint32_t pixelFormat) {
    int32_t cursorX = 0;
    int32_t cursorY = 0;
    int32_t cursorWidth = 0;
    int32_t cursorHeight = 0;
    const bool cursor = RoboDesktopCursorRect(&cursorX, &cursorY, &cursorWidth, &cursorHeight);
    for (int row = 0; row < height; ++row) {
        const int screenY = y + row;
        const uint8_t* from = pixels + static_cast<uint64_t>(row) * static_cast<uint32_t>(pixelStride);
        uint8_t* to = gCard + (static_cast<uint64_t>(screenY) * static_cast<uint32_t>(gCardWidth) + static_cast<uint32_t>(x)) * 4u;
        const bool cursorRow = cursor && screenY >= cursorY && screenY < cursorY + cursorHeight;
        for (int column = 0; column < width; ++column) {
            uint32_t red = from[0];
            uint32_t green = from[1];
            uint32_t blue = from[2];
            uint32_t cursorRgb = 0;
            if (cursorRow && RoboDesktopCursorSample(x + column - cursorX, screenY - cursorY, &cursorRgb)) {
                red = (cursorRgb >> 16) & 0xFFu;
                green = (cursorRgb >> 8) & 0xFFu;
                blue = cursorRgb & 0xFFu;
            }
            uint32_t packed = pixelFormat == ROBO_PIXEL_RGBX ? (red << 16) | (green << 8) | blue : (blue << 16) | (green << 8) | red;
            *reinterpret_cast<uint32_t*>(to) = packed;
            from += 4;
            to += 4;
        }
    }
}

bool RoboCardFlush(int x, int y, int width, int height) {
    struct RoboRectCommand {
        uint32_t x;
        uint32_t y;
        uint32_t width;
        uint32_t height;
        uint64_t offset;
        uint32_t resource;
        uint32_t padding;
    } rect{};
    rect.x = static_cast<uint32_t>(x);
    rect.y = static_cast<uint32_t>(y);
    rect.width = static_cast<uint32_t>(width);
    rect.height = static_cast<uint32_t>(height);
    rect.offset = (static_cast<uint64_t>(y) * static_cast<uint32_t>(gCardWidth) + static_cast<uint32_t>(x)) * 4u;
    rect.resource = 1;
    if (!RoboVirtioCommand(ROBO_GPU_CMD_TRANSFER, &rect, sizeof(rect))) {
        return false;
    }
    struct RoboFlushCommand {
        uint32_t x;
        uint32_t y;
        uint32_t width;
        uint32_t height;
        uint32_t resource;
        uint32_t padding;
    } flush{};
    flush.x = rect.x;
    flush.y = rect.y;
    flush.width = rect.width;
    flush.height = rect.height;
    flush.resource = 1;
    return RoboVirtioCommand(ROBO_GPU_CMD_FLUSH, &flush, sizeof(flush));
}

bool RoboGPUVendorPresent(const uint8_t* pixels, int pixelStride, int x, int y, int width, int height, int surfaceWidth, int surfaceHeight, uint32_t pixelFormat) {
    if (pixels == nullptr || width <= 0 || height <= 0 || pixelStride < width * 4) {
        return false;
    }
    if (!gVirtioReady && !gVirtioFailed) {
        gVirtioFailed = !RoboVirtioStart(surfaceWidth, surfaceHeight, pixelFormat);
    }
    if (!gVirtioReady || surfaceWidth != gCardWidth || surfaceHeight != gCardHeight) {
        return false;
    }
    RoboCardStore(pixels, pixelStride, x, y, width, height, pixelFormat);
    if (!RoboCardFlush(x, y, width, height)) {
        gVirtioReady = false;
        gVirtioFailed = true;
        return false;
    }
    return true;
}

void RoboGPUVendorFlush(void) {
    if (!gVirtioReady) {
        return;
    }
    RoboCardFlush(0, 0, gCardWidth, gCardHeight);
}

uint8_t* RoboGPUVendorCard(int* stride) {
    if (!gVirtioReady) {
        return nullptr;
    }
    if (stride != nullptr) {
        *stride = gCardWidth * 4;
    }
    return gCard;
}

void RoboGPUVendorFlushCursor(int x, int y) {
    if (!gVirtioReady) {
        return;
    }
    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }
    int width = 32;
    int height = 32;
    if (x + width > gCardWidth) {
        width = gCardWidth - x;
    }
    if (y + height > gCardHeight) {
        height = gCardHeight - y;
    }
    if (width > 0 && height > 0) {
        RoboCardFlush(x, y, width, height);
    }
}
