#include "RoboPower.hpp"

#include "RoboArch.hpp"

void RoboPowerShutdown(void) {
#if !defined(__aarch64__)
    RoboPortOut16(0x604, 0x2000);
    RoboPortOut16(0xB004, 0x2000);
    RoboPortOut8(0x64, 0xFE);
#endif
    RoboHalt();
}

void RoboPowerReboot(void) {
#if !defined(__aarch64__)
    RoboPortOut8(0x64, 0xFE);
#endif
    RoboHalt();
}
