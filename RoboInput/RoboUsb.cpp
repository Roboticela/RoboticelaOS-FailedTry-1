#include "RoboUsb.hpp"

#include "RoboArch.hpp"
#include "RoboInput.hpp"
#include "RoboMemory.hpp"
#include "RoboPci.hpp"

namespace {

constexpr uint32_t ROBO_USB_DEVICE_MAX = 4;
constexpr uint32_t ROBO_HID_FIELD_MAX = 12;
constexpr uint32_t ROBO_TRB_COUNT = 256;
constexpr uint32_t ROBO_USB_KIND_MOUSE = 1;
constexpr uint32_t ROBO_USB_KIND_TOUCHPAD = 2;
constexpr uint32_t ROBO_USB_KIND_TOUCH = 3;
constexpr uint32_t ROBO_PORT_CHANGE = 0x00FE0000u;

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
    uint16_t maxPacket;
    RoboRing endpoint;
    uint64_t output;
    uint64_t input;
    uint64_t buffers;
    RoboHidField fields[ROBO_HID_FIELD_MAX];
    uint8_t fieldCount;
    uint8_t contact;
    int32_t lastX;
    int32_t lastY;
    int32_t lastXMax;
    int32_t lastYMax;
};

RoboPciDevice gPci{};
uint64_t gOperation = 0;
uint64_t gRuntime = 0;
uint64_t gDoorbell = 0;
uint32_t gContextBytes = 32;
uint32_t gMaxPorts = 0;
uint8_t gRunning = 0;
uint8_t gPolling = 0;
RoboRing gCommandRing{};
RoboTrb* gEvents = nullptr;
uint64_t gEventPhysical = 0;
uint32_t gEventDequeue = 0;
uint32_t gEventCycle = 1;
uint64_t gControlData = 0;
uint64_t gDcba = 0;
uint64_t gWaitTrb = 0;
uint8_t gWaitDone = 0;
uint8_t gWaitCode = 0;
uint8_t gCommandDone = 0;
uint8_t gCommandCode = 0;
uint8_t gCommandSlot = 0;
RoboUsbDevice gDevices[ROBO_USB_DEVICE_MAX]{};

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
    bool useful = (page == 0x01u && (usage == 0x30u || usage == 0x31u)) || (page == 0x0Du && (usage == 0x30u || usage == 0x31u || usage == 0x42u));
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
    if (appPage == 0x0Du && appUsage == 0x04u) {
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
    RoboHidAddField(device, 0x01u, 0x30u, 8u, 8u, 0, 1u, -127, 127);
    RoboHidAddField(device, 0x01u, 0x31u, 16u, 8u, 0, 1u, -127, 127);
    device->kind = ROBO_USB_KIND_MOUSE;
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
        const RoboHidField* button = RoboHidFind(device, 0x09u, 0x01u, reportId);
        uint8_t left = button != nullptr && RoboHidValue(report, length, button) != 0 ? 1u : 0u;
        uint8_t changed = left != device->contact ? 1u : 0u;
        if (x == 0 && y == 0 && changed == 0) {
            return;
        }
        device->contact = left;
        event.kind = ROBO_INPUT_MOTION;
        event.dx = x;
        event.dy = y;
        event.pressed = left;
        event.code = changed;
        RoboInputPush(&event);
        return;
    }
    const RoboHidField* tip = RoboHidFind(device, 0x0Du, 0x42u, reportId);
    uint8_t pressed = 1u;
    if (tip != nullptr) {
        pressed = RoboHidValue(report, length, tip) != 0 ? 1u : 0u;
    }
    if (pressed == 0) {
        if (device->contact == 0) {
            return;
        }
        device->contact = 0;
        event.kind = device->kind == ROBO_USB_KIND_TOUCH ? ROBO_INPUT_TOUCH : ROBO_INPUT_TOUCHPAD;
        event.pressed = 0;
        event.code = 1;
        event.x = device->lastX;
        event.y = device->lastY;
        event.xMax = device->lastXMax;
        event.yMax = device->lastYMax;
        RoboInputPush(&event);
        return;
    }
    if (xField->logicalMax <= 0 || yField->logicalMax <= 0) {
        return;
    }
    event.code = device->contact == 0 ? 1u : 0u;
    device->contact = 1;
    device->lastX = x;
    device->lastY = y;
    device->lastXMax = xField->logicalMax;
    device->lastYMax = yField->logicalMax;
    event.kind = device->kind == ROBO_USB_KIND_TOUCH ? ROBO_INPUT_TOUCH : ROBO_INPUT_TOUCHPAD;
    event.pressed = pressed;
    event.x = x;
    event.y = y;
    event.xMax = xField->logicalMax;
    event.yMax = yField->logicalMax;
    RoboInputPush(&event);
}

