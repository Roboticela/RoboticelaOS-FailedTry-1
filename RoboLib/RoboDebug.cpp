#include "RoboArch.hpp"

#include <stdint.h>

namespace {

constexpr uint16_t ROBO_COM1 = 0x3F8;

}  // namespace

extern "C" void RoboDebugWrite(const char* text) {
    if (text == nullptr) {
        return;
    }
    for (uint32_t index = 0; text[index] != 0; ++index) {
        for (uint32_t spin = 0; spin < 100000u; ++spin) {
            if ((RoboPortIn8(ROBO_COM1 + 5u) & 0x20u) != 0) {
                break;
            }
        }
        RoboPortOut8(ROBO_COM1, static_cast<uint8_t>(text[index]));
    }
}

extern "C" void RoboDebugLine(const char* text) {
    RoboDebugWrite(text);
    RoboDebugWrite("\r\n");
}
