#pragma once
#include <cstddef>

namespace dsp {

void applyHannWindow(float* buffer, size_t size);
void applyHammingWindow(float* buffer, size_t size);

// Parabolic interpolation around a peak at index `peak`.
float parabolicPeak(const float* buffer, size_t peak);

// Convert between pitch representations.
float hzToMidi(float hz, float a4Hz = 440.0f);
float midiToHz(float midi, float a4Hz = 440.0f);
float hzToCents(float hz, float referenceHz);
float semitoneRatio(float semitones);

// Find the nearest MIDI note (or scale degree) to the given frequency.
float nearestScaleNote(float hz, const float* scaleSemitonesFromC, int scaleLen, float a4Hz = 440.0f);

float computeRMS(const float* buffer, size_t size);
void normalizeBuffer(float* buffer, size_t size);

} // namespace dsp
