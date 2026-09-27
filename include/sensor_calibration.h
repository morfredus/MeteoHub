#pragma once

// -----------------------------------------------------------------------------
// Calibration des capteurs (AHT20 du hub, sonde extérieure) : deux corrections
// additives et indépendantes par capteur, réglées par l'utilisateur contre une
// référence (thermostat, hygromètre étalonné). Logique pure (ni Arduino, ni NVS) : testée sur l'hôte
// (pio test -e native). Persistance : modules/calibration_store ; application :
// SensorManager (intérieur) et EspNowReceiver (sonde extérieure).
//
// Pourquoi deux décalages indépendants, et pas un recalcul de l'humidité à partir
// de la température corrigée : ce recalcul suppose que TOUT l'écart vient d'un air
// réellement plus chaud autour du capteur (chaleur de la carte), qui fait aussi
// lire une humidité relative trop basse. Rien ne garantit que ce soit la seule
// cause (biais propre du capteur, placement de la référence). Constat de terrain du
// 27/09/2026 : 26,8 °C / 60 % au hub contre 24,0 °C / 67 % au thermostat.
// L'humidité basse confirme un air réchauffé, mais le recalcul physique donnerait
// ~71 % et non 67 % : il surcorrige. Deux décalages mesurés (-2,8 °C, +7 points)
// collent exactement à la référence. Chaque grandeur a donc sa propre correction.
//
// Les bornes limitent l'effet d'une saisie aberrante : un écart supérieur trahit
// un capteur défectueux ou mal placé, qu'une correction ne doit pas masquer.
// -----------------------------------------------------------------------------

namespace mhcal {

constexpr float kTempOffsetMax = 10.0f;   // °C
constexpr float kHumOffsetMax  = 20.0f;   // points d'humidité relative

struct Offsets {
    float temperature = 0.0f;   // °C, ajouté à la température lue
    float humidity    = 0.0f;   // points de %HR, ajoutés à l'humidité lue
};

inline float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// Ramène un décalage saisi dans ses bornes (une valeur non numérique vaut 0).
inline Offsets sanitize(Offsets o) {
    if (!(o.temperature == o.temperature)) o.temperature = 0.0f;   // NaN
    if (!(o.humidity == o.humidity)) o.humidity = 0.0f;
    o.temperature = clampf(o.temperature, -kTempOffsetMax, kTempOffsetMax);
    o.humidity    = clampf(o.humidity, -kHumOffsetMax, kHumOffsetMax);
    return o;
}

inline float correctTemperature(float raw, const Offsets& o) {
    return raw + o.temperature;
}

// Une humidité relative reste dans [0, 100] quel que soit le décalage.
inline float correctHumidity(float raw, const Offsets& o) {
    return clampf(raw + o.humidity, 0.0f, 100.0f);
}

}  // namespace mhcal
