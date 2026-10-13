#include "RoboVideo.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

namespace Robo {
bool RoboDecodeJpeg(const std::uint8_t* bytes, std::size_t size, std::vector<std::uint8_t>& pixels, int& width, int& height);
}

namespace {

constexpr uint32_t ROBO_VIDEO_IDLE = 0;
constexpr uint32_t ROBO_VIDEO_BUSY = 1;
constexpr uint32_t ROBO_VIDEO_READY = 2;
constexpr uint32_t ROBO_VIDEO_WORKERS = 3;

extern "C" int RoboHeapHas(size_t bytes);

struct RoboVideoSlot {
    volatile uint32_t state;
    volatile uint32_t online;
    volatile int request;
    volatile int ready;
    std::vector<uint8_t> pixels;
};

const uint8_t* gFile = nullptr;
uint32_t gFileBytes = 0;
int gFrameCount = 0;
int gWidth = 0;
int gHeight = 0;
int gBuffers = 0;
RoboVideoSlot gSlot[ROBO_VIDEO_WORKERS];

int RoboVideoRead32(const uint8_t* bytes) {
    return static_cast<int>(bytes[0]) | (static_cast<int>(bytes[1]) << 8) | (static_cast<int>(bytes[2]) << 16) | (static_cast<int>(bytes[3]) << 24);
}

}  // namespace

void RoboVideoArm(const uint8_t* file, uint32_t fileBytes, int frameCount, int width, int height) {
    if (file == nullptr || fileBytes < 24u || frameCount <= 1 || width <= 0 || height <= 0) {
        return;
    }
    gFile = file;
    gFileBytes = fileBytes;
    gFrameCount = frameCount;
    gWidth = width;
    gHeight = height;
    const std::size_t bytes = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u;
    gBuffers = 0;
    for (uint32_t worker = 0; worker < ROBO_VIDEO_WORKERS; ++worker) {
        if (gSlot[worker].pixels.size() != bytes) {
            if (RoboHeapHas(bytes + 64u) == 0) {
                break;
            }
            gSlot[worker].state = ROBO_VIDEO_IDLE;
            gSlot[worker].pixels.assign(bytes, 0);
        }
        gBuffers += 1;
    }
}

uint32_t RoboVideoOnline(void) {
    uint32_t online = 0;
    uint32_t limit = static_cast<uint32_t>(gBuffers);
    if (limit > ROBO_VIDEO_WORKERS) {
        limit = ROBO_VIDEO_WORKERS;
    }
    for (uint32_t worker = 0; worker < limit; ++worker) {
        if (gSlot[worker].online != 0) {
            online += 1;
        }
    }
    return online;
}

bool RoboVideoQueued(int index) {
    for (int worker = 0; worker < gBuffers; ++worker) {
        uint32_t state = gSlot[worker].state;
        if (state == ROBO_VIDEO_BUSY && gSlot[worker].request == index) {
            return true;
        }
        if (state == ROBO_VIDEO_READY && gSlot[worker].ready == index) {
            return true;
        }
    }
    return false;
}

void RoboVideoRequest(int index) {
    if (gFile == nullptr || gFrameCount <= 1 || gBuffers <= 0) {
        return;
    }
    if (index < 0) {
        index = 0;
    }
    uint32_t online = RoboVideoOnline();
    if (online == 0) {
        return;
    }
    for (uint32_t step = 0; step < online; ++step) {
        int frame = index + static_cast<int>(step);
        frame %= gFrameCount;
        if (RoboVideoQueued(frame)) {
            continue;
        }
        for (int worker = 0; worker < gBuffers; ++worker) {
            if (gSlot[worker].online == 0 || gSlot[worker].state != ROBO_VIDEO_IDLE) {
                continue;
            }
            if (gSlot[worker].pixels.size() != static_cast<std::size_t>(gWidth) * static_cast<std::size_t>(gHeight) * 4u) {
                continue;
            }
            gSlot[worker].request = frame;
            gSlot[worker].ready = -1;
            __asm__ volatile("" ::: "memory");
            gSlot[worker].state = ROBO_VIDEO_BUSY;
            break;
        }
    }
}

bool RoboVideoCollect(int index, uint8_t* dest, uint32_t destBytes) {
    if (dest == nullptr) {
        return false;
    }
    for (int worker = 0; worker < gBuffers; ++worker) {
        if (gSlot[worker].state != ROBO_VIDEO_READY || gSlot[worker].ready != index) {
            continue;
        }
        if (destBytes != gSlot[worker].pixels.size()) {
            return false;
        }
        memcpy(dest, gSlot[worker].pixels.data(), destBytes);
        __asm__ volatile("" ::: "memory");
        gSlot[worker].state = ROBO_VIDEO_IDLE;
        int next = index + 1;
        if (next >= gFrameCount) {
            next = 0;
        }
        RoboVideoRequest(next);
        return true;
    }
    return false;
}

void RoboVideoPoll(uint32_t worker) {
    if (worker >= ROBO_VIDEO_WORKERS) {
        return;
    }
    gSlot[worker].online = 1;
    if (gSlot[worker].state != ROBO_VIDEO_BUSY || gFile == nullptr || gSlot[worker].pixels.empty()) {
        return;
    }
    const int index = gSlot[worker].request;
    const std::size_t table = 20u + static_cast<std::size_t>(index) * 4u;
    if (table + 8u > gFileBytes) {
        gSlot[worker].state = ROBO_VIDEO_IDLE;
        return;
    }
    const int start = RoboVideoRead32(gFile + table);
    const int end = RoboVideoRead32(gFile + table + 4);
    int width = 0;
    int height = 0;
    if (start < 0 || end <= start || static_cast<uint32_t>(end) > gFileBytes ||
        !Robo::RoboDecodeJpeg(gFile + start, static_cast<std::size_t>(end - start), gSlot[worker].pixels, width, height)) {
        gSlot[worker].state = ROBO_VIDEO_IDLE;
        return;
    }
    gSlot[worker].ready = index;
    __asm__ volatile("" ::: "memory");
    gSlot[worker].state = ROBO_VIDEO_READY;
}
