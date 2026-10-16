#include <stddef.h>
#include <stdint.h>

#include "RoboArch.hpp"

#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)
#include "RoboMemory.hpp"
#endif

namespace {

struct RoboBlock {
    size_t size;
    size_t free;
    RoboBlock* next;
    size_t pad;
};

#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)
uint8_t* gHeap = nullptr;
size_t gHeapBytes = 0;
#else
constexpr size_t ROBO_HEAP_BYTES = 48u * 1024u * 1024u;
alignas(16) uint8_t gHeapStorage[ROBO_HEAP_BYTES];
uint8_t* gHeap = gHeapStorage;
size_t gHeapBytes = ROBO_HEAP_BYTES;
#endif
RoboBlock* gFree = nullptr;
bool gHeapReady = false;

}  // namespace

extern "C" void RoboHeapInstall(void* base, uint64_t bytes) {
    if (base == nullptr || bytes < sizeof(RoboBlock) + 64u) {
        return;
    }
    gHeap = static_cast<uint8_t*>(base);
    gHeapBytes = static_cast<size_t>(bytes);
    gFree = nullptr;
    gHeapReady = false;
}

void RoboHeapStart(void) {
    if (gHeapReady) {
        return;
    }
#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)
    if (gHeap == nullptr) {
        gHeap = static_cast<uint8_t*>(RoboHeapBase());
        gHeapBytes = static_cast<size_t>(RoboHeapBytes());
    }
#endif
    if (gHeap == nullptr || gHeapBytes < sizeof(RoboBlock) + 64u) {
        RoboHalt();
    }
    gFree = reinterpret_cast<RoboBlock*>(gHeap);
    gFree->size = gHeapBytes - sizeof(RoboBlock);
    gFree->free = 1;
    gFree->next = nullptr;
    gFree->pad = 0;
    gHeapReady = true;
}

extern "C" void* malloc(size_t bytes) {
    RoboHeapStart();
    if (bytes == 0) {
        bytes = 1;
    }
    bytes = (bytes + 15u) & ~size_t{15};
    RoboBlock* previous = nullptr;
    RoboBlock* block = gFree;
    while (block != nullptr) {
        if (block->free != 0 && block->size >= bytes) {
            size_t extra = block->size - bytes;
            if (extra > sizeof(RoboBlock) + 32u) {
                uint8_t* splitAt = reinterpret_cast<uint8_t*>(block + 1) + bytes;
                RoboBlock* split = reinterpret_cast<RoboBlock*>(splitAt);
                split->size = extra - sizeof(RoboBlock);
                split->free = 1;
                split->next = block->next;
                split->pad = 0;
                block->size = bytes;
                block->next = split;
            }
            block->free = 0;
            return block + 1;
        }
        previous = block;
        block = block->next;
        (void)previous;
    }
    RoboHalt();
    return nullptr;
}

extern "C" int RoboHeapHas(size_t bytes) {
    RoboHeapStart();
    if (bytes == 0) {
        bytes = 1;
    }
    bytes = (bytes + 15u) & ~size_t{15};
    RoboBlock* block = gFree;
    while (block != nullptr) {
        if (block->free != 0 && block->size >= bytes) {
            return 1;
        }
        block = block->next;
    }
    return 0;
}

extern "C" int __cxa_atexit(void (*)(void*), void*, void*) {
    return 0;
}

extern "C" void free(void* pointer) {
    if (pointer == nullptr) {
        return;
    }
    RoboBlock* block = reinterpret_cast<RoboBlock*>(pointer) - 1;
    block->free = 1;
    RoboBlock* current = gFree;
    while (current != nullptr) {
        uint8_t* end = reinterpret_cast<uint8_t*>(current + 1) + current->size;
        if (current->free != 0 && end == reinterpret_cast<uint8_t*>(current->next) && current->next->free != 0) {
            current->size += sizeof(RoboBlock) + current->next->size;
            current->next = current->next->next;
            continue;
        }
        current = current->next;
    }
}

void* operator new(size_t bytes) {
    return malloc(bytes);
}

void* operator new[](size_t bytes) {
    return malloc(bytes);
}

void operator delete(void* pointer) noexcept {
    free(pointer);
}

void operator delete[](void* pointer) noexcept {
    free(pointer);
}

void operator delete(void* pointer, size_t) noexcept {
    free(pointer);
}

void operator delete[](void* pointer, size_t) noexcept {
    free(pointer);
}
