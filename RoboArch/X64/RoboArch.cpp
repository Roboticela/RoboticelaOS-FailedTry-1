#include "RoboArch.hpp"

uint8_t RoboPortIn8(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void RoboPortOut8(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

uint32_t RoboPortIn32(uint16_t port) {
    uint32_t value;
    __asm__ volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void RoboPortOut32(uint16_t port, uint32_t value) {
    __asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

void RoboArchCli(void) {
    __asm__ volatile("cli");
}

void RoboArchSti(void) {
    __asm__ volatile("sti");
}

void RoboArchIdle(void) {
    __asm__ volatile("sti; hlt");
}

void RoboHalt(void) {
    for (;;) {
        __asm__ volatile("cli");
        __asm__ volatile("hlt");
    }
}
