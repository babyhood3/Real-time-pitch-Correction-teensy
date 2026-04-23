#pragma once
#include <Arduino.h>
#include <AudioStream.h>

// ---- Buffer and grain constants ----
// Buffer is power-of-2 so modulo reduces to a bitmask.
static const uint32_t PITCH_BUF_SIZE = 4096;              // samples, 8 KB at int16_t
static const uint32_t PITCH_BUF_MASK = PITCH_BUF_SIZE - 1;
static const uint32_t GRAIN_SIZE     = 930;               // ~21.1 ms at 44100 Hz
static const uint32_t GRAIN_HALF     = GRAIN_SIZE / 2;    // 465 samples

// ---- Algorithm: twin-pointer granular pitch shifter ----
//
// Two read pointers (A and B) advance through a circular input buffer at
// `ratio` samples per real sample, while the write pointer advances at 1.
// A and B are permanently offset by GRAIN_HALF.
// Each pointer is cross-faded with a Hann window 180° out of phase.
// Because hann(x) + hann(x + N/2) = 1, the two overlapping grains reconstruct
// the signal perfectly, yielding pitch shift without duration change and
// without audible clicks at any ratio.
//
// Ratio range: [0.5, 2.0]  (one octave down to one octave up)
//
// Thread safety: setRatio() may be called from loop(); update() runs in the
// audio interrupt.  A one-instruction critical section (disable/enable IRQ)
// around the volatile float write is sufficient on Cortex-M7.

class AudioEffectPitchShift : public AudioStream {
public:
    AudioEffectPitchShift();

    // Call from loop() with the smoothed correction ratio.
    // 1.0 = no pitch change.  Thread-safe.
    void setRatio(float ratio);

    // Called by the Teensy Audio Library every 128 samples.
    virtual void update() override;

private:
    audio_block_t* inputQueueArray[1];

    int16_t  _buffer[PITCH_BUF_SIZE];   // circular input buffer
    float    _hannWindow[GRAIN_SIZE];    // precomputed Hann coefficients

    uint32_t _writePos;     // monotonically increasing, masked on access
    float    _readPosA;     // fractional index; pointer B = _readPosA + GRAIN_HALF
    uint32_t _grainPhase;   // 0 .. GRAIN_SIZE-1, shared window phase counter

    volatile float _ratio;  // current pitch ratio, updated via setRatio()

    void _buildHannWindow();
    void _snapReadPointerIfNeeded();
};
