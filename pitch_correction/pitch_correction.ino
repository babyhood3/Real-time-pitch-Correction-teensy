// Real-Time Autotune / Pitch Correction — Teensy 4.0 + Audio Shield
//
// Arduino IDE settings:
//   Board  : Teensy 4.0
//   USB    : Serial + MIDI + Audio
//   CPU    : 600 MHz

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

#include "AudioEffectPitchShift.h"
#include "ScaleMapper.h"
#include "ControlManager.h"
#include "PitchUtils.h"

// ============================================================
// Audio graph objects
// ============================================================

AudioInputI2S             i2s1;           // mic from audio shield LINE IN
AudioInputUSB             usbIn;          // audio coming from computer / FL Studio
AudioAnalyzeNoteFrequency pitchDetector;  // built-in YIN pitch detection
AudioEffectPitchShift     pitchShifter;   // twin-pointer granular pitch correction
AudioMixer4               mixerL;         // left output mixer
AudioMixer4               mixerR;         // right output mixer
AudioOutputI2S            i2s2;           // audio shield headphone/line out
AudioOutputUSB            usb1;           // send pitch-corrected mic to FL Studio
AudioControlSGTL5000      sgtl5000;

// ============================================================
// Audio connections
// ============================================================

// Mic → pitch detector (analysis only, no effect on audio path)
AudioConnection c0(i2s1,         0, pitchDetector, 0);

// Mic → pitch shifter
AudioConnection c1(i2s1,         0, pitchShifter,  0);

// Pitch-corrected mic → headphone mixers
AudioConnection c2(pitchShifter, 0, mixerL,        0);
AudioConnection c3(pitchShifter, 0, mixerR,        0);

// Pitch-corrected mic → USB to FL Studio
AudioConnection c4(pitchShifter, 0, usb1,          0);
AudioConnection c5(pitchShifter, 0, usb1,          1);

// FL Studio playback → headphone mixers (hear backing track in headphones)
AudioConnection c6(usbIn,        0, mixerL,        1);
AudioConnection c7(usbIn,        1, mixerR,        1);

// Mixers → headphone output
AudioConnection c8(mixerL,       0, i2s2,          0);
AudioConnection c9(mixerR,       0, i2s2,          1);

// ============================================================
// Application state
// ============================================================

ScaleMapper    scaleMapper;
ControlManager controls(scaleMapper);

float smoothedRatio = 1.0f;

static const float FREQ_MIN      = 60.0f;    // ~B1, lowest vocal note
static const float FREQ_MAX      = 1100.0f;  // ~C6, highest vocal note
static const float YIN_THRESHOLD = 0.9f;     // pitch detector confidence

// ============================================================
// Setup
// ============================================================

void setup() {
    Serial.begin(115200);
    AudioMemory(32);

    sgtl5000.enable();
    sgtl5000.inputSelect(AUDIO_INPUT_LINEIN);
    sgtl5000.lineInLevel(10);   // 10 works well for MAX9814 mic board
    sgtl5000.volume(0.5f);      // headphone volume — matches your original

    // Mixer gains — same as your original working setup
    mixerL.gain(0, 0.7f);   // pitch-corrected mic → left headphone
    mixerR.gain(0, 0.7f);   // pitch-corrected mic → right headphone
    mixerL.gain(1, 0.7f);   // FL Studio playback  → left headphone
    mixerR.gain(1, 0.7f);   // FL Studio playback  → right headphone

    pitchDetector.begin(YIN_THRESHOLD);
    controls.begin();

    Serial.println("Pitch correction ready. Key=C Scale=Minor");
}

// ============================================================
// Main loop
// ============================================================

void loop() {
    controls.update();

    float retuneAlpha        = controls.getRetuneAlpha();
    float correctionStrength = controls.getCorrectionStrength();

    if (pitchDetector.available()) {
        float detectedFreq = pitchDetector.read();

        if (detectedFreq > FREQ_MIN && detectedFreq < FREQ_MAX) {
            float midiFloat   = freqToMidi(detectedFreq);
            int   snappedMidi = scaleMapper.snapMidi((int)roundf(midiFloat));
            float targetFreq  = midiToFreq((float)snappedMidi);

            float rawRatio = targetFreq / detectedFreq;
            if (rawRatio < 0.5f) rawRatio = 0.5f;
            if (rawRatio > 2.0f) rawRatio = 2.0f;

            float blendedRatio = 1.0f + (rawRatio - 1.0f) * correctionStrength;
            smoothedRatio += retuneAlpha * (blendedRatio - smoothedRatio);
        }
    }

    pitchShifter.setRatio(smoothedRatio);

    // Serial diagnostics — open Tools > Serial Monitor at 115200 baud
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 250) {
        lastPrint = millis();
        Serial.print("Key=");  Serial.print(scaleMapper.getKey());
        Serial.print(" Scale=");
        Serial.print(scaleMapper.getScale() == ScaleMapper::MAJOR ? "Major" : "Minor");
        Serial.print(" Ratio="); Serial.print(smoothedRatio, 3);
        Serial.print(" CPU=");   Serial.print(AudioProcessorUsageMax(), 1);
        Serial.print("% Blocks="); Serial.println(AudioMemoryUsageMax());
        AudioProcessorUsageMaxReset();
        AudioMemoryUsageMaxReset();
    }
}
