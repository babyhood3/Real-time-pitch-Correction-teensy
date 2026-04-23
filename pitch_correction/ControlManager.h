#pragma once
#include <Arduino.h>
#include "ScaleMapper.h"

// ---- Pin definitions (per hardware spec) ----
static const int PIN_POT_RETUNE_SPEED  = 14;   // A0
static const int PIN_POT_CORRECTION    = 15;   // A1
static const int PIN_BTN_KEY_UP        = 2;
static const int PIN_BTN_KEY_DOWN      = 3;
static const int PIN_BTN_SCALE_TOGGLE  = 16;
static const int PIN_LED0              = 4;
static const int PIN_LED1              = 5;
static const int PIN_LED2              = 6;
static const int PIN_LED3              = 9;

static const unsigned long DEBOUNCE_MS = 30;

// ControlManager reads buttons, pots, and drives LEDs each loop() iteration.
// It owns the ScaleMapper reference and updates key/scale state on button press.

class ControlManager {
public:
    explicit ControlManager(ScaleMapper& mapper);

    void begin();
    void update();   // call every loop() iteration

    // Smoothed readings — safe to call any time after update()
    float getRetuneAlpha()        const { return _retuneAlpha; }       // 0.01..0.30
    float getCorrectionStrength() const { return _correctionStrength; } // 0.0..1.0

private:
    ScaleMapper& _mapper;

    float _retuneAlpha;
    float _correctionStrength;

    // IIR smoothing state for potentiometers
    float _smoothedRetune;
    float _smoothedCorrection;

    struct ButtonState {
        int           pin;
        bool          lastReading;
        bool          stableState;
        unsigned long lastChangeMs;
    };

    ButtonState _btnKeyUp;
    ButtonState _btnKeyDown;
    ButtonState _btnScaleToggle;

    // Returns true on a confirmed press (rising edge after debounce).
    bool _processButton(ButtonState& btn);

    void _updateLEDs();

    // ADC 0-1023 → alpha in [0.01, 0.30] with an exponential curve
    // so the pot feels perceptually uniform (slow glide at low end, snap at high).
    static float _adcToAlpha(int adc);

    // ADC 0-1023 → linear [0.0, 1.0]
    static float _adcToLinear(int adc);
};
