#include "RoboMemory.hpp"

#include "RoboBootInfo.hpp"

namespace {

constexpr uint64_t ROBO_PAGE_PRESENT = 1ull;
constexpr uint64_t ROBO_PAGE_WRITE = 2ull;
constexpr uint64_t ROBO_PAGE_USER = 4ull;
constexpr uint64_t ROBO_PAGE_PWT = 1ull << 3;
constexpr uint64_t ROBO_PAGE_PCD = 1ull << 4;
constexpr uint64_t ROBO_PAGE_SIZE = 1ull << 7;
constexpr uint64_t ROBO_PAGE_BYTES = 4096ull;
constexpr uint64_t ROBO_TWO_MB = 2ull * 1024ull * 1024ull;
constexpr uint64_t ROBO_ONE_GB = 1024ull * 1024ull * 1024ull;
constexpr uint32_t ROBO_PD_MAX = static_cast<uint32_t>(ROBO_RAM_MAP_MAX_BYTES / ROBO_ONE_GB);
constexpr uint32_t ROBO_FREE_LIMIT = 8192u;
constexpr uint64_t ROBO_HEAP_MIN_BYTES = 48ull * 1024ull * 1024ull;
constexpr uint64_t ROBO_HEAP_TARGET_BYTES = 96ull * 1024ull * 1024ull;
constexpr uint64_t ROBO_HEAP_MAX_BYTES = 256ull * 1024ull * 1024ull;

alignas(4096) uint8_t gTablePages[(2u + ROBO_PD_MAX) * 4096u];
uint64_t gFreePages[ROBO_FREE_LIMIT];
uint32_t gFreeCount = 0;
uint64_t gKernelCr3 = 0;
uint64_t gMapBytes = ROBO_RAM_MIN_BYTES;
uint32_t gMapGigabytes = 1u;
bool gPatReady = false;
RoboArena gFrameArena{};
RoboArena gLongArena{};
uint8_t* gHeapCarve = nullptr;
uint64_t gHeapCarveBytes = 0;
uint64_t gHeapCarveEnd = 0;

uint64_t* RoboTable(uint32_t index) {
    return reinterpret_cast<uint64_t*>(gTablePages + index * ROBO_PAGE_BYTES);
}

uint64_t RoboRoundUpGigabyte(uint64_t bytes) {
    if (bytes < ROBO_RAM_MIN_BYTES) {
        return ROBO_RAM_MIN_BYTES;
    }
    const uint64_t rounded = (bytes + ROBO_ONE_GB - 1ull) & ~(ROBO_ONE_GB - 1ull);
    if (rounded > ROBO_RAM_MAP_MAX_BYTES || rounded < bytes) {
        return ROBO_RAM_MAP_MAX_BYTES;
    }
    return rounded;
}

uint64_t RoboComputeMapBytes(const RoboBootInfo* info) {
    uint64_t end = ROBO_RAM_MIN_BYTES;
    auto raise = [&](uint64_t value) {
        if (value > end) {
            end = value;
        }
    };
    raise(ROBO_KERNEL_LOAD_ADDRESS + ROBO_KERNEL_BYTES);
    raise(ROBO_DEVKIT_LOAD_ADDRESS + ROBO_DEVKIT_BYTES);
    if (info != nullptr) {
        if (info->frameBufferBase != 0 && info->height != 0 && info->stride != 0) {
            raise(info->frameBufferBase + static_cast<uint64_t>(info->height) * info->stride);
        }
        for (uint32_t index = 0; index < info->memoryRangeCount && index < ROBO_MEMORY_RANGE_MAX; ++index) {
            const RoboMemoryRange* range = &info->memoryRanges[index];
            raise(range->base + range->pageCount * ROBO_PAGE_BYTES);
        }
        for (uint32_t file = 0; file < info->fileCount && file < ROBO_BOOT_FILE_MAX; ++file) {
            const RoboBootFile* held = &info->files[file];
            if (held->size != 0 && held->address != 0) {
                raise(held->address + held->size);
            }
        }
    }
    return RoboRoundUpGigabyte(end);
}

struct RoboSpan {
    uint64_t base;
    uint64_t end;
};

constexpr uint32_t ROBO_FORBID_MAX = 32u;

void RoboAddForbidden(RoboSpan* spans, uint32_t* count, uint64_t base, uint64_t end) {
    if (base >= end || *count >= ROBO_FORBID_MAX) {
        return;
    }
    if (end > gMapBytes) {
        end = gMapBytes;
    }
    if (base >= end) {
        return;
    }
    spans[*count].base = base;
    spans[*count].end = end;
    *count += 1u;
}

uint32_t RoboBuildForbidden(const RoboBootInfo* info, uint64_t stack, RoboSpan* spans) {
    uint32_t count = 0;
    RoboAddForbidden(spans, &count, ROBO_KERNEL_LOAD_ADDRESS, ROBO_KERNEL_LOAD_ADDRESS + ROBO_KERNEL_BYTES);
    RoboAddForbidden(spans, &count, ROBO_DEVKIT_LOAD_ADDRESS, ROBO_DEVKIT_LOAD_ADDRESS + ROBO_DEVKIT_BYTES);
    if (gHeapCarve != nullptr) {
        RoboAddForbidden(spans, &count, reinterpret_cast<uint64_t>(gHeapCarve), gHeapCarveEnd);
    }
    if (info != nullptr) {
        for (uint32_t file = 0; file < info->fileCount && file < ROBO_BOOT_FILE_MAX; ++file) {
            const RoboBootFile* held = &info->files[file];
            if (held->size == 0 || held->address == 0) {
                continue;
            }
            RoboAddForbidden(spans, &count, held->address, held->address + held->size);
        }
        if (info->frameBufferBase != 0 && info->height != 0 && info->stride != 0) {
            uint64_t frameBase = info->frameBufferBase & ~(ROBO_PAGE_BYTES - 1ull);
            uint64_t frameEnd = info->frameBufferBase + static_cast<uint64_t>(info->height) * info->stride;
            RoboAddForbidden(spans, &count, frameBase, frameEnd);
        }
    }
    uint64_t stackBase = stack & ~(ROBO_TWO_MB - 1ull);
    RoboAddForbidden(spans, &count, stackBase, stackBase + ROBO_TWO_MB);
    return count;
}

uint64_t RoboNextForbidden(const RoboSpan* spans, uint32_t count, uint64_t cursor, uint64_t limit, uint64_t* forbidEnd) {
    uint64_t next = limit;
    *forbidEnd = limit;
    for (uint32_t index = 0; index < count; ++index) {
        if (spans[index].end <= cursor || spans[index].base >= limit) {
            continue;
        }
        if (spans[index].base <= cursor) {
            if (spans[index].end < *forbidEnd) {
                *forbidEnd = spans[index].end;
            }
            next = cursor;
        } else if (spans[index].base < next) {
            next = spans[index].base;
            *forbidEnd = spans[index].end;
        }
    }
    return next;
}

template <typename RoboVisit>
void RoboVisitFreeSpans(const RoboBootInfo* info, uint64_t stack, RoboVisit visit) {
    RoboSpan forbidden[ROBO_FORBID_MAX];
    const uint32_t forbidCount = RoboBuildForbidden(info, stack, forbidden);
    for (uint32_t rangeIndex = 0; rangeIndex < info->memoryRangeCount; ++rangeIndex) {
        const RoboMemoryRange* range = &info->memoryRanges[rangeIndex];
        if ((range->type != 3u && range->type != 4u && range->type != 7u) || range->base >= gMapBytes) {
            continue;
        }
        uint64_t cursor = range->base;
        if (cursor < 0x100000ull) {
            cursor = 0x100000ull;
        }
        uint64_t rangeEnd = range->base + range->pageCount * ROBO_PAGE_BYTES;
        if (rangeEnd > gMapBytes) {
            rangeEnd = gMapBytes;
        }
        while (cursor < rangeEnd) {
            uint64_t forbidEnd = rangeEnd;
            const uint64_t next = RoboNextForbidden(forbidden, forbidCount, cursor, rangeEnd, &forbidEnd);
            if (next > cursor) {
                visit(cursor, next);
            }
            if (next >= rangeEnd) {
                break;
            }
            uint64_t advance = forbidEnd;
            if (advance <= cursor) {
                advance = cursor + ROBO_PAGE_BYTES;
            }
            cursor = advance;
        }
    }
}

void RoboMapPhysical(const RoboBootInfo* info) {
    gMapBytes = RoboComputeMapBytes(info);
    gMapGigabytes = static_cast<uint32_t>(gMapBytes / ROBO_ONE_GB);
    if (gMapGigabytes == 0u) {
        gMapGigabytes = 1u;
        gMapBytes = ROBO_ONE_GB;
    }
    if (gMapGigabytes > ROBO_PD_MAX) {
        gMapGigabytes = ROBO_PD_MAX;
        gMapBytes = static_cast<uint64_t>(gMapGigabytes) * ROBO_ONE_GB;
    }

    uint64_t* pml4 = RoboTable(0);
    uint64_t* pdpt = RoboTable(1);
    for (uint32_t index = 0; index < 512u; ++index) {
        pml4[index] = 0;
        pdpt[index] = 0;
    }
    for (uint32_t directory = 0; directory < gMapGigabytes; ++directory) {
        uint64_t* pd = RoboTable(2u + directory);
        for (uint32_t entry = 0; entry < 512u; ++entry) {
            uint64_t address = (static_cast<uint64_t>(directory) * 512ull + entry) * ROBO_TWO_MB;
            uint64_t flags = ROBO_PAGE_PRESENT | ROBO_PAGE_WRITE | ROBO_PAGE_SIZE;
            pd[entry] = address | flags;
        }
        pdpt[directory] = reinterpret_cast<uint64_t>(pd) | ROBO_PAGE_PRESENT | ROBO_PAGE_WRITE;
    }
    pml4[0] = reinterpret_cast<uint64_t>(pdpt) | ROBO_PAGE_PRESENT | ROBO_PAGE_WRITE;
    gKernelCr3 = reinterpret_cast<uint64_t>(pml4);
    __asm__ volatile("mov %0, %%cr3" : : "r"(gKernelCr3) : "memory");
}

uint64_t RoboPickHeapBytes(uint64_t usableBytes) {
    if (usableBytes < ROBO_HEAP_MIN_BYTES) {
        return 0;
    }
    uint64_t want = ROBO_HEAP_TARGET_BYTES;
    if (usableBytes >= ROBO_HEAP_MAX_BYTES * 4ull) {
        want = ROBO_HEAP_MAX_BYTES;
    } else if (usableBytes < ROBO_HEAP_TARGET_BYTES) {
        want = usableBytes & ~(ROBO_PAGE_BYTES - 1ull);
        if (want < ROBO_HEAP_MIN_BYTES) {
            return 0;
        }
    }
    if (want > usableBytes) {
        want = usableBytes & ~(ROBO_PAGE_BYTES - 1ull);
    }
    if (want > ROBO_HEAP_MAX_BYTES) {
        want = ROBO_HEAP_MAX_BYTES;
    }
    return want;
}

void RoboCarveHeap(const RoboBootInfo* info, uint64_t stack) {
    gHeapCarve = nullptr;
    gHeapCarveBytes = 0;
    gHeapCarveEnd = 0;
    uint64_t bestBase = 0;
    uint64_t bestBytes = 0;
    RoboVisitFreeSpans(info, stack, [&](uint64_t base, uint64_t end) {
        if (end <= base) {
            return;
        }
        const uint64_t bytes = end - base;
        if (bytes > bestBytes) {
            bestBase = base;
            bestBytes = bytes;
        }
    });
    const uint64_t heapBytes = RoboPickHeapBytes(bestBytes);
    if (heapBytes == 0) {
        return;
    }
    gHeapCarve = reinterpret_cast<uint8_t*>(bestBase);
    gHeapCarveBytes = heapBytes;
    gHeapCarveEnd = bestBase + heapBytes;
    RoboHeapInstall(gHeapCarve, gHeapCarveBytes);
}

void RoboCollectPages(const RoboBootInfo* info, uint64_t stack) {
    RoboVisitFreeSpans(info, stack, [&](uint64_t base, uint64_t end) {
        uint64_t address = (base + ROBO_PAGE_BYTES - 1ull) & ~(ROBO_PAGE_BYTES - 1ull);
        while (address + ROBO_PAGE_BYTES <= end && gFreeCount < ROBO_FREE_LIMIT) {
            gFreePages[gFreeCount] = address;
            gFreeCount += 1;
            address += ROBO_PAGE_BYTES;
        }
    });
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
    RoboMapPhysical(info);
    uint64_t stack = 0;
    __asm__ volatile("mov %%rsp, %0" : "=r"(stack));
    RoboCarveHeap(info, stack);
    RoboCollectPages(info, stack);
    gFrameArena = RoboMakeArena();
    gLongArena = RoboMakeArena();
}

void* RoboHeapBase(void) {
    return gHeapCarve;
}

uint64_t RoboHeapBytes(void) {
    return gHeapCarveBytes;
}

uint64_t RoboMemoryKernelCr3(void) {
    return gKernelCr3;
}

uint64_t RoboMemoryMapBytes(void) {
    return gMapBytes;
}

void RoboPatWriteCombine(void) {
    if (gPatReady) {
        return;
    }
    uint32_t low = 0;
    uint32_t high = 0;
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(0x277));
    high = (high & 0xFF00FFFFu) | (0x01u << 16);
    __asm__ volatile("wrmsr" : : "a"(low), "d"(high), "c"(0x277) : "memory");
    __asm__ volatile("wbinvd" ::: "memory");
    gPatReady = true;
}

