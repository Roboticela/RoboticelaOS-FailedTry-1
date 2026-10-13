#include "RoboVideo.hpp"

#include <cstdint>
#include <vector>

namespace Robo {
bool RoboDecodeJpeg(const std::uint8_t* bytes, std::size_t size, std::vector<std::uint8_t>& pixels, int& width, int& height);
}

namespace {

constexpr uint32_t ROBO_VIDEO_IDLE = 0;
constexpr uint32_t ROBO_VIDEO_BUSY = 1;
constexpr uint32_t ROBO_VIDEO_READY = 2;

extern "C" int RoboHeapHas(size_t bytes);

const uint8_t* gFile = nullptr;
uint32_t gFileBytes = 0;
int gFrameCount = 0;
int gWidth = 0;
int gHeight = 0;
volatile uint32_t gState = ROBO_VIDEO_IDLE;
volatile int gRequest = -1;
volatile int gReady = -1;
std::vector<uint8_t> gBack;

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
    if (gBack.size() != bytes) {
        if (RoboHeapHas(bytes + 64u) == 0) {
            gFile = nullptr;
            return;
        }
        gBack.assign(bytes, 0);
    }
}

void RoboVideoRequest(int index) {
    if (gFile == nullptr || gState != ROBO_VIDEO_IDLE || gFrameCount <= 1) {
        return;
    }
    if (index < 0) {
        index = 0;
    }
    if (index >= gFrameCount) {
        index %= gFrameCount;
    }
    gRequest = index;
    __asm__ volatile("" ::: "memory");
    gState = ROBO_VIDEO_BUSY;
}

bool RoboVideoCollect(int index, uint8_t* dest, uint32_t destBytes) {
    if (dest == nullptr || destBytes != gBack.size() || gState != ROBO_VIDEO_READY || gReady != index) {
        if (gState == ROBO_VIDEO_READY) {
            gState = ROBO_VIDEO_IDLE;
        }
        return false;
    }
    memcpy(dest, gBack.data(), destBytes);
    __asm__ volatile("" ::: "memory");
    gState = ROBO_VIDEO_IDLE;
    int next = index + 1;
    if (next >= gFrameCount) {
        next = 0;
    }
    RoboVideoRequest(next);
    return true;
}

void RoboVideoPoll(void) {
    if (gState != ROBO_VIDEO_BUSY || gFile == nullptr) {
        return;
    }
    const int index = gRequest;
    const std::size_t table = 20u + static_cast<std::size_t>(index) * 4u;
    if (table + 8u > gFileBytes) {
        gState = ROBO_VIDEO_IDLE;
        return;
    }
    const int start = RoboVideoRead32(gFile + table);
    const int end = RoboVideoRead32(gFile + table + 4);
    int width = 0;
    int height = 0;
    if (start < 0 || end <= start || static_cast<uint32_t>(end) > gFileBytes ||
        !Robo::RoboDecodeJpeg(gFile + start, static_cast<std::size_t>(end - start), gBack, width, height)) {
        gState = ROBO_VIDEO_IDLE;
        return;
    }
    gReady = index;
    __asm__ volatile("" ::: "memory");
    gState = ROBO_VIDEO_READY;
}
