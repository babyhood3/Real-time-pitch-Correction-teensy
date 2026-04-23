#include <unity.h>
#include <cmath>
#include "dsp_utils.h"

static constexpr float PI = 3.14159265358979323846f;

void setUp()    {}
void tearDown() {}

void test_semitone_ratio_octave_up() {
    float ratio = dsp::semitoneRatio(12.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, ratio);
}

void test_semitone_ratio_octave_down() {
    float ratio = dsp::semitoneRatio(-12.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, ratio);
}

void test_semitone_ratio_unison() {
    float ratio = dsp::semitoneRatio(0.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 1.0f, ratio);
}

void test_hz_to_midi_a4() {
    float midi = dsp::hzToMidi(440.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 69.0f, midi);
}

void test_midi_to_hz_a4() {
    float hz = dsp::midiToHz(69.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 440.0f, hz);
}

void test_hz_to_midi_and_back_roundtrip() {
    float original = 261.63f;  // C4
    float midi = dsp::hzToMidi(original);
    float back = dsp::midiToHz(midi);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, original, back);
}

void test_hz_to_cents_unison() {
    float cents = dsp::hzToCents(440.0f, 440.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, cents);
}

void test_hz_to_cents_octave() {
    float cents = dsp::hzToCents(880.0f, 440.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 1200.0f, cents);
}

void test_compute_rms_dc() {
    float buf[64];
    for (int i = 0; i < 64; ++i) buf[i] = 0.5f;
    float rms = dsp::computeRMS(buf, 64);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, rms);
}

void test_compute_rms_empty_returns_zero() {
    float rms = dsp::computeRMS(nullptr, 0);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, rms);
}

void test_hann_window_endpoints_near_zero() {
    float buf[64];
    for (int i = 0; i < 64; ++i) buf[i] = 1.0f;
    dsp::applyHannWindow(buf, 64);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, buf[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, buf[63]);
}

void test_hann_window_centre_near_one() {
    float buf[64];
    for (int i = 0; i < 64; ++i) buf[i] = 1.0f;
    dsp::applyHannWindow(buf, 64);
    // Centre sample should be close to 1.0
    TEST_ASSERT_GREATER_THAN_FLOAT(0.95f, buf[32]);
}

// NOT covered yet (see analysis):
//  - normalizeBuffer (zero-signal case, already-normalised signal)
//  - nearestScaleNote with custom scales
//  - hammingWindow shape verification
//  - parabolicPeak correctness
//  - hzToMidi with non-standard A4 reference

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_semitone_ratio_octave_up);
    RUN_TEST(test_semitone_ratio_octave_down);
    RUN_TEST(test_semitone_ratio_unison);
    RUN_TEST(test_hz_to_midi_a4);
    RUN_TEST(test_midi_to_hz_a4);
    RUN_TEST(test_hz_to_midi_and_back_roundtrip);
    RUN_TEST(test_hz_to_cents_unison);
    RUN_TEST(test_hz_to_cents_octave);
    RUN_TEST(test_compute_rms_dc);
    RUN_TEST(test_compute_rms_empty_returns_zero);
    RUN_TEST(test_hann_window_endpoints_near_zero);
    RUN_TEST(test_hann_window_centre_near_one);
    return UNITY_END();
}
