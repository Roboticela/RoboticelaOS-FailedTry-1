#include "RoboArch.hpp"
#include "RoboPS2Keyboard.hpp"
#include "RoboPS2Mouse.hpp"
#include "RoboSchedule.hpp"
#include "RoboTimer.hpp"
#include "RoboUsb.hpp"

namespace {

constexpr uint16_t ROBO_PIC_MASTER_COMMAND = 0x20;
constexpr uint16_t ROBO_PIC_MASTER_DATA = 0x21;
constexpr uint16_t ROBO_PIC_SLAVE_COMMAND = 0xA0;
constexpr uint16_t ROBO_PIC_SLAVE_DATA = 0xA1;
constexpr uint8_t ROBO_PIC_EOI = 0x20;
constexpr uint8_t ROBO_PIC_ICW1 = 0x11;
constexpr uint8_t ROBO_PIC_ICW4 = 0x01;
constexpr uint8_t ROBO_PIC_MASTER_VECTOR = 0x20;
constexpr uint8_t ROBO_PIC_SLAVE_VECTOR = 0x28;
constexpr uint8_t ROBO_IRQ_TIMER = 0x20;
constexpr uint8_t ROBO_IRQ_KEYBOARD = 0x21;
constexpr uint8_t ROBO_IRQ_MOUSE = 0x2C;
constexpr uint8_t ROBO_PIC_MASTER_MASK = 0xF8;
constexpr uint32_t ROBO_APIC_BASE_MSR = 0x1B;
constexpr uint16_t ROBO_IMCR_INDEX = 0x22;
constexpr uint16_t ROBO_IMCR_DATA = 0x23;
constexpr uint16_t ROBO_IDT_GATE = 0x8E;

struct RoboInterruptFrame {
    uint64_t instructionPointer;
    uint64_t codeSegment;
    uint64_t flags;
    uint64_t stackPointer;
    uint64_t stackSegment;
};

struct RoboIdtEntry {
    uint16_t offsetLow;
    uint16_t selector;
    uint8_t ist;
    uint8_t typeAttr;
    uint16_t offsetMid;
    uint32_t offsetHigh;
    uint32_t reserved;
};

struct RoboIdtPointer {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

alignas(16) RoboIdtEntry gIdt[256];
uint16_t gCodeSelector = 0;
uint8_t gPicMasterMask = ROBO_PIC_MASTER_MASK;
uint8_t gPicSlaveMask = 0xEF;
uint8_t gClaimedIrq = 0;

void RoboPicDelay(void) {
    RoboPortOut8(0x80, 0);
}

void RoboPicEndMaster(void) {
    RoboPortOut8(ROBO_PIC_MASTER_COMMAND, ROBO_PIC_EOI);
}

void RoboPicEndSlave(void) {
    RoboPortOut8(ROBO_PIC_SLAVE_COMMAND, ROBO_PIC_EOI);
    RoboPortOut8(ROBO_PIC_MASTER_COMMAND, ROBO_PIC_EOI);
}

void RoboApicDisable(void) {
    uint32_t low = 0;
    uint32_t high = 0;
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(ROBO_APIC_BASE_MSR));
    low &= ~(1u << 11);
    __asm__ volatile("wrmsr" : : "a"(low), "d"(high), "c"(ROBO_APIC_BASE_MSR));
    RoboPortOut8(ROBO_IMCR_INDEX, 0x70);
    RoboPortOut8(ROBO_IMCR_DATA, 0x00);
}

void RoboPicStart(void) {
    RoboPortOut8(ROBO_PIC_MASTER_COMMAND, ROBO_PIC_ICW1);
    RoboPicDelay();
    RoboPortOut8(ROBO_PIC_SLAVE_COMMAND, ROBO_PIC_ICW1);
    RoboPicDelay();
    RoboPortOut8(ROBO_PIC_MASTER_DATA, ROBO_PIC_MASTER_VECTOR);
    RoboPicDelay();
    RoboPortOut8(ROBO_PIC_SLAVE_DATA, ROBO_PIC_SLAVE_VECTOR);
    RoboPicDelay();
    RoboPortOut8(ROBO_PIC_MASTER_DATA, 0x04);
    RoboPicDelay();
    RoboPortOut8(ROBO_PIC_SLAVE_DATA, 0x02);
    RoboPicDelay();
    RoboPortOut8(ROBO_PIC_MASTER_DATA, ROBO_PIC_ICW4);
    RoboPicDelay();
    RoboPortOut8(ROBO_PIC_SLAVE_DATA, ROBO_PIC_ICW4);
    RoboPicDelay();
    RoboPortOut8(ROBO_PIC_MASTER_DATA, gPicMasterMask);
    RoboPortOut8(ROBO_PIC_SLAVE_DATA, gPicSlaveMask);
}

__attribute__((interrupt)) void RoboInterruptException(RoboInterruptFrame* frame) {
    if (frame != nullptr && frame->codeSegment == 0x23u) {
        RoboThreadEnd(3u);
        return;
    }
    RoboHalt();
}

__attribute__((interrupt)) void RoboInterruptExceptionError(RoboInterruptFrame* frame, uint64_t error) {
    (void)error;
    if (frame != nullptr && frame->codeSegment == 0x23u) {
        RoboThreadEnd(3u);
        return;
    }
    RoboHalt();
}

__attribute__((interrupt)) void RoboInterruptTimer(RoboInterruptFrame* frame) {
    (void)frame;
    RoboTimerAdvance();
    RoboScheduleNotify();
    RoboPicEndMaster();
}

__attribute__((interrupt)) void RoboInterruptKeyboard(RoboInterruptFrame* frame) {
    (void)frame;
    RoboPS2KeyboardInterrupt();
    RoboPicEndMaster();
}

__attribute__((interrupt)) void RoboInterruptMouse(RoboInterruptFrame* frame) {
    (void)frame;
    RoboPS2MouseInterrupt();
    RoboPicEndSlave();
}

__attribute__((interrupt)) void RoboInterruptUsb(RoboInterruptFrame* frame) {
    (void)frame;
    RoboUsbCollect();
    if (gClaimedIrq >= 8u) {
        RoboPicEndSlave();
    } else {
        RoboPicEndMaster();
    }
}

__attribute__((interrupt)) void RoboInterruptMaster(RoboInterruptFrame* frame) {
    (void)frame;
    RoboPicEndMaster();
}

__attribute__((interrupt)) void RoboInterruptSlave(RoboInterruptFrame* frame) {
    (void)frame;
    RoboPicEndSlave();
}

void RoboIdtSet(uint8_t vector, void (*handler)(RoboInterruptFrame*)) {
    uint64_t address = reinterpret_cast<uint64_t>(handler);
    RoboIdtEntry entry{};
    entry.offsetLow = static_cast<uint16_t>(address & 0xFFFFu);
    entry.selector = gCodeSelector;
    entry.ist = 0;
    entry.typeAttr = ROBO_IDT_GATE;
    entry.offsetMid = static_cast<uint16_t>((address >> 16) & 0xFFFFu);
    entry.offsetHigh = static_cast<uint32_t>(address >> 32);
    entry.reserved = 0;
    gIdt[vector] = entry;
}

void RoboIdtSetError(uint8_t vector, void (*handler)(RoboInterruptFrame*, uint64_t)) {
    RoboIdtSet(vector, reinterpret_cast<void (*)(RoboInterruptFrame*)>(handler));
}

}  // namespace

