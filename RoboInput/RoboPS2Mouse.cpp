#include "RoboPS2Mouse.hpp"

#include "RoboArch.hpp"

namespace {

constexpr uint16_t ROBO_PS2_DATA_PORT = 0x60;
constexpr uint16_t ROBO_PS2_STATUS_PORT = 0x64;
constexpr uint8_t ROBO_PS2_STATUS_OUTPUT = 0x01;
constexpr uint8_t ROBO_PS2_STATUS_INPUT = 0x02;
constexpr uint32_t ROBO_PS2_TIMEOUT = 100000u;
constexpr int32_t ROBO_PS2_DRAIN_LIMIT = 96;

bool RoboPS2WaitInput(void) {
    for (uint32_t spin = 0; spin < ROBO_PS2_TIMEOUT; ++spin) {
        if ((RoboPortIn8(ROBO_PS2_STATUS_PORT) & ROBO_PS2_STATUS_INPUT) == 0) {
            return true;
        }
    }
    return false;
}

bool RoboPS2WaitOutput(void) {
    for (uint32_t spin = 0; spin < ROBO_PS2_TIMEOUT; ++spin) {
        if ((RoboPortIn8(ROBO_PS2_STATUS_PORT) & ROBO_PS2_STATUS_OUTPUT) != 0) {
            return true;
        }
    }
    return false;
}

bool RoboPS2WriteCommand(uint8_t command) {
    if (!RoboPS2WaitInput()) {
        return false;
    }
    RoboPortOut8(ROBO_PS2_STATUS_PORT, command);
    return true;
}

bool RoboPS2WriteMouse(uint8_t value) {
    if (!RoboPS2WriteCommand(0xD4)) {
        return false;
    }
    if (!RoboPS2WaitInput()) {
        return false;
    }
    RoboPortOut8(ROBO_PS2_DATA_PORT, value);
    return true;
}

bool RoboPS2Read(uint8_t* value) {
    if (!RoboPS2WaitOutput()) {
        return false;
    }
    *value = RoboPortIn8(ROBO_PS2_DATA_PORT);
    return true;
}

void RoboPS2Flush(void) {
    for (int32_t count = 0; count < 16; ++count) {
        if ((RoboPortIn8(ROBO_PS2_STATUS_PORT) & ROBO_PS2_STATUS_OUTPUT) == 0) {
            return;
        }
        RoboPortIn8(ROBO_PS2_DATA_PORT);
    }
}

uint8_t gPacket[3];
int32_t gPacketIndex = 0;

}  // namespace

bool RoboPS2MouseStart(void) {
    gPacketIndex = 0;
    if (!RoboPS2WriteCommand(0xAD) || !RoboPS2WriteCommand(0xA7)) {
        return false;
    }
    RoboPS2Flush();
    if (!RoboPS2WriteCommand(0xA8) || !RoboPS2WriteCommand(0x20)) {
        return false;
    }
    uint8_t config = 0;
    if (!RoboPS2Read(&config)) {
        return false;
    }
    config &= static_cast<uint8_t>(~0x23u);
    config |= 0x10u;
    if (!RoboPS2WriteCommand(0x60) || !RoboPS2WaitInput()) {
        return false;
    }
    RoboPortOut8(ROBO_PS2_DATA_PORT, config);
    if (!RoboPS2WriteMouse(0xFF)) {
        return false;
    }
    uint8_t ack = 0;
    if (!RoboPS2Read(&ack) || ack != 0xFA) {
        return false;
    }
    uint8_t selfTest = 0;
    uint8_t deviceId = 0;
    if (!RoboPS2Read(&selfTest) || !RoboPS2Read(&deviceId)) {
        return false;
    }
    if (!RoboPS2WriteMouse(0xF4)) {
        return false;
    }
    if (!RoboPS2Read(&ack) || ack != 0xFA) {
        return false;
    }
    return true;
}

void RoboPS2MouseDrain(int32_t* dx, int32_t* dy) {
    *dx = 0;
    *dy = 0;
    int32_t reads = 0;
    while (reads < ROBO_PS2_DRAIN_LIMIT && (RoboPortIn8(ROBO_PS2_STATUS_PORT) & ROBO_PS2_STATUS_OUTPUT) != 0) {
        uint8_t byte = RoboPortIn8(ROBO_PS2_DATA_PORT);
        reads += 1;
        if (gPacketIndex == 0 && (byte & 0x08u) == 0) {
            continue;
        }
        gPacket[gPacketIndex] = byte;
        gPacketIndex += 1;
        if (gPacketIndex < 3) {
            continue;
        }
        gPacketIndex = 0;
        uint8_t flags = gPacket[0];
        if ((flags & 0xC0u) != 0) {
            continue;
        }
        int32_t packetX = gPacket[1];
        int32_t packetY = gPacket[2];
        if ((flags & 0x10u) != 0) {
            packetX -= 256;
        }
        if ((flags & 0x20u) != 0) {
            packetY -= 256;
        }
        *dx += packetX;
        *dy -= packetY;
    }
}
