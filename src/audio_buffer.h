#pragma once
#include <cstddef>
#include <cstdint>

class AudioBuffer {
public:
    static constexpr size_t BUFFER_SIZE = 4096;

    AudioBuffer();

    void write(const float* samples, size_t count);
    size_t read(float* samples, size_t count);
    size_t available() const;
    size_t space() const;
    void clear();
    bool isEmpty() const;
    bool isFull() const;

private:
    float buffer_[BUFFER_SIZE];
    size_t head_;
    size_t tail_;
    size_t count_;
};
