#include "board_config.h"        // doit être en premier
#include "neopixel_status.h"
#include <Adafruit_NeoPixel.h>
#include <Preferences.h>

static Adafruit_NeoPixel neo(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

static Preferences neoPrefs;
static const char* NEO_PREFS_NAMESPACE = "meteohub";
static const char* NEO_PREFS_KEY = "neo_bright";
static const uint8_t NEO_DEFAULT_PERCENT = 16; // ~40/255, comportement historique

static uint8_t g_brightnessPercent = NEO_DEFAULT_PERCENT;

// Convertit un pourcentage (0..100) vers l'échelle 0..255 attendue par la librairie.
static uint8_t percentToRaw(uint8_t percent) {
    if (percent > 100) percent = 100;
    return static_cast<uint8_t>((static_cast<uint16_t>(percent) * 255) / 100);
}

void neoInit() {
    neoPrefs.begin(NEO_PREFS_NAMESPACE, false);
    g_brightnessPercent = neoPrefs.getUChar(NEO_PREFS_KEY, NEO_DEFAULT_PERCENT);
    if (g_brightnessPercent > 100) g_brightnessPercent = 100;

    neo.begin();
    neo.setBrightness(percentToRaw(g_brightnessPercent));
    neo.show();
}

uint8_t neoGetBrightnessPercent() {
    return g_brightnessPercent;
}

void neoSetBrightnessPercent(uint8_t percent) {
    if (percent > 100) percent = 100;
    g_brightnessPercent = percent;
    neoPrefs.putUChar(NEO_PREFS_KEY, percent);
    neo.setBrightness(percentToRaw(percent));
    neo.show();
    // La couleur réelle est ré-appliquée par la boucle principale (toutes les 500 ms),
    // ce qui restitue une teinte propre à la nouvelle intensité.
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