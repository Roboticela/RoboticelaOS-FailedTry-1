#include "RoboUsb.hpp"

#include "RoboArch.hpp"
#include "RoboInput.hpp"
#include "RoboMemory.hpp"
#include "RoboPci.hpp"

namespace {

constexpr uint32_t ROBO_USB_DEVICE_MAX = 32;
constexpr uint32_t ROBO_USB_HOST_MAX = 8;
constexpr uint32_t ROBO_HID_FIELD_MAX = 16;
constexpr uint32_t ROBO_TRB_COUNT = 256;
constexpr uint32_t ROBO_USB_KIND_MOUSE = 1;
constexpr uint32_t ROBO_USB_KIND_TOUCHPAD = 2;
constexpr uint32_t ROBO_USB_KIND_TOUCH = 3;
constexpr uint32_t ROBO_USB_KIND_KEYBOARD = 4;
constexpr uint32_t ROBO_USB_KIND_HUB = 5;
constexpr uint8_t ROBO_PORT_KIND_USB = 1;
constexpr uint8_t ROBO_PORT_KIND_USBC = 2;
constexpr uint8_t ROBO_PORT_KIND_THUNDERBOLT = 3;
constexpr uint32_t ROBO_PORT_CHANGE = 0x00FE0000u;

const uint8_t ROBO_HID_LETTERS[] = {
    0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24,
    0x25, 0x26, 0x32, 0x31, 0x18, 0x19, 0x10, 0x13, 0x1F, 0x14,
    0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C
};
const uint8_t ROBO_HID_MODIFIER_BITS[] = {0x01u, 0x02u, 0x04u, 0x10u, 0x20u, 0x40u};
const uint16_t ROBO_HID_MODIFIER_CODES[] = {0x1Du, 0x2Au, 0x38u, 0xE01Du, 0x36u, 0xE038u};

struct RoboTrb {
    uint32_t data0;
    uint32_t data1;
    uint32_t status;
    uint32_t control;
};

struct RoboRing {
    RoboTrb* trb;
    uint64_t physical;
    uint32_t enqueue;
    uint32_t cycle;
};

struct RoboHidField {
    uint16_t usagePage;
    uint16_t usage;
    uint16_t bitOffset;
    uint8_t bitSize;
    uint8_t reportId;
    uint8_t relative;
    int32_t logicalMin;
    int32_t logicalMax;
};

struct RoboUsbDevice {
    uint8_t used;
    uint8_t slot;
    uint8_t speed;
    uint8_t port;
    uint8_t kind;
    uint8_t dci;
    uint8_t running;
    uint8_t parentSlot;
    uint8_t parentPort;
    uint8_t isHub;
    uint8_t hubPorts;
    uint16_t maxPacket;
    RoboRing ep0Ring;
    RoboRing intRing;
    uint64_t output;
    uint64_t input;
    uint64_t buffers;
    RoboHidField fields[ROBO_HID_FIELD_MAX];
    uint8_t fieldCount;
    uint8_t contact;
    uint8_t rightContact;
    uint8_t host;
    uint8_t modifier;
    uint8_t keys[6];
    int32_t lastX;
    int32_t lastY;
    int32_t lastXMax;
    int32_t lastYMax;
};

struct RoboUsbHost {
    uint64_t operation;
    uint64_t runtime;
    uint64_t doorbell;
    uint32_t contextBytes;
    uint32_t maxPorts;
    uint32_t maxSlots;
    uint8_t running;
    uint8_t index;
    uint8_t irq;
    uint8_t portKind[33];
    uint32_t pending;
    RoboRing commandRing;
    RoboTrb* events;
    uint64_t eventPhysical;
    uint32_t eventDequeue;
    uint32_t eventCycle;
    uint64_t controlData;
    uint64_t dcba;
    uint64_t waitTrb;
    uint8_t waitSlot;
    uint8_t waitDone;
    uint8_t waitCode;
    uint8_t commandDone;
    uint8_t commandCode;
    uint8_t commandSlot;
    uint32_t scanPort;
    uint32_t scanCounter;
};

RoboUsbHost gHosts[ROBO_USB_HOST_MAX]{};
RoboUsbHost* gHost = nullptr;
uint8_t gPolling = 0;
uint8_t gThunderbolt = 0;
RoboUsbDevice gDevices[ROBO_USB_DEVICE_MAX]{};

#define gOperation gHost->operation
#define gRuntime gHost->runtime
#define gDoorbell gHost->doorbell
#define gContextBytes gHost->contextBytes
#define gMaxPorts gHost->maxPorts
#define gCommandRing gHost->commandRing
#define gEvents gHost->events
#define gEventPhysical gHost->eventPhysical
#define gEventDequeue gHost->eventDequeue
#define gEventCycle gHost->eventCycle
#define gControlData gHost->controlData
#define gDcba gHost->dcba
#define gWaitTrb gHost->waitTrb
#define gWaitSlot gHost->waitSlot
#define gWaitDone gHost->waitDone
#define gWaitCode gHost->waitCode
#define gCommandDone gHost->commandDone
#define gCommandCode gHost->commandCode
#define gCommandSlot gHost->commandSlot

uint32_t RoboMmioRead32(uint64_t address) {
    return *reinterpret_cast<volatile uint32_t*>(address);
}

void RoboMmioWrite32(uint64_t address, uint32_t value) {
    *reinterpret_cast<volatile uint32_t*>(address) = value;
}

void RoboMmioWrite64(uint64_t address, uint64_t value) {
    *reinterpret_cast<volatile uint64_t*>(address) = value;
}

void RoboZeroPage(uint64_t page) {
    uint8_t* bytes = reinterpret_cast<uint8_t*>(page);
    for (uint32_t index = 0; index < 4096u; ++index) {
        bytes[index] = 0;
    }
}

void RoboUsbDelay(uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        __asm__ volatile("" ::: "memory");
    }
}

void RoboUsbDelayMs(uint32_t ms) {
    for (uint32_t i = 0; i < ms * 1000u; ++i) {
        RoboPortIn8(0x80);
    }
}

uint32_t* RoboContext(uint64_t base, uint32_t index) {
    return reinterpret_cast<uint32_t*>(base + static_cast<uint64_t>(index) * gContextBytes);
}

void RoboRingInit(RoboRing* ring, uint64_t page) {
    ring->trb = reinterpret_cast<RoboTrb*>(page);
    ring->physical = page;
    ring->enqueue = 0;
    ring->cycle = 1;
    RoboTrb* link = &ring->trb[ROBO_TRB_COUNT - 1u];
    link->data0 = static_cast<uint32_t>(page);
    link->data1 = static_cast<uint32_t>(page >> 32);
    link->status = 0;
    link->control = (6u << 10) | (1u << 1) | 1u;
}

uint64_t RoboRingPush(RoboRing* ring, uint32_t data0, uint32_t data1, uint32_t status, uint32_t control) {
    uint64_t address = ring->physical + static_cast<uint64_t>(ring->enqueue) * sizeof(RoboTrb);
    RoboTrb* trb = &ring->trb[ring->enqueue];
    trb->data0 = data0;
    trb->data1 = data1;
    trb->status = status;
    *reinterpret_cast<volatile uint32_t*>(&trb->control) = control | ring->cycle;
    ring->enqueue += 1u;
    if (ring->enqueue == ROBO_TRB_COUNT - 1u) {
        RoboTrb* link = &ring->trb[ring->enqueue];
        *reinterpret_cast<volatile uint32_t*>(&link->control) = (6u << 10) | (1u << 1) | ring->cycle;
        ring->enqueue = 0;
        ring->cycle ^= 1u;
    }
    return address;
}

