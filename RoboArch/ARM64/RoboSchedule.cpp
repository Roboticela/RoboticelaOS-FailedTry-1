#include "RoboSchedule.hpp"

#include "RoboCpu.hpp"
#include "RoboMemory.hpp"
#include "RoboCall.hpp"
#include "RoboFrame.hpp"
#include "RoboProcess.hpp"
#include "RoboTimer.hpp"

extern "C" uint64_t gRoboKernelRsp;

namespace {

constexpr uint32_t ROBO_THREAD_LIMIT = 6u;
constexpr uint8_t ROBO_THREAD_READY = 1u;
constexpr uint8_t ROBO_THREAD_SLEEP = 2u;

struct RoboThread {
    uint64_t rsp;
    uint64_t cr3;
    uint64_t wakeTick;
    uint8_t state;
    uint8_t used;
};

RoboThread gThreads[ROBO_THREAD_LIMIT];
uint32_t gThreadCount = 0;
uint32_t gCurrent = 0;
volatile uint8_t gScheduleDue = 0;
volatile uint8_t gScheduleReady = 0;

void RoboWorkerMain(void) {
    for (;;) {
        RoboThreadSleep(RoboTimerTicks() + 5ull);
    }
}

void RoboEnter(uint64_t tables, uint64_t rip, uint64_t rsp) {
    gThreads[gCurrent].cr3 = tables;
    __asm__ volatile("msr ttbr0_el1, %0" : : "r"(tables) : "memory");
    __asm__ volatile("tlbi vmalle1; dsb ish; isb" ::: "memory");
    RoboArchEnterUser(rip, rsp);
}

void RoboUserMain(void) {
    RoboUserMap map{};
    if (RoboProcessLoad(&map)) {
        RoboEnter(map.tables, map.rip, map.rsp);
    }
    uint64_t rip = 0;
    uint64_t rsp = 0;
    const uint8_t code[] = {
        0x28, 0x00, 0x80, 0xD2,
        0x01, 0x00, 0x00, 0xD4,
        0xFE, 0xFF, 0xFF, 0x17,
    };
    uint64_t userTables = RoboMemoryCreateUserSpace(code, sizeof(code), &rip, &rsp);
    if (userTables == 0) {
        for (;;) {
            RoboThreadSleep(RoboTimerTicks() + 50ull);
        }
    }
    RoboEnter(userTables, rip, rsp);
}

void RoboThreadPrepare(void (*entry)(void), uint64_t tables) {
    if (gThreadCount >= ROBO_THREAD_LIMIT) {
        return;
    }
    uint64_t page = RoboPageAlloc();
    if (page == 0) {
        return;
    }
    uint64_t* frame = reinterpret_cast<uint64_t*>(page + 4096ull) - 12;
    frame[0] = 0;
    frame[1] = reinterpret_cast<uint64_t>(entry);
    for (int index = 2; index < 12; ++index) {
        frame[index] = 0;
    }
    RoboThread* thread = &gThreads[gThreadCount];
    thread->rsp = reinterpret_cast<uint64_t>(frame);
    thread->cr3 = tables;
    thread->wakeTick = 0;
    thread->state = ROBO_THREAD_READY;
    thread->used = 1;
    gThreadCount += 1;
    if (entry == RoboUserMain) {
        RoboKernelRspSet(page + 4096ull);
    }
}

int RoboScheduleNext(uint32_t from) {
    for (uint32_t step = 1; step <= gThreadCount; ++step) {
        uint32_t index = (from + step) % gThreadCount;
        if (gThreads[index].used && gThreads[index].state == ROBO_THREAD_READY && gThreads[index].rsp != 0) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

void RoboScheduleSwitch(uint32_t next) {
    uint32_t previous = gCurrent;
    gCurrent = next;
    RoboContextSwitch(&gThreads[previous].rsp, gThreads[next].rsp, gThreads[next].cr3);
}

}  // namespace

uint32_t RoboThreadCurrent(void) {
    return gCurrent;
}

uint32_t RoboThreadSpawn(void (*entry)(void)) {
    uint32_t before = gThreadCount;
    RoboThreadPrepare(entry, RoboMemoryKernelCr3());
    if (gThreadCount == before) {
        return 0;
    }
    return gThreadCount - 1u;
}

void RoboThreadStop(uint32_t index) {
    if (index == 0 || index >= gThreadCount || gThreads[index].used == 0) {
        return;
    }
    if (index == gCurrent) {
        RoboThreadEnd(2u);
        return;
    }
    gThreads[index].used = 0;
    gThreads[index].state = 0;
    gThreads[index].rsp = 0;
}

void RoboKernelRspSet(uint64_t stackTop) {
    gRoboKernelRsp = stackTop;
}

void RoboScheduleNotify(void) {
    if (!gScheduleReady) {
        return;
    }
    uint64_t ticks = RoboTimerTicks();
    for (uint32_t index = 0; index < gThreadCount; ++index) {
        if (gThreads[index].used && gThreads[index].state == ROBO_THREAD_SLEEP && ticks >= gThreads[index].wakeTick) {
            gThreads[index].state = ROBO_THREAD_READY;
        }
    }
    gScheduleDue = 1;
}

void RoboSchedule(void) {
    if (!gScheduleReady || gScheduleDue == 0) {
        return;
    }
    gScheduleDue = 0;
    int next = RoboScheduleNext(gCurrent);
    if (next < 0) {
        return;
    }
    RoboScheduleSwitch(static_cast<uint32_t>(next));
}

void RoboThreadSleep(uint64_t wakeTick) {
    gThreads[gCurrent].state = ROBO_THREAD_SLEEP;
    gThreads[gCurrent].wakeTick = wakeTick;
    gScheduleDue = 0;
    int next = RoboScheduleNext(gCurrent);
    if (next < 0) {
        gThreads[gCurrent].state = ROBO_THREAD_READY;
        return;
    }
    RoboScheduleSwitch(static_cast<uint32_t>(next));
}

void RoboThreadEnd(uint32_t reason) {
    uint32_t thread = gCurrent;
    RoboCallBlur();
    RoboFrameRelease(thread);
    RoboProcessNote(reason);
    gThreads[gCurrent].used = 0;
    gThreads[gCurrent].state = 0;
    gThreads[gCurrent].rsp = 0;
    int next = RoboScheduleNext(gCurrent);
    if (next < 0) {
        return;
    }
    RoboScheduleSwitch(static_cast<uint32_t>(next));
}

extern "C" void RoboArmLowerFrame(uint64_t* frame) {
    uint64_t syndrome = 0;
    __asm__ volatile("mrs %0, esr_el1" : "=r"(syndrome));
    if (frame == nullptr || ((syndrome >> 26) & 0x3Fu) != 0x15u) {
        RoboThreadEnd(3u);
        return;
    }
    uint64_t result = RoboSyscallDispatch(frame[10], frame[18], frame[19], frame[16], frame[17]);
    frame[18] = result;
}

extern "C" uint64_t RoboSyscallDispatch(uint64_t number, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    if (number == ROBO_CALL_EXIT) {
        RoboThreadEnd(2u);
        return 0u;
    }
    if (number == ROBO_CALL_YIELD) {
        int next = RoboScheduleNext(gCurrent);
        gScheduleDue = 0;
        if (next >= 0) {
            RoboScheduleSwitch(static_cast<uint32_t>(next));
        }
        return 0u;
    }
    return RoboCallService(number, arg0, arg1, arg2, arg3);
}

void RoboScheduleStart(void) {
    gThreads[0].rsp = 0;
    gThreads[0].cr3 = RoboMemoryKernelCr3();
    gThreads[0].wakeTick = 0;
    gThreads[0].state = ROBO_THREAD_READY;
    gThreads[0].used = 1;
    gThreadCount = 1;
    gCurrent = 0;
    uint64_t kernelTables = RoboMemoryKernelCr3();
    RoboThreadPrepare(RoboWorkerMain, kernelTables);
    RoboThreadPrepare(RoboUserMain, kernelTables);
    gScheduleReady = 1;
}
