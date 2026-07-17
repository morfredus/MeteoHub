#include "board_config.h"        // doit être en premier
#include "neopixel_status.h"
#include <Adafruit_NeoPixel.h>

static Adafruit_NeoPixel neo(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

void neoInit() {
    neo.begin();
    neo.setBrightness(40);
    neo.show();
}

void neoWifiOK() {
    neo.setPixelColor(0, neo.Color(0, 150, 0)); // vert
    neo.show();
}

void neoWifiKO() {
    neo.setPixelColor(0, neo.Color(150, 0, 0)); // rouge
    neo.show();
}

void neoWifiLost() {
    neo.setPixelColor(0, neo.Color(150, 0, 255)); // violet
    neo.show();
}

void neoOff() {
    neo.setPixelColor(0, neo.Color(0, 0, 0)); // éteint
    neo.show();
}

void neoAlertYellow() {
    neo.setPixelColor(0, neo.Color(255, 255, 0)); // jaune très vif
    neo.show();
}

void neoAlertOrange() {
    neo.setPixelColor(0, neo.Color(255, 100, 0)); // orange soutenu
    neo.show();
}

void neoAlertRed() {
    neo.setPixelColor(0, neo.Color(255, 0, 0)); // rouge pur
    neo.show();
}

//