void RoboEventAdvance(void) {
    gEventDequeue += 1u;
    if (gEventDequeue == ROBO_TRB_COUNT) {
        gEventDequeue = 0;
        gEventCycle ^= 1u;
    }
    uint64_t pointer = gEventPhysical + static_cast<uint64_t>(gEventDequeue) * sizeof(RoboTrb);
    RoboMmioWrite64(gRuntime + 0x20 + 0x18, pointer | (1ull << 3));
}

int32_t RoboHidSigned(uint32_t raw, uint32_t sizeBytes) {
    uint32_t bits = sizeBytes * 8u;
    if (bits == 0 || bits >= 32u) {
        return static_cast<int32_t>(raw);
    }
    if ((raw & (1u << (bits - 1u))) != 0) {
        return static_cast<int32_t>(raw | (~0u << bits));
    }
    return static_cast<int32_t>(raw);
}

void RoboHidAddField(RoboUsbDevice* device, uint16_t page, uint16_t usage, uint32_t bitOffset, uint32_t bitSize, uint8_t reportId, uint8_t relative, int32_t logicalMin, int32_t logicalMax) {
    if (device->fieldCount >= ROBO_HID_FIELD_MAX || bitSize == 0 || bitSize > 32u) {
        return;
    }
    bool useful = (page == 0x01u && (usage == 0x30u || usage == 0x31u || usage == 0x38u))
        || (page == 0x0Cu && usage == 0x0238u)
        || (page == 0x0Du && (usage == 0x30u || usage == 0x31u || usage == 0x42u || usage == 0x54u))
        || (page == 0x09u && usage >= 0x01u && usage <= 0x03u);
    if (!useful) {
        return;
    }
    RoboHidField* field = &device->fields[device->fieldCount];
    field->usagePage = page;
    field->usage = usage;
    field->bitOffset = static_cast<uint16_t>(bitOffset);
    field->bitSize = static_cast<uint8_t>(bitSize);
    field->reportId = reportId;
    field->relative = relative;
    field->logicalMin = logicalMin;
    field->logicalMax = logicalMax;
    device->fieldCount += 1u;
}

void RoboHidParse(RoboUsbDevice* device, const uint8_t* report, uint32_t length) {
    uint16_t usagePage = 0;
    int32_t logicalMin = 0;
    int32_t logicalMax = 0;
    uint32_t reportSize = 0;
    uint32_t reportCount = 0;
    uint8_t reportId = 0;
    bool reportIds = false;
    uint32_t bitOffset = 0;
    uint16_t usages[16]{};
    uint8_t usageCount = 0;
    uint16_t usageMin = 0;
    uint16_t usageMax = 0;
    bool usageRange = false;
    uint16_t pendingUsage = 0;
    uint16_t appPage = 0;
    uint16_t appUsage = 0;
    for (uint32_t offset = 0; offset < length;) {
        uint8_t prefix = report[offset];
        offset += 1u;
        if (prefix == 0xFEu) {
            if (offset >= length) {
                break;
            }
            uint8_t dataSize = report[offset];
            offset += 2u + dataSize;
            continue;
        }
        uint32_t sizeCode = prefix & 0x03u;
        uint32_t size = sizeCode == 3u ? 4u : sizeCode;
        uint32_t type = (prefix >> 2) & 0x03u;
        uint32_t tag = (prefix >> 4) & 0x0Fu;
        if (offset + size > length) {
            break;
        }
        uint32_t raw = 0;
        for (uint32_t byte = 0; byte < size; ++byte) {
            raw |= static_cast<uint32_t>(report[offset + byte]) << (8u * byte);
        }
        offset += size;
        int32_t signedValue = RoboHidSigned(raw, size);
        if (type == 1u) {
            if (tag == 0u) {
                usagePage = static_cast<uint16_t>(raw);
            } else if (tag == 1u) {
                logicalMin = signedValue;
            } else if (tag == 2u) {
                logicalMax = signedValue;
            } else if (tag == 7u) {
                reportSize = raw;
            } else if (tag == 8u) {
                reportIds = true;
                reportId = static_cast<uint8_t>(raw);
                bitOffset = 8u;
            } else if (tag == 9u) {
                reportCount = raw;
            }
        } else if (type == 2u) {
            if (tag == 0u && usageCount < 16u) {
                usages[usageCount] = static_cast<uint16_t>(raw);
                usageCount += 1u;
                pendingUsage = static_cast<uint16_t>(raw);
            } else if (tag == 1u) {
                usageMin = static_cast<uint16_t>(raw);
                usageRange = true;
            } else if (tag == 2u) {
                usageMax = static_cast<uint16_t>(raw);
                usageRange = true;
            }
        } else if (type == 0u && tag == 10u && raw == 1u) {
            appPage = usagePage;
            appUsage = pendingUsage;
        } else if (type == 0u && tag == 8u) {
            uint32_t count = reportCount;
            for (uint32_t index = 0; index < count; ++index) {
                uint16_t usage = 0;
                if (usageRange && usageMin + index <= usageMax) {
                    usage = static_cast<uint16_t>(usageMin + index);
                } else if (index < usageCount) {
                    usage = usages[index];
                } else if (usageCount > 0) {
                    usage = usages[usageCount - 1u];
                }
                if ((raw & 1u) == 0) {
                    RoboHidAddField(device, usagePage, usage, bitOffset, reportSize, reportId, (raw & 4u) != 0 ? 1u : 0u, logicalMin, logicalMax);
                }
                bitOffset += reportSize;
            }
            usageCount = 0;
            usageRange = false;
            (void)reportIds;
        }
    }
    bool absolute = false;
    for (uint8_t index = 0; index < device->fieldCount; ++index) {
        if ((device->fields[index].usage == 0x30u || device->fields[index].usage == 0x31u) && device->fields[index].relative == 0) {
            absolute = true;
        }
    }
    if (appPage == 0x01u && appUsage == 0x06u) {
        device->kind = ROBO_USB_KIND_KEYBOARD;
    } else if (appPage == 0x0Du && appUsage == 0x04u) {
        device->kind = ROBO_USB_KIND_TOUCH;
    } else if (appPage == 0x0Du && appUsage == 0x05u) {
        device->kind = ROBO_USB_KIND_TOUCHPAD;
    } else if (absolute) {
        device->kind = ROBO_USB_KIND_TOUCHPAD;
    } else {
        device->kind = ROBO_USB_KIND_MOUSE;
    }
}

void RoboHidBootMouse(RoboUsbDevice* device) {
    device->fieldCount = 0;
    RoboHidAddField(device, 0x09u, 0x01u, 0u, 1u, 0, 0u, 0, 1);
    RoboHidAddField(device, 0x01u, 0x30u, 8u, 8u, 0, 1u, -127, 127);
    RoboHidAddField(device, 0x01u, 0x31u, 16u, 8u, 0, 1u, -127, 127);
    device->kind = ROBO_USB_KIND_MOUSE;
}

uint16_t RoboHidScan(uint8_t usage) {
    if (usage >= 0x04u && usage <= 0x1Du) {
        return ROBO_HID_LETTERS[usage - 0x04u];
    }
    if (usage >= 0x1Eu && usage <= 0x26u) {
        return static_cast<uint16_t>(0x02u + (usage - 0x1Eu));
    }
    if (usage == 0x27u) {
        return 0x0Bu;
    }
    if (usage == 0x28u) {
        return 0x1Cu;
    }
    if (usage == 0x29u) {
        return 0x01u;
    }
    if (usage == 0x2Au) {
        return 0x0Eu;
    }
    if (usage == 0x2Bu) {
        return 0x0Fu;
    }
    if (usage == 0x2Cu) {
        return 0x39u;
    }
    if (usage == 0x4Fu) {
        return 0xE04Du;
    }
    if (usage == 0x50u) {
        return 0xE04Bu;
    }
    if (usage == 0x51u) {
        return 0xE050u;
    }
    if (usage == 0x52u) {
        return 0xE048u;
    }
    return 0;
}