void RoboMemoryMarkWriteCombine(uint64_t address, uint64_t bytes) {
    if (bytes == 0 || gKernelCr3 == 0) {
        return;
    }
    RoboPatWriteCombine();
    uint64_t end = address + bytes;
    if (end < address) {
        end = gMapBytes;
    }
    uint64_t page = address & ~(ROBO_TWO_MB - 1ull);
    while (page < end && page < gMapBytes) {
        uint32_t directory = static_cast<uint32_t>(page / ROBO_ONE_GB);
        uint32_t entry = static_cast<uint32_t>((page / ROBO_TWO_MB) & 511ull);
        if (directory < gMapGigabytes) {
            uint64_t* directoryEntries = RoboTable(2u + directory);
            uint64_t flags = directoryEntries[entry];
            flags &= ~(ROBO_PAGE_PWT);
            flags |= ROBO_PAGE_PCD | (1ull << 12);
            directoryEntries[entry] = flags;
        }
        page += ROBO_TWO_MB;
    }
    __asm__ volatile("mov %0, %%cr3" : : "r"(gKernelCr3) : "memory");
}

uint64_t RoboMemoryVideoBuffer(void) {
    return 0;
}

void RoboMemoryMarkUncached(uint64_t address, uint64_t bytes) {
    if (bytes == 0 || gKernelCr3 == 0) {
        return;
    }
    uint64_t end = address + bytes;
    if (end < address) {
        return;
    }
    uint64_t* pdpt = RoboTable(1u);
    uint64_t page = address & ~(ROBO_TWO_MB - 1ull);
    // Map MMIO (APIC, GPU BARs) even when above the RAM identity window.
    while (page < end && page < ROBO_RAM_MAP_MAX_BYTES) {
        uint32_t directory = static_cast<uint32_t>(page / ROBO_ONE_GB);
        uint32_t entry = static_cast<uint32_t>((page / ROBO_TWO_MB) & 511ull);
        if (directory >= ROBO_PD_MAX) {
            break;
        }
        if ((pdpt[directory] & ROBO_PAGE_PRESENT) == 0) {
            uint64_t* pd = RoboTable(2u + directory);
            for (uint32_t index = 0; index < 512u; ++index) {
                pd[index] = 0;
            }
            pdpt[directory] = reinterpret_cast<uint64_t>(pd) | ROBO_PAGE_PRESENT | ROBO_PAGE_WRITE;
            if (directory >= gMapGigabytes) {
                gMapGigabytes = directory + 1u;
            }
        }
        uint64_t* directoryEntries = RoboTable(2u + directory);
        uint64_t flags = ROBO_PAGE_PRESENT | ROBO_PAGE_WRITE | ROBO_PAGE_SIZE | ROBO_PAGE_PCD | ROBO_PAGE_PWT;
        if ((directoryEntries[entry] & ROBO_PAGE_PRESENT) == 0) {
            directoryEntries[entry] = page | flags;
        } else {
            directoryEntries[entry] |= ROBO_PAGE_PCD | ROBO_PAGE_PWT;
        }
        page += ROBO_TWO_MB;
    }
    __asm__ volatile("mov %0, %%cr3" : : "r"(gKernelCr3) : "memory");
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

bool RoboMemoryMapUser(const void* code, uint64_t codeSize, uint64_t heapBytes, RoboUserMap* map) {
    if (code == nullptr || map == nullptr || codeSize == 0 || heapBytes == 0) {
        return false;
    }
    if (codeSize > static_cast<uint64_t>(ROBO_PROGRAM_CODE_PAGES) * ROBO_PAGE_BYTES) {
        return false;
    }
    if (heapBytes > static_cast<uint64_t>(ROBO_PROGRAM_HEAP_PAGES) * ROBO_PAGE_BYTES) {
        return false;
    }
    uint32_t codePages = static_cast<uint32_t>((codeSize + ROBO_PAGE_BYTES - 1ull) / ROBO_PAGE_BYTES);
    uint32_t heapPages = static_cast<uint32_t>((heapBytes + ROBO_PAGE_BYTES - 1ull) / ROBO_PAGE_BYTES);
    uint64_t pml4 = RoboPageAlloc();
    uint64_t pdpt = RoboPageAlloc();
    uint64_t directory = RoboPageAlloc();
    uint64_t table = RoboPageAlloc();
    uint64_t stackPage = RoboPageAlloc();
    if (pml4 == 0 || pdpt == 0 || directory == 0 || table == 0 || stackPage == 0) {
        return false;
    }
    uint64_t codePhysical[ROBO_PROGRAM_CODE_PAGES];
    uint64_t heapPhysical[ROBO_PROGRAM_HEAP_PAGES];
    for (uint32_t index = 0; index < codePages; ++index) {
        codePhysical[index] = RoboPageAlloc();
        if (codePhysical[index] == 0) {
            return false;
        }
    }
    for (uint32_t index = 0; index < heapPages; ++index) {
        heapPhysical[index] = RoboPageAlloc();
        if (heapPhysical[index] == 0) {
            return false;
        }
    }
    uint64_t flags = ROBO_PAGE_PRESENT | ROBO_PAGE_WRITE | ROBO_PAGE_USER;
    uint64_t* pml4Entries = reinterpret_cast<uint64_t*>(pml4);
    uint64_t* userPdpt = reinterpret_cast<uint64_t*>(pdpt);
    uint64_t* userDirectory = reinterpret_cast<uint64_t*>(directory);
    uint64_t* userTable = reinterpret_cast<uint64_t*>(table);
    pml4Entries[0] = reinterpret_cast<uint64_t>(RoboTable(1)) | ROBO_PAGE_PRESENT | ROBO_PAGE_WRITE;
    userPdpt[(ROBO_USER_CODE >> 30) & 0x1FFull] = directory | flags;
    userDirectory[(ROBO_USER_CODE >> 21) & 0x1FFull] = table | flags;
    pml4Entries[(ROBO_USER_CODE >> 39) & 0x1FFull] = pdpt | flags;
    const uint8_t* bytes = static_cast<const uint8_t*>(code);
    uint64_t copied = 0;
    for (uint32_t page = 0; page < codePages; ++page) {
        userTable[page] = codePhysical[page] | flags;
        uint8_t* dest = reinterpret_cast<uint8_t*>(codePhysical[page]);
        for (uint32_t offset = 0; offset < ROBO_PAGE_BYTES && copied < codeSize; ++offset) {
            dest[offset] = bytes[copied];
            copied += 1ull;
        }
    }
    uint32_t stackIndex = static_cast<uint32_t>((ROBO_USER_STACK >> 12) & 0x1FFull);
    uint32_t heapIndex = static_cast<uint32_t>((ROBO_USER_HEAP >> 12) & 0x1FFull);
    userTable[stackIndex] = stackPage | flags;
    for (uint32_t page = 0; page < heapPages; ++page) {
        userTable[heapIndex + page] = heapPhysical[page] | flags;
    }
    map->tables = pml4;
    map->rip = ROBO_USER_CODE;
    map->rsp = ROBO_USER_STACK + ROBO_PAGE_BYTES - 16ull;
    map->heap = ROBO_USER_HEAP;
    map->heapPhysical = heapPhysical[0];
    return true;
}
