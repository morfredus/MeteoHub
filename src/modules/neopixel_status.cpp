#include "board_config.h"        // doit être en premier
#include "neopixel_status.h"
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>

static Adafruit_NeoPixel neo(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

#define NEO_DEFAULT_BRIGHTNESS 40
static uint8_t neo_brightness = NEO_DEFAULT_BRIGHTNESS;

void neoInit() {
    // Charge la luminosité persistée en NVS (namespace "meteohub").
    Preferences prefs;
    if (prefs.begin("meteohub", true)) { // lecture seule
        neo_brightness = prefs.getUChar("led_bright", NEO_DEFAULT_BRIGHTNESS);
        prefs.end();
    }
    neo.begin();
    neo.setBrightness(neo_brightness);
    neo.show();
}

void neoSetBrightness(uint8_t brightness) {
    neo_brightness = brightness;
    neo.setBrightness(neo_brightness);
    neo.show(); // réapplique immédiatement la couleur courante à la nouvelle luminosité

    Preferences prefs;
    if (prefs.begin("meteohub", false)) { // lecture/écriture
        prefs.putUChar("led_bright", neo_brightness);
        prefs.end();
    }
}

uint8_t neoGetBrightness() {
    return neo_brightness;
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