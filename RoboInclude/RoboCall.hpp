#pragma once

#include <stdint.h>

// Rings cross here. Generated C++ keeps calling the platform; the platform uses these calls.
constexpr uint64_t ROBO_CALL_YIELD = 1u;
constexpr uint64_t ROBO_CALL_EXIT = 2u;
constexpr uint64_t ROBO_CALL_PRESENT = 3u;
constexpr uint64_t ROBO_CALL_INPUT = 4u;
constexpr uint64_t ROBO_CALL_CLOCK = 5u;
constexpr uint64_t ROBO_CALL_READ = 6u;
constexpr uint64_t ROBO_CALL_WRITE = 7u;
constexpr uint64_t ROBO_CALL_CREATE = 8u;
constexpr uint64_t ROBO_CALL_DELETE = 9u;

constexpr int32_t ROBO_CALL_WINDOW_X = 64;
constexpr int32_t ROBO_CALL_WINDOW_Y = 48;
constexpr int32_t ROBO_CALL_WINDOW_W = 280;
constexpr int32_t ROBO_CALL_WINDOW_H = 160;
constexpr uint32_t ROBO_CALL_FILE_MAX = 4096u;

// RGBA bytes. x, y, width, and height are the dirty rectangle in the app surface.
struct RoboCallPresent {
    uint64_t pixels;
    int32_t stride;
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    int32_t surfaceWidth;
    int32_t surfaceHeight;
    uint32_t pixelFormat;
};

struct RoboCallInputEvent {
    uint32_t kind;
    uint32_t pressed;
    uint32_t code;
    int32_t x;
    int32_t y;
    int32_t dx;
    int32_t dy;
};

struct RoboDesktop;
struct RoboInputEvent;

void RoboCallBind(RoboDesktop* desktop);
void RoboCallFocus(uint32_t codeBytes, uint32_t heapBytes);
void RoboCallBlur(void);
bool RoboCallOffer(const RoboInputEvent* event, int32_t x, int32_t y);
extern "C" uint64_t RoboCallService(uint64_t number, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3);

inline uint64_t RoboUserCall(uint64_t number, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
#if defined(__aarch64__)
    register uint64_t result asm("x0") = arg0;
    register uint64_t second asm("x1") = arg1;
    register uint64_t third asm("x2") = arg2;
    register uint64_t fourth asm("x3") = arg3;
    register uint64_t call asm("x8") = number;
    __asm__ volatile("svc #0" : "+r"(result) : "r"(second), "r"(third), "r"(fourth), "r"(call) : "memory");
    return result;
#elif defined(__x86_64__)
    uint64_t result = 0;
    register uint64_t fourth asm("r10") = arg3;
    __asm__ volatile("syscall"
                     : "=a"(result)
                     : "a"(number), "D"(arg0), "S"(arg1), "d"(arg2), "r"(fourth)
                     : "rcx", "r11", "memory");
    return result;
#else
    (void)number;
    (void)arg0;
    (void)arg1;
    (void)arg2;
    (void)arg3;
    return 0;
#endif
}
