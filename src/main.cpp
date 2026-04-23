#ifndef NATIVE_BUILD

#include <Arduino.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include "pitch_correction.h"

// Teensy Audio library objects
AudioInputI2S            audioIn;
AudioOutputI2S           audioOut;
AudioControlSGTL5000     audioShield;

// Raw audio blocks from the Teensy Audio library use int16 samples.
// We bridge to our float-based pipeline via a custom AudioStream node.

static constexpr float SAMPLE_RATE = 44100.0f;
static constexpr int   BLOCK_SIZE  = 128; // Teensy Audio block size

static PitchCorrection* gCorrector = nullptr;

// Simple bridge: reads from audioIn, processes, writes to audioOut.
class PitchCorrectionStream : public AudioStream {
public:
    PitchCorrectionStream() : AudioStream(1, inputQueueArray_) {}

    void update() override {
        audio_block_t* in = receiveReadOnly(0);
        if (!in) return;

        audio_block_t* out = allocate();
        if (!out) {
            release(in);
            return;
        }

        // Convert int16 -> float, process, convert back
        float inputF[BLOCK_SIZE], outputF[BLOCK_SIZE];
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            inputF[i] = in->data[i] / 32768.0f;
        }

        if (gCorrector) {
            gCorrector->process(inputF, outputF, BLOCK_SIZE);
        } else {
            memcpy(outputF, inputF, sizeof(float) * BLOCK_SIZE);
        }

        for (int i = 0; i < BLOCK_SIZE; ++i) {
            float clamped = outputF[i] < -1.0f ? -1.0f : (outputF[i] > 1.0f ? 1.0f : outputF[i]);
            out->data[i] = static_cast<int16_t>(clamped * 32767.0f);
        }

        transmit(out);
        release(in);
        release(out);
    }

private:
    audio_block_t* inputQueueArray_[1];
};

static PitchCorrectionStream pitchStream;
AudioConnection patchIn(audioIn,     0, pitchStream, 0);
AudioConnection patchOut(pitchStream, 0, audioOut,    0);

void setup() {
    AudioMemory(20);
    audioShield.enable();
    audioShield.inputSelect(AUDIO_INPUT_MIC);
    audioShield.micGain(36);
    audioShield.volume(0.8f);

    PitchCorrection::Config cfg;
    cfg.sampleRate         = SAMPLE_RATE;
    cfg.correctionStrength = 1.0f;
    gCorrector = new PitchCorrection(cfg);
}

void loop() {
    // Audio processing happens in interrupt context via update().
    // Main loop can handle UI / MIDI / serial commands here.
    delay(10);
}

#endif // NATIVE_BUILD
