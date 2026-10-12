#include "RoboArch.hpp"

uint8_t RoboPortIn8(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

void RoboPortOut8(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

void RoboArchCli(void) {
    __asm__ volatile("cli");
}

void RoboHalt(void) {
    for (;;) {
        __asm__ volatile("cli");
        __asm__ volatile("hlt");
    }
}
