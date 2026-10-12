#include "RoboGPUVendor.hpp"

#include "RoboArch.hpp"
#include "RoboBootInfo.hpp"
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

}  // namespace

bool RoboGPUVendorBind(RoboGPU* gpu, const RoboBootInfo* info) {
    if (gpu == nullptr || info == nullptr) {
        return false;
    }
    uint64_t bar = RoboQemuVgaBar();
    if (bar == 0 || !RoboVbeReady()) {
        return false;
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
    RoboMemoryMarkUncached(bar, static_cast<uint64_t>(height) * static_cast<uint64_t>(virtWidth) * 4ull);
    return true;
}

void RoboGPUVendorFallback(void) {
}