void RoboUsbPushKey(uint16_t code, uint8_t pressed) {
    if (code == 0) {
        return;
    }
    RoboInputEvent event{};
    event.kind = ROBO_INPUT_KEY;
    event.code = code;
    event.pressed = pressed;
    RoboInputPush(&event);
}

void RoboUsbPushKeyboard(RoboUsbDevice* device, const uint8_t* report, uint32_t length) {
    if (length < 8u) {
        return;
    }
    uint8_t modifier = report[0];
    for (uint32_t index = 0; index < 6u; ++index) {
        uint8_t next = (modifier & ROBO_HID_MODIFIER_BITS[index]) != 0 ? 1u : 0u;
        uint8_t previous = (device->modifier & ROBO_HID_MODIFIER_BITS[index]) != 0 ? 1u : 0u;
        if (next != previous) {
            RoboUsbPushKey(ROBO_HID_MODIFIER_CODES[index], next);
        }
    }
    device->modifier = modifier;
    uint8_t nextKeys[6];
    for (uint32_t index = 0; index < 6u; ++index) {
        nextKeys[index] = report[2u + index];
    }
    for (uint32_t oldIndex = 0; oldIndex < 6u; ++oldIndex) {
        uint8_t usage = device->keys[oldIndex];
        if (usage == 0) {
            continue;
        }
        bool still = false;
        for (uint32_t index = 0; index < 6u; ++index) {
            if (nextKeys[index] == usage) {
                still = true;
            }
        }
        if (!still) {
            RoboUsbPushKey(RoboHidScan(usage), 0);
        }
    }
    for (uint32_t index = 0; index < 6u; ++index) {
        uint8_t usage = nextKeys[index];
        if (usage == 0) {
            continue;
        }
        bool seen = false;
        for (uint32_t oldIndex = 0; oldIndex < 6u; ++oldIndex) {
            if (device->keys[oldIndex] == usage) {
                seen = true;
            }
        }
        if (!seen) {
            RoboUsbPushKey(RoboHidScan(usage), 1);
        }
    }
    for (uint32_t index = 0; index < 6u; ++index) {
        device->keys[index] = nextKeys[index];
    }
}

int32_t RoboHidValue(const uint8_t* report, uint32_t length, const RoboHidField* field) {
    uint32_t bit = field->bitOffset;
    uint32_t size = field->bitSize;
    if (size == 0 || (bit + size) > length * 8u) {
        return 0;
    }
    uint32_t raw = 0;
    for (uint32_t index = 0; index < size; ++index) {
        uint32_t source = bit + index;
        if ((report[source / 8u] & (1u << (source % 8u))) != 0) {
            raw |= 1u << index;
        }
    }
    if (field->logicalMin < 0 && size < 32u && (raw & (1u << (size - 1u))) != 0) {
        return static_cast<int32_t>(raw | (~0u << size));
    }
    return static_cast<int32_t>(raw);
}

const RoboHidField* RoboHidFind(const RoboUsbDevice* device, uint16_t page, uint16_t usage, uint8_t reportId) {
    for (uint8_t index = 0; index < device->fieldCount; ++index) {
        const RoboHidField* field = &device->fields[index];
        if (field->usagePage == page && field->usage == usage && field->reportId == reportId) {
            return field;
        }
    }
    return nullptr;
}

void RoboUsbPushReport(RoboUsbDevice* device, const uint8_t* report, uint32_t length) {
    if (device->kind == ROBO_USB_KIND_KEYBOARD) {
        RoboUsbPushKeyboard(device, report, length);
        return;
    }
    bool hasReportId = false;
    for (uint8_t index = 0; index < device->fieldCount; ++index) {
        if (device->fields[index].reportId != 0) {
            hasReportId = true;
        }
    }
    uint8_t reportId = 0;
    if (hasReportId) {
        if (length == 0) {
            return;
        }
        reportId = report[0];
    }
    const RoboHidField* xField = RoboHidFind(device, 0x01u, 0x30u, reportId);
    const RoboHidField* yField = RoboHidFind(device, 0x01u, 0x31u, reportId);
    if (xField == nullptr || yField == nullptr) {
        xField = RoboHidFind(device, 0x0Du, 0x30u, reportId);
        yField = RoboHidFind(device, 0x0Du, 0x31u, reportId);
    }
    if (xField == nullptr || yField == nullptr) {
        return;
    }
    int32_t x = RoboHidValue(report, length, xField);
    int32_t y = RoboHidValue(report, length, yField);
    RoboInputEvent event{};
    if (xField->relative != 0) {
        const RoboHidField* wheelField = RoboHidFind(device, 0x01u, 0x38u, reportId);
        const RoboHidField* panField = RoboHidFind(device, 0x0Cu, 0x0238u, reportId);
        int32_t wheel = wheelField != nullptr ? RoboHidValue(report, length, wheelField) : 0;
        int32_t pan = panField != nullptr ? RoboHidValue(report, length, panField) : 0;
        if (wheel != 0 || pan != 0) {
            RoboInputEvent wheelEvent{};
            wheelEvent.kind = ROBO_INPUT_WHEEL;
            wheelEvent.dx = pan * 24;
            wheelEvent.dy = -wheel * 24;
            RoboInputPush(&wheelEvent);
        }
        const RoboHidField* button = RoboHidFind(device, 0x09u, 0x01u, reportId);
        const RoboHidField* rightButton = RoboHidFind(device, 0x09u, 0x02u, reportId);
        uint8_t left = button != nullptr && RoboHidValue(report, length, button) != 0 ? 1u : 0u;
        uint8_t right = rightButton != nullptr && RoboHidValue(report, length, rightButton) != 0 ? 1u : 0u;
        uint8_t changed = left != device->contact ? 1u : 0u;
        uint8_t rightChanged = right != device->rightContact ? 1u : 0u;
        if (x == 0 && y == 0 && changed == 0 && rightChanged == 0) {
            return;
        }
        device->contact = left;
        device->rightContact = right;
        if (x != 0 || y != 0 || changed != 0) {
            event.kind = ROBO_INPUT_MOTION;
            event.dx = x;
            event.dy = y;
            event.pressed = left;
            event.code = changed;
            RoboInputPush(&event);
        }
        if (rightChanged != 0) {
            RoboInputEvent rightEvent{};
            rightEvent.kind = ROBO_INPUT_MOTION;
            rightEvent.button = 1;
            rightEvent.pressed = right;
            rightEvent.code = 1;
            RoboInputPush(&rightEvent);
        }
        return;
    }
    const RoboHidField* tip = RoboHidFind(device, 0x0Du, 0x42u, reportId);
    const RoboHidField* button = RoboHidFind(device, 0x09u, 0x01u, reportId);
    const RoboHidField* rightButton = RoboHidFind(device, 0x09u, 0x02u, reportId);
    uint8_t pressed = 0;
    if (device->kind == ROBO_USB_KIND_TOUCH) {
        pressed = tip == nullptr || RoboHidValue(report, length, tip) != 0 ? 1u : 0u;
    } else if (button != nullptr) {
        pressed = RoboHidValue(report, length, button) != 0 ? 1u : 0u;
    } else if (tip != nullptr) {
        pressed = RoboHidValue(report, length, tip) != 0 ? 1u : 0u;
    }
    if (xField->logicalMax <= 0 || yField->logicalMax <= 0) {
        return;
    }
    const RoboHidField* contactCountField = RoboHidFind(device, 0x0Du, 0x54u, reportId);
    int32_t contactCount = contactCountField != nullptr ? RoboHidValue(report, length, contactCountField) : 0;
    const bool moved = x != device->lastX || y != device->lastY;
    if (device->kind == ROBO_USB_KIND_TOUCHPAD && contactCount >= 2) {
        if (moved && device->lastXMax > 0 && device->lastYMax > 0) {
            int32_t dx = x - device->lastX;
            int32_t dy = y - device->lastY;
            RoboInputEvent wheelEvent{};
            wheelEvent.kind = ROBO_INPUT_WHEEL;
            wheelEvent.dx = -dx * 200 / xField->logicalMax;
            wheelEvent.dy = -dy * 200 / yField->logicalMax;
            if (wheelEvent.dx != 0 || wheelEvent.dy != 0) {
                RoboInputPush(&wheelEvent);
            }
        }
        device->lastX = x;
        device->lastY = y;
        device->lastXMax = xField->logicalMax;
        device->lastYMax = yField->logicalMax;
        return;
    }
    uint8_t right = 0;
    if (device->kind != ROBO_USB_KIND_TOUCH && rightButton != nullptr) {
        right = RoboHidValue(report, length, rightButton) != 0 ? 1u : 0u;
    }
    const uint8_t changed = pressed != device->contact ? 1u : 0u;
    const uint8_t rightChanged = right != device->rightContact ? 1u : 0u;
    if (changed == 0 && rightChanged == 0 && x == device->lastX && y == device->lastY) {
        return;
    }
    device->contact = pressed;
    device->rightContact = right;
    device->lastX = x;
    device->lastY = y;
    device->lastXMax = xField->logicalMax;
    device->lastYMax = yField->logicalMax;
    event.kind = device->kind == ROBO_USB_KIND_TOUCH ? ROBO_INPUT_TOUCH : ROBO_INPUT_TOUCHPAD;
    event.pressed = pressed;
    event.code = changed;
    event.x = x;
    event.y = y;
    event.xMax = xField->logicalMax;
    event.yMax = yField->logicalMax;
    if (changed != 0 || moved) {
        RoboInputPush(&event);
    }
    if (rightChanged != 0) {
        RoboInputEvent rightEvent = event;
        rightEvent.button = 1;
        rightEvent.pressed = right;
        rightEvent.code = 1;
        RoboInputPush(&rightEvent);
    }
}

