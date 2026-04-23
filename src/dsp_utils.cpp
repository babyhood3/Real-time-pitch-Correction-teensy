#include "dsp_utils.h"
#include <cmath>
#include <algorithm>
#include <limits>

namespace dsp {

static constexpr float PI = 3.14159265358979323846f;
static constexpr float TWO_PI = 2.0f * PI;

void applyHannWindow(float* buffer, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        float w = 0.5f * (1.0f - cosf(TWO_PI * i / (size - 1)));
        buffer[i] *= w;
    }
}

void applyHammingWindow(float* buffer, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        float w = 0.54f - 0.46f * cosf(TWO_PI * i / (size - 1));
        buffer[i] *= w;
    }
}

float parabolicPeak(const float* buffer, size_t peak) {
    if (peak == 0) return 0.0f;
    float s0 = buffer[peak - 1];
    float s1 = buffer[peak];
    float s2 = buffer[peak + 1];
    float denom = 2.0f * s1 - s2 - s0;
    if (fabsf(denom) < std::numeric_limits<float>::epsilon()) {
        return static_cast<float>(peak);
    }
    return static_cast<float>(peak) + 0.5f * (s2 - s0) / denom;
}

float hzToMidi(float hz, float a4Hz) {
    if (hz <= 0.0f) return -1.0f;
    return 69.0f + 12.0f * log2f(hz / a4Hz);
}

float midiToHz(float midi, float a4Hz) {
    return a4Hz * powf(2.0f, (midi - 69.0f) / 12.0f);
}

float hzToCents(float hz, float referenceHz) {
    if (hz <= 0.0f || referenceHz <= 0.0f) return 0.0f;
    return 1200.0f * log2f(hz / referenceHz);
}

float semitoneRatio(float semitones) {
    return powf(2.0f, semitones / 12.0f);
}

float nearestScaleNote(float hz, const float* scaleSemitonesFromC, int scaleLen, float a4Hz) {
    float midi = hzToMidi(hz, a4Hz);
    if (midi < 0.0f) return hz;

    float octave = floorf((midi - 12.0f) / 12.0f);
    float pitchClass = midi - (octave + 1.0f) * 12.0f;

    float best = scaleSemitonesFromC[0];
    float bestDist = fabsf(pitchClass - scaleSemitonesFromC[0]);

    for (int i = 1; i < scaleLen; ++i) {
        float dist = fabsf(pitchClass - scaleSemitonesFromC[i]);
        if (dist < bestDist) {
            bestDist = dist;
            best = scaleSemitonesFromC[i];
        }
    }

    float targetMidi = (octave + 1.0f) * 12.0f + best;
    return midiToHz(targetMidi, a4Hz);
}

float computeRMS(const float* buffer, size_t size) {
    if (size == 0) return 0.0f;
    float sum = 0.0f;
    for (size_t i = 0; i < size; ++i) {
        sum += buffer[i] * buffer[i];
    }
    return sqrtf(sum / static_cast<float>(size));
}

void normalizeBuffer(float* buffer, size_t size) {
    float peak = 0.0f;
    for (size_t i = 0; i < size; ++i) {
        float a = fabsf(buffer[i]);
        if (a > peak) peak = a;
    }
    if (peak < std::numeric_limits<float>::epsilon()) return;
    float inv = 1.0f / peak;
    for (size_t i = 0; i < size; ++i) {
        buffer[i] *= inv;
    }
}

} // namespace dsp
