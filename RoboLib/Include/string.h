#pragma once

#include <stddef.h>

extern "C" {
void* memcpy(void* dest, const void* src, size_t count);
void* memset(void* dest, int value, size_t count);
void* memmove(void* dest, const void* src, size_t count);
size_t strlen(const char* text);
int strcmp(const char* left, const char* right);
int memcmp(const void* left, const void* right, size_t count);
}