void RoboUsbRequeue(RoboUsbDevice* device, uint64_t trbAddress) {
    if (trbAddress < device->endpoint.physical) {
        return;
    }
    uint64_t offset = trbAddress - device->endpoint.physical;
    if ((offset % sizeof(RoboTrb)) != 0 || offset / sizeof(RoboTrb) >= ROBO_TRB_COUNT - 1u) {
        return;
    }
    RoboTrb* completed = reinterpret_cast<RoboTrb*>(trbAddress);
    uint32_t length = completed->status & 0x1FFFFu;
    if (length == 0 || length > 1024u) {
        length = device->maxPacket;
    }
    RoboRingPush(&device->endpoint, completed->data0, completed->data1, length, (1u << 10) | (1u << 5));
    __asm__ volatile("" : : : "memory");
    RoboMmioWrite32(gDoorbell + static_cast<uint64_t>(device->slot) * 4u, device->dci);
}

void RoboUsbOnTransfer(uint32_t slot, uint64_t trbAddress, uint32_t code, uint32_t residual) {
    if (gWaitTrb != 0 && (trbAddress & ~0xFull) == (gWaitTrb & ~0xFull)) {
        gWaitDone = 1;
        gWaitCode = static_cast<uint8_t>(code);
        return;
    }
    RoboUsbDevice* device = nullptr;
    for (uint32_t index = 0; index < ROBO_USB_DEVICE_MAX; ++index) {
        if (gDevices[index].used && gDevices[index].slot == slot) {
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
    if (trbAddress < device->endpoint.physical) {
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
    uint64_t reg = gOperation + 0x400u + (port - 1u) * 0x10u;
    uint32_t portsc = RoboMmioRead32(reg);
    RoboMmioWrite32(reg, (1u << 9) | (portsc & ROBO_PORT_CHANGE));
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
    for (uint32_t spin = 0; spin < 500000u; ++spin) {
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
    uint32_t setupControl = (2u << 10) | (1u << 6) | (1u << 4) | (transferType << 16);
    RoboRingPush(&device->endpoint, setup0, setup1, 8u, setupControl);
    if (length != 0) {
        uint32_t dataControl = (3u << 10) | (1u << 4) | (dataIn ? (1u << 16) : 0u);
        RoboRingPush(&device->endpoint, static_cast<uint32_t>(gControlData), static_cast<uint32_t>(gControlData >> 32), length, dataControl);
    }
    uint32_t statusDirection = (length != 0 && dataIn) ? 0u : 1u;
    uint64_t statusTrb = RoboRingPush(&device->endpoint, 0, 0, 0, (4u << 10) | (1u << 5) | (statusDirection << 16));
    gWaitTrb = statusTrb;
    gWaitDone = 0;
    gWaitCode = 0;
    __asm__ volatile("" : : : "memory");
    RoboMmioWrite32(gDoorbell + static_cast<uint64_t>(device->slot) * 4u, 1u);
    for (uint32_t spin = 0; spin < 500000u; ++spin) {
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
    slot[1] = static_cast<uint32_t>(device->port) << 16;
}

void RoboUsbFillEndpoint(uint32_t* endpoint, uint32_t type, uint32_t maxPacket, uint32_t interval, uint64_t ring) {
    for (uint32_t index = 0; index < 8u; ++index) {
        endpoint[index] = 0;
    }
    endpoint[0] = (interval & 0xFFu) << 16;
    endpoint[1] = (3u << 1) | (type << 3) | (maxPacket << 16);
    endpoint[2] = static_cast<uint32_t>(ring) | 1u;
    endpoint[3] = static_cast<uint32_t>(ring >> 32);
    endpoint[4] = (type == 4u ? 8u : maxPacket) | (maxPacket << 16);
}

bool RoboUsbAddress(RoboUsbDevice* device) {
    RoboZeroPage(device->input);
    RoboUsbFillSlot(device, 1u);
    RoboUsbFillEndpoint(RoboContext(device->input, 2u), 4u, device->maxPacket, 0, device->endpoint.physical);
    uint32_t* control = RoboContext(device->input, 0u);
    control[1] = 0x3u;
    return RoboUsbCommand(static_cast<uint32_t>(device->input), static_cast<uint32_t>(device->input >> 32), 0, (11u << 10) | (static_cast<uint32_t>(device->slot) << 24));
}

bool RoboUsbConfigureEndpoint(RoboUsbDevice* device, uint32_t interval) {
    RoboZeroPage(device->input);
    RoboUsbFillSlot(device, device->dci);
    RoboUsbFillEndpoint(RoboContext(device->input, 1u + device->dci), 7u, device->maxPacket, interval, device->endpoint.physical);
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
        RoboRingPush(&device->endpoint, static_cast<uint32_t>(buffer), static_cast<uint32_t>(buffer >> 32), length, (1u << 10) | (1u << 5));
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
    for (uint32_t offset = 0; offset + 2u <= total;) {
        uint8_t len = descriptor[offset];
        uint8_t type = descriptor[offset + 1u];
        if (len < 2u || offset + len > total) {
            break;
        }
        if (type == 4u && len >= 9u) {
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
    if (device->fieldCount == 0 && protocol == 2u) {
        RoboHidBootMouse(device);
    }
    if (device->fieldCount == 0) {
        return false;
    }
    if (!RoboUsbControl(device, 0x00u, 9u, configuration, 0, 0, false)) {
        return false;
    }
    RoboUsbControl(device, 0x21u, 0x0Au, 0, interfaceNumber, 0, false);
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
    RoboRingInit(&device->endpoint, interruptPage);
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
    if (speed < 1u || speed > 4u) {
        speed = 1u;
    }
    if (!RoboUsbCommand(0, 0, 0, 9u << 10) || gCommandSlot == 0 || gCommandSlot > 4u) {
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
    device->speed = speed;
    device->port = static_cast<uint8_t>(port);
    device->maxPacket = speed >= 3u ? 64u : 8u;
    device->output = output;
    device->input = input;
    RoboRingInit(&device->endpoint, ep0);
    reinterpret_cast<uint64_t*>(gDcba)[device->slot] = output;
    if (!RoboUsbAddress(device)) {
        device->used = 0;
        return;
    }
    RoboUsbPrepare(device, 0);
}

void RoboUsbScanPorts(void) {
    for (uint32_t port = 1; port <= gMaxPorts && port <= 32u; ++port) {
        uint64_t reg = gOperation + 0x400ull + (static_cast<uint64_t>(port) - 1ull) * 0x10ull;
        uint32_t portsc = RoboMmioRead32(reg);
        if ((portsc & 1u) == 0) {
            continue;
        }
        RoboMmioWrite32(reg, (1u << 9) | (1u << 4));
        bool resetDone = false;
        for (uint32_t spin = 0; spin < 200000u; ++spin) {
            portsc = RoboMmioRead32(reg);
            if ((portsc & (1u << 4)) == 0) {
                resetDone = true;
                break;
            }
        }
        if (!resetDone) {
            continue;
        }
        RoboMmioWrite32(reg, (1u << 9) | (portsc & ROBO_PORT_CHANGE));
        portsc = RoboMmioRead32(reg);
        if ((portsc & 1u) == 0 || (portsc & 2u) == 0) {
            continue;
        }
        RoboUsbEnumerate(port, portsc);
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
            for (uint32_t spin = 0; spin < 100000u; ++spin) {
                if ((RoboMmioRead32(cap) & (1u << 16)) == 0) {
                    break;
                }
            }
        }
        if (next == 0) {
            break;
        }
        cap += static_cast<uint64_t>(next) * 4ull;
    }
}

bool RoboUsbReset(void) {
    uint32_t command = RoboMmioRead32(gOperation);
    if ((command & 1u) != 0) {
        RoboMmioWrite32(gOperation, command & ~1u);
        bool halted = false;
        for (uint32_t spin = 0; spin < 200000u; ++spin) {
            if ((RoboMmioRead32(gOperation + 4u) & 1u) != 0) {
                halted = true;
                break;
            }
        }
        if (!halted) {
            return false;
        }
    }
    RoboMmioWrite32(gOperation, RoboMmioRead32(gOperation) | 2u);
    bool resetDone = false;
    for (uint32_t spin = 0; spin < 200000u; ++spin) {
        if ((RoboMmioRead32(gOperation) & 2u) == 0) {
            resetDone = true;
            break;
        }
    }
    if (!resetDone) {
        return false;
    }
    for (uint32_t spin = 0; spin < 200000u; ++spin) {
        uint32_t status = RoboMmioRead32(gOperation + 4u);
        if ((status & (1u << 12)) != 0) {
            return false;
        }
        if ((status & (1u << 11)) == 0) {
            return true;
        }
    }
    return false;
}

}  // namespace

void RoboUsbCollect(void) {
    if (gRunning == 0) {
        return;
    }
    RoboUsbDrain();
}

void RoboUsbStart(void) {
    if (!RoboPciFindXhci(&gPci)) {
        return;
    }
    RoboMemoryMarkUncached(gPci.bar, 4096u);
    uint32_t cap0 = RoboMmioRead32(gPci.bar);
    uint32_t capLength = cap0 & 0xFFu;
    if (capLength < 0x20u) {
        return;
    }
    uint32_t hcs1 = RoboMmioRead32(gPci.bar + 4u);
    uint32_t hcs2 = RoboMmioRead32(gPci.bar + 8u);
    uint32_t hcc = RoboMmioRead32(gPci.bar + 0x10u);
    uint32_t doorbell = RoboMmioRead32(gPci.bar + 0x14u) & ~0x3u;
    uint32_t runtime = RoboMmioRead32(gPci.bar + 0x18u) & ~0x1Fu;
    gContextBytes = (hcc & (1u << 2)) != 0 ? 64u : 32u;
    gMaxPorts = (hcs1 >> 24) & 0xFFu;
    uint32_t maxSlots = hcs1 & 0xFFu;
    if (gMaxPorts == 0 || maxSlots == 0) {
        return;
    }
    if (maxSlots > ROBO_USB_DEVICE_MAX) {
        maxSlots = ROBO_USB_DEVICE_MAX;
    }
    RoboUsbLegacy(gPci.bar, hcc);
    gOperation = gPci.bar + capLength;
    gDoorbell = gPci.bar + doorbell;
    gRuntime = gPci.bar + runtime;
    if ((RoboMmioRead32(gOperation + 8u) & 1u) == 0 || !RoboUsbReset()) {
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
    for (uint32_t spin = 0; spin < 200000u; ++spin) {
        if ((RoboMmioRead32(gOperation + 4u) & 1u) == 0) {
            running = true;
            break;
        }
    }
    if (!running) {
        return;
    }
    gRunning = 1;
    if (gPci.irq >= 1u && gPci.irq <= 15u) {
        RoboArchIrqClaim(gPci.irq);
    }
    RoboUsbScanPorts();
}