void RoboUsbRequeue(RoboUsbDevice* device, uint64_t trbAddress) {
    if (trbAddress < device->intRing.physical) {
        return;
    }
    uint64_t offset = trbAddress - device->intRing.physical;
    if ((offset % sizeof(RoboTrb)) != 0 || offset / sizeof(RoboTrb) >= ROBO_TRB_COUNT - 1u) {
        return;
    }
    RoboTrb* completed = reinterpret_cast<RoboTrb*>(trbAddress);
    uint32_t length = completed->status & 0x1FFFFu;
    if (length == 0 || length > 1024u) {
        length = device->maxPacket;
    }
    RoboRingPush(&device->intRing, completed->data0, completed->data1, length, (1u << 10) | (1u << 5));
    __asm__ volatile("" : : : "memory");
    RoboMmioWrite32(gDoorbell + static_cast<uint64_t>(device->slot) * 4u, device->dci);
}

void RoboUsbOnTransfer(uint32_t slot, uint64_t trbAddress, uint32_t code, uint32_t residual) {
    if (gWaitTrb != 0 && slot == gWaitSlot) {
        if ((trbAddress & ~0xFull) == (gWaitTrb & ~0xFull) || (code != 1u && code != 13u)) {
            gWaitDone = 1;
            gWaitCode = static_cast<uint8_t>(code);
            return;
        }
    }
    RoboUsbDevice* device = nullptr;
    for (uint32_t index = 0; index < ROBO_USB_DEVICE_MAX; ++index) {
        if (gDevices[index].used && gDevices[index].host == gHost->index && gDevices[index].slot == slot) {
            device = &gDevices[index];
            break;
        }
    }
    if (device == nullptr || device->running == 0) {
        return;
    }
    if (code != 1u && code != 13u) {
        return;
    }
    if (trbAddress < device->intRing.physical) {
        return;
    }
    RoboTrb* completed = reinterpret_cast<RoboTrb*>(trbAddress);
    uint32_t requested = completed->status & 0x1FFFFu;
    uint32_t actual = requested;
    if (residual <= requested) {
        actual = requested - residual;
    }
    uint64_t buffer = completed->data0 | (static_cast<uint64_t>(completed->data1) << 32);
    if (actual > 0 && buffer != 0) {
        RoboUsbPushReport(device, reinterpret_cast<uint8_t*>(buffer), actual);
    }
    RoboUsbRequeue(device, trbAddress);
}

void RoboUsbClearPort(uint32_t port) {
    if (port == 0 || port > gMaxPorts || gOperation == 0) {
        return;
    }
    uint64_t reg = gOperation + 0x400ull + (static_cast<uint64_t>(port) - 1ull) * 0x10ull;
    uint32_t portsc = RoboMmioRead32(reg);
    RoboMmioWrite32(reg, (1u << 9) | (portsc & ROBO_PORT_CHANGE));
    portsc = RoboMmioRead32(reg);
    if ((portsc & 1u) == 0 || port > 32u) {
        return;
    }
    for (uint32_t index = 0; index < ROBO_USB_DEVICE_MAX; ++index) {
        if (gDevices[index].used && gDevices[index].host == gHost->index && gDevices[index].port == port && gDevices[index].parentSlot == 0) {
            return;
        }
    }
    gHost->pending |= 1u << port;
}

void RoboUsbDrain(void) {
    if (gPolling != 0 || gEvents == nullptr || gRuntime == 0) {
        return;
    }
    gPolling = 1;
    for (uint32_t count = 0; count < ROBO_TRB_COUNT; ++count) {
        RoboTrb* event = &gEvents[gEventDequeue];
        uint32_t control = *reinterpret_cast<volatile uint32_t*>(&event->control);
        if ((control & 1u) != gEventCycle) {
            break;
        }
        uint32_t type = (control >> 10) & 0x3Fu;
        uint32_t code = event->status >> 24;
        uint64_t parameter = event->data0 | (static_cast<uint64_t>(event->data1) << 32);
        RoboEventAdvance();
        if (type == 33u) {
            gCommandDone = 1;
            gCommandCode = static_cast<uint8_t>(code);
            gCommandSlot = static_cast<uint8_t>(control >> 24);
        } else if (type == 32u) {
            RoboUsbOnTransfer(control >> 24, parameter, code, event->status & 0x00FFFFFFu);
        } else if (type == 34u) {
            RoboUsbClearPort(event->data0 >> 24);
        }
    }
    uint32_t pending = RoboMmioRead32(gRuntime + 0x20);
    if ((pending & 1u) != 0) {
        RoboMmioWrite32(gRuntime + 0x20, pending | 1u);
    }
    uint32_t status = RoboMmioRead32(gOperation + 4u);
    if ((status & (1u << 3)) != 0) {
        RoboMmioWrite32(gOperation + 4u, 1u << 3);
    }
    gPolling = 0;
}

bool RoboUsbWaitCommand(void) {
    for (uint32_t spin = 0; spin < 50000u; ++spin) {
        RoboUsbDrain();
        if (gCommandDone != 0) {
            return gCommandCode == 1u;
        }
    }
    return false;
}

bool RoboUsbCommand(uint32_t data0, uint32_t data1, uint32_t status, uint32_t control) {
    gCommandDone = 0;
    gCommandCode = 0;
    RoboRingPush(&gCommandRing, data0, data1, status, control);
    __asm__ volatile("" : : : "memory");
    RoboMmioWrite32(gDoorbell, 0);
    return RoboUsbWaitCommand();
}

