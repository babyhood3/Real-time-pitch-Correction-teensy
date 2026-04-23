#include "ControlManager.h"
#include <math.h>

ControlManager::ControlManager(ScaleMapper& mapper)
    : _mapper(mapper),
      _retuneAlpha(0.05f),
      _correctionStrength(1.0f),
      _smoothedRetune(512.0f),
      _smoothedCorrection(1023.0f)
{
    _btnKeyUp        = { PIN_BTN_KEY_UP,       false, false, 0 };
    _btnKeyDown      = { PIN_BTN_KEY_DOWN,      false, false, 0 };
    _btnScaleToggle  = { PIN_BTN_SCALE_TOGGLE,  false, false, 0 };
}

void ControlManager::begin() {
    pinMode(PIN_BTN_KEY_UP,       INPUT_PULLUP);
    pinMode(PIN_BTN_KEY_DOWN,     INPUT_PULLUP);
    pinMode(PIN_BTN_SCALE_TOGGLE, INPUT_PULLUP);

    pinMode(PIN_LED0, OUTPUT);
    pinMode(PIN_LED1, OUTPUT);
    pinMode(PIN_LED2, OUTPUT);
    pinMode(PIN_LED3, OUTPUT);

    _updateLEDs();
}

void ControlManager::update() {
    // ---- Buttons ----
    if (_processButton(_btnKeyUp)) {
        _mapper.setKey((_mapper.getKey() + 1) % 12);
    }
    if (_processButton(_btnKeyDown)) {
        _mapper.setKey((_mapper.getKey() + 11) % 12);  // +11 mod 12 = -1 mod 12
    }
    if (_processButton(_btnScaleToggle)) {
        _mapper.toggleScale();
    }

    // ---- Potentiometers (IIR smoothing to filter ADC noise) ----
    const float POT_ALPHA = 0.1f;
    _smoothedRetune     += POT_ALPHA * (analogRead(PIN_POT_RETUNE_SPEED) - _smoothedRetune);
    _smoothedCorrection += POT_ALPHA * (analogRead(PIN_POT_CORRECTION)   - _smoothedCorrection);

    _retuneAlpha        = _adcToAlpha((int)_smoothedRetune);
    _correctionStrength = _adcToLinear((int)_smoothedCorrection);

    // ---- LEDs ----
    _updateLEDs();
}

bool ControlManager::_processButton(ButtonState& btn) {
    // Buttons are active-low (INPUT_PULLUP): pressed = LOW = true here
    bool reading = (digitalRead(btn.pin) == LOW);

    if (reading != btn.lastReading) {
        btn.lastChangeMs = millis();
        btn.lastReading  = reading;
    }

    if ((millis() - btn.lastChangeMs) >= DEBOUNCE_MS) {
        if (reading != btn.stableState) {
            btn.stableState = reading;
            if (reading) return true;   // rising edge = confirmed press
        }
    }
    return false;
}

void ControlManager::_updateLEDs() {
    uint8_t key = _mapper.getKey();
    digitalWrite(PIN_LED0, (key & 0x01) ? HIGH : LOW);
    digitalWrite(PIN_LED1, (key & 0x02) ? HIGH : LOW);
    digitalWrite(PIN_LED2, (key & 0x04) ? HIGH : LOW);
    digitalWrite(PIN_LED3, (key & 0x08) ? HIGH : LOW);
}

float ControlManager::_adcToAlpha(int adc) {
    // Exponential map: adc=0 → alpha=0.01, adc=1023 → alpha=0.30
    // alpha = exp(lerp(ln(0.01), ln(0.30), t))  where t = adc/1023
    float t = (float)adc / 1023.0f;
    float logAlpha = -4.605f + t * 3.401f;   // ln(0.01)=-4.605, ln(0.30)=-1.204
    return expf(logAlpha);
}

float ControlManager::_adcToLinear(int adc) {
    return (float)adc / 1023.0f;
}
