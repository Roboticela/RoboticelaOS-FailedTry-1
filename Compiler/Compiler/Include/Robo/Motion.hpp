#pragma once

#include <string>
#include <vector>

namespace Robo {

struct MotionEasing {
    float x1 = 0.0f;
    float y1 = 0.0f;
    float x2 = 1.0f;
    float y2 = 1.0f;
    bool linear = true;
};

struct MotionKeyframe {
    float at = 0.0f;
    float x = 0.0f;
    float y = 0.0f;
    float opacity = 1.0f;
    float scale = 1.0f;
    float rotate = 0.0f;
    bool hasX = false;
    bool hasY = false;
    bool hasOpacity = false;
    bool hasScale = false;
    bool hasRotate = false;
};

struct MotionClip {
    std::string name;
    float durationMs = 1000.0f;
    float delayMs = 0.0f;
    bool infinite = false;
    MotionEasing easing;
    std::vector<MotionKeyframe> frames;
};

struct MotionSample {
    float x = 0.0f;
    float y = 0.0f;
    float opacity = 1.0f;
    float scale = 1.0f;
    float rotate = 0.0f;
    bool hasX = false;
    bool hasY = false;
    bool hasOpacity = false;
    bool hasScale = false;
    bool hasRotate = false;
    bool playing = false;
};

MotionEasing ParseEasing(const std::string& text);
float Ease(float t, const MotionEasing& easing);
float ParseDurationMs(const std::string& text);

// Moves a 0..1 theme mix toward the current theme with the Apple ease.
void AdvanceThemeMix(float deltaMs,
                     float durationMs,
                     bool& primed,
                     bool& mixing,
                     float& mix,
                     float& mixFrom,
                     float& mixTo,
                     float& mixElapsed);

void RegisterMotion(MotionClip clip);
const MotionClip* FindMotion(const std::string& name);
MotionSample SampleMotion(const std::string& name, float elapsedMs, float extraDelayMs);

}  // namespace Robo
