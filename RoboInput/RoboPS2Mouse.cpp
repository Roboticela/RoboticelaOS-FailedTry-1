#include "RoboPS2Mouse.hpp"

#include "RoboArch.hpp"

constexpr uint16_t ROBO_PS2_DATA_PORT = 0x60;
constexpr uint16_t ROBO_PS2_STATUS_PORT = 0x64;
constexpr uint8_t ROBO_PS2_STATUS_OUTPUT = 0x01;
constexpr uint8_t ROBO_PS2_STATUS_INPUT = 0x02;
constexpr uint8_t ROBO_PS2_STATUS_AUX = 0x20;
constexpr uint32_t ROBO_PS2_TIMEOUT = 100000u;
constexpr int32_t ROBO_PS2_DRAIN_LIMIT = 96;

bool RoboPS2WaitInput(void) {
    for (uint32_t spin = 0; spin < ROBO_PS2_TIMEOUT; ++spin) {
        if ((RoboPortIn8(ROBO_PS2_STATUS_PORT) & ROBO_PS2_STATUS_INPUT) == 0) {
            return true;
        }
        RoboPortIn8(0x80);
    }
    return false;
}

bool RoboPS2WaitOutput(void) {
    for (uint32_t spin = 0; spin < ROBO_PS2_TIMEOUT; ++spin) {
        if ((RoboPortIn8(ROBO_PS2_STATUS_PORT) & ROBO_PS2_STATUS_OUTPUT) != 0) {
            return true;
        }
        RoboPortIn8(0x80);
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

constexpr uint32_t ROBO_MOUSE_RING = 128;

uint8_t gRing[ROBO_MOUSE_RING];
volatile uint32_t gRingHead = 0;
volatile uint32_t gRingTail = 0;
uint8_t gPacket[3];
int32_t gPacketIndex = 0;
bool gMouseReady = false;

void RoboMousePush(uint8_t byte) {
    uint32_t next = (gRingHead + 1u) & (ROBO_MOUSE_RING - 1u);
    if (next == gRingTail) {
        return;
    }
    gRing[gRingHead] = byte;
    gRingHead = next;
}

bool RoboMousePop(uint8_t* byte) {
    if (gRingHead == gRingTail) {
        return false;
    }
    *byte = gRing[gRingTail];
    gRingTail = (gRingTail + 1u) & (ROBO_MOUSE_RING - 1u);
    return true;
}

void RoboPS2MouseInterrupt(void) {
    for (int32_t count = 0; count < 8; ++count) {
        uint8_t status = RoboPortIn8(ROBO_PS2_STATUS_PORT);
        if ((status & ROBO_PS2_STATUS_OUTPUT) == 0 || (status & ROBO_PS2_STATUS_AUX) == 0) {
            return;
        }
        RoboMousePush(RoboPortIn8(ROBO_PS2_DATA_PORT));
    }
}

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
    if (RoboPS2WriteMouse(0xFF)) {
        uint8_t ack = 0;
        if (RoboPS2Read(&ack) && ack == 0xFA) {
            uint8_t selfTest = 0;
            uint8_t deviceId = 0;
            (void)RoboPS2Read(&selfTest);
            (void)RoboPS2Read(&deviceId);
        }
    }
    if (RoboPS2WriteMouse(0xF4)) {
        uint8_t ack = 0;
        (void)RoboPS2Read(&ack);
    }
    gMouseReady = true;
    return true;
}

void RoboPS2MouseArm(void) {
    if (!gMouseReady) {
        return;
    }
    if (!RoboPS2WriteCommand(0xA7)) {
        gMouseReady = false;
        return;
    }
    RoboPS2Flush();
    if (!RoboPS2WriteCommand(0x20)) {
        gMouseReady = false;
        return;
    }
    uint8_t config = 0;
    if (!RoboPS2Read(&config)) {
        gMouseReady = false;
        return;
    }
    config = static_cast<uint8_t>((config | 0x43u) & ~0x30u);
    if (!RoboPS2WriteCommand(0x60) || !RoboPS2WaitInput()) {
        gMouseReady = false;
        return;
    }
    RoboPortOut8(ROBO_PS2_DATA_PORT, config);
    RoboPS2WriteCommand(0xA8);
}

uint8_t gLeftButton = 0;
uint8_t gRightButton = 0;

bool RoboPS2MouseReadPacket(int32_t* dx, int32_t* dy, uint8_t* leftPressed, uint8_t* leftChanged, uint8_t* rightPressed, uint8_t* rightChanged) {
    if (dx != nullptr) {
        *dx = 0;
    }
    if (dy != nullptr) {
        *dy = 0;
    }
    if (leftPressed != nullptr) {
        *leftPressed = gLeftButton;
    }
    if (leftChanged != nullptr) {
        *leftChanged = 0;
    }
    if (rightPressed != nullptr) {
        *rightPressed = gRightButton;
    }
    if (rightChanged != nullptr) {
        *rightChanged = 0;
    }
    uint8_t byte = 0;
    while (RoboMousePop(&byte)) {
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
        if (dx != nullptr) {
            *dx = packetX;
        }
        if (dy != nullptr) {
            *dy = -packetY;
        }
        uint8_t left = (flags & 0x01u) != 0 ? 1u : 0u;
        if (leftPressed != nullptr) {
            *leftPressed = left;
        }
        if (left != gLeftButton) {
            gLeftButton = left;
            if (leftChanged != nullptr) {
                *leftChanged = 1u;
            }
        }
        uint8_t right = (flags & 0x02u) != 0 ? 1u : 0u;
        if (rightPressed != nullptr) {
            *rightPressed = right;
        }
        if (right != gRightButton) {
            gRightButton = right;
            if (rightChanged != nullptr) {
                *rightChanged = 1u;
            }
        }
        return true;
    }
    return false;
}

void RoboPS2MouseDrain(int32_t* dx, int32_t* dy, uint8_t* leftPressed, uint8_t* leftChanged, uint8_t* rightPressed, uint8_t* rightChanged) {
    *dx = 0;
    *dy = 0;
    if (leftPressed != nullptr) {
        *leftPressed = gLeftButton;
    }
    if (leftChanged != nullptr) {
        *leftChanged = 0;
    }
    if (rightPressed != nullptr) {
        *rightPressed = gRightButton;
    }
    if (rightChanged != nullptr) {
        *rightChanged = 0;
    }
    int32_t reads = 0;
    while (reads < ROBO_PS2_DRAIN_LIMIT) {
        int32_t packetDx = 0;
        int32_t packetDy = 0;
        uint8_t pressed = 0;
        uint8_t changed = 0;
        uint8_t right = 0;
        uint8_t rightEdge = 0;
        if (!RoboPS2MouseReadPacket(&packetDx, &packetDy, &pressed, &changed, &right, &rightEdge)) {
            return;
        }
        reads += 1;
        *dx += packetDx;
        *dy += packetDy;
        if (leftPressed != nullptr) {
            *leftPressed = pressed;
        }
        if (changed != 0 && leftChanged != nullptr) {
            *leftChanged = 1u;
        }
        if (rightPressed != nullptr) {
            *rightPressed = right;
        }
        if (rightEdge != 0 && rightChanged != nullptr) {
            *rightChanged = 1u;
        }
    }
}
