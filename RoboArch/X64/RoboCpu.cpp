#include "RoboCpu.hpp"

#include "RoboMemory.hpp"

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
    cr0 &= ~uint64_t{0xC};
    cr0 |= 0x2;
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= 0x600;
    __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
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
        "movq %rax, %rdi\n"
        "call RoboSyscallDispatch\n"
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