bool RoboUsbControl(RoboUsbDevice* device, uint8_t requestType, uint8_t request, uint16_t value, uint16_t index, uint16_t length, bool dataIn) {
    if (gControlData == 0) {
        return false;
    }
    RoboZeroPage(gControlData);
    uint32_t setup0 = requestType | (static_cast<uint32_t>(request) << 8) | (static_cast<uint32_t>(value) << 16);
    uint32_t setup1 = index | (static_cast<uint32_t>(length) << 16);
    uint32_t transferType = length == 0 ? 0u : (dataIn ? 3u : 2u);
    uint32_t setupControl = (2u << 10) | (1u << 6) | (transferType << 16);
    RoboRingPush(&device->ep0Ring, setup0, setup1, 8u, setupControl);
    if (length != 0) {
        uint32_t dataControl = (3u << 10) | (dataIn ? (1u << 16) : 0u);
        RoboRingPush(&device->ep0Ring, static_cast<uint32_t>(gControlData), static_cast<uint32_t>(gControlData >> 32), length, dataControl);
    }
    uint32_t statusDirection = (length != 0 && dataIn) ? 0u : 1u;
    uint64_t statusTrb = RoboRingPush(&device->ep0Ring, 0, 0, 0, (4u << 10) | (1u << 5) | (statusDirection << 16));
    gWaitTrb = statusTrb;
    gWaitSlot = device->slot;
    gWaitDone = 0;
    gWaitCode = 0;
    __asm__ volatile("" : : : "memory");
    RoboMmioWrite32(gDoorbell + static_cast<uint64_t>(device->slot) * 4u, 1u);
    for (uint32_t spin = 0; spin < 50000u; ++spin) {
        RoboUsbDrain();
        if (gWaitDone != 0) {
            gWaitTrb = 0;
            return gWaitCode == 1u || gWaitCode == 13u;
        }
    }
    gWaitTrb = 0;
    return false;
}

uint32_t RoboUsbInterval(uint8_t speed, uint8_t interval) {
    if (interval == 0) {
        interval = 1;
    }
    if (speed >= 3u) {
        return interval - 1u;
    }
    uint32_t value = static_cast<uint32_t>(interval) * 8u;
    uint32_t exponent = 0;
    while (value > 1u && exponent < 15u) {
        value >>= 1u;
        exponent += 1u;
    }
    return exponent;
}

void RoboUsbFillSlot(RoboUsbDevice* device, uint32_t entries) {
    uint32_t* slot = RoboContext(device->input, 1u);
    for (uint32_t index = 0; index < 8u; ++index) {
        slot[index] = 0;
    }
    slot[0] = (entries << 27) | (static_cast<uint32_t>(device->speed) << 20);
    if (device->isHub != 0) {
        slot[0] |= (1u << 26);
    }
    slot[1] = static_cast<uint32_t>(device->port) << 16;
    if (device->isHub != 0) {
        slot[1] |= static_cast<uint32_t>(device->hubPorts) << 24;
    }
    if (device->parentSlot != 0) {
        if (device->speed == 1u || device->speed == 2u) {
            slot[2] = static_cast<uint32_t>(device->parentSlot) | (static_cast<uint32_t>(device->parentPort) << 8);
        } else if (device->speed >= 4u) {
            slot[2] = static_cast<uint32_t>(device->parentSlot) | (static_cast<uint32_t>(device->parentPort) << 8);
            slot[0] |= (static_cast<uint32_t>(device->parentPort) & 0x0Fu);
        }
    }
}

void RoboUsbFillEndpoint(uint32_t* endpoint, uint32_t type, uint32_t maxPacket, uint32_t interval, uint64_t ring) {
    for (uint32_t index = 0; index < 8u; ++index) {
        endpoint[index] = 0;
    }
    endpoint[0] = (interval & 0xFFu) << 16;
    endpoint[1] = (3u << 1) | (type << 3) | (maxPacket << 16);
    endpoint[2] = static_cast<uint32_t>(ring) | 1u;
    endpoint[3] = static_cast<uint32_t>(ring >> 32);
    endpoint[4] = (type == 4u ? 8u : (maxPacket | (maxPacket << 16)));
}

bool RoboUsbAddress(RoboUsbDevice* device) {
    RoboZeroPage(device->input);
    RoboUsbFillSlot(device, 1u);
    RoboUsbFillEndpoint(RoboContext(device->input, 2u), 4u, device->maxPacket, 0, device->ep0Ring.physical);
    uint32_t* control = RoboContext(device->input, 0u);
    control[1] = 0x3u;
    return RoboUsbCommand(static_cast<uint32_t>(device->input), static_cast<uint32_t>(device->input >> 32), 0, (11u << 10) | (static_cast<uint32_t>(device->slot) << 24));
}

bool RoboUsbConfigureEndpoint(RoboUsbDevice* device, uint32_t interval) {
    RoboZeroPage(device->input);
    RoboUsbFillSlot(device, device->dci);
    RoboUsbFillEndpoint(RoboContext(device->input, 1u + device->dci), 7u, device->maxPacket, interval, device->intRing.physical);
    RoboContext(device->input, 0)[1] = (1u << device->dci) | 1u;
    return RoboUsbCommand(static_cast<uint32_t>(device->input), static_cast<uint32_t>(device->input >> 32), 0, (12u << 10) | (static_cast<uint32_t>(device->slot) << 24));
}

void RoboUsbQueueReports(RoboUsbDevice* device) {
    uint32_t length = device->maxPacket;
    if (length == 0 || length > 64u) {
        length = 8u;
    }
    device->maxPacket = static_cast<uint16_t>(length);
    for (uint32_t index = 0; index < 8u; ++index) {
        uint64_t buffer = device->buffers + index * 64u;
        RoboRingPush(&device->intRing, static_cast<uint32_t>(buffer), static_cast<uint32_t>(buffer >> 32), length, (1u << 10) | (1u << 5));
    }
    device->running = 1;
    __asm__ volatile("" : : : "memory");
    RoboMmioWrite32(gDoorbell + static_cast<uint64_t>(device->slot) * 4u, device->dci);
}

