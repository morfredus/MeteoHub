#pragma once

// -----------------------------------------------------------------------------
// Calibration du capteur intérieur (AHT20) : deux corrections additives,
// indépendantes, réglées par l'utilisateur contre une référence (thermostat,
// hygromètre étalonné). Logique pure (ni Arduino, ni NVS) : testée sur l'hôte
// (pio test -e native) ; la persistance et l'application vivent dans SensorManager.
//
// Pourquoi deux décalages indépendants, et pas un recalcul de l'humidité à partir
// de la température corrigée : ce recalcul n'est juste que si le capteur baigne
// dans un air RÉELLEMENT plus chaud (auto-échauffement de la carte). Dans ce cas,
// il lit aussi une humidité relative trop basse, et les deux erreurs sont liées.
// Constat de terrain du 27/09/2026 : 26,8 °C / 67 % au hub contre 24,0 °C / 67 %
// au thermostat. L'humidité concorde : l'air autour du capteur est le même, c'est
// la LECTURE de température qui est décalée. Recalculer l'humidité l'aurait
// faussée (67 % -> ~78 %). Chaque grandeur a donc sa propre correction.
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
