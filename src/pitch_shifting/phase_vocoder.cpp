#include "phase_vocoder.h"
#include <cmath>
#include <cstring>
#include <algorithm>

PhaseVocoder::PhaseVocoder(float sampleRate)
    : sampleRate_(sampleRate), pitchShift_(0.0f), shiftFactor_(1.0f),
      inputWritePos_(0), outputReadPos_(0) {

    inputFifo_          = new float[FFT_SIZE * 2]();
    outputAccum_        = new float[FFT_SIZE * 2]();
    analysisWindow_     = new float[FFT_SIZE]();
    synthesisWindow_    = new float[FFT_SIZE]();
    lastAnalysisPhase_  = new float[FFT_SIZE]();
    lastSynthesisPhase_ = new float[FFT_SIZE]();
    spectrum_           = new std::complex<float>[FFT_SIZE]();

    // Hann windows
    for (size_t i = 0; i < FFT_SIZE; ++i) {
        float w = 0.5f * (1.0f - cosf(TWO_PI * i / (FFT_SIZE - 1)));
        analysisWindow_[i]  = w;
        synthesisWindow_[i] = w;
    }
}

PhaseVocoder::~PhaseVocoder() {
    delete[] inputFifo_;
    delete[] outputAccum_;
    delete[] analysisWindow_;
    delete[] synthesisWindow_;
    delete[] lastAnalysisPhase_;
    delete[] lastSynthesisPhase_;
    delete[] spectrum_;
}

void PhaseVocoder::setPitchShift(float semitones) {
    pitchShift_  = semitones;
    shiftFactor_ = powf(2.0f, semitones / 12.0f);
}

float PhaseVocoder::getPitchShift() const { return pitchShift_; }

void PhaseVocoder::fft(std::complex<float>* data, size_t n, bool inverse) {
    // Bit-reversal permutation
    for (size_t i = 1, j = 0; i < n; ++i) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) std::swap(data[i], data[j]);
    }
    // Cooley-Tukey
    for (size_t len = 2; len <= n; len <<= 1) {
        float ang = TWO_PI / static_cast<float>(len) * (inverse ? -1.0f : 1.0f);
        std::complex<float> wlen(cosf(ang), sinf(ang));
        for (size_t i = 0; i < n; i += len) {
            std::complex<float> w(1.0f, 0.0f);
            for (size_t j = 0; j < len / 2; ++j) {
                std::complex<float> u = data[i + j];
                std::complex<float> v = data[i + j + len / 2] * w;
                data[i + j]           = u + v;
                data[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }
    if (inverse) {
        float inv = 1.0f / static_cast<float>(n);
        for (size_t i = 0; i < n; ++i) data[i] *= inv;
    }
}

void PhaseVocoder::analyzeFrame() {
    // Copy windowed frame into spectrum
    size_t frameStart = (inputWritePos_ + FFT_SIZE) % (FFT_SIZE * 2);
    for (size_t i = 0; i < FFT_SIZE; ++i) {
        size_t idx = (frameStart + i) % (FFT_SIZE * 2);
        spectrum_[i] = std::complex<float>(inputFifo_[idx] * analysisWindow_[i], 0.0f);
    }
    fft(spectrum_, FFT_SIZE, false);

    // Compute instantaneous frequency and resynthesize phases
    float hopAngleFactor = TWO_PI * static_cast<float>(HOP_SIZE) / static_cast<float>(FFT_SIZE);
    for (size_t k = 0; k < FFT_SIZE; ++k) {
        float mag   = std::abs(spectrum_[k]);
        float phase = std::arg(spectrum_[k]);

        float phaseDiff = phase - lastAnalysisPhase_[k] - static_cast<float>(k) * hopAngleFactor;
        // Wrap to [-pi, pi]
        phaseDiff -= TWO_PI * roundf(phaseDiff / TWO_PI);

        float instFreq = static_cast<float>(k) * hopAngleFactor + phaseDiff;
        float newPhase = lastSynthesisPhase_[k] + instFreq * shiftFactor_;

        lastAnalysisPhase_[k]  = phase;
        lastSynthesisPhase_[k] = newPhase;

        spectrum_[k] = std::polar(mag, newPhase);
    }
}

void PhaseVocoder::synthesizeFrame() {
    fft(spectrum_, FFT_SIZE, true);

    // Overlap-add
    size_t outPos = outputReadPos_;
    for (size_t i = 0; i < FFT_SIZE; ++i) {
        outputAccum_[(outPos + i) % (FFT_SIZE * 2)] +=
            spectrum_[i].real() * synthesisWindow_[i];
    }
}

bool PhaseVocoder::process(const float* input, float* output, size_t frameSize) {
    // Push input into FIFO
    for (size_t i = 0; i < frameSize; ++i) {
        inputFifo_[inputWritePos_] = input[i];
        inputWritePos_ = (inputWritePos_ + 1) % (FFT_SIZE * 2);
    }

    // Process when a full hop has accumulated
    static size_t samplesUntilProcess = HOP_SIZE;
    bool produced = false;

    if (frameSize >= samplesUntilProcess) {
        analyzeFrame();
        synthesizeFrame();
        samplesUntilProcess = HOP_SIZE;
        produced = true;
    } else {
        samplesUntilProcess -= frameSize;
    }

    // Read output
    for (size_t i = 0; i < frameSize; ++i) {
        output[i] = outputAccum_[outputReadPos_];
        outputAccum_[outputReadPos_] = 0.0f;
        outputReadPos_ = (outputReadPos_ + 1) % (FFT_SIZE * 2);
    }

    return produced;
}

void PhaseVocoder::reset() {
    memset(inputFifo_,          0, sizeof(float) * FFT_SIZE * 2);
    memset(outputAccum_,        0, sizeof(float) * FFT_SIZE * 2);
    memset(lastAnalysisPhase_,  0, sizeof(float) * FFT_SIZE);
    memset(lastSynthesisPhase_, 0, sizeof(float) * FFT_SIZE);
    inputWritePos_ = 0;
    outputReadPos_ = 0;
}
