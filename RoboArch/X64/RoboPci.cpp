#include "RoboPci.hpp"

#include "RoboArch.hpp"

namespace {

constexpr uint16_t ROBO_PCI_ADDRESS = 0xCF8;
constexpr uint16_t ROBO_PCI_DATA = 0xCFC;
constexpr uint8_t ROBO_PCI_CLASS_SERIAL = 0x0C;
constexpr uint8_t ROBO_PCI_SUBCLASS_USB = 0x03;
constexpr uint8_t ROBO_PCI_PROG_XHCI = 0x30;

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

}  // namespace

bool RoboPciFindXhci(RoboPciDevice* device) {
    if (device == nullptr) {
        return false;
    }
    device->found = 0;
    for (uint8_t bus = 0; bus < 8u; ++bus) {
        for (uint8_t slot = 0; slot < 32u; ++slot) {
            uint32_t vendor = RoboPciRead32(bus, slot, 0, 0);
            if ((vendor & 0xFFFFu) == 0xFFFFu) {
                continue;
            }
            uint8_t header = static_cast<uint8_t>(RoboPciRead32(bus, slot, 0, 0x0C) >> 16);
            uint8_t functions = (header & 0x80u) != 0 ? 8u : 1u;
            for (uint8_t function = 0; function < functions; ++function) {
                if (function > 0 && (RoboPciRead32(bus, slot, function, 0) & 0xFFFFu) == 0xFFFFu) {
                    continue;
                }
                uint32_t classRegister = RoboPciRead32(bus, slot, function, 0x08);
                uint8_t classCode = static_cast<uint8_t>(classRegister >> 24);
                uint8_t subclass = static_cast<uint8_t>(classRegister >> 16);
                uint8_t progIf = static_cast<uint8_t>(classRegister >> 8);
                if (classCode != ROBO_PCI_CLASS_SERIAL || subclass != ROBO_PCI_SUBCLASS_USB || progIf != ROBO_PCI_PROG_XHCI) {
                    continue;
                }
                uint32_t bar0 = RoboPciRead32(bus, slot, function, 0x10);
                if ((bar0 & 1u) != 0) {
                    continue;
                }
                bool bar64 = ((bar0 >> 1) & 3u) == 2u;
                uint32_t bar1 = 0;
                if (bar64) {
                    bar1 = RoboPciRead32(bus, slot, function, 0x14);
                }
                uint64_t bar = (bar0 & ~0xFull) | (static_cast<uint64_t>(bar1) << 32);
                if (bar == 0 || bar >= 0x100000000ull) {
                    constexpr uint32_t ROBO_PCI_XHCI_BAR = 0xFEB00000u;
                    uint32_t command = RoboPciRead32(bus, slot, function, 0x04);
                    RoboPciWrite32(bus, slot, function, 0x04, command & ~0x7u);
                    RoboPciWrite32(bus, slot, function, 0x10, ROBO_PCI_XHCI_BAR | (bar0 & 0xFu));
                    if (bar64) {
                        RoboPciWrite32(bus, slot, function, 0x14, 0);
                    }
                    bar = ROBO_PCI_XHCI_BAR;
                }
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
        }
    }
    return false;
}