void RoboArchInterruptsStart(void) {
    uint16_t selector = 0;
    __asm__ volatile("mov %%cs, %0" : "=r"(selector));
    gCodeSelector = selector;

    for (uint32_t vector = 0; vector < 256; ++vector) {
        gIdt[vector] = {};
    }
    for (uint8_t vector = 0; vector < 32; ++vector) {
        RoboIdtSet(vector, RoboInterruptException);
    }
    const uint8_t errorVectors[] = {8, 10, 11, 12, 13, 14, 17, 21, 29, 30};
    for (uint8_t errorVector : errorVectors) {
        RoboIdtSetError(errorVector, RoboInterruptExceptionError);
    }
    RoboIdtSet(ROBO_IRQ_TIMER, RoboInterruptTimer);
    RoboIdtSet(ROBO_IRQ_KEYBOARD, RoboInterruptKeyboard);
    RoboIdtSet(ROBO_IRQ_MOUSE, RoboInterruptMouse);
    for (uint8_t vector = 0x21; vector < 0x28; ++vector) {
        if (vector != ROBO_IRQ_KEYBOARD) {
            RoboIdtSet(vector, RoboInterruptMaster);
        }
    }
    for (uint8_t vector = 0x28; vector < 0x30; ++vector) {
        if (vector != ROBO_IRQ_MOUSE) {
            RoboIdtSet(vector, RoboInterruptSlave);
        }
    }

    RoboIdtPointer pointer{};
    pointer.limit = static_cast<uint16_t>(sizeof(gIdt) - 1);
    pointer.base = reinterpret_cast<uint64_t>(&gIdt);
    __asm__ volatile("lidt %0" : : "m"(pointer));

    RoboApicDisable();
    RoboPicStart();
    RoboTimerStart();
}

void RoboArchIrqClaim(uint8_t irq) {
    if (irq < 1u || irq > 15u) {
        return;
    }
    gClaimedIrq = irq;
    RoboIdtSet(static_cast<uint8_t>(0x20u + irq), RoboInterruptUsb);
    if (irq < 8u) {
        gPicMasterMask = static_cast<uint8_t>(gPicMasterMask & ~(1u << irq));
        RoboPortOut8(ROBO_PIC_MASTER_DATA, gPicMasterMask);
    } else {
        gPicSlaveMask = static_cast<uint8_t>(gPicSlaveMask & ~(1u << (irq - 8u)));
        RoboPortOut8(ROBO_PIC_SLAVE_DATA, gPicSlaveMask);
    }
}
