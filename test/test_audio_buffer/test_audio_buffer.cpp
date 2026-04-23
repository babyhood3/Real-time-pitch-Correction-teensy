#include <unity.h>
#include "audio_buffer.h"

static AudioBuffer buf;

void setUp() {
    buf.clear();
}

void tearDown() {}

void test_initial_state() {
    TEST_ASSERT_TRUE(buf.isEmpty());
    TEST_ASSERT_FALSE(buf.isFull());
    TEST_ASSERT_EQUAL_UINT(0, buf.available());
    TEST_ASSERT_EQUAL_UINT(AudioBuffer::BUFFER_SIZE, buf.space());
}

void test_write_and_read_small_block() {
    float in[8]  = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};
    float out[8] = {};

    buf.write(in, 8);
    TEST_ASSERT_EQUAL_UINT(8, buf.available());

    size_t read = buf.read(out, 8);
    TEST_ASSERT_EQUAL_UINT(8, read);
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(in, out, 8);
    TEST_ASSERT_TRUE(buf.isEmpty());
}

void test_available_and_space_tracking() {
    float samples[100] = {};
    buf.write(samples, 100);
    TEST_ASSERT_EQUAL_UINT(100, buf.available());
    TEST_ASSERT_EQUAL_UINT(AudioBuffer::BUFFER_SIZE - 100, buf.space());
}

void test_partial_read() {
    float in[10]  = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f,
                     0.6f, 0.7f, 0.8f, 0.9f, 1.0f};
    float out[5]  = {};
    buf.write(in, 10);
    buf.read(out, 5);
    TEST_ASSERT_EQUAL_FLOAT(0.1f, out[0]);
    TEST_ASSERT_EQUAL_FLOAT(0.5f, out[4]);
    TEST_ASSERT_EQUAL_UINT(5, buf.available());
}

void test_clear_resets_state() {
    float samples[64] = {};
    buf.write(samples, 64);
    buf.clear();
    TEST_ASSERT_TRUE(buf.isEmpty());
    TEST_ASSERT_EQUAL_UINT(0, buf.available());
    TEST_ASSERT_EQUAL_UINT(AudioBuffer::BUFFER_SIZE, buf.space());
}

void test_write_capped_at_capacity() {
    // Writing more than BUFFER_SIZE should silently drop overflow.
    float big[AudioBuffer::BUFFER_SIZE + 64] = {};
    buf.write(big, AudioBuffer::BUFFER_SIZE + 64);
    TEST_ASSERT_TRUE(buf.isFull());
    TEST_ASSERT_EQUAL_UINT(AudioBuffer::BUFFER_SIZE, buf.available());
}

void test_read_returns_zero_when_empty() {
    float out[8] = {};
    size_t read = buf.read(out, 8);
    TEST_ASSERT_EQUAL_UINT(0, read);
}

// NOTE: wrap-around behaviour (head crossing the end of the internal array)
// is NOT tested here. See coverage analysis.

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_initial_state);
    RUN_TEST(test_write_and_read_small_block);
    RUN_TEST(test_available_and_space_tracking);
    RUN_TEST(test_partial_read);
    RUN_TEST(test_clear_resets_state);
    RUN_TEST(test_write_capped_at_capacity);
    RUN_TEST(test_read_returns_zero_when_empty);
    return UNITY_END();
}
