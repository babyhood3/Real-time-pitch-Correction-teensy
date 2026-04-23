#include <unity.h>
#include <cmath>
#include "pitch_detection/yin.h"

static constexpr float SAMPLE_RATE = 44100.0f;
static constexpr float PI          = 3.14159265358979323846f;

// Generate a pure sine wave at `freq` Hz into `buf`.
static void makeSine(float* buf, size_t len, float freq, float sr = SAMPLE_RATE) {
    for (size_t i = 0; i < len; ++i) {
        buf[i] = sinf(2.0f * PI * freq * static_cast<float>(i) / sr);
    }
}

void setUp()    {}
void tearDown() {}

void test_detect_440hz() {
    YINPitchDetector yin(SAMPLE_RATE);
    float buf[YINPitchDetector::BUFFER_SIZE];
    makeSine(buf, YINPitchDetector::BUFFER_SIZE, 440.0f);

    float pitch = yin.detect(buf, YINPitchDetector::BUFFER_SIZE);
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 440.0f, pitch);
    TEST_ASSERT_TRUE(yin.isPitched());
}

void test_detect_220hz() {
    YINPitchDetector yin(SAMPLE_RATE);
    float buf[YINPitchDetector::BUFFER_SIZE];
    makeSine(buf, YINPitchDetector::BUFFER_SIZE, 220.0f);

    float pitch = yin.detect(buf, YINPitchDetector::BUFFER_SIZE);
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 220.0f, pitch);
}

void test_detect_880hz() {
    YINPitchDetector yin(SAMPLE_RATE);
    float buf[YINPitchDetector::BUFFER_SIZE];
    makeSine(buf, YINPitchDetector::BUFFER_SIZE, 880.0f);

    float pitch = yin.detect(buf, YINPitchDetector::BUFFER_SIZE);
    TEST_ASSERT_FLOAT_WITHIN(5.0f, 880.0f, pitch);
}

void test_silence_returns_no_pitch() {
    YINPitchDetector yin(SAMPLE_RATE);
    float buf[YINPitchDetector::BUFFER_SIZE] = {};  // all zeros

    float pitch = yin.detect(buf, YINPitchDetector::BUFFER_SIZE);
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, pitch);
    TEST_ASSERT_FALSE(yin.isPitched());
}

void test_insufficient_buffer_returns_no_pitch() {
    YINPitchDetector yin(SAMPLE_RATE);
    float buf[512];
    makeSine(buf, 512, 440.0f);

    float pitch = yin.detect(buf, 512);  // < BUFFER_SIZE
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, pitch);
}

void test_confidence_high_for_pure_tone() {
    YINPitchDetector yin(SAMPLE_RATE);
    float buf[YINPitchDetector::BUFFER_SIZE];
    makeSine(buf, YINPitchDetector::BUFFER_SIZE, 440.0f);

    yin.detect(buf, YINPitchDetector::BUFFER_SIZE);
    TEST_ASSERT_GREATER_THAN_FLOAT(0.8f, yin.getConfidence());
}

void test_threshold_setter() {
    YINPitchDetector yin(SAMPLE_RATE);
    yin.setThreshold(0.25f);
    TEST_ASSERT_EQUAL_FLOAT(0.25f, yin.getThreshold());
}

// NOT covered yet (see analysis):
//  - Noisy / real-world signals
//  - Signals at the exact MIN_FREQ / MAX_FREQ boundaries
//  - Multi-harmonic signals (e.g. sawtooth wave)
//  - Threshold sensitivity sweep
//  - Different sample rates

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_detect_440hz);
    RUN_TEST(test_detect_220hz);
    RUN_TEST(test_detect_880hz);
    RUN_TEST(test_silence_returns_no_pitch);
    RUN_TEST(test_insufficient_buffer_returns_no_pitch);
    RUN_TEST(test_confidence_high_for_pure_tone);
    RUN_TEST(test_threshold_setter);
    return UNITY_END();
}
