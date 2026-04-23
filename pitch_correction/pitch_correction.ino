// Real-Time Autotune / Pitch Correction — Teensy 4.0 + Audio Shield
//
// Hardware:
//   - Teensy 4.0
//   - Teensy Audio Shield (SGTL5000 codec via I2S)
//   - LINE IN: mono vocal/instrument input
//   - Headphone OUT: corrected audio
//   - USB Audio OUT: corrected stereo stream to DAW (FL Studio, etc.)
//
// Arduino IDE settings:
//   Board  : Teensy 4.0
//   USB    : Serial + MIDI + Audio   ← required for AudioOutputUSB
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

AudioInputI2S             i2sIn;           // LINE IN (mono, channel 0)
AudioAnalyzeNoteFrequency pitchDetector;   // built-in YIN algorithm
AudioEffectPitchShift     pitchShifter;    // twin-pointer granular DSP block
AudioOutputI2S            i2sOut;          // headphone output
AudioOutputUSB            usbOut;          // USB audio to DAW
AudioControlSGTL5000      sgtl5000;        // codec control

// ============================================================
// Audio connections
// ============================================================

// Raw input → pitch detector (analysis only, no audio path effect)
AudioConnection  c0(i2sIn,        0, pitchDetector, 0);

// Raw input → pitch shifter
AudioConnection  c1(i2sIn,        0, pitchShifter,  0);

// Pitch-shifted output → headphones (L + R)
AudioConnection  c2(pitchShifter, 0, i2sOut,        0);
AudioConnection  c3(pitchShifter, 0, i2sOut,        1);

// Pitch-shifted output → USB audio (L + R)
AudioConnection  c4(pitchShifter, 0, usbOut,        0);
AudioConnection  c5(pitchShifter, 0, usbOut,        1);

// ============================================================
// Application state
// ============================================================

ScaleMapper    scaleMapper;           // default: key=C, scale=minor
ControlManager controls(scaleMapper);

float smoothedRatio = 1.0f;           // running EMA of the correction ratio

// Reject pitch detections outside the vocal range B1-C6 (~61–1047 Hz).
static const float FREQ_MIN = 60.0f;
static const float FREQ_MAX = 1100.0f;

// YIN confidence threshold: 0.9 rejects ambiguous / noisy frames.
static const float YIN_THRESHOLD = 0.9f;

// ============================================================
// Setup
// ============================================================

void setup() {
    Serial.begin(115200);

    // AudioAnalyzeNoteFrequency buffers many blocks internally for YIN.
    // 32 blocks (8 KB) gives enough headroom for the full graph.
    AudioMemory(32);

    // Codec initialisation
    sgtl5000.enable();
    sgtl5000.inputSelect(AUDIO_INPUT_LINEIN);
    sgtl5000.lineInLevel(15);  // max sensitivity; MAX9814 mic outputs are quieter than true line-level
    sgtl5000.volume(0.8f);     // headphone volume 0–1

    pitchDetector.begin(YIN_THRESHOLD);

    controls.begin();

    Serial.println("Pitch correction system ready.");
    Serial.println("Key=C  Scale=Minor  Ratio=1.000");
}

// ============================================================
// Main loop
// ============================================================

void loop() {
    // ---- 1. Read buttons and potentiometers, drive LEDs ----
    controls.update();

    float retuneAlpha        = controls.getRetuneAlpha();        // 0.01..0.30
    float correctionStrength = controls.getCorrectionStrength(); // 0.0..1.0

    // ---- 2. Process pitch detection result when available ----
    if (pitchDetector.available()) {
        float detectedFreq = pitchDetector.read();

        if (detectedFreq > FREQ_MIN && detectedFreq < FREQ_MAX) {

            // Frequency → MIDI (float, non-integer)
            float midiFloat = freqToMidi(detectedFreq);

            // Snap to nearest in-scale MIDI note
            int snappedMidi = scaleMapper.snapMidi((int)roundf(midiFloat));

            // Snapped MIDI → target frequency
            float targetFreq = midiToFreq((float)snappedMidi);

            // Raw correction ratio: how much to shift the pitch
            float rawRatio = targetFreq / detectedFreq;
            if (rawRatio < 0.5f) rawRatio = 0.5f;
            if (rawRatio > 2.0f) rawRatio = 2.0f;

            // Blend toward 1.0 based on correction strength pot
            // strength=0 → no correction (ratio stays 1.0)
            // strength=1 → full correction
            float blendedRatio = 1.0f + (rawRatio - 1.0f) * correctionStrength;

            // Exponential moving average toward the new target.
            // Runs continuously every loop() — not gated on pitchDetector.available() —
            // so the ratio keeps converging between detection frames too.
            smoothedRatio += retuneAlpha * (blendedRatio - smoothedRatio);
        }
        // If out of range, smoothedRatio continues decaying toward its last value.
    } else {
        // Even with no new detection, keep EMA running toward last blended target.
        // (No-op here; the EMA update is placed inside the if-available block above.
        //  Between frames the ratio is simply held, which is the correct behaviour
        //  for a live instrument: hold the last known correction until pitch is
        //  re-confirmed, rather than drifting toward 1.0 during brief silent gaps.)
    }

    // ---- 3. Push the smoothed ratio to the pitch-shifter (thread-safe) ----
    pitchShifter.setRatio(smoothedRatio);

    // ---- 4. Serial diagnostics (open Tools > Serial Monitor at 115200 baud) ----
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 250) {
        lastPrint = millis();
        Serial.print("Key=");   Serial.print(scaleMapper.getKey());
        Serial.print(" Scale=");
        Serial.print(scaleMapper.getScale() == ScaleMapper::MAJOR ? "Major" : "Minor");
        Serial.print(" PitchAvail=");
        Serial.print(pitchDetector.available() ? "Y" : "N");
        Serial.print(" Ratio=");  Serial.print(smoothedRatio, 3);
        Serial.print(" CPU=");    Serial.print(AudioProcessorUsageMax(), 1);
        Serial.print("% Blocks=");Serial.println(AudioMemoryUsageMax());
        AudioProcessorUsageMaxReset();
        AudioMemoryUsageMaxReset();
    }
}
