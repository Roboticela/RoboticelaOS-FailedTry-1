#include "RoboCpu.hpp"

#include "RoboArch.hpp"
#include "RoboMemory.hpp"

namespace {

constexpr uint64_t ROBO_APIC_BASE = 0xFEE00000ull;
constexpr uint32_t ROBO_APIC_BASE_MSR = 0x1B;
constexpr uint16_t ROBO_IMCR_INDEX = 0x22;
constexpr uint16_t ROBO_IMCR_DATA = 0x23;
constexpr uint32_t ROBO_APIC_ID = 0x20;
constexpr uint32_t ROBO_APIC_ICR_LOW = 0x300;
constexpr uint32_t ROBO_APIC_ICR_HIGH = 0x310;
constexpr uint64_t ROBO_AP_TRAMPOLINE = 0x8000ull;
constexpr uint32_t ROBO_CPU_LIMIT = 4u;

volatile uint32_t gRoboCpuReady = 1;
alignas(16) uint8_t gVideoStack[3][64u * 1024u];

const uint8_t gRoboApTrampoline[] = {
    0xFA, 0xFC, 0x31, 0xC0, 0x8E, 0xD8, 0x0F, 0x01, 0x16, 0xB8, 0x80, 0x66, 0x0F, 0x20, 0xC0, 0x66, 0x83, 0xC8, 0x01, 0x66, 0x0F, 0x22, 0xC0, 0x66, 0xEA, 0x1F, 0x80, 0x00, 0x00, 0x08, 0x00, 0xB8, 0x10, 0x00, 0x00, 0x00, 0x8E, 0xD8, 0x8E, 0xC0, 0x8E, 0xD0, 0x0F, 0x20, 0xE0, 0x83, 0xC8, 0x20, 0x0F, 0x22, 0xE0, 0xA1, 0x78, 0x80, 0x00, 0x00, 0x0F, 0x22, 0xD8, 0xB9, 0x80, 0x00, 0x00, 0xC0, 0x0F, 0x32, 0x0D, 0x00, 0x01, 0x00, 0x00, 0x0F, 0x30, 0x0F, 0x20, 0xC0, 0x0D, 0x00, 0x00, 0x00, 0x80, 0x0F, 0x22, 0xC0, 0xEA, 0x5B, 0x80, 0x00, 0x00, 0x18, 0x00, 0x66, 0xB8, 0x20, 0x00, 0x8E, 0xD8, 0x8E, 0xC0, 0x8E, 0xD0, 0x48, 0x8B, 0x24, 0x25, 0x80, 0x80, 0x00, 0x00, 0x48, 0x8B, 0x04, 0x25, 0x88, 0x80, 0x00, 0x00, 0xFF, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x9A, 0xCF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x92, 0xCF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x9A, 0xAF, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x92, 0xAF, 0x00, 0x27, 0x00, 0x90, 0x80, 0x00, 0x00};

void RoboApicWrite(uint32_t offset, uint32_t value) {
    *reinterpret_cast<volatile uint32_t*>(ROBO_APIC_BASE + offset) = value;
}

uint32_t RoboApicRead(uint32_t offset) {
    return *reinterpret_cast<volatile uint32_t*>(ROBO_APIC_BASE + offset);
}

void RoboCpuPause(void) {
    // Short INIT/SIPI gap; multi-million spins here stuck the painter after first Present.
    for (uint32_t count = 0; count < 2000u; ++count) {
        __asm__ volatile("pause");
    }
}

void RoboApicEnable(void) {
    uint32_t low = 0;
    uint32_t high = 0;
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(ROBO_APIC_BASE_MSR));
    // Bit 11 = APIC global enable. Clear x2APIC (bit 10) so MMIO ICR works.
    low |= 1u << 11;
    low &= ~(1u << 10);
    __asm__ volatile("wrmsr" : : "a"(low), "d"(high), "c"(ROBO_APIC_BASE_MSR));
}

void RoboApicIpi(uint32_t apicId, uint32_t command) {
    RoboApicWrite(ROBO_APIC_ICR_HIGH, apicId << 24);
    RoboApicWrite(ROBO_APIC_ICR_LOW, command);
    // Bound tightly: a disabled or missing APIC must not stall the BSP after Present.
    for (uint32_t count = 0; count < 5000u; ++count) {
        if ((RoboApicRead(ROBO_APIC_ICR_LOW) & (1u << 12)) == 0) {
            return;
        }
        __asm__ volatile("pause");
    }
}

