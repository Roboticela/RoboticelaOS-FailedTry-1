#include "RoboMemory.hpp"

#include "RoboBootInfo.hpp"

namespace {

constexpr uint64_t ROBO_PAGE_PRESENT = 1ull;
constexpr uint64_t ROBO_PAGE_WRITE = 2ull;
constexpr uint64_t ROBO_PAGE_USER = 4ull;
constexpr uint64_t ROBO_PAGE_PCD = 1ull << 4;
constexpr uint64_t ROBO_PAGE_SIZE = 1ull << 7;
constexpr uint64_t ROBO_PAGE_BYTES = 4096ull;
constexpr uint64_t ROBO_TWO_MB = 2ull * 1024ull * 1024ull;
constexpr uint64_t ROBO_MAP_BYTES = 4ull * 1024ull * 1024ull * 1024ull;
constexpr uint32_t ROBO_FREE_LIMIT = 4096u;
constexpr uint64_t ROBO_USER_CODE = 0x00007FFF00000000ull;
constexpr uint64_t ROBO_USER_STACK = 0x00007FFF00001000ull;

alignas(4096) uint8_t gTablePages[7 * 4096];
uint64_t gFreePages[ROBO_FREE_LIMIT];
uint32_t gFreeCount = 0;
uint64_t gKernelCr3 = 0;
RoboArena gFrameArena{};
RoboArena gLongArena{};

uint64_t* RoboTable(uint32_t index) {
    return reinterpret_cast<uint64_t*>(gTablePages + index * ROBO_PAGE_BYTES);
}

bool RoboExcluded(uint64_t address, uint64_t stack, const RoboBootInfo* info) {
    if (address < 0x100000ull) {
        return true;
    }
    if (address >= ROBO_KERNEL_LOAD_ADDRESS && address < ROBO_KERNEL_LOAD_ADDRESS + ROBO_TWO_MB) {
        return true;
    }
    uint64_t stackBase = stack & ~(ROBO_TWO_MB - 1ull);
    if (address >= stackBase && address < stackBase + ROBO_TWO_MB) {
        return true;
    }
    uint64_t frameEnd = info->frameBufferBase + static_cast<uint64_t>(info->height) * info->stride;
    uint64_t frameBase = info->frameBufferBase & ~(ROBO_PAGE_BYTES - 1ull);
    return address >= frameBase && address < frameEnd;
}

void RoboMapFourGigabytes(const RoboBootInfo* info) {
    uint64_t* pml4 = RoboTable(0);
    uint64_t* pdpt = RoboTable(1);
    for (uint32_t index = 0; index < 512; ++index) {
        pml4[index] = 0;
        pdpt[index] = 0;
    }
    for (uint32_t directory = 0; directory < 4; ++directory) {
        uint64_t* pd = RoboTable(2 + directory);
        for (uint32_t entry = 0; entry < 512; ++entry) {
            uint64_t address = (static_cast<uint64_t>(directory) * 512ull + entry) * ROBO_TWO_MB;
            uint64_t flags = ROBO_PAGE_PRESENT | ROBO_PAGE_WRITE | ROBO_PAGE_SIZE;
            uint64_t frame = info->frameBufferBase & ~(ROBO_TWO_MB - 1ull);
            if (address == frame) {
                flags |= ROBO_PAGE_PCD;
            }
            pd[entry] = address | flags;
        }
        pdpt[directory] = reinterpret_cast<uint64_t>(pd) | ROBO_PAGE_PRESENT | ROBO_PAGE_WRITE;
    }
    pml4[0] = reinterpret_cast<uint64_t>(pdpt) | ROBO_PAGE_PRESENT | ROBO_PAGE_WRITE;
    gKernelCr3 = reinterpret_cast<uint64_t>(pml4);
    __asm__ volatile("mov %0, %%cr3" : : "r"(gKernelCr3) : "memory");
}

void RoboCollectPages(const RoboBootInfo* info, uint64_t stack) {
    for (uint32_t rangeIndex = 0; rangeIndex < info->memoryRangeCount && gFreeCount < ROBO_FREE_LIMIT; ++rangeIndex) {
        const RoboMemoryRange* range = &info->memoryRanges[rangeIndex];
        if (range->base >= ROBO_MAP_BYTES) {
            continue;
        }
        uint64_t pages = range->pageCount;
        if (range->base + pages * ROBO_PAGE_BYTES > ROBO_MAP_BYTES) {
            pages = (ROBO_MAP_BYTES - range->base) / ROBO_PAGE_BYTES;
        }
        for (uint64_t page = 0; page < pages && gFreeCount < ROBO_FREE_LIMIT; ++page) {
            uint64_t address = range->base + page * ROBO_PAGE_BYTES;
            if (RoboExcluded(address, stack, info)) {
                continue;
            }
            gFreePages[gFreeCount] = address;
            gFreeCount += 1;
        }
    }
}

RoboArena RoboMakeArena(void) {
    RoboArena arena{};
    uint64_t page = RoboPageAlloc();
    if (page == 0) {
        return arena;
    }
    arena.base = reinterpret_cast<uint8_t*>(page);
    arena.used = 0;
    arena.capacity = ROBO_PAGE_BYTES;
    return arena;
}

}  // namespace

