#include "button.h"
#include "config.h"

void Button::begin(uint8_t pin) {
    _pin = pin;
    pinMode(_pin, INPUT_PULLUP);
    _rawLast = (digitalRead(_pin) == LOW);
    _stable = _rawLast;
    _rawChangedMs = millis();
    // Bouton déjà enfoncé au démarrage : on ignore cet appui (ni court ni long).
    _longFired = _stable;
}

void Button::update() {
    const unsigned long now = millis();
    const bool raw = (digitalRead(_pin) == LOW);

    // Anti-rebond : un changement n'est retenu que s'il reste stable
    // BUTTON_DEBOUNCE_MS (les contacts d'un poussoir rebondissent quelques ms).
    if (raw != _rawLast) {
        _rawLast = raw;
        _rawChangedMs = now;
    }
    if (raw != _stable && (now - _rawChangedMs) >= BUTTON_DEBOUNCE_MS) {
        _stable = raw;
        if (_stable) {
            // Front d'appui.
            _pressedMs = now;
            _longFired = false;
        } else if (!_longFired) {
            // Relâché avant le seuil long : c'est un appui court.
            _shortFlag = true;
        }
    }

    if (_stable && !_longFired && (now - _pressedMs) >= BUTTON_LONG_PRESS_MS) {
        _longFired = true;
        _longFlag = true;
    }
}

bool Button::shortPressed() {
    if (!_shortFlag) return false;
    _shortFlag = false;
    return true;
}

bool Button::longPressed() {
    if (!_longFlag) return false;
    _longFlag = false;
    return true;
}
