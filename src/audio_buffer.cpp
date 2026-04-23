#include "audio_buffer.h"
#include <algorithm>
#include <cstring>

AudioBuffer::AudioBuffer() : head_(0), tail_(0), count_(0) {
    memset(buffer_, 0, sizeof(buffer_));
}

void AudioBuffer::write(const float* samples, size_t count) {
    size_t toWrite = std::min(count, space());
    for (size_t i = 0; i < toWrite; ++i) {
        buffer_[head_] = samples[i];
        head_ = (head_ + 1) % BUFFER_SIZE;
    }
    count_ += toWrite;
}

size_t AudioBuffer::read(float* samples, size_t count) {
    size_t toRead = std::min(count, count_);
    for (size_t i = 0; i < toRead; ++i) {
        samples[i] = buffer_[tail_];
        tail_ = (tail_ + 1) % BUFFER_SIZE;
    }
    count_ -= toRead;
    return toRead;
}

size_t AudioBuffer::available() const {
    return count_;
}

size_t AudioBuffer::space() const {
    return BUFFER_SIZE - count_;
}

void AudioBuffer::clear() {
    head_ = 0;
    tail_ = 0;
    count_ = 0;
}

bool AudioBuffer::isEmpty() const {
    return count_ == 0;
}

bool AudioBuffer::isFull() const {
    return count_ == BUFFER_SIZE;
}
