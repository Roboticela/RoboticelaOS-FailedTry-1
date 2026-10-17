#include "RoboCall.hpp"

#include "RoboDesktop.hpp"
#include "RoboFS.hpp"
#include "RoboFrame.hpp"
#include "RoboInput.hpp"
#include "RoboProgram.hpp"
#include "RoboSchedule.hpp"
#include "RoboTimer.hpp"
#include "RoboNet.hpp"
#include "RoboSound.hpp"
#include "RoboPower.hpp"

namespace {

constexpr uint32_t ROBO_CALL_QUEUE = 16u;
constexpr char ROBO_CALL_APP[] = "App";

RoboDesktop* gDesktop = nullptr;
uint8_t gFocused = 0;
uint64_t gCodeBytes = 0;
uint64_t gHeapBytes = 0;
RoboCallInputEvent gQueue[ROBO_CALL_QUEUE];
uint32_t gQueueHead = 0;
uint32_t gQueueTail = 0;
uint8_t gFile[ROBO_CALL_FILE_MAX];

bool RoboCallInside(uint64_t address, uint64_t bytes, uint64_t base, uint64_t size) {
    if (size == 0 || bytes == 0 || bytes > size) {
        return false;
    }
    if (address < base) {
        return false;
    }
    return address - base <= size - bytes;
}

bool RoboCallUser(uint64_t address, uint64_t bytes) {
    if (address + bytes < address) {
        return false;
    }
    if (RoboCallInside(address, bytes, ROBO_USER_CODE, gCodeBytes)) {
        return true;
    }
    if (RoboCallInside(address, bytes, ROBO_USER_STACK, ROBO_PROGRAM_PAGE)) {
        return true;
    }
    return RoboCallInside(address, bytes, ROBO_USER_HEAP, gHeapBytes);
}

bool RoboCallCopyString(uint64_t address, char* dest, uint32_t capacity) {
    if (dest == nullptr || capacity < 2u || !RoboCallUser(address, 1u)) {
        return false;
    }
    for (uint32_t index = 0; index + 1u < capacity; ++index) {
        if (!RoboCallUser(address + index, 1u)) {
            return false;
        }
        char letter = *reinterpret_cast<const char*>(address + index);
        dest[index] = letter;
        if (letter == 0) {
            return index > 0;
        }
    }
    return false;
}

void RoboCallEnqueue(const RoboCallInputEvent* event) {
    uint32_t next = (gQueueTail + 1u) % ROBO_CALL_QUEUE;
    if (next == gQueueHead) {
        return;
    }
    gQueue[gQueueTail] = *event;
    gQueueTail = next;
}

bool RoboCallHit(int32_t x, int32_t y) {
    RoboFrameView view{};
    if (gFocused == 0 || !RoboFrameAppView(&view)) {
        return false;
    }
    return x >= view.screenX && y >= view.screenY && x < view.screenX + view.width && y < view.screenY + view.height;
}

uint32_t RoboCallPack(uint8_t red, uint8_t green, uint8_t blue) {
    return (static_cast<uint32_t>(red) << 16) | (static_cast<uint32_t>(green) << 8) | blue;
}

int RoboCallFile(uint64_t number, uint64_t path, uint64_t data, uint64_t capacityArg, uint64_t sizeOut) {
    char name[96];
    if (!RoboCallCopyString(path, name, sizeof(name))) {
        return ROBO_FS_DENIED;
    }
    uint32_t capacity = static_cast<uint32_t>(capacityArg);
    if (capacity > ROBO_CALL_FILE_MAX) {
        capacity = ROBO_CALL_FILE_MAX;
    }
    int status = ROBO_FS_DENIED;
    uint32_t copied = 0;
    if (number == ROBO_CALL_READ) {
        if (capacity == 0 || !RoboCallUser(data, capacity)) {
            return ROBO_FS_DENIED;
        }
        uint32_t got = 0;
        status = RoboFsReadApp(ROBO_CALL_APP, name, gFile, capacity, &got);
        if (status == ROBO_FS_OK) {
            copied = got < capacity ? got : capacity;
            uint8_t* dest = reinterpret_cast<uint8_t*>(data);
            for (uint32_t index = 0; index < copied; ++index) {
                dest[index] = gFile[index];
            }
        }
    } else if (number == ROBO_CALL_DELETE) {
        status = RoboFsDeleteFile(ROBO_CALL_APP, name);
    } else {
        uint32_t size = capacity;
        if (size > ROBO_CALL_FILE_MAX) {
            size = ROBO_CALL_FILE_MAX;
        }
        if (size > 0 && !RoboCallUser(data, size)) {
            return ROBO_FS_DENIED;
        }
        const uint8_t* source = reinterpret_cast<const uint8_t*>(data);
        for (uint32_t index = 0; index < size; ++index) {
            gFile[index] = source[index];
        }
        if (number == ROBO_CALL_WRITE) {
            status = RoboFsWriteFile(ROBO_CALL_APP, name, gFile, size);
        } else {
            status = RoboFsCreateFile(ROBO_CALL_APP, name, gFile, size);
        }
        copied = size;
    }
    if (sizeOut != 0 && RoboCallUser(sizeOut, sizeof(uint32_t))) {
        *reinterpret_cast<uint32_t*>(sizeOut) = copied;
    }
    return status;
}

uint64_t RoboCallPresentRect(uint64_t requestAddress) {
    RoboFrameView view{};
    if (!gFocused || gDesktop == nullptr || gDesktop->gpu == nullptr || !RoboFrameAppView(&view) || !RoboCallUser(requestAddress, sizeof(RoboCallPresent))) {
        return 1u;
    }
    RoboCallPresent request = *reinterpret_cast<const RoboCallPresent*>(requestAddress);
    if (request.surfaceWidth <= 0 || request.surfaceHeight <= 0 || request.stride < 4) {
        return 1u;
    }
    int32_t x = request.x;
    int32_t y = request.y;
    int32_t width = request.width;
    int32_t height = request.height;
    if (x < 0) {
        width += x;
        x = 0;
    }
    if (y < 0) {
        height += y;
        y = 0;
    }
    if (width <= 0 || height <= 0 || x >= request.surfaceWidth || y >= request.surfaceHeight) {
        return 1u;
    }
    if (x + width > request.surfaceWidth) {
        width = request.surfaceWidth - x;
    }
    if (y + height > request.surfaceHeight) {
        height = request.surfaceHeight - y;
    }
    if (x >= view.width || y >= view.height) {
        return 1u;
    }
    if (x + width > view.width) {
        width = view.width - x;
    }
    if (y + height > view.height) {
        height = view.height - y;
    }
    if (width <= 0 || height <= 0) {
        return 1u;
    }
    uint64_t span = static_cast<uint64_t>(y) * static_cast<uint32_t>(request.stride) + static_cast<uint64_t>(x) * 4u;
    uint64_t bytes = static_cast<uint64_t>(height - 1) * static_cast<uint32_t>(request.stride) + static_cast<uint64_t>(width) * 4u;
    if (!RoboCallUser(request.pixels, span + bytes)) {
        return 1u;
    }
    const uint8_t* source = reinterpret_cast<const uint8_t*>(request.pixels + span);
    for (int32_t row = 0; row < height; ++row) {
        const uint8_t* from = source + static_cast<uint64_t>(row) * static_cast<uint32_t>(request.stride);
        uint32_t* dest = view.pixels + static_cast<int64_t>(view.originY + y + row) * view.stride + view.originX + x;
        for (int32_t column = 0; column < width; ++column) {
            dest[column] = RoboCallPack(from[0], from[1], from[2]);
            from += 4;
        }
    }
    RoboFrameShow(RoboThreadCurrent(), x, y, width, height);
    (void)source;
    return 0u;
}

}  // namespace