void RoboApStore(uint32_t offset, uint64_t value) {
    uint8_t* slot = reinterpret_cast<uint8_t*>(ROBO_AP_TRAMPOLINE + offset);
    for (uint32_t index = 0; index < 8u; ++index) {
        slot[index] = static_cast<uint8_t>(value >> (index * 8u));
    }
}

}  // namespace

extern "C" void RoboVideoPoll(uint32_t worker);
void RoboArchFpuStart(void);

extern "C" void RoboApMain(void) {
    RoboArchFpuStart();
    uint32_t slot = gRoboCpuReady;
    gRoboCpuReady += 1;
    if (slot >= 1u && slot <= 3u) {
        for (;;) {
            RoboVideoPoll(slot - 1u);
            __asm__ volatile("pause");
        }
    }
    for (;;) {
        __asm__ volatile("cli");
        __asm__ volatile("hlt");
    }
}

namespace {

struct RoboTss {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t ioMapBase;
} __attribute__((packed));

alignas(16) uint64_t gGdt[7];
RoboTss gTss{};

void RoboWriteMsr(uint32_t msr, uint64_t value) {
    uint32_t low = static_cast<uint32_t>(value);
    uint32_t high = static_cast<uint32_t>(value >> 32);
    __asm__ volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

uint64_t RoboReadMsr(uint32_t msr) {
    uint32_t low = 0;
    uint32_t high = 0;
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return (static_cast<uint64_t>(high) << 32) | low;
}

uint64_t RoboTssDescriptorLow(uint64_t base, uint64_t limit) {
    return (limit & 0xFFFFull)
        | ((base & 0xFFFFFFull) << 16)
        | (0x89ull << 40)
        | (((limit >> 16) & 0xFull) << 48)
        | (((base >> 24) & 0xFFull) << 56);
}

}  // namespace

extern "C" uint64_t gRoboUserRsp = 0;
extern "C" uint64_t gRoboKernelRsp = 0;

void RoboArchFpuStart(void) {
    uint64_t cr0 = 0;
    uint64_t cr4 = 0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~uint64_t{0xC};  // clear EM|TS
    cr0 |= 0x22;            // MP|NE
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0) : "memory");
    __asm__ volatile("fninit");
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= 0x600;  // OSFXSR|OSXMMEXCPT for SSE2 shell (-msse2)
    // Leave firmware OSXSAVE alone; do not xsetbv (that #GP under qemu64/WHPX).
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4) : "memory");
}

void RoboArchCpuStart(void) {
    gGdt[0] = 0;
    gGdt[1] = 0x00AF9A000000FFFFull;
    gGdt[2] = 0x00CF92000000FFFFull;
    gGdt[3] = 0x00CFF2000000FFFFull;
    gGdt[4] = 0x00AFFA000000FFFFull;
    gTss.ioMapBase = sizeof(gTss);
    uint64_t interruptStack = RoboPageAlloc();
    if (interruptStack != 0) {
        gTss.rsp0 = interruptStack + 4096ull;
    }
    uint64_t tssBase = reinterpret_cast<uint64_t>(&gTss);
    gGdt[5] = RoboTssDescriptorLow(tssBase, sizeof(gTss) - 1ull);
    gGdt[6] = tssBase >> 32;

    struct RoboGdtPointer {
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed)) pointer;
    pointer.limit = sizeof(gGdt) - 1u;
    pointer.base = reinterpret_cast<uint64_t>(&gGdt);
    __asm__ volatile(
        "lgdt %0\n"
        "pushq $0x08\n"
        "lea 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%ss\n"
        "ltr %1\n"
        :
        : "m"(pointer), "d"(static_cast<uint16_t>(0x28))
        : "rax", "memory");

    uint64_t efer = RoboReadMsr(0xC0000080u);
    RoboWriteMsr(0xC0000080u, efer | 1ull);
    RoboWriteMsr(0xC0000081u, (0x10ull << 48) | (0x08ull << 32));
    RoboWriteMsr(0xC0000082u, reinterpret_cast<uint64_t>(&RoboSyscallEntry));
    RoboWriteMsr(0xC0000084u, 0x200ull);
}

