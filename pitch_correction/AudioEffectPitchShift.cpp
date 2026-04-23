#include "AudioEffectPitchShift.h"
#include <math.h>

AudioEffectPitchShift::AudioEffectPitchShift()
    : AudioStream(1, inputQueueArray),
      _writePos(0),
      _readPosA((float)(PITCH_BUF_SIZE - GRAIN_SIZE)),  // start one grain behind write
      _grainPhase(0),
      _ratio(1.0f)
{
    memset(_buffer, 0, sizeof(_buffer));
    _buildHannWindow();
}

void AudioEffectPitchShift::_buildHannWindow() {
    const float twoPiOverN = 2.0f * 3.14159265f / (float)GRAIN_SIZE;
    for (uint32_t i = 0; i < GRAIN_SIZE; i++) {
        _hannWindow[i] = 0.5f * (1.0f - cosf((float)i * twoPiOverN));
    }
}

void AudioEffectPitchShift::setRatio(float ratio) {
    if (ratio < 0.5f) ratio = 0.5f;
    if (ratio > 2.0f) ratio = 2.0f;
    __disable_irq();
    _ratio = ratio;
    __enable_irq();
}

void AudioEffectPitchShift::_snapReadPointerIfNeeded() {
    // Compute how many samples _readPosA lags behind _writePos in the ring.
    uint32_t writeInt = _writePos & PITCH_BUF_MASK;
    uint32_t readInt  = (uint32_t)_readPosA & PITCH_BUF_MASK;
    uint32_t lag      = (writeInt - readInt + PITCH_BUF_SIZE) & PITCH_BUF_MASK;

    // Safe zone: read pointer must stay between GRAIN_SIZE and
    // (PITCH_BUF_SIZE - GRAIN_SIZE) samples behind the write head.
    if (lag < GRAIN_SIZE) {
        // Read pointer caught up too close — snap back one grain.
        _readPosA = (float)((writeInt - GRAIN_SIZE + PITCH_BUF_SIZE) & PITCH_BUF_MASK);
    } else if (lag > (PITCH_BUF_SIZE - GRAIN_SIZE)) {
        // Read pointer fell too far behind — snap forward.
        _readPosA = (float)((writeInt - (PITCH_BUF_SIZE - GRAIN_SIZE) + PITCH_BUF_SIZE)
                             & PITCH_BUF_MASK);
    }
}

void AudioEffectPitchShift::update() {
    audio_block_t* inBlock = receiveReadOnly(0);
    if (!inBlock) return;

    audio_block_t* outBlock = allocate();
    if (!outBlock) {
        release(inBlock);
        return;
    }

    // Snapshot ratio once — all 128 samples in this block use the same value.
    float ratio = _ratio;

    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {

        // ---- Write new sample into circular buffer ----
        _buffer[_writePos & PITCH_BUF_MASK] = inBlock->data[i];
        _writePos++;

        // ---- Read pointer A (linear interpolation) ----
        uint32_t idxA0 = (uint32_t)_readPosA & PITCH_BUF_MASK;
        uint32_t idxA1 = (idxA0 + 1) & PITCH_BUF_MASK;
        float    fracA  = _readPosA - floorf(_readPosA);
        float    sA0    = (float)_buffer[idxA0];
        float    sA1    = (float)_buffer[idxA1];
        float    sampleA = sA0 + fracA * (sA1 - sA0);

        // ---- Read pointer B — half a grain ahead of A ----
        float    readPosB = _readPosA + (float)GRAIN_HALF;
        if (readPosB >= (float)PITCH_BUF_SIZE) readPosB -= (float)PITCH_BUF_SIZE;
        uint32_t idxB0 = (uint32_t)readPosB & PITCH_BUF_MASK;
        uint32_t idxB1 = (idxB0 + 1) & PITCH_BUF_MASK;
        float    fracB  = readPosB - floorf(readPosB);
        float    sB0    = (float)_buffer[idxB0];
        float    sB1    = (float)_buffer[idxB1];
        float    sampleB = sB0 + fracB * (sB1 - sB0);

        // ---- Hann cross-fade ----
        // winA + winB = 1.0 at all phases (50%-overlap Hann identity).
        float winA = _hannWindow[_grainPhase];
        float winB = _hannWindow[(_grainPhase + GRAIN_HALF) % GRAIN_SIZE];
        float out  = winA * sampleA + winB * sampleB;

        // ---- Clamp and write output ----
        if (out >  32767.0f) out =  32767.0f;
        if (out < -32768.0f) out = -32768.0f;
        outBlock->data[i] = (int16_t)out;

        // ---- Advance read pointer and grain phase ----
        _readPosA += ratio;
        if (_readPosA >= (float)PITCH_BUF_SIZE) _readPosA -= (float)PITCH_BUF_SIZE;
        if (_readPosA <  0.0f)                  _readPosA += (float)PITCH_BUF_SIZE;

        _grainPhase++;
        if (_grainPhase >= GRAIN_SIZE) _grainPhase = 0;
    }

    // ---- Snap check once per block (corrects drift AND float rounding) ----
    _snapReadPointerIfNeeded();

    transmit(outBlock);
    release(outBlock);
    release(inBlock);
}
