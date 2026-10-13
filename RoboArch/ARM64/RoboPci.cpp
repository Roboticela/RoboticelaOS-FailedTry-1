#include "RoboPci.hpp"

namespace {

constexpr uint64_t ROBO_PCI_ECAM = 0x3F000000ull;

}  // namespace

uint32_t RoboPciRead32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    const uint64_t address = ROBO_PCI_ECAM
        + static_cast<uint64_t>(bus) * 0x100000ull
        + static_cast<uint64_t>(device) * 0x8000ull
        + static_cast<uint64_t>(function) * 0x1000ull
        + (offset & 0xFCu);
    return *reinterpret_cast<volatile uint32_t*>(address);
}

void RoboPciWrite32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    const uint64_t address = ROBO_PCI_ECAM
        + static_cast<uint64_t>(bus) * 0x100000ull
        + static_cast<uint64_t>(device) * 0x8000ull
        + static_cast<uint64_t>(function) * 0x1000ull
        + (offset & 0xFCu);
    *reinterpret_cast<volatile uint32_t*>(address) = value;
}

bool RoboPciFindXhci(RoboPciDevice* device) {
    if (device != nullptr) {
        device->found = 0;
    }
    return false;
}
