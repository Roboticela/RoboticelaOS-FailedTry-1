#include "Robo/Motion.hpp"

#include "Robo/Foundation.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>

namespace Robo {

namespace {

float Bezier1(float t, float a, float b) {
    const float u = 1.0f - t;
    return 3.0f * u * u * t * a + 3.0f * u * t * t * b + t * t * t;
}

float BezierDerivative(float t, float a, float b) {
    const float u = 1.0f - t;
    return 3.0f * u * u * a + 6.0f * u * t * (b - a) + 3.0f * t * t * (1.0f - b);
}

float SolveBezier(float x, float x1, float x2) {
    float t = x;
    for (int i = 0; i < 8; ++i) {
        const float delta = Bezier1(t, x1, x2) - x;
        const float slope = BezierDerivative(t, x1, x2);
        if (std::fabs(slope) < 0.0001f) {
            break;
        }
        t = std::max(0.0f, std::min(1.0f, t - delta / slope));
    }
    return t;
}

std::map<std::string, MotionClip>& Clips() {
    static std::map<std::string, MotionClip> clips;
    return clips;
}

float Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

}  // namespace

MotionEasing ParseEasing(const std::string& text) {
    MotionEasing easing;
    if (text == "linear" || text.empty()) {
        easing.linear = true;
        return easing;
    }
    easing.linear = false;
    if (text == "ease") {
        easing.x1 = 0.25f;
        easing.y1 = 0.1f;
        easing.x2 = 0.25f;
        easing.y2 = 1.0f;
        return easing;
    }
    if (text == "ease-in") {
        easing.x1 = 0.42f;
        easing.y1 = 0.0f;
        easing.x2 = 1.0f;
        easing.y2 = 1.0f;
        return easing;
    }
    if (text == "ease-out") {
        easing.x1 = 0.0f;
        easing.y1 = 0.0f;
        easing.x2 = 0.58f;
        easing.y2 = 1.0f;
        return easing;
    }
    if (text == "ease-in-out") {
        easing.x1 = 0.42f;
        easing.y1 = 0.0f;
        easing.x2 = 0.58f;
        easing.y2 = 1.0f;
        return easing;
    }
    const std::size_t open = text.find('(');
    const std::size_t close = text.find(')');
    if (open != std::string::npos && close != std::string::npos && close > open) {
        std::string body = text.substr(open + 1, close - open - 1);
        float values[4] = {0.0f, 0.0f, 1.0f, 1.0f};
        int count = 0;
        std::string token;
        for (std::size_t i = 0; i <= body.size() && count < 4; ++i) {
            if (i == body.size() || body[i] == ',') {
                if (!token.empty()) {
                    values[count++] = static_cast<float>(std::atof(token.c_str()));
                    token.clear();
                }
            } else if (body[i] != ' ') {
                token.push_back(body[i]);
            }
        }
        easing.x1 = values[0];
        easing.y1 = values[1];
        easing.x2 = values[2];
        easing.y2 = values[3];
    }
    return easing;
}

float Ease(float t, const MotionEasing& easing) {
    t = std::max(0.0f, std::min(1.0f, t));
    if (easing.linear) {
        return t;
    }
    const float solved = SolveBezier(t, easing.x1, easing.x2);
    return Bezier1(solved, easing.y1, easing.y2);
}

float ParseDurationMs(const std::string& text) {
    if (text.size() > 2 && text.compare(text.size() - 2, 2, "ms") == 0) {
        return static_cast<float>(std::atof(text.c_str()));
    }
    if (!text.empty() && text.back() == 's') {
        return static_cast<float>(std::atof(text.c_str())) * 1000.0f;
    }
    return static_cast<float>(std::atof(text.c_str()));
}

void AdvanceThemeMix(float deltaMs,
                     float durationMs,
                     bool& primed,
                     bool& mixing,
                     float& mix,
                     float& mixFrom,
                     float& mixTo,
                     float& mixElapsed) {
    const float target = CurrentTheme() == Theme::Dark ? 1.0f : 0.0f;
    if (!primed) {
        mix = target;
        mixTo = target;
        primed = true;
        mixing = false;
        return;
    }
    if (std::fabs(mixTo - target) <= 0.0001f && std::fabs(mix - target) <= 0.0001f) {
        return;
    }
    if (!mixing || std::fabs(mixTo - target) > 0.0001f) {
        mixFrom = mix;
        mixTo = target;
        mixElapsed = 0.0f;
        mixing = durationMs > 0.0f;
    }
    if (!mixing) {
        mix = target;
        return;
    }
    mixElapsed += deltaMs;
    const float progress = std::min(1.0f, mixElapsed / std::max(1.0f, durationMs));
    MotionEasing easing;
    easing.linear = false;
    easing.x1 = 0.28f;
    easing.y1 = 0.11f;
    easing.x2 = 0.32f;
    easing.y2 = 1.0f;
    mix = mixFrom + (mixTo - mixFrom) * Ease(progress, easing);
    if (progress >= 1.0f) {
        mixing = false;
        mix = target;
    }
}

void RegisterMotion(MotionClip clip) {
    Clips()[clip.name] = std::move(clip);
}

const MotionClip* FindMotion(const std::string& name) {
    const auto found = Clips().find(name);
    if (found == Clips().end()) {
        return nullptr;
    }
    return &found->second;
}

MotionSample SampleMotion(const std::string& name, float elapsedMs, float extraDelayMs) {
    MotionSample sample;
    const MotionClip* clip = FindMotion(name);
    if (clip == nullptr || clip->frames.empty()) {
        return sample;
    }
    const float start = clip->delayMs + extraDelayMs;
    if (elapsedMs < start) {
        sample.playing = clip->infinite;
        return sample;
    }
    const float duration = std::max(1.0f, clip->durationMs);
    float local = elapsedMs - start;
    if (local >= duration && !clip->infinite) {
        local = duration;
    }
    const float turns = local / duration;
    const float cycle = clip->infinite ? (turns - std::floor(turns)) : std::min(1.0f, turns);
    const MotionEasing& easing = clip->easing;
    const std::vector<MotionKeyframe>& frames = clip->frames;
    if (frames.size() == 1) {
        sample = {};
        sample.playing = clip->infinite;
        sample.x = frames[0].x;
        sample.y = frames[0].y;
        sample.opacity = frames[0].opacity;
        sample.scale = frames[0].scale;
        sample.rotate = frames[0].rotate;
        sample.hasX = frames[0].hasX;
        sample.hasY = frames[0].hasY;
        sample.hasOpacity = frames[0].hasOpacity;
        sample.hasScale = frames[0].hasScale;
        sample.hasRotate = frames[0].hasRotate;
        return sample;
    }
    std::size_t upper = 1;
    while (upper + 1 < frames.size() && frames[upper].at < cycle) {
        ++upper;
    }
    const MotionKeyframe& a = frames[upper - 1];
    const MotionKeyframe& b = frames[upper];
    const float span = std::max(0.0001f, b.at - a.at);
    const float localT = Ease(std::max(0.0f, std::min(1.0f, (cycle - a.at) / span)), easing);
    sample.playing = clip->infinite || cycle < 1.0f;
    if (a.hasX || b.hasX) {
        sample.hasX = true;
        sample.x = Lerp(a.x, b.x, localT);
    }
    if (a.hasY || b.hasY) {
        sample.hasY = true;
        sample.y = Lerp(a.y, b.y, localT);
    }
    if (a.hasOpacity || b.hasOpacity) {
        sample.hasOpacity = true;
        sample.opacity = Lerp(a.opacity, b.opacity, localT);
    }
    if (a.hasScale || b.hasScale) {
        sample.hasScale = true;
        sample.scale = Lerp(a.scale, b.scale, localT);
    }
    if (a.hasRotate || b.hasRotate) {
        sample.hasRotate = true;
        sample.rotate = Lerp(a.rotate, b.rotate, localT);
    }
    return sample;
}

}  // namespace Robo
