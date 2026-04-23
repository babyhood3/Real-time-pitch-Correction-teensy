#pragma once
#include <cstddef>

// YIN pitch detection algorithm (de Cheveigne & Kawahara 2002).
// Operates on a mono float buffer sampled at `sampleRate`.
class YINPitchDetector {
public:
    static constexpr size_t BUFFER_SIZE = 2048;
    static constexpr float  DEFAULT_THRESHOLD = 0.15f;
    static constexpr float  MIN_FREQ = 60.0f;
    static constexpr float  MAX_FREQ = 2000.0f;

    explicit YINPitchDetector(float sampleRate, float threshold = DEFAULT_THRESHOLD);

    // Returns detected frequency in Hz, or -1 if no pitch found.
    float detect(const float* buffer, size_t size);

    void  setThreshold(float threshold);
    float getThreshold() const;

    // Confidence in range [0, 1]. Valid after detect().
    float getConfidence() const;
    bool  isPitched() const;

private:
    void  computeDifference(const float* buffer, size_t size);
    void  computeCumulativeMeanNormalized();
    int   absoluteThreshold() const;
    float parabolicInterpolation(int tauEstimate) const;

    float sampleRate_;
    float threshold_;
    float confidence_;
    int   minPeriod_;
    int   maxPeriod_;
    float yinBuffer_[BUFFER_SIZE / 2];
};
