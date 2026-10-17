#pragma once

#include <cstddef>
#include <memory>
#include <new>
#include <utility>

namespace Robo {

struct RoboPoolSlot {
    RoboPoolSlot* next = nullptr;
};

inline RoboPoolSlot** RoboPoolLists() {
    static RoboPoolSlot* lists[8] = {};
    return lists;
}

inline const std::size_t* RoboPoolClasses() {
    static const std::size_t classes[8] = {64u, 128u, 256u, 512u, 1024u, 2048u, 4096u, 8192u};
    return classes;
}

inline void* RoboPoolAlloc(std::size_t bytes) {
    RoboPoolSlot** lists = RoboPoolLists();
    const std::size_t* classes = RoboPoolClasses();
    if (bytes == 0) {
        bytes = 1;
    }
    for (int bin = 0; bin < 8; ++bin) {
        if (bytes <= classes[bin]) {
            if (lists[bin] != nullptr) {
                RoboPoolSlot* slot = lists[bin];
                lists[bin] = slot->next;
                return slot;
            }
            return ::operator new(classes[bin]);
        }
    }
    return ::operator new(bytes);
}

inline void RoboPoolFree(void* pointer, std::size_t bytes) {
    if (pointer == nullptr) {
        return;
    }
    RoboPoolSlot** lists = RoboPoolLists();
    const std::size_t* classes = RoboPoolClasses();
    for (int bin = 0; bin < 8; ++bin) {
        if (bytes <= classes[bin]) {
            RoboPoolSlot* slot = static_cast<RoboPoolSlot*>(pointer);
            slot->next = lists[bin];
            lists[bin] = slot;
            return;
        }
    }
    ::operator delete(pointer);
}

template <typename T>
struct RoboPoolAllocator {
    using value_type = T;
    RoboPoolAllocator() = default;
    template <typename U>
    RoboPoolAllocator(const RoboPoolAllocator<U>&) {}
    T* allocate(std::size_t count) {
        return static_cast<T*>(RoboPoolAlloc(sizeof(T) * count));
    }
    void deallocate(T* pointer, std::size_t count) noexcept {
        RoboPoolFree(pointer, sizeof(T) * count);
    }
};

template <typename T, typename U>
bool operator==(const RoboPoolAllocator<T>&, const RoboPoolAllocator<U>&) {
    return true;
}

template <typename T>
void RoboPoolDelete(void* pointer) {
    T* typed = static_cast<T*>(pointer);
    typed->~T();
    RoboPoolFree(pointer, sizeof(T));
}

template <typename T, typename... Args>
std::shared_ptr<T> RoboMake(Args&&... args) {
    void* memory = RoboPoolAlloc(sizeof(T));
    T* object = new (memory) T(std::forward<Args>(args)...);
    return std::shared_ptr<T>(object, RoboPoolDelete<T>);
}

}  // namespace Robo
