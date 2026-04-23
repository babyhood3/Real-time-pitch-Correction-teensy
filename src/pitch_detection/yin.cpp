#include "yin.h"
#include <cmath>
#include <algorithm>
#include <limits>

YINPitchDetector::YINPitchDetector(float sampleRate, float threshold)
    : sampleRate_(sampleRate), threshold_(threshold), confidence_(0.0f) {
    minPeriod_ = static_cast<int>(sampleRate / MAX_FREQ);
    maxPeriod_ = static_cast<int>(sampleRate / MIN_FREQ);
    if (maxPeriod_ >= static_cast<int>(BUFFER_SIZE / 2)) {
        maxPeriod_ = static_cast<int>(BUFFER_SIZE / 2) - 1;
    }
    if (minPeriod_ < 2) minPeriod_ = 2;
}

float YINPitchDetector::detect(const float* buffer, size_t size) {
    if (size < BUFFER_SIZE) {
        confidence_ = 0.0f;
        return -1.0f;
    }
    computeDifference(buffer, size);
    computeCumulativeMeanNormalized();

    int tauEstimate = absoluteThreshold();
    if (tauEstimate < 0) {
        confidence_ = 0.0f;
        return -1.0f;
    }

    float refinedTau = parabolicInterpolation(tauEstimate);
    confidence_ = 1.0f - yinBuffer_[tauEstimate];
    return sampleRate_ / refinedTau;
}

void YINPitchDetector::computeDifference(const float* buffer, size_t size) {
    int halfLen = static_cast<int>(size) / 2;
    int limit = std::min(maxPeriod_ + 1, static_cast<int>(BUFFER_SIZE / 2));
    for (int tau = 0; tau < limit; ++tau) {
        yinBuffer_[tau] = 0.0f;
        for (int i = 0; i < halfLen; ++i) {
            float delta = buffer[i] - buffer[i + tau];
            yinBuffer_[tau] += delta * delta;
        }
    }
}

void YINPitchDetector::computeCumulativeMeanNormalized() {
    yinBuffer_[0] = 1.0f;
    float runningSum = 0.0f;
    int limit = std::min(maxPeriod_ + 1, static_cast<int>(BUFFER_SIZE / 2));
    for (int tau = 1; tau < limit; ++tau) {
        runningSum += yinBuffer_[tau];
        if (runningSum > std::numeric_limits<float>::epsilon()) {
            yinBuffer_[tau] *= static_cast<float>(tau) / runningSum;
        } else {
            yinBuffer_[tau] = 1.0f;
        }
    }
}

int YINPitchDetector::absoluteThreshold() const {
    for (int tau = minPeriod_; tau <= maxPeriod_; ++tau) {
        if (yinBuffer_[tau] < threshold_) {
            while (tau + 1 <= maxPeriod_ && yinBuffer_[tau + 1] < yinBuffer_[tau]) {
                ++tau;
            }
            return tau;
        }
    }
    return -1;
}

float YINPitchDetector::parabolicInterpolation(int tau) const {
    if (tau <= 0 || tau >= maxPeriod_) {
        return static_cast<float>(tau);
    }
    float s0 = yinBuffer_[tau - 1];
    float s1 = yinBuffer_[tau];
    float s2 = yinBuffer_[tau + 1];
    float denom = 2.0f * (2.0f * s1 - s2 - s0);
    if (fabsf(denom) < std::numeric_limits<float>::epsilon()) {
        return static_cast<float>(tau);
    }
    return static_cast<float>(tau) + (s2 - s0) / denom;
}

void  YINPitchDetector::setThreshold(float t) { threshold_ = t; }
float YINPitchDetector::getThreshold()  const  { return threshold_; }
float YINPitchDetector::getConfidence() const  { return confidence_; }
bool  YINPitchDetector::isPitched()     const  { return confidence_ > 0.5f; }