void RoboCallBind(RoboDesktop* desktop) {
    gDesktop = desktop;
}

void RoboCallFocus(uint32_t codeBytes, uint32_t heapBytes) {
    gFocused = 1u;
    gCodeBytes = (static_cast<uint64_t>(codeBytes) + ROBO_PROGRAM_PAGE - 1u) & ~(static_cast<uint64_t>(ROBO_PROGRAM_PAGE) - 1u);
    gHeapBytes = (static_cast<uint64_t>(heapBytes) + ROBO_PROGRAM_PAGE - 1u) & ~(static_cast<uint64_t>(ROBO_PROGRAM_PAGE) - 1u);
    if (gCodeBytes > static_cast<uint64_t>(ROBO_PROGRAM_CODE_PAGES) * ROBO_PROGRAM_PAGE) {
        gCodeBytes = static_cast<uint64_t>(ROBO_PROGRAM_CODE_PAGES) * ROBO_PROGRAM_PAGE;
    }
    if (gHeapBytes > static_cast<uint64_t>(ROBO_PROGRAM_HEAP_PAGES) * ROBO_PROGRAM_PAGE) {
        gHeapBytes = static_cast<uint64_t>(ROBO_PROGRAM_HEAP_PAGES) * ROBO_PROGRAM_PAGE;
    }
    gQueueHead = 0;
    gQueueTail = 0;
    RoboFrameOpen(RoboThreadCurrent(), 48, 250, 260, 150, 1u);
}

