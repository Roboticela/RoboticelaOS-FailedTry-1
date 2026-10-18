#include "RoboPS2Keyboard.hpp"

#include "RoboArch.hpp"
#include "RoboInput.hpp"
#include "RoboPS2Mouse.hpp"

constexpr uint16_t ROBO_PS2_DATA_PORT = 0x60;
constexpr uint16_t ROBO_PS2_STATUS_PORT = 0x64;
constexpr uint8_t ROBO_PS2_STATUS_OUTPUT = 0x01;
constexpr uint8_t ROBO_PS2_STATUS_INPUT = 0x02;
constexpr uint8_t ROBO_PS2_STATUS_AUX = 0x20;
constexpr uint32_t ROBO_PS2_TIMEOUT = 100000u;
constexpr uint32_t ROBO_KEY_RING = 64;
constexpr uint16_t ROBO_KEY_EXTENDED = 0xE000u;

uint8_t gKeyRing[ROBO_KEY_RING];
volatile uint32_t gKeyHead = 0;
volatile uint32_t gKeyTail = 0;
bool gKeyTranslated = true;
bool gKeyExtended = false;
bool gKeyBreak = false;
uint8_t gKeySkip = 0;
bool gKeyDown[128];
bool gKeyDownExtended[128];

bool RoboKeyWaitInput(void) {
    for (uint32_t spin = 0; spin < ROBO_PS2_TIMEOUT; ++spin) {
        if ((RoboPortIn8(ROBO_PS2_STATUS_PORT) & ROBO_PS2_STATUS_INPUT) == 0) {
            return true;
        }
        RoboPortIn8(0x80);
    }
    return false;
}

bool RoboKeyCommand(uint8_t command) {
    if (!RoboKeyWaitInput()) {
        return false;
    }
    RoboPortOut8(ROBO_PS2_STATUS_PORT, command);
    return true;
}

bool RoboKeyWrite(uint8_t value) {
    if (!RoboKeyWaitInput()) {
        return false;
    }
    RoboPortOut8(ROBO_PS2_DATA_PORT, value);
    return true;
}

bool RoboKeyRead(uint8_t* value) {
    for (uint32_t spin = 0; spin < ROBO_PS2_TIMEOUT; ++spin) {
        uint8_t status = RoboPortIn8(ROBO_PS2_STATUS_PORT);
        if ((status & ROBO_PS2_STATUS_OUTPUT) == 0) {
            RoboPortIn8(0x80);
            continue;
        }
        if ((status & ROBO_PS2_STATUS_AUX) != 0) {
            RoboPS2MouseInterrupt();
            continue;
        }
        *value = RoboPortIn8(ROBO_PS2_DATA_PORT);
        return true;
    }
    return false;
}

void RoboKeyPush(uint8_t byte) {
    uint32_t next = (gKeyHead + 1u) & (ROBO_KEY_RING - 1u);
    if (next == gKeyTail) {
        return;
    }
    gKeyRing[gKeyHead] = byte;
    gKeyHead = next;
}

bool RoboKeyPop(uint8_t* byte) {
    if (gKeyHead == gKeyTail) {
        return false;
    }
    *byte = gKeyRing[gKeyTail];
    gKeyTail = (gKeyTail + 1u) & (ROBO_KEY_RING - 1u);
    return true;
}

void RoboPS2KeyboardInterrupt(void) {
    for (uint32_t spin = 0; spin < 8u; ++spin) {
        uint8_t status = RoboPortIn8(ROBO_PS2_STATUS_PORT);
        if ((status & ROBO_PS2_STATUS_OUTPUT) == 0 || (status & ROBO_PS2_STATUS_AUX) != 0) {
            return;
        }
        RoboKeyPush(RoboPortIn8(ROBO_PS2_DATA_PORT));
    }
}

void RoboKeyFlush(void) {
    for (uint32_t spin = 0; spin < 32u; ++spin) {
        if ((RoboPortIn8(ROBO_PS2_STATUS_PORT) & ROBO_PS2_STATUS_OUTPUT) == 0) {
            return;
        }
        RoboPortIn8(ROBO_PS2_DATA_PORT);
    }
}

bool RoboPS2KeyboardStart(void) {
    if (!RoboKeyCommand(0xAE)) {
        return false;
    }
    RoboKeyFlush();
    if (!RoboKeyCommand(0x20)) {
        return false;
    }
    uint8_t config = 0;
    if (!RoboKeyRead(&config)) {
        return false;
    }
    config = static_cast<uint8_t>((config | 0x41u) & ~0x10u);
    gKeyTranslated = true;
    if (!RoboKeyCommand(0x60) || !RoboKeyWaitInput()) {
        return false;
    }
    RoboPortOut8(ROBO_PS2_DATA_PORT, config);
    RoboKeyFlush();
    if (RoboKeyWrite(0xF4)) {
        uint8_t ack = 0;
        (void)RoboKeyRead(&ack);
    }
    return true;
}

void RoboPS2KeyboardCollect(void) {
    for (uint32_t spin = 0; spin < 16u; ++spin) {
        uint8_t status = RoboPortIn8(ROBO_PS2_STATUS_PORT);
        if ((status & ROBO_PS2_STATUS_OUTPUT) == 0 || (status & ROBO_PS2_STATUS_AUX) != 0) {
            break;
        }
        RoboKeyPush(RoboPortIn8(ROBO_PS2_DATA_PORT));
    }
    uint8_t byte = 0;
    while (RoboKeyPop(&byte)) {
        if (gKeySkip > 0) {
            gKeySkip -= 1;
            continue;
        }
        if (byte == 0xE0) {
            gKeyExtended = true;
            continue;
        }
        if (byte == 0xE1) {
            gKeySkip = 5;
            gKeyExtended = false;
            gKeyBreak = false;
            continue;
        }
        bool pressed = true;
        uint16_t code = byte;
        if (!gKeyTranslated && byte == 0xF0) {
            gKeyBreak = true;
            continue;
        }
        if (gKeyTranslated) {
            pressed = (byte & 0x80u) == 0;
            code = static_cast<uint16_t>(byte & 0x7Fu);
        } else {
            pressed = !gKeyBreak;
            gKeyBreak = false;
            code = byte;
        }
        if (gKeyExtended) {
            code = static_cast<uint16_t>(code | ROBO_KEY_EXTENDED);
            gKeyExtended = false;
        }
        bool* held = (code & ROBO_KEY_EXTENDED) != 0 ? gKeyDownExtended : gKeyDown;
        uint16_t plain = static_cast<uint16_t>(code & 0x7Fu);
        RoboInputEvent key{};
        key.kind = ROBO_INPUT_KEY;
        key.code = code;
        if (!pressed && !held[plain]) {
            key.pressed = 1u;
            RoboInputPush(&key);
        }
        held[plain] = pressed;
        key.pressed = pressed ? 1u : 0u;
        RoboInputPush(&key);
    }
}