extern "C" __attribute__((naked)) void RoboSyscallEntry(void) {
    __asm__ volatile(
        "movq %rsp, gRoboUserRsp(%rip)\n"
        "movq gRoboKernelRsp(%rip), %rsp\n"
        "pushq %rcx\n"
        "pushq %r11\n"
        "pushq %rbx\n"
        "pushq %rbp\n"
        "pushq %r12\n"
        "pushq %r13\n"
        "pushq %r14\n"
        "pushq %r15\n"
        "pushq %r9\n"
        "pushq %r8\n"
        "pushq %r10\n"
        "movq (%rsp), %r8\n"
        "movq %rdx, %rcx\n"
        "movq %rsi, %rdx\n"
        "movq %rdi, %rsi\n"
        "movq %rax, %rdi\n"
        "call RoboSyscallDispatch\n"
        "popq %r10\n"
        "popq %r8\n"
        "popq %r9\n"
        "popq %r15\n"
        "popq %r14\n"
        "popq %r13\n"
        "popq %r12\n"
        "popq %rbp\n"
        "popq %rbx\n"
        "popq %r11\n"
        "popq %rcx\n"
        "movq gRoboUserRsp(%rip), %rsp\n"
        "sysretq\n");
}

__attribute__((naked)) void RoboArchEnterUser(uint64_t, uint64_t) {
    __asm__ volatile(
        "pushq $0x1b\n"
        "pushq %rsi\n"
        "pushq $0x202\n"
        "pushq $0x23\n"
        "pushq %rdi\n"
        "iretq\n");
}

extern "C" __attribute__((naked)) void RoboContextSwitch(uint64_t*, uint64_t, uint64_t) {
    __asm__ volatile(
        "pushq %rbx\n"
        "pushq %rbp\n"
        "pushq %r12\n"
        "pushq %r13\n"
        "pushq %r14\n"
        "pushq %r15\n"
        "movq %rsp, (%rdi)\n"
        "movq %rdx, %cr3\n"
        "movq %rsi, %rsp\n"
        "popq %r15\n"
        "popq %r14\n"
        "popq %r13\n"
        "popq %r12\n"
        "popq %rbp\n"
        "popq %rbx\n"
        "ret\n");
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

void RoboCpuJoin(void) {
    // InterruptsStart disables the local APIC for PIC routing; re-enable only for INIT/SIPI.
    RoboApicEnable();
    RoboMemoryMarkUncached(ROBO_APIC_BASE, 4096);
    uint8_t* dest = reinterpret_cast<uint8_t*>(ROBO_AP_TRAMPOLINE);
    for (uint32_t index = 0; index < sizeof(gRoboApTrampoline); ++index) {
        dest[index] = gRoboApTrampoline[index];
    }
    uint32_t bsp = RoboApicRead(ROBO_APIC_ID) >> 24;
    uint32_t misses = 0;
    for (uint32_t apicId = 0; apicId < ROBO_CPU_LIMIT; ++apicId) {
        if (apicId == bsp) {
            continue;
        }
        // Stop once enough APs joined; do not burn the BSP on empty APIC IDs.
        if (gRoboCpuReady >= ROBO_CPU_LIMIT) {
            break;
        }
        uint64_t stackTop = 0;
        if (gRoboCpuReady >= 1u && gRoboCpuReady <= 3u) {
            stackTop = reinterpret_cast<uint64_t>(&gVideoStack[gRoboCpuReady - 1u]) + sizeof(gVideoStack[0]) - 8ull;
        } else {
            uint64_t stack = RoboPageAlloc();
            if (stack == 0) {
                break;
            }
            stackTop = stack + 4096ull - 8ull;
        }
        uint32_t before = gRoboCpuReady;
        RoboApStore(0x78, RoboMemoryKernelCr3());
        RoboApStore(0x80, stackTop);
        RoboApStore(0x88, reinterpret_cast<uint64_t>(&RoboApMain));
        RoboApicIpi(apicId, 0x00004500u);
        RoboCpuPause();
        RoboApicIpi(apicId, 0x00004608u);
        RoboCpuPause();
        RoboApicIpi(apicId, 0x00004608u);
        for (uint32_t spin = 0; spin < 8000u; ++spin) {
            if (gRoboCpuReady != before) {
                break;
            }
            __asm__ volatile("pause");
        }
        // Missing APIC id: try the next one so sparse AP lists still join workers.
        if (gRoboCpuReady == before) {
            misses += 1;
            // No AP answered two probes: stay single-core rather than stall the shell.
            if (misses >= 2u) {
                break;
            }
            continue;
        }
        misses = 0;
    }
    // Restore PIC ownership so timer/keyboard/mouse IRQs wake the idle loop.
    RoboApicDisable();
}
