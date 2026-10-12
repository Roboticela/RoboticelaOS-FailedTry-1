#pragma once

#include <stddef.h>

extern "C" {
void* malloc(size_t bytes);
void free(void* pointer);
void abort(void);
int abs(int value);
int atoi(const char* text);
}