bool RoboUsbPrepare(RoboUsbDevice* device, uint8_t protocol) {
    if (!RoboUsbControl(device, 0x80u, 6u, 0x0100u, 0, 18u, true)) {
        return false;
    }
    uint8_t* descriptor = reinterpret_cast<uint8_t*>(gControlData);
    uint8_t devClass = descriptor[4];
    uint8_t realMaxPacket0 = descriptor[7];
    if (device->speed < 4u && realMaxPacket0 >= 8u && realMaxPacket0 <= 64u && realMaxPacket0 != device->maxPacket) {
        device->maxPacket = realMaxPacket0;
        RoboZeroPage(device->input);
        RoboUsbFillSlot(device, 1u);
        RoboUsbFillEndpoint(RoboContext(device->input, 2u), 4u, device->maxPacket, 0, device->ep0Ring.physical);
        RoboContext(device->input, 0)[1] = 0x2u;
        RoboUsbCommand(static_cast<uint32_t>(device->input), static_cast<uint32_t>(device->input >> 32), 0, (13u << 10) | (static_cast<uint32_t>(device->slot) << 24));
    }
    if (!RoboUsbControl(device, 0x80u, 6u, 0x0200u, 0, 9u, true)) {
        return false;
    }
    uint16_t total = descriptor[2] | (static_cast<uint16_t>(descriptor[3]) << 8);
    uint8_t configuration = descriptor[5];
    if (total < 9u) {
        total = 9u;
    }
    if (total > 1024u) {
        total = 1024u;
    }
    if (!RoboUsbControl(device, 0x80u, 6u, 0x0200u, 0, total, true)) {
        return false;
    }
    uint8_t interfaceNumber = 0;
    uint8_t endpointAddress = 0;
    uint16_t endpointPacket = 0;
    uint8_t endpointInterval = 1;
    uint16_t reportLength = 0;
    bool hid = false;
    bool isHub = devClass == 0x09u;
    for (uint32_t offset = 0; offset + 2u <= total;) {
        uint8_t len = descriptor[offset];
        uint8_t type = descriptor[offset + 1u];
        if (len < 2u || offset + len > total) {
            break;
        }
        if (type == 4u && len >= 9u) {
            if (descriptor[offset + 5u] == 0x09u) {
                isHub = true;
            }
            hid = descriptor[offset + 5u] == 3u;
            interfaceNumber = descriptor[offset + 2u];
            protocol = descriptor[offset + 7u];
            endpointAddress = 0;
        } else if (type == 0x21u && hid && len >= 9u) {
            reportLength = static_cast<uint16_t>(descriptor[offset + 7u] | (descriptor[offset + 8u] << 8));
        } else if (type == 5u && hid && len >= 7u && endpointAddress == 0) {
            uint8_t address = descriptor[offset + 2u];
            uint8_t attributes = descriptor[offset + 3u];
            if ((address & 0x80u) != 0 && (attributes & 3u) == 3u) {
                endpointAddress = address;
                endpointPacket = static_cast<uint16_t>((descriptor[offset + 4u] | (descriptor[offset + 5u] << 8)) & 0x7FFu);
                endpointInterval = descriptor[offset + 6u];
            }
        }
        offset += len;
    }
    if (isHub) {
        if (!RoboUsbControl(device, 0x00u, 9u, configuration, 0, 0, false)) {
            return false;
        }
        uint16_t hubDescType = device->speed >= 4u ? 0x2A00u : 0x2900u;
        uint8_t numPorts = 4u;
        if (RoboUsbControl(device, 0xA0u, 6u, hubDescType, 0, 8u, true)) {
            numPorts = descriptor[2];
            if (numPorts == 0 || numPorts > 16u) {
                numPorts = 4u;
            }
        }
        device->isHub = 1;
        device->hubPorts = numPorts;
        device->kind = ROBO_USB_KIND_HUB;
        for (uint8_t port = 1; port <= numPorts; ++port) {
            RoboUsbControl(device, 0x23u, 3u, 8u, port, 0, false);
        }
        RoboUsbDelay(20000u);
        RoboZeroPage(device->input);
        RoboUsbFillSlot(device, 1u);
        RoboContext(device->input, 0)[1] = 1u;
        RoboUsbCommand(static_cast<uint32_t>(device->input), static_cast<uint32_t>(device->input >> 32), 0, (13u << 10) | (static_cast<uint32_t>(device->slot) << 24));
        return true;
    }
    if (endpointAddress == 0) {
        return false;
    }
    device->fieldCount = 0;
    if (reportLength > 0) {
        if (reportLength > 1024u) {
            reportLength = 1024u;
        }
        if (RoboUsbControl(device, 0x81u, 6u, 0x2200u, interfaceNumber, reportLength, true)) {
            RoboHidParse(device, descriptor, reportLength);
        }
    }
    if (device->kind == ROBO_USB_KIND_KEYBOARD || (device->fieldCount == 0 && protocol == 1u)) {
        device->kind = ROBO_USB_KIND_KEYBOARD;
    } else if (device->fieldCount == 0 && (protocol == 2u || protocol == 0u)) {
        RoboHidBootMouse(device);
    }
    if (device->fieldCount == 0 && device->kind != ROBO_USB_KIND_KEYBOARD) {
        return false;
    }
    if (!RoboUsbControl(device, 0x00u, 9u, configuration, 0, 0, false)) {
        return false;
    }
    RoboUsbControl(device, 0x21u, 0x0Au, 0, interfaceNumber, 0, false);
    if (device->kind == ROBO_USB_KIND_KEYBOARD) {
        RoboUsbControl(device, 0x21u, 0x0Bu, 0, interfaceNumber, 0, false);
    }
    uint8_t endpointNumber = endpointAddress & 0x0Fu;
    device->dci = static_cast<uint8_t>(endpointNumber * 2u + 1u);
    if (device->dci < 2u || device->dci > 31u) {
        return false;
    }
    if (endpointPacket > 0 && endpointPacket <= 64u) {
        device->maxPacket = endpointPacket;
    }
    uint64_t interruptPage = RoboPageAlloc();
    uint64_t bufferPage = RoboPageAlloc();
    if (interruptPage == 0 || bufferPage == 0) {
        return false;
    }
    device->buffers = bufferPage;
    RoboRingInit(&device->intRing, interruptPage);
    if (!RoboUsbConfigureEndpoint(device, RoboUsbInterval(device->speed, endpointInterval))) {
        return false;
    }
    RoboUsbQueueReports(device);
    return true;
}

void RoboUsbEnumerate(uint32_t port, uint32_t portsc) {
    uint32_t slotIndex = 0;
    while (slotIndex < ROBO_USB_DEVICE_MAX && gDevices[slotIndex].used) {
        slotIndex += 1u;
    }
    if (slotIndex >= ROBO_USB_DEVICE_MAX || gDcba == 0) {
        return;
    }
    uint8_t speed = static_cast<uint8_t>((portsc >> 10) & 0x0Fu);
    if (speed < 1u || speed > 5u) {
        speed = 1u;
    }
    if (!RoboUsbCommand(0, 0, 0, 9u << 10) || gCommandSlot == 0 || gCommandSlot > gHost->maxSlots) {
        return;
    }
    uint64_t output = RoboPageAlloc();
    uint64_t input = RoboPageAlloc();
    uint64_t ep0 = RoboPageAlloc();
    if (output == 0 || input == 0 || ep0 == 0) {
        return;
    }
    RoboUsbDevice* device = &gDevices[slotIndex];
    *device = {};
    device->used = 1;
    device->slot = gCommandSlot;
    device->host = gHost->index;
    device->speed = speed;
    device->port = static_cast<uint8_t>(port);
    device->maxPacket = (speed >= 4u) ? 512u : (speed == 3u ? 64u : (speed == 1u ? 64u : 8u));
    device->output = output;
    device->input = input;
    RoboRingInit(&device->ep0Ring, ep0);
    reinterpret_cast<uint64_t*>(gDcba)[device->slot] = output;
    if (!RoboUsbAddress(device)) {
        device->used = 0;
        return;
    }
    RoboUsbPrepare(device, 0);
}

void RoboUsbEnumerateChild(RoboUsbDevice* parentHub, uint8_t parentPort, uint8_t childSpeed) {
    uint32_t slotIndex = 0;
    while (slotIndex < ROBO_USB_DEVICE_MAX && gDevices[slotIndex].used) {
        slotIndex += 1u;
    }
    if (slotIndex >= ROBO_USB_DEVICE_MAX || gDcba == 0) {
        return;
    }
    if (childSpeed < 1u || childSpeed > 5u) {
        childSpeed = 1u;
    }
    if (!RoboUsbCommand(0, 0, 0, 9u << 10) || gCommandSlot == 0 || gCommandSlot > gHost->maxSlots) {
        return;
    }
    uint64_t output = RoboPageAlloc();
    uint64_t input = RoboPageAlloc();
    uint64_t ep0 = RoboPageAlloc();
    if (output == 0 || input == 0 || ep0 == 0) {
        return;
    }
    RoboUsbDevice* device = &gDevices[slotIndex];
    *device = {};
    device->used = 1;
    device->slot = gCommandSlot;
    device->host = parentHub->host;
    device->speed = childSpeed;
    device->port = parentHub->port;
    device->parentSlot = parentHub->slot;
    device->parentPort = parentPort;
    device->maxPacket = (childSpeed >= 4u) ? 512u : (childSpeed == 3u ? 64u : (childSpeed == 1u ? 64u : 8u));
    device->output = output;
    device->input = input;
    RoboRingInit(&device->ep0Ring, ep0);
    reinterpret_cast<uint64_t*>(gDcba)[device->slot] = output;
    if (!RoboUsbAddress(device)) {
        device->used = 0;
        return;
    }
    RoboUsbPrepare(device, 0);
}

bool RoboUsbPortFree(uint32_t port) {
    for (uint32_t index = 0; index < ROBO_USB_DEVICE_MAX; ++index) {
        if (gDevices[index].used && gDevices[index].host == gHost->index && gDevices[index].port == port && gDevices[index].parentSlot == 0) {
            return false;
        }
    }
    return true;
}

