#include "RoboMemory.hpp"

#include "RoboBootInfo.hpp"
#include "RoboProgram.hpp"

namespace {

constexpr uint64_t ROBO_PAGE_BYTES = 4096ull;
constexpr uint64_t ROBO_TWO_MB = 2ull * 1024ull * 1024ull;
constexpr uint64_t ROBO_MAP_BYTES = 4ull * 1024ull * 1024ull * 1024ull;
constexpr uint64_t ROBO_RAM_BASE = 0x40000000ull;
constexpr uint64_t ROBO_ARM_KERNEL = 0x42000000ull;
constexpr uint64_t ROBO_ARM_KERNEL_BYTES = 128ull * 1024ull * 1024ull;
constexpr uint32_t ROBO_FREE_LIMIT = 4096u;
constexpr uint64_t ROBO_DESC_TABLE = 3ull;
constexpr uint64_t ROBO_DESC_PAGE = 3ull;
constexpr uint64_t ROBO_DESC_BLOCK = 1ull;
constexpr uint64_t ROBO_DESC_AF = 1ull << 10;
constexpr uint64_t ROBO_DESC_SH = 3ull << 8;
constexpr uint64_t ROBO_DESC_AP_USER = 1ull << 6;
constexpr uint64_t ROBO_DESC_UXN = 1ull << 54;
constexpr uint64_t ROBO_DESC_ATTR_DEVICE = 1ull << 2;
constexpr uint64_t ROBO_DESC_ATTR_NC = 2ull << 2;
constexpr uint64_t ROBO_MAIR = 0x00000000004400FFull;
constexpr uint64_t ROBO_TCR = 16ull | (1ull << 8) | (1ull << 10) | (3ull << 12) | (1ull << 23) | (2ull << 32);
constexpr uint64_t ROBO_SCTLR = 0x30501805ull;

alignas(4096) uint8_t gTables[6 * 4096];
uint64_t gFreePages[ROBO_FREE_LIMIT];
uint32_t gFreeCount = 0;
uint64_t gKernelTtbr = 0;
RoboArena gFrameArena{};
RoboArena gLongArena{};
constexpr uint64_t ROBO_HEAP_CARVE_BYTES = 96ull * 1024ull * 1024ull;
uint8_t* gHeapCarve = nullptr;
uint64_t gHeapCarveBytes = 0;
uint64_t gHeapCarveEnd = 0;

extern "C" char RoboVectorTable[];
extern "C" void RoboMemoryEntered(const RoboBootInfo* info);

uint64_t* RoboTable(uint32_t index) {
    return reinterpret_cast<uint64_t*>(gTables + index * ROBO_PAGE_BYTES);
}

void RoboClean(const void* address, uint64_t bytes) {
    uint64_t line = reinterpret_cast<uint64_t>(address) & ~63ull;
    uint64_t end = reinterpret_cast<uint64_t>(address) + bytes;
    while (line < end) {
        __asm__ volatile("dc civac, %0" : : "r"(line) : "memory");
        line += 64ull;
    }
    __asm__ volatile("dsb ish" ::: "memory");
}

void RoboBuildTables(void) {
    for (uint32_t index = 0; index < 6u * 512u; ++index) {
        reinterpret_cast<uint64_t*>(gTables)[index] = 0;
    }
    uint64_t* level0 = RoboTable(0);
    uint64_t* level1 = RoboTable(1);
    level0[0] = reinterpret_cast<uint64_t>(level1) | ROBO_DESC_TABLE;
    for (uint32_t gigabyte = 0; gigabyte < 4u; ++gigabyte) {
        uint64_t* level2 = RoboTable(2u + gigabyte);
        level1[gigabyte] = reinterpret_cast<uint64_t>(level2) | ROBO_DESC_TABLE;
        for (uint32_t entry = 0; entry < 512u; ++entry) {
            uint64_t address = (static_cast<uint64_t>(gigabyte) * 512ull + entry) * ROBO_TWO_MB;
            uint64_t flags = ROBO_DESC_BLOCK | ROBO_DESC_AF | ROBO_DESC_SH | ROBO_DESC_UXN;
            if (address < ROBO_RAM_BASE) {
                flags |= ROBO_DESC_ATTR_DEVICE;
            }
            level2[entry] = address | flags;
        }
    }
    RoboClean(gTables, sizeof(gTables));
    gKernelTtbr = reinterpret_cast<uint64_t>(level0);
}

void RoboProgramEl2(void) {
    uint64_t hypervisor = 1ull << 31;
    __asm__ volatile("msr hcr_el2, %0" : : "r"(hypervisor));
    __asm__ volatile("msr cnthctl_el2, %0" : : "r"(3ull));
}

void RoboProgramEl1(void) {
    uint64_t vector = reinterpret_cast<uint64_t>(RoboVectorTable);
    __asm__ volatile("msr vbar_el1, %0" : : "r"(vector));
    __asm__ volatile("msr mair_el1, %0" : : "r"(ROBO_MAIR));
    __asm__ volatile("msr tcr_el1, %0" : : "r"(ROBO_TCR));
    __asm__ volatile("msr ttbr0_el1, %0" : : "r"(gKernelTtbr));
    __asm__ volatile("isb" ::: "memory");
    __asm__ volatile("tlbi vmalle1" ::: "memory");
    __asm__ volatile("dsb ish" ::: "memory");
    __asm__ volatile("isb" ::: "memory");
    __asm__ volatile("msr sctlr_el1, %0" : : "r"(ROBO_SCTLR));
    __asm__ volatile("isb" ::: "memory");
}

bool RoboExcluded(uint64_t address, uint64_t stack, const RoboBootInfo* info) {
    if (address < 0x100000ull) {
        return true;
    }
    if (address >= ROBO_ARM_KERNEL && address < ROBO_ARM_KERNEL + ROBO_ARM_KERNEL_BYTES) {
        return true;
    }
    if (gHeapCarve != nullptr && address >= reinterpret_cast<uint64_t>(gHeapCarve) && address < gHeapCarveEnd) {
        return true;
    }
    for (uint32_t file = 0; file < info->fileCount && file < ROBO_BOOT_FILE_MAX; ++file) {
        const RoboBootFile* held = &info->files[file];
        if (held->size == 0 || held->address == 0) {
            continue;
        }
        uint64_t end = held->address + held->size;
        if (address >= held->address && address < end) {
            return true;
        }
    }
    uint64_t stackBase = stack & ~(ROBO_TWO_MB - 1ull);
    if (address >= stackBase && address < stackBase + ROBO_TWO_MB) {
        return true;
    }
    uint64_t frameEnd = info->frameBufferBase + static_cast<uint64_t>(info->height) * info->stride;
    uint64_t frameBase = info->frameBufferBase & ~(ROBO_PAGE_BYTES - 1ull);
    return address >= frameBase && address < frameEnd;
}

void RoboCarveHeap(const RoboBootInfo* info, uint64_t stack) {
    gHeapCarve = nullptr;
    gHeapCarveBytes = 0;
    gHeapCarveEnd = 0;
    uint64_t bestBase = 0;
    uint64_t bestPages = 0;
    for (uint32_t rangeIndex = 0; rangeIndex < info->memoryRangeCount; ++rangeIndex) {
        const RoboMemoryRange* range = &info->memoryRanges[rangeIndex];
        if ((range->type != 3u && range->type != 4u && range->type != 7u) || range->base >= ROBO_MAP_BYTES) {
            continue;
        }
        uint64_t pages = range->pageCount;
        if (range->base + pages * ROBO_PAGE_BYTES > ROBO_MAP_BYTES) {
            pages = (ROBO_MAP_BYTES - range->base) / ROBO_PAGE_BYTES;
        }
        uint64_t usable = 0;
        uint64_t start = 0;
        for (uint64_t page = 0; page < pages; ++page) {
            uint64_t address = range->base + page * ROBO_PAGE_BYTES;
            if (RoboExcluded(address, stack, info)) {
                usable = 0;
                start = 0;
                continue;
            }
            if (usable == 0) {
                start = address;
            }
            usable += 1;
            if (usable * ROBO_PAGE_BYTES >= ROBO_HEAP_CARVE_BYTES && usable > bestPages) {
                bestBase = start;
                bestPages = usable;
            }
        }
    }
    if (bestPages * ROBO_PAGE_BYTES < ROBO_HEAP_CARVE_BYTES) {
        return;
    }
    gHeapCarve = reinterpret_cast<uint8_t*>(bestBase);
    gHeapCarveBytes = ROBO_HEAP_CARVE_BYTES;
    gHeapCarveEnd = bestBase + ROBO_HEAP_CARVE_BYTES;
    RoboHeapInstall(gHeapCarve, gHeapCarveBytes);
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

void RoboSetBlockAttr(uint64_t address, uint64_t bytes, uint64_t attribute) {
    uint64_t end = address + bytes;
    if (end < address) {
        end = ROBO_MAP_BYTES;
    }
    uint64_t page = address & ~(ROBO_TWO_MB - 1ull);
    while (page < end && page < ROBO_MAP_BYTES) {
        uint32_t gigabyte = static_cast<uint32_t>(page / (512ull * ROBO_TWO_MB));
        uint32_t entry = static_cast<uint32_t>((page / ROBO_TWO_MB) & 511ull);
        if (gigabyte < 4u) {
            uint64_t* level2 = RoboTable(2u + gigabyte);
            uint64_t flags = level2[entry];
            flags &= ~(7ull << 2);
            flags |= attribute;
            level2[entry] = flags;
        }
        page += ROBO_TWO_MB;
    }
    RoboClean(gTables, sizeof(gTables));
    __asm__ volatile("dsb ish; tlbi vmalle1; dsb ish; isb" ::: "memory");
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

__attribute__((naked)) void RoboDropEl2(const RoboBootInfo*) {
    __asm__ volatile(
        "mov x2, sp\n"
        "msr sp_el1, x2\n"
        "ldr x2, 1f\n"
        "msr elr_el2, x2\n"
        "mov x2, #0x3c5\n"
        "msr spsr_el2, x2\n"
        "eret\n"
        "1: .quad RoboMemoryEntered\n");
}

extern "C" void RoboMemoryEntered(const RoboBootInfo* info) {
    uint64_t stack = 0;
    __asm__ volatile("mov %0, sp" : "=r"(stack));
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
    RoboBuildTables();
    uint64_t level = 0;
    __asm__ volatile("mrs %0, CurrentEL" : "=r"(level));
    if (((level >> 2) & 3ull) == 2ull) {
        RoboProgramEl2();
        RoboProgramEl1();
        RoboDropEl2(info);
    }
    RoboProgramEl1();
    RoboMemoryEntered(info);
}

uint64_t RoboMemoryKernelCr3(void) {
    return gKernelTtbr;
}

void RoboMemoryMarkWriteCombine(uint64_t address, uint64_t bytes) {
    if (bytes == 0 || gKernelTtbr == 0) {
        return;
    }
    RoboSetBlockAttr(address, bytes, ROBO_DESC_ATTR_NC);
}

uint64_t RoboMemoryVideoBuffer(void) {
    return 0;
}

void RoboMemoryMarkUncached(uint64_t address, uint64_t bytes) {
    if (bytes == 0 || gKernelTtbr == 0) {
        return;
    }
    RoboSetBlockAttr(address, bytes, ROBO_DESC_ATTR_DEVICE);
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
    uint64_t level0 = RoboPageAlloc();
    uint64_t level1 = RoboPageAlloc();
    uint64_t level2 = RoboPageAlloc();
    uint64_t level3 = RoboPageAlloc();
    uint64_t codePage = RoboPageAlloc();
    uint64_t stackPage = RoboPageAlloc();
    if (level0 == 0 || level1 == 0 || level2 == 0 || level3 == 0 || codePage == 0 || stackPage == 0) {
        return 0;
    }
    uint64_t* userLevel0 = reinterpret_cast<uint64_t*>(level0);
    uint64_t* userLevel1 = reinterpret_cast<uint64_t*>(level1);
    uint64_t* userLevel2 = reinterpret_cast<uint64_t*>(level2);
    uint64_t* userLevel3 = reinterpret_cast<uint64_t*>(level3);
    userLevel0[0] = RoboTable(0)[0];
    uint64_t level0Index = (ROBO_USER_CODE >> 39) & 0x1FFull;
    uint64_t level1Index = (ROBO_USER_CODE >> 30) & 0x1FFull;
    uint64_t level2Index = (ROBO_USER_CODE >> 21) & 0x1FFull;
    uint64_t pageIndex = (ROBO_USER_CODE >> 12) & 0x1FFull;
    uint64_t userFlags = ROBO_DESC_PAGE | ROBO_DESC_AF | ROBO_DESC_SH | ROBO_DESC_AP_USER;
    userLevel0[level0Index] = level1 | ROBO_DESC_TABLE;
    userLevel1[level1Index] = level2 | ROBO_DESC_TABLE;
    userLevel2[level2Index] = level3 | ROBO_DESC_TABLE;
    userLevel3[pageIndex] = codePage | userFlags;
    userLevel3[(ROBO_USER_STACK >> 12) & 0x1FFull] = stackPage | userFlags | ROBO_DESC_UXN;
    const uint8_t* bytes = static_cast<const uint8_t*>(code);
    uint8_t* dest = reinterpret_cast<uint8_t*>(codePage);
    for (uint64_t index = 0; index < codeSize && index < ROBO_PAGE_BYTES; ++index) {
        dest[index] = bytes[index];
    }
    RoboClean(userLevel0, ROBO_PAGE_BYTES);
    RoboClean(userLevel1, ROBO_PAGE_BYTES);
    RoboClean(userLevel2, ROBO_PAGE_BYTES);
    RoboClean(userLevel3, ROBO_PAGE_BYTES);
    RoboClean(dest, ROBO_PAGE_BYTES);
    *userRip = ROBO_USER_CODE;
    *userRsp = ROBO_USER_STACK + ROBO_PAGE_BYTES - 16ull;
    return level0;
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
    uint64_t level0 = RoboPageAlloc();
    uint64_t level1 = RoboPageAlloc();
    uint64_t level2 = RoboPageAlloc();
    uint64_t level3 = RoboPageAlloc();
    uint64_t stackPage = RoboPageAlloc();
    if (level0 == 0 || level1 == 0 || level2 == 0 || level3 == 0 || stackPage == 0) {
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
    uint64_t* userLevel0 = reinterpret_cast<uint64_t*>(level0);
    uint64_t* userLevel1 = reinterpret_cast<uint64_t*>(level1);
    uint64_t* userLevel2 = reinterpret_cast<uint64_t*>(level2);
    uint64_t* userLevel3 = reinterpret_cast<uint64_t*>(level3);
    userLevel0[0] = RoboTable(0)[0];
    uint64_t userFlags = ROBO_DESC_PAGE | ROBO_DESC_AF | ROBO_DESC_SH | ROBO_DESC_AP_USER;
    userLevel0[(ROBO_USER_CODE >> 39) & 0x1FFull] = level1 | ROBO_DESC_TABLE;
    userLevel1[(ROBO_USER_CODE >> 30) & 0x1FFull] = level2 | ROBO_DESC_TABLE;
    userLevel2[(ROBO_USER_CODE >> 21) & 0x1FFull] = level3 | ROBO_DESC_TABLE;
    const uint8_t* bytes = static_cast<const uint8_t*>(code);
    uint64_t copied = 0;
    for (uint32_t page = 0; page < codePages; ++page) {
        userLevel3[page] = codePhysical[page] | userFlags;
        uint8_t* dest = reinterpret_cast<uint8_t*>(codePhysical[page]);
        for (uint32_t offset = 0; offset < ROBO_PAGE_BYTES && copied < codeSize; ++offset) {
            dest[offset] = bytes[copied];
            copied += 1ull;
        }
        RoboClean(dest, ROBO_PAGE_BYTES);
    }
    uint32_t stackIndex = static_cast<uint32_t>((ROBO_USER_STACK >> 12) & 0x1FFull);
    uint32_t heapIndex = static_cast<uint32_t>((ROBO_USER_HEAP >> 12) & 0x1FFull);
    userLevel3[stackIndex] = stackPage | userFlags | ROBO_DESC_UXN;
    for (uint32_t page = 0; page < heapPages; ++page) {
        userLevel3[heapIndex + page] = heapPhysical[page] | userFlags | ROBO_DESC_UXN;
        RoboClean(reinterpret_cast<void*>(heapPhysical[page]), ROBO_PAGE_BYTES);
    }
    RoboClean(userLevel0, ROBO_PAGE_BYTES);
    RoboClean(userLevel1, ROBO_PAGE_BYTES);
    RoboClean(userLevel2, ROBO_PAGE_BYTES);
    RoboClean(userLevel3, ROBO_PAGE_BYTES);
    RoboClean(reinterpret_cast<void*>(stackPage), ROBO_PAGE_BYTES);
    map->tables = level0;
    map->rip = ROBO_USER_CODE;
    map->rsp = ROBO_USER_STACK + ROBO_PAGE_BYTES - 16ull;
    map->heap = ROBO_USER_HEAP;
    map->heapPhysical = heapPhysical[0];
    return true;
}
