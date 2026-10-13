#include "RoboCall.hpp"

namespace {

constexpr uint64_t ROBO_APP_HEAP = 0x00007FFF00030000ull;

struct RoboAppBuffers {
    RoboCallPresent present;
    RoboCallInputEvent input;
    uint32_t pixel;
    uint32_t size;
    uint8_t rgba[4];
};

}  // namespace

extern "C" __attribute__((section(".text.startup"))) void RoboAppMain(void) {
    RoboAppBuffers* buffers = reinterpret_cast<RoboAppBuffers*>(ROBO_APP_HEAP);
    const char* path = "Documents/Notes";
    buffers->rgba[0] = 0x58;
    buffers->rgba[1] = 0x58;
    buffers->rgba[2] = 0x58;
    buffers->rgba[3] = 0x58;
    (void)RoboUserCall(ROBO_CALL_CLOCK, 0, 0, 0, 0);
    (void)RoboUserCall(ROBO_CALL_INPUT, reinterpret_cast<uint64_t>(&buffers->input), 0, 0, 0);
    (void)RoboUserCall(ROBO_CALL_READ, reinterpret_cast<uint64_t>(path), reinterpret_cast<uint64_t>(buffers->rgba), 4u, reinterpret_cast<uint64_t>(&buffers->size));
    buffers->present.pixels = reinterpret_cast<uint64_t>(buffers->rgba);
    buffers->present.stride = 4;
    buffers->present.x = 0;
    buffers->present.y = 0;
    buffers->present.width = 1;
    buffers->present.height = 1;
    buffers->present.surfaceWidth = 1;
    buffers->present.surfaceHeight = 1;
    buffers->present.pixelFormat = 0;
    (void)RoboUserCall(ROBO_CALL_PRESENT, reinterpret_cast<uint64_t>(&buffers->present), 0, 0, 0);
    (void)RoboUserCall(ROBO_CALL_EXIT, 0, 0, 0, 0);
    for (;;) {
    }
}