void RoboUsbResetPort(uint32_t port) {
    uint8_t kind = port < 33u ? gHost->portKind[port] : 0;
    if (kind != ROBO_PORT_KIND_USB && kind != ROBO_PORT_KIND_USBC && kind != ROBO_PORT_KIND_THUNDERBOLT) {
        return;
    }
    if (!RoboUsbPortFree(port)) {
        return;
    }
    uint64_t reg = gOperation + 0x400ull + (static_cast<uint64_t>(port) - 1ull) * 0x10ull;
    uint32_t portsc = RoboMmioRead32(reg);
    if ((portsc & (1u << 9)) == 0) {
        RoboMmioWrite32(reg, (portsc & ~ROBO_PORT_CHANGE) | (1u << 9));
        RoboUsbDelayMs(20u);
        portsc = RoboMmioRead32(reg);
    }
    if ((portsc & 1u) == 0) {
        return;
    }
    if ((portsc & 2u) == 0) {
        RoboMmioWrite32(reg, (portsc & ~ROBO_PORT_CHANGE) | (1u << 9) | (1u << 4));
        bool resetDone = false;
        for (uint32_t spin = 0; spin < 100u; ++spin) {
            RoboUsbDelayMs(1u);
            portsc = RoboMmioRead32(reg);
            if ((portsc & (1u << 4)) == 0) {
                resetDone = true;
                break;
            }
        }
        if (!resetDone) {
            return;
        }
        RoboMmioWrite32(reg, (1u << 9) | (portsc & ROBO_PORT_CHANGE));
        RoboUsbDelayMs(10u);
        portsc = RoboMmioRead32(reg);
    }
    if ((portsc & 1u) == 0 || (portsc & 2u) == 0) {
        return;
    }
    RoboUsbEnumerate(port, portsc);
}

void RoboUsbScanPending(uint32_t pending) {
    for (uint32_t port = 1; port <= gMaxPorts && port <= 31u; ++port) {
        if ((pending & (1u << port)) != 0) {
            RoboUsbResetPort(port);
        }
    }
}

bool RoboUsbScratchpad(uint32_t count) {
    if (count == 0) {
        return true;
    }
    if (count > 16u || gDcba == 0) {
        return false;
    }
    uint64_t array = RoboPageAlloc();
    if (array == 0) {
        return false;
    }
    for (uint32_t index = 0; index < count; ++index) {
        uint64_t page = RoboPageAlloc();
        if (page == 0) {
            return false;
        }
        reinterpret_cast<uint64_t*>(array)[index] = page;
    }
    reinterpret_cast<uint64_t*>(gDcba)[0] = array;
    return true;
}

void RoboUsbLegacy(uint64_t bar, uint32_t hcc) {
    uint32_t ecp = (hcc >> 16) & 0xFFFFu;
    if (ecp == 0) {
        return;
    }
    uint64_t cap = bar + static_cast<uint64_t>(ecp) * 4ull;
    for (uint32_t guard = 0; guard < 16u && cap != 0; ++guard) {
        uint32_t value = RoboMmioRead32(cap);
        uint8_t id = static_cast<uint8_t>(value);
        uint8_t next = static_cast<uint8_t>(value >> 8);
        if (id == 1u) {
            RoboMmioWrite32(cap, value | (1u << 24));
            bool handedOff = false;
            for (uint32_t spin = 0; spin < 10000u; ++spin) {
                if ((RoboMmioRead32(cap) & (1u << 16)) == 0) {
                    handedOff = true;
                    break;
                }
                RoboUsbDelay(500u);
            }
            if (!handedOff) {
                RoboMmioWrite32(cap, (RoboMmioRead32(cap) & ~(1u << 16)) | (1u << 24));
            }
            RoboMmioWrite32(cap + 4u, 0xE0000000u);
        }
        if (next == 0) {
            break;
        }
        cap += static_cast<uint64_t>(next) * 4ull;
    }
}

bool RoboUsbReset(void) {
    for (uint32_t spin = 0; spin < 50000u; ++spin) {
        if ((RoboMmioRead32(gOperation + 4u) & (1u << 12)) == 0) {
            break;
        }
        RoboUsbDelay(500u);
    }
    uint32_t command = RoboMmioRead32(gOperation);
    if ((command & 1u) != 0) {
        RoboMmioWrite32(gOperation, command & ~1u);
        bool halted = false;
        for (uint32_t spin = 0; spin < 10000u; ++spin) {
            if ((RoboMmioRead32(gOperation + 4u) & 1u) != 0) {
                halted = true;
                break;
            }
            RoboUsbDelay(500u);
        }
        if (!halted) {
            return false;
        }
    }
    RoboMmioWrite32(gOperation, RoboMmioRead32(gOperation) | 2u);
    bool resetDone = false;
    for (uint32_t spin = 0; spin < 20000u; ++spin) {
        if ((RoboMmioRead32(gOperation) & 2u) == 0) {
            resetDone = true;
            break;
        }
        RoboUsbDelay(500u);
    }
    if (!resetDone) {
        return false;
    }
    bool ready = false;
    for (uint32_t spin = 0; spin < 50000u; ++spin) {
        uint32_t status = RoboMmioRead32(gOperation + 4u);
        if ((status & (1u << 12)) == 0) {
            ready = true;
            break;
        }
        RoboUsbDelay(500u);
    }
    return ready;
}

void RoboUsbClassify(uint64_t bar, uint32_t hcc) {
    uint32_t ports = gMaxPorts > 32u ? 32u : gMaxPorts;
    for (uint32_t port = 1; port <= ports; ++port) {
        gHost->portKind[port] = ROBO_PORT_KIND_USB;
    }
    uint32_t ecp = (hcc >> 16) & 0xFFFFu;
    if (ecp == 0) {
        return;
    }
    uint64_t cap = bar + static_cast<uint64_t>(ecp) * 4ull;
    for (uint32_t guard = 0; guard < 16u && cap != 0; ++guard) {
        uint32_t value = RoboMmioRead32(cap);
        uint8_t id = static_cast<uint8_t>(value);
        uint8_t next = static_cast<uint8_t>(value >> 8);
        if (id == 2u) {
            uint32_t major = value >> 24;
            uint32_t info = RoboMmioRead32(cap + 8u);
            uint32_t offset = info & 0xFFu;
            uint32_t count = (info >> 8) & 0xFFu;
            uint8_t kind = ROBO_PORT_KIND_USB;
            if (major >= 3u) {
                kind = gThunderbolt != 0 ? ROBO_PORT_KIND_THUNDERBOLT : ROBO_PORT_KIND_USBC;
            }
            for (uint32_t index = 0; index < count; ++index) {
                uint32_t port = offset + index;
                if (port >= 1u && port <= ports) {
                    gHost->portKind[port] = kind;
                }
            }
        }
        if (next == 0) {
            break;
        }
        cap += static_cast<uint64_t>(next) * 4ull;
    }
}

