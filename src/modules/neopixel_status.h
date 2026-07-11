#pragma once
#include <stdint.h>
//
void neoInit();

// Intensité de la NeoPixel exprimée en pourcentage (0 = éteinte, 100 = maximum).
// La valeur est persistée en NVS et rechargée au démarrage.
void neoSetBrightnessPercent(uint8_t percent);
uint8_t neoGetBrightnessPercent();

void neoWifiOK();
void neoWifiKO();
void neoWifiLost();
void neoOff();
void neoAlertYellow();
void neoAlertOrange();
void neoAlertRed();