uint64_t RoboPageAlloc(void) {
    if (gFreeCount == 0) {
        return 0;
    }
    gFreeCount -= 1;
    uint64_t page = gFreePages[gFreeCount];
    uint8_t* bytes = reinterpret_cast<uint8_t*>(page);
    for (uint32_t index = 0; index < ROBO_PAGE_BYTES; ++index) {
        bytes[index] = 0;
    }
    return page;
}

void RoboMemoryStart(const RoboBootInfo* info) {
    RoboMapFourGigabytes(info);
    uint64_t stack = 0;
    __asm__ volatile("mov %%rsp, %0" : "=r"(stack));
    RoboCollectPages(info, stack);
    gFrameArena = RoboMakeArena();
    gLongArena = RoboMakeArena();
}

uint64_t RoboMemoryKernelCr3(void) {
    return gKernelCr3;
}

void RoboArenaReset(RoboArena* arena) {
    if (arena != nullptr) {
        arena->used = 0;
    }
}

void* RoboArenaAlloc(RoboArena* arena, uint64_t size, uint64_t align) {
    if (arena == nullptr || arena->base == nullptr || align == 0 || (align & (align - 1ull)) != 0) {
        return nullptr;
    }
    uint64_t start = (arena->used + align - 1ull) & ~(align - 1ull);
    if (start + size > arena->capacity) {
        return nullptr;
    }
    arena->used = start + size;
    return arena->base + start;
}

RoboArena* RoboArenaFrame(void) {
    return &gFrameArena;
}

RoboArena* RoboArenaLong(void) {
    return &gLongArena;
}

uint64_t RoboMemoryCreateUserSpace(const void* code, uint64_t codeSize, uint64_t* userRip, uint64_t* userRsp) {
    uint64_t pml4 = RoboPageAlloc();
    uint64_t pdpt = RoboPageAlloc();
    uint64_t directory = RoboPageAlloc();
    uint64_t table = RoboPageAlloc();
    uint64_t codePage = RoboPageAlloc();
    uint64_t stackPage = RoboPageAlloc();
    if (pml4 == 0 || pdpt == 0 || directory == 0 || table == 0 || codePage == 0 || stackPage == 0) {
        return 0;
    }
    uint64_t* pml4Entries = reinterpret_cast<uint64_t*>(pml4);
    pml4Entries[0] = reinterpret_cast<uint64_t>(RoboTable(1)) | ROBO_PAGE_PRESENT | ROBO_PAGE_WRITE;
    uint64_t* userPdpt = reinterpret_cast<uint64_t*>(pdpt);
    uint64_t* userDirectory = reinterpret_cast<uint64_t*>(directory);
    uint64_t* userTable = reinterpret_cast<uint64_t*>(table);
    uint64_t directoryIndex = (ROBO_USER_CODE >> 21) & 0x1FFull;
    uint64_t tableIndex = (ROBO_USER_CODE >> 12) & 0x1FFull;
    userPdpt[(ROBO_USER_CODE >> 30) & 0x1FFull] = directory | ROBO_PAGE_PRESENT | ROBO_PAGE_WRITE | ROBO_PAGE_USER;
    userDirectory[directoryIndex] = table | ROBO_PAGE_PRESENT | ROBO_PAGE_WRITE | ROBO_PAGE_USER;
    userTable[tableIndex] = codePage | ROBO_PAGE_PRESENT | ROBO_PAGE_WRITE | ROBO_PAGE_USER;
    userTable[(ROBO_USER_STACK >> 12) & 0x1FFull] = stackPage | ROBO_PAGE_PRESENT | ROBO_PAGE_WRITE | ROBO_PAGE_USER;
    pml4Entries[(ROBO_USER_CODE >> 39) & 0x1FFull] = pdpt | ROBO_PAGE_PRESENT | ROBO_PAGE_WRITE | ROBO_PAGE_USER;
    const uint8_t* bytes = static_cast<const uint8_t*>(code);
    uint8_t* dest = reinterpret_cast<uint8_t*>(codePage);
    for (uint64_t index = 0; index < codeSize && index < ROBO_PAGE_BYTES; ++index) {
        dest[index] = bytes[index];
    }
    *userRip = ROBO_USER_CODE;
    *userRsp = ROBO_USER_STACK + ROBO_PAGE_BYTES - 16ull;
    return pml4;
}