void RoboCallBlur(void) {
    gFocused = 0;
    gCodeBytes = 0;
    gHeapBytes = 0;
    gQueueHead = 0;
    gQueueTail = 0;
}

bool RoboCallOffer(const RoboInputEvent* event, int32_t x, int32_t y) {
    if (event == nullptr || gFocused == 0) {
        return false;
    }
    if (event->kind == ROBO_INPUT_KEY) {
        RoboCallInputEvent queued{};
        queued.kind = event->kind;
        queued.pressed = event->pressed;
        queued.code = event->code;
        RoboFrameView view{};
        queued.x = 0;
        queued.y = 0;
        if (RoboFrameAppView(&view)) {
            queued.x = x - view.screenX;
            queued.y = y - view.screenY;
        }
        RoboCallEnqueue(&queued);
        return true;
    }
    if (!RoboCallHit(x, y)) {
        return false;
    }
    RoboFrameView view{};
    RoboCallInputEvent queued{};
    queued.kind = event->kind;
    queued.pressed = event->pressed;
    queued.code = event->code;
    queued.x = 0;
    queued.y = 0;
    if (RoboFrameAppView(&view)) {
        queued.x = x - view.screenX;
        queued.y = y - view.screenY;
    }
    queued.dx = event->dx;
    queued.dy = event->dy;
    RoboCallEnqueue(&queued);
    return true;
}

extern "C" uint64_t RoboCallService(uint64_t number, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    if (number == ROBO_CALL_CLOCK) {
        return RoboTimerTicks();
    }
    if (number == ROBO_CALL_INPUT) {
        if (gQueueHead == gQueueTail || !RoboCallUser(arg0, sizeof(RoboCallInputEvent))) {
            return 0u;
        }
        *reinterpret_cast<RoboCallInputEvent*>(arg0) = gQueue[gQueueHead];
        gQueueHead = (gQueueHead + 1u) % ROBO_CALL_QUEUE;
        return 1u;
    }
    if (number == ROBO_CALL_PRESENT) {
        return RoboCallPresentRect(arg0);
    }
    if (number == ROBO_CALL_READ || number == ROBO_CALL_WRITE || number == ROBO_CALL_CREATE || number == ROBO_CALL_DELETE) {
        return static_cast<uint64_t>(RoboCallFile(number, arg0, arg1, arg2, arg3));
    }
    if (number == ROBO_CALL_NET) {
        if (arg1 == 0 || !RoboCallUser(arg0, arg1) || arg1 > 1514u) {
            return static_cast<uint64_t>(-1);
        }
        const int sent = arg2 == 0 ? RoboUdpSend(reinterpret_cast<const uint8_t*>(arg0), static_cast<uint32_t>(arg1))
                                   : RoboTcpSend(reinterpret_cast<const uint8_t*>(arg0), static_cast<uint32_t>(arg1));
        return static_cast<uint64_t>(sent);
    }
    if (number == ROBO_CALL_SOUND) {
        if (arg1 == 0 || (arg1 & 1u) != 0 || !RoboCallUser(arg0, arg1) || arg1 > 8192u) {
            return 0u;
        }
        return RoboSoundWrite(reinterpret_cast<const int16_t*>(arg0), static_cast<uint32_t>(arg1 / 2u)) ? 1u : 0u;
    }
    if (number == ROBO_CALL_POWER) {
        if (arg0 == 1u) {
            RoboPowerReboot();
        }
        RoboPowerShutdown();
        return 0u;
    }
    return 0u;
}
