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
// Pression (1.52.0) : même principe, un décalage additif en hPa contre une
// référence (station officielle voisine ramenée à la même altitude, baromètre
// étalonné). Une pression absente (0 : BMP280 muet) n'est jamais corrigée.
//
// Altitude (1.53.0) : chaque capteur a la sienne (hub et sonde ne sont pas
// forcément au même étage ni au même endroit). La pression PUBLIÉE par le hub
// (écran, historique, tendances, API, morfAnalytics) est ramenée au NIVEAU DE
// LA MER avec l'altitude du capteur qui l'a mesurée. Règle du parc : le
// composant qui connaît la réalité physique de la mesure la normalise ; les
// consommateurs ne reconstituent jamais l'installation (morfAnalytics ne
// connaît aucune altitude). Déplacer la sonde = changer son altitude ICI, rien
// d'autre. Les deux pressions (intérieur, extérieur) sont alors comparables.
//
// Les bornes limitent l'effet d'une saisie aberrante : un écart supérieur trahit
// un capteur défectueux ou mal placé, qu'une correction ne doit pas masquer.
// -----------------------------------------------------------------------------

#include <math.h>

namespace mhcal {

constexpr float kTempOffsetMax = 10.0f;   // °C
constexpr float kHumOffsetMax  = 20.0f;   // points d'humidité relative
constexpr float kPresOffsetMax = 10.0f;   // hPa
constexpr float kAltitudeMin   = -500.0f; // m (sous le niveau de la mer : rare mais réel)
constexpr float kAltitudeMax   = 5000.0f; // m

struct Offsets {
    float temperature = 0.0f;   // °C, ajouté à la température lue
    float humidity    = 0.0f;   // points de %HR, ajoutés à l'humidité lue
    float pressure    = 0.0f;   // hPa, ajoutés à la pression lue
    float altitude    = 0.0f;   // m, altitude du capteur (réduction au niveau de la mer)
};

inline float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// Ramène un décalage saisi dans ses bornes (une valeur non numérique vaut 0).
inline Offsets sanitize(Offsets o) {
    if (!(o.temperature == o.temperature)) o.temperature = 0.0f;   // NaN
    if (!(o.humidity == o.humidity)) o.humidity = 0.0f;
    if (!(o.pressure == o.pressure)) o.pressure = 0.0f;
    if (!(o.altitude == o.altitude)) o.altitude = 0.0f;
    o.temperature = clampf(o.temperature, -kTempOffsetMax, kTempOffsetMax);
    o.humidity    = clampf(o.humidity, -kHumOffsetMax, kHumOffsetMax);
    o.pressure    = clampf(o.pressure, -kPresOffsetMax, kPresOffsetMax);
    o.altitude    = clampf(o.altitude, kAltitudeMin, kAltitudeMax);
    return o;
}

inline float correctTemperature(float raw, const Offsets& o) {
    return raw + o.temperature;
}

// Une humidité relative reste dans [0, 100] quel que soit le décalage.
inline float correctHumidity(float raw, const Offsets& o) {
    return clampf(raw + o.humidity, 0.0f, 100.0f);
}

// Réduction au niveau de la mer (formule hypsométrique, gradient standard
// 0,0065 K/m), à partir de la pression STATION et de la température du capteur.
// Environ +0,12 hPa par mètre près du sol. Altitude nulle : pression inchangée.
inline float seaLevelPressure(float stationHpa, float tempC, float altitudeM) {
    if (altitudeM > -0.5f && altitudeM < 0.5f) return stationHpa;
    const float denom = tempC + 0.0065f * altitudeM + 273.15f;
    return stationHpa * powf(1.0f - (0.0065f * altitudeM) / denom, -5.257f);
}

// Pression publiée : brute + décalage de calibration (pression station juste),
// puis ramenée au niveau de la mer avec l'altitude du capteur. `tempC` : la
// température corrigée du même capteur (NAN si inconnue : 15 °C, atmosphère
// standard ; l'effet de la température est de l'ordre du centième de hPa à
// quelques dizaines de mètres). 0 signifie « pas de pression » (capteur absent
// ou muet) : rien n'est corrigé, on ne fabrique pas une fausse mesure.
inline float correctPressure(float raw, const Offsets& o, float tempC) {
    if (!(raw > 0.0f)) return raw;
    const float t = (tempC == tempC) ? tempC : 15.0f;
    return seaLevelPressure(raw + o.pressure, t, o.altitude);
}

}  // namespace mhcal
