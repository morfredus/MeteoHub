#pragma once
#include <Arduino.h>
//
void neoInit();
void neoWifiOK();
void neoWifiKO();
void neoWifiLost();
void neoOff();
void neoAlertYellow();
void neoAlertOrange();
void neoAlertRed();

// Luminosité de la NeoLED (0-255), réglable et persistée en NVS.
void neoSetBrightness(uint8_t brightness);
uint8_t neoGetBrightness();
