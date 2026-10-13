#include "RoboCpu.hpp"

extern "C" uint64_t gRoboKernelRsp = 0;
extern "C" uint64_t gRoboUserRsp = 0;

void RoboArchCpuStart(void) {
}

void RoboCpuJoin(void) {
}

__attribute__((naked)) void RoboArchEnterUser(uint64_t, uint64_t) {
    __asm__ volatile(
        "msr elr_el1, x0\n"
        "msr sp_el0, x1\n"
        "msr spsr_el1, xzr\n"
        "isb\n"
        "eret\n");
}

extern "C" __attribute__((naked)) void RoboContextSwitch(uint64_t*, uint64_t, uint64_t) {
    __asm__ volatile(
        "stp x19, x20, [sp, #-16]!\n"
        "stp x21, x22, [sp, #-16]!\n"
        "stp x23, x24, [sp, #-16]!\n"
        "stp x25, x26, [sp, #-16]!\n"
        "stp x27, x28, [sp, #-16]!\n"
        "stp x29, x30, [sp, #-16]!\n"
        "mov x9, sp\n"
        "str x9, [x0]\n"
        "msr ttbr0_el1, x2\n"
        "tlbi vmalle1\n"
        "dsb ish\n"
        "isb\n"
        "mov sp, x1\n"
        "ldp x29, x30, [sp], #16\n"
        "ldp x27, x28, [sp], #16\n"
        "ldp x25, x26, [sp], #16\n"
        "ldp x23, x24, [sp], #16\n"
        "ldp x21, x22, [sp], #16\n"
        "ldp x19, x20, [sp], #16\n"
        "ret\n");
}

extern "C" void RoboSyscallEntry(void) {
}