void RoboUsbBringUp(const RoboPciDevice* pci, uint8_t index) {
    if (pci == nullptr || index >= ROBO_USB_HOST_MAX) {
        return;
    }
    gHost = &gHosts[index];
    *gHost = {};
    gHost->index = index;
    gHost->irq = pci->irq;
    gHost->contextBytes = 32;
    RoboMemoryMarkUncached(pci->bar, 0x100000ull);
    uint32_t cap0 = RoboMmioRead32(pci->bar);
    uint32_t capLength = cap0 & 0xFFu;
    if (capLength < 0x20u) {
        return;
    }
    uint32_t hcs1 = RoboMmioRead32(pci->bar + 4u);
    uint32_t hcs2 = RoboMmioRead32(pci->bar + 8u);
    uint32_t hcc = RoboMmioRead32(pci->bar + 0x10u);
    uint32_t doorbell = RoboMmioRead32(pci->bar + 0x14u) & ~0x3u;
    uint32_t runtime = RoboMmioRead32(pci->bar + 0x18u) & ~0x1Fu;
    gContextBytes = (hcc & (1u << 2)) != 0 ? 64u : 32u;
    gHost->contextBytes = gContextBytes;
    gMaxPorts = (hcs1 >> 24) & 0xFFu;
    uint32_t maxSlots = hcs1 & 0xFFu;
    if (gMaxPorts == 0 || maxSlots == 0) {
        return;
    }
    if (maxSlots > ROBO_USB_DEVICE_MAX) {
        maxSlots = ROBO_USB_DEVICE_MAX;
    }
    gHost->maxSlots = maxSlots;
    RoboUsbLegacy(pci->bar, hcc);
    RoboUsbClassify(pci->bar, hcc);
    gOperation = pci->bar + capLength;
    gDoorbell = pci->bar + doorbell;
    gRuntime = pci->bar + runtime;
    if (!RoboUsbReset() || (RoboMmioRead32(gOperation + 8u) & 1u) == 0) {
        gOperation = 0;
        return;
    }
    gDcba = RoboPageAlloc();
    uint64_t command = RoboPageAlloc();
    uint64_t events = RoboPageAlloc();
    uint64_t erst = RoboPageAlloc();
    gControlData = RoboPageAlloc();
    if (gDcba == 0 || command == 0 || events == 0 || erst == 0 || gControlData == 0) {
        return;
    }
    uint32_t scratch = ((hcs2 >> 27) & 0x1Fu) | (((hcs2 >> 21) & 0x1Fu) << 5);
    if (!RoboUsbScratchpad(scratch)) {
        return;
    }
    RoboRingInit(&gCommandRing, command);
    gEvents = reinterpret_cast<RoboTrb*>(events);
    gEventPhysical = events;
    gEventDequeue = 0;
    gEventCycle = 1;
    *reinterpret_cast<uint64_t*>(erst) = events;
    *reinterpret_cast<uint32_t*>(erst + 8u) = ROBO_TRB_COUNT;
    RoboMmioWrite64(gOperation + 0x30u, gDcba);
    RoboMmioWrite64(gOperation + 0x18u, gCommandRing.physical | 1ull);
    RoboMmioWrite32(gRuntime + 0x28u, 1u);
    RoboMmioWrite64(gRuntime + 0x30u, erst);
    RoboMmioWrite64(gRuntime + 0x38u, events);
    RoboMmioWrite32(gOperation + 0x38u, maxSlots);
    RoboMmioWrite32(gRuntime + 0x20u, 2u);
    RoboMmioWrite32(gOperation, RoboMmioRead32(gOperation) | 1u | 4u);
    bool running = false;
    for (uint32_t spin = 0; spin < 2000u; ++spin) {
        if ((RoboMmioRead32(gOperation + 4u) & 1u) == 0) {
            running = true;
            break;
        }
    }
    if (!running) {
        return;
    }
    gHost->running = 1;
    gHost->scanPort = 1;
    for (uint32_t port = 1; port <= gMaxPorts && port <= 32u; ++port) {
        uint64_t reg = gOperation + 0x400ull + (static_cast<uint64_t>(port) - 1ull) * 0x10ull;
        uint32_t portsc = RoboMmioRead32(reg);
        RoboMmioWrite32(reg, (portsc & ~ROBO_PORT_CHANGE) | (1u << 9));
    }
    RoboUsbDelayMs(20u);
    if (gHost->irq >= 1u && gHost->irq <= 15u) {
        RoboArchIrqClaim(gHost->irq);
    }
}

void RoboUsbScanHubs(void) {
    for (uint32_t d = 0; d < ROBO_USB_DEVICE_MAX; ++d) {
        RoboUsbDevice* hub = &gDevices[d];
        if (!hub->used || !hub->isHub || hub->host != gHost->index) {
            continue;
        }
        for (uint8_t p = 1; p <= hub->hubPorts; ++p) {
            bool childFound = false;
            for (uint32_t c = 0; c < ROBO_USB_DEVICE_MAX; ++c) {
                if (gDevices[c].used && gDevices[c].parentSlot == hub->slot && gDevices[c].parentPort == p) {
                    childFound = true;
                    break;
                }
            }
            if (childFound) {
                continue;
            }
            if (!RoboUsbControl(hub, 0xA3u, 0u, 0u, p, 4u, true)) {
                continue;
            }
            uint32_t portStatus = *reinterpret_cast<uint32_t*>(gControlData);
            if ((portStatus & 1u) != 0) {
                if ((portStatus & 2u) == 0) {
                    RoboUsbControl(hub, 0x23u, 3u, 4u, p, 0, false);
                    bool resetDone = false;
                    for (uint32_t spin = 0; spin < 50u; ++spin) {
                        RoboUsbDelayMs(2u);
                        if (RoboUsbControl(hub, 0xA3u, 0u, 0u, p, 4u, true)) {
                            portStatus = *reinterpret_cast<uint32_t*>(gControlData);
                            if ((portStatus & (1u << 4)) == 0) {
                                resetDone = true;
                                break;
                            }
                        }
                    }
                    if (!resetDone) {
                        continue;
                    }
                    RoboUsbControl(hub, 0x23u, 1u, 20u, p, 0, false);
                    RoboUsbDelayMs(10u);
                    if (RoboUsbControl(hub, 0xA3u, 0u, 0u, p, 4u, true)) {
                        portStatus = *reinterpret_cast<uint32_t*>(gControlData);
                    }
                }
                if ((portStatus & 1u) != 0 && (portStatus & 2u) != 0) {
                    uint8_t childSpeed = 1u;
                    if ((portStatus & (1u << 9)) != 0) {
                        childSpeed = 2u;
                    } else if ((portStatus & (1u << 10)) != 0) {
                        childSpeed = 3u;
                    } else if (hub->speed >= 4u) {
                        childSpeed = 4u;
                    }
                    RoboUsbEnumerateChild(hub, p, childSpeed);
                }
            }
        }
    }
}

}  // namespace

void RoboUsbCollect(void) {
    if (gPolling != 0) {
        return;
    }
    for (uint32_t index = 0; index < ROBO_USB_HOST_MAX; ++index) {
        if (gHosts[index].running == 0) {
            continue;
        }
        gHost = &gHosts[index];
        RoboUsbDrain();
        if (gHost->pending != 0) {
            uint32_t pending = gHost->pending;
            gHost->pending = 0;
            RoboUsbScanPending(pending);
        }
        if (gHost->scanPort != 0) {
            gMaxPorts = gHost->maxPorts;
            RoboUsbResetPort(gHost->scanPort);
            gHost->scanPort += 1u;
            if (gHost->scanPort > gHost->maxPorts || gHost->scanPort > 32u) {
                gHost->scanPort = 0;
            }
        } else {
            if (++gHost->scanCounter >= 30u) {
                gHost->scanCounter = 0;
                gHost->scanPort = 1;
            }
        }
        RoboUsbScanHubs();
    }
}

RoboPciDevice gUsbHosts[ROBO_USB_HOST_MAX];
uint32_t gUsbHostCount = 0;
uint32_t gUsbHostNext = 0;
bool gUsbHostsListed = false;

bool RoboUsbStart(void) {
    if (!gUsbHostsListed) {
        gUsbHostsListed = true;
        if (!RoboPciCollectUsb(gUsbHosts, ROBO_USB_HOST_MAX, &gUsbHostCount, &gThunderbolt) || gUsbHostCount == 0) {
            return true;
        }
        return false;
    }
    if (gUsbHostNext >= gUsbHostCount) {
        return true;
    }
    RoboUsbBringUp(&gUsbHosts[gUsbHostNext], static_cast<uint8_t>(gUsbHostNext));
    gUsbHostNext += 1u;
    return gUsbHostNext >= gUsbHostCount;
}
