#pragma once
#include <cstddef>
#include <complex>

// Pitch-shifting via a phase vocoder (overlap-add, FFT-based).
// Shifts pitch by a fixed number of semitones without changing tempo.
class PhaseVocoder {
public:
    static constexpr size_t FFT_SIZE  = 2048;
    static constexpr size_t HOP_SIZE  = 512;

    explicit PhaseVocoder(float sampleRate);
    ~PhaseVocoder();

    // Shift in semitones; positive = up, negative = down. Range: ±24.
    void  setPitchShift(float semitones);
    float getPitchShift() const;

    // Process one block. `frameSize` must be <= HOP_SIZE.
    // Returns false if the internal buffer lacks enough data to produce output.
    bool process(const float* input, float* output, size_t frameSize);

    void reset();

private:
    // In-place Cooley-Tukey FFT; inverse flag selects IFFT.
    static void fft(std::complex<float>* data, size_t n, bool inverse);

    void analyzeFrame();
    void synthesizeFrame();

    float sampleRate_;
    float pitchShift_;     // semitones
    float shiftFactor_;    // linear ratio derived from pitchShift_

    float* inputFifo_;
    float* outputAccum_;
    float* analysisWindow_;
    float* synthesisWindow_;

    float* lastAnalysisPhase_;
    float* lastSynthesisPhase_;

    std::complex<float>* spectrum_;

    size_t inputWritePos_;
    size_t outputReadPos_;

    static constexpr float PI     = 3.14159265358979323846f;
    static constexpr float TWO_PI = 2.0f * PI;
};
