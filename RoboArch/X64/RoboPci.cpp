#include "RoboPci.hpp"

#include "RoboArch.hpp"

namespace {

constexpr uint16_t ROBO_PCI_ADDRESS = 0xCF8;
constexpr uint16_t ROBO_PCI_DATA = 0xCFC;
constexpr uint8_t ROBO_PCI_CLASS_SERIAL = 0x0C;
constexpr uint8_t ROBO_PCI_SUBCLASS_USB = 0x03;
constexpr uint8_t ROBO_PCI_PROG_XHCI = 0x30;
constexpr uint8_t ROBO_PCI_PROG_USB4 = 0x40;
constexpr uint8_t ROBO_PCI_CLASS_BASE = 0x08;
constexpr uint8_t ROBO_PCI_SUBCLASS_OTHER = 0x80;
constexpr uint8_t ROBO_PCI_SUBCLASS_THUNDERBOLT = 0x0A;
constexpr uint16_t ROBO_PCI_VENDOR_INTEL = 0x8086;
constexpr uint32_t ROBO_PCI_USB_BARS[3] = {0xFEB00000u, 0xFEB04000u, 0xFEB08000u};
constexpr uint16_t ROBO_PCI_THUNDERBOLT_IDS[] = {
    0x1137, 0x1138, 0x1575, 0x1577, 0x15BF, 0x15D2, 0x15D9, 0x15DC,
    0x15DD, 0x15DE, 0x15E8, 0x15EB, 0x15EF, 0x15F0, 0x463E, 0x466D,
    0x5780, 0x5781, 0x5786, 0x8A0D, 0x8A17, 0x9A1B, 0x9A1D
};

bool RoboPciThunderboltNhi(uint16_t vendor, uint16_t deviceId) {
    if (vendor != ROBO_PCI_VENDOR_INTEL) {
        return false;
    }
    for (uint32_t index = 0; index < sizeof(ROBO_PCI_THUNDERBOLT_IDS) / sizeof(ROBO_PCI_THUNDERBOLT_IDS[0]); ++index) {
        if (ROBO_PCI_THUNDERBOLT_IDS[index] == deviceId) {
            return true;
        }
    }
    return false;
}

}  // namespace

uint32_t RoboPciRead32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = 0x80000000u
        | (static_cast<uint32_t>(bus) << 16)
        | (static_cast<uint32_t>(device) << 11)
        | (static_cast<uint32_t>(function) << 8)
        | (offset & 0xFCu);
    RoboPortOut32(ROBO_PCI_ADDRESS, address);
    return RoboPortIn32(ROBO_PCI_DATA);
}

void RoboPciWrite32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    uint32_t address = 0x80000000u
        | (static_cast<uint32_t>(bus) << 16)
        | (static_cast<uint32_t>(device) << 11)
        | (static_cast<uint32_t>(function) << 8)
        | (offset & 0xFCu);
    RoboPortOut32(ROBO_PCI_ADDRESS, address);
    RoboPortOut32(ROBO_PCI_DATA, value);
}

bool RoboPciClaimUsb(uint8_t bus, uint8_t slot, uint8_t function, uint32_t barIndex, RoboPciDevice* device) {
    uint32_t bar0 = RoboPciRead32(bus, slot, function, 0x10);
    if ((bar0 & 1u) != 0 || device == nullptr) {
        return false;
    }
    bool bar64 = ((bar0 >> 1) & 3u) == 2u;
    uint32_t bar1 = 0;
    if (bar64) {
        bar1 = RoboPciRead32(bus, slot, function, 0x14);
    }
    uint32_t placed = ROBO_PCI_USB_BARS[barIndex < 3u ? barIndex : 0u];
    uint32_t commandOff = RoboPciRead32(bus, slot, function, 0x04);
    RoboPciWrite32(bus, slot, function, 0x04, commandOff & ~0x7u);
    RoboPciWrite32(bus, slot, function, 0x10, placed | (bar0 & 0xFu));
    if (bar64) {
        RoboPciWrite32(bus, slot, function, 0x14, 0);
    }
    uint64_t bar = placed;
    (void)bar1;
    uint32_t command = RoboPciRead32(bus, slot, function, 0x04);
    RoboPciWrite32(bus, slot, function, 0x04, command | 0x06u);
    uint32_t interrupt = RoboPciRead32(bus, slot, function, 0x3C);
    device->bus = bus;
    device->device = slot;
    device->function = function;
    device->irq = (interrupt & 0xFF00u) != 0 ? static_cast<uint8_t>(interrupt) : 0;
    device->bar = bar;
    device->found = 1;
    return true;
}

bool RoboPciCollectUsb(RoboPciDevice* devices, uint32_t capacity, uint32_t* count, uint8_t* thunderbolt) {
    if (count != nullptr) {
        *count = 0;
    }
    if (thunderbolt != nullptr) {
        *thunderbolt = 0;
    }
    if (devices == nullptr || capacity == 0 || count == nullptr) {
        return false;
    }
    for (uint8_t bus = 0; bus < 8u; ++bus) {
        for (uint8_t slot = 0; slot < 32u; ++slot) {
            uint32_t vendor = RoboPciRead32(bus, slot, 0, 0);
            if ((vendor & 0xFFFFu) == 0xFFFFu) {
                continue;
            }
            uint8_t header = static_cast<uint8_t>(RoboPciRead32(bus, slot, 0, 0x0C) >> 16);
            uint8_t functions = (header & 0x80u) != 0 ? 8u : 1u;
            for (uint8_t function = 0; function < functions; ++function) {
                uint32_t identity = RoboPciRead32(bus, slot, function, 0);
                if ((identity & 0xFFFFu) == 0xFFFFu) {
                    continue;
                }
                uint32_t classRegister = RoboPciRead32(bus, slot, function, 0x08);
                uint8_t classCode = static_cast<uint8_t>(classRegister >> 24);
                uint8_t subclass = static_cast<uint8_t>(classRegister >> 16);
                uint8_t progIf = static_cast<uint8_t>(classRegister >> 8);
                bool usb4 = classCode == ROBO_PCI_CLASS_SERIAL && subclass == ROBO_PCI_SUBCLASS_USB && progIf == ROBO_PCI_PROG_USB4;
                bool nhi = classCode == ROBO_PCI_CLASS_BASE && subclass == ROBO_PCI_SUBCLASS_OTHER
                    && RoboPciThunderboltNhi(static_cast<uint16_t>(identity), static_cast<uint16_t>(identity >> 16));
                bool thunder = classCode == ROBO_PCI_CLASS_SERIAL && subclass == ROBO_PCI_SUBCLASS_THUNDERBOLT;
                if ((usb4 || nhi || thunder) && thunderbolt != nullptr) {
                    *thunderbolt = 1;
                }
                if (classCode != ROBO_PCI_CLASS_SERIAL || subclass != ROBO_PCI_SUBCLASS_USB || progIf != ROBO_PCI_PROG_XHCI) {
                    continue;
                }
                if (*count >= capacity) {
                    continue;
                }
                if (RoboPciClaimUsb(bus, slot, function, *count, &devices[*count])) {
                    *count += 1u;
                }
            }
        }
    }
    return *count != 0;
}

bool RoboPciFindXhci(RoboPciDevice* device) {
    if (device == nullptr) {
        return false;
    }
    device->found = 0;
    RoboPciDevice found[1]{};
    uint32_t count = 0;
    if (!RoboPciCollectUsb(found, 1u, &count, nullptr) || count == 0) {
        return false;
    }
    *device = found[0];
    return device->found != 0;
}
