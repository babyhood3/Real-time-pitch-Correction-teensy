#pragma once
#include <math.h>

// A4 = 440 Hz, MIDI note 69
// midi  = 69 + 12 * log2(freq / 440)
// freq  = 440 * 2^((midi - 69) / 12)
//
// Uses logf/log(2) instead of log2f for broadest Teensyduino compatibility.

inline float freqToMidi(float freq) {
    if (freq <= 0.0f) return 0.0f;
    return 69.0f + 12.0f * (logf(freq / 440.0f) / logf(2.0f));
}

inline float midiToFreq(float midi) {
    return 440.0f * powf(2.0f, (midi - 69.0f) / 12.0f);
}
