#pragma once
#include "pitch_detection/yin.h"
#include "pitch_shifting/phase_vocoder.h"
#include "audio_buffer.h"

// Top-level pitch correction pipeline.
// Detects the input pitch with YIN, computes the correction in semitones
// needed to snap to the target scale, then applies it via a phase vocoder.
class PitchCorrection {
public:
    struct Config {
        float sampleRate          = 44100.0f;
        float correctionStrength  = 1.0f;  // 0 = none, 1 = full snap
        float detectionThreshold  = YINPitchDetector::DEFAULT_THRESHOLD;
        // Optional quantization scale. If null, snap to nearest semitone (chromatic).
        const float* scaleSemitones = nullptr;
        int          scaleLength    = 0;
    };

    explicit PitchCorrection(const Config& config);
    ~PitchCorrection();

    void process(const float* input, float* output, size_t frameSize);

    float getDetectedPitch()  const;  // Hz
    float getCorrectedPitch() const;  // Hz
    float getCorrectionCents() const; // cents applied

    void setConfig(const Config& config);
    const Config& getConfig() const;

private:
    float computeTargetHz(float detectedHz) const;

    Config             config_;
    YINPitchDetector*  detector_;
    PhaseVocoder*      vocoder_;
    AudioBuffer*       analysisBuffer_;

    float detectedPitch_;
    float correctedPitch_;
    float correctionCents_;

    // Chromatic scale: 12 semitones from C
    static const float CHROMATIC[12];
};
