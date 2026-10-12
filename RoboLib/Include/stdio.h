#pragma once

#include <stdarg.h>
#include <stddef.h>

extern "C" int snprintf(char* buffer, size_t capacity, const char* format, ...);
