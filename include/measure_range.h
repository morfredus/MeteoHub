#pragma once
#include <cmath>

// -----------------------------------------------------------------------------
// Plages de plausibilite des mesures archivees (logique pure, testee sur l'hote).
//
// Memes bornes que le contrôle des capteurs INTERIEURS (main.cpp) : une mesure hors
// plage, ou absente (NaN), n'est jamais archivee comme une vraie valeur. Une
// humidite ou une pression a 0.0 n'est pas une mesure : c'est un capteur muet.
// Les anciens fichiers .bin contenant ces 0.0 restent lisibles tels quels ; les
// agregations les ignorent (voir keptValues et queryRangeImpl).
// -----------------------------------------------------------------------------
namespace mhrange {

constexpr float kHumMin = 0.0f, kHumMax = 100.0f;      // % HR
constexpr float kPresMin = 800.0f, kPresMax = 1200.0f; // hPa

// Humidite exploitable : mesuree (drapeau de la sonde), finie, dans la plage ET
// strictement positive (0 % = capteur muet, jamais un air reellement sec).
inline bool humidityUsable(bool measured, float h) {
    return measured && std::isfinite(h) && h > kHumMin && h <= kHumMax;
}
inline bool pressureUsable(bool measured, float p) {
    return measured && std::isfinite(p) && p >= kPresMin && p <= kPresMax;
}

// Valeur a ARCHIVER : la mesure telle quelle si exploitable, sinon NaN.
inline float humidityToStore(bool measured, float h) {
    return humidityUsable(measured, h) ? h : NAN;
}
inline float pressureToStore(bool measured, float p) {
    return pressureUsable(measured, p) ? p : NAN;
}

// Valeur a AGREGER / afficher issue d'un fichier (anciens 0.0 compris) : vrai si
// le nombre est une vraie mesure.
inline bool storedHumidityOk(float h) { return std::isfinite(h) && h > kHumMin && h <= kHumMax; }
inline bool storedPressureOk(float p) { return std::isfinite(p) && p >= kPresMin && p <= kPresMax; }

}  // namespace mhrange
