#include <stddef.h>
#include <stdint.h>

extern "C" void* memcpy(void* dest, const void* src, size_t count) {
    uint8_t* out = static_cast<uint8_t*>(dest);
    const uint8_t* in = static_cast<const uint8_t*>(src);
    for (size_t index = 0; index < count; ++index) {
        out[index] = in[index];
    }
    return dest;
}

extern "C" void* memset(void* dest, int value, size_t count) {
    uint8_t* out = static_cast<uint8_t*>(dest);
    uint8_t byte = static_cast<uint8_t>(value);
    for (size_t index = 0; index < count; ++index) {
        out[index] = byte;
    }
    return dest;
}

extern "C" void* memmove(void* dest, const void* src, size_t count) {
    uint8_t* out = static_cast<uint8_t*>(dest);
    const uint8_t* in = static_cast<const uint8_t*>(src);
    if (out == in || count == 0) {
        return dest;
    }
    if (out < in) {
        for (size_t index = 0; index < count; ++index) {
            out[index] = in[index];
        }
        return dest;
    }
    for (size_t index = count; index > 0; --index) {
        out[index - 1] = in[index - 1];
    }
    return dest;
}
