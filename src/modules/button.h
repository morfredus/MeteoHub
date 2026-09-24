#pragma once
#include <Arduino.h>

// -----------------------------------------------------------------------------
// Button : bouton poussoir unique (vers GND, pull-up interne), qui distingue
// l'appui COURT de l'appui LONG.
//
// - Appui court : émis au RELÂCHEMENT, si le bouton a été tenu moins de
//   BUTTON_LONG_PRESS_MS.
// - Appui long : émis DÈS que la durée est atteinte, bouton encore enfoncé
//   (l'utilisateur voit la réaction sans avoir à deviner quand lâcher). Le
//   relâchement qui suit n'émet alors rien : pas de « court » parasite.
//
// Non bloquant : update() à chaque tour de loop(), puis on consomme les
// événements avec shortPressed() / longPressed() (chacun vrai une seule fois).
// -----------------------------------------------------------------------------
class Button {
public:
    void begin(uint8_t pin);
    void update();

    bool shortPressed();
    bool longPressed();

private:
    uint8_t _pin = 0;
    bool _rawLast = false;          // dernière lecture brute (pour l'anti-rebond)
    bool _stable = false;           // état stable après anti-rebond (true = enfoncé)
    unsigned long _rawChangedMs = 0;
    unsigned long _pressedMs = 0;   // instant du début de l'appui stable
    bool _longFired = false;        // appui long déjà émis pour cet appui

    bool _shortFlag = false;
    bool _longFlag = false;
};
