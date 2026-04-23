#include "pitch_correction.h"
#include "dsp_utils.h"
#include <cmath>
#include <cstring>
#include <algorithm>

const float PitchCorrection::CHROMATIC[12] = {
    0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f,
    6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f
};

PitchCorrection::PitchCorrection(const Config& config)
    : config_(config),
      detectedPitch_(0.0f),
      correctedPitch_(0.0f),
      correctionCents_(0.0f) {
    detector_       = new YINPitchDetector(config_.sampleRate, config_.detectionThreshold);
    vocoder_        = new PhaseVocoder(config_.sampleRate);
    analysisBuffer_ = new AudioBuffer();
}

PitchCorrection::~PitchCorrection() {
    delete detector_;
    delete vocoder_;
    delete analysisBuffer_;
}

void PitchCorrection::process(const float* input, float* output, size_t frameSize) {
    analysisBuffer_->write(input, frameSize);

    // Run detection when we have enough data
    if (analysisBuffer_->available() >= YINPitchDetector::BUFFER_SIZE) {
        float detectionBuf[YINPitchDetector::BUFFER_SIZE];
        analysisBuffer_->read(detectionBuf, YINPitchDetector::BUFFER_SIZE);

        float detected = detector_->detect(detectionBuf, YINPitchDetector::BUFFER_SIZE);
        if (detected > 0.0f && detector_->isPitched()) {
            detectedPitch_ = detected;
            float targetHz = computeTargetHz(detected);
            correctedPitch_ = targetHz;
            correctionCents_ = dsp::hzToCents(targetHz, detected) * config_.correctionStrength;

            float semitones = correctionCents_ / 100.0f;
            vocoder_->setPitchShift(semitones);
        } else {
            // No pitch detected: pass through unchanged
            correctionCents_ = 0.0f;
            vocoder_->setPitchShift(0.0f);
        }
    }

    vocoder_->process(input, output, frameSize);
}

float PitchCorrection::computeTargetHz(float detectedHz) const {
    if (config_.scaleSemitones && config_.scaleLength > 0) {
        return dsp::nearestScaleNote(detectedHz, config_.scaleSemitones,
                                     config_.scaleLength, 440.0f);
    }
    // Chromatic snap
    return dsp::nearestScaleNote(detectedHz, CHROMATIC, 12, 440.0f);
}

float PitchCorrection::getDetectedPitch()   const { return detectedPitch_; }
float PitchCorrection::getCorrectedPitch()  const { return correctedPitch_; }
float PitchCorrection::getCorrectionCents() const { return correctionCents_; }

void PitchCorrection::setConfig(const Config& cfg) {
    config_ = cfg;
    detector_->setThreshold(cfg.detectionThreshold);
}

const PitchCorrection::Config& PitchCorrection::getConfig() const { return config_; }
