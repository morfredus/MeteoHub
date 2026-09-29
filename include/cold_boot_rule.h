#pragma once
#include <cmath>
#include <cstdint>

// -----------------------------------------------------------------------------
// Première mesure après un DÉMARRAGE À FROID de la sonde (flash, passage par
// l'USB, remise sous tension) : garder ou marquer ? Logique pure, testée sur
// l'hôte (pio test -e native).
//
// Une carte restée éveillée a pu chauffer ses capteurs : cette mesure est
// suspecte, pas forcément fausse. Règle (Fred, 2026-09-29) :
//   - elle est CONFORME à la dernière mesure archivée, et celle-ci date de
//     10 min au plus : mesure normale, archivée comme les autres ;
//   - sinon (écart, ou pas de mesure récente pour en juger) : archivée AVEC la
//     marque kFlagColdBoot. Le hub ne l'affiche pas ; morfAnalytics l'écarte des
//     analyses comme un point suspect, et elle se réintègre à la main.
//
// Tolérances = planchers de bruit du filtre temporel de l'historique (1 °C,
// 6 points d'humidité, 1 hPa) : « conforme » veut dire « dans le bruit normal
// d'une mesure à la suivante ».
// -----------------------------------------------------------------------------

namespace mhcold {

// Marque d'historique (champ `flags` des enregistrements, format v2).
constexpr uint32_t kFlagColdBoot = 1u;

constexpr int64_t kWindowS = 600;      // dernière mesure : 10 min au plus
constexpr float   kTolTemp = 1.0f;     // °C
constexpr float   kTolHum  = 6.0f;     // points de %HR
constexpr float   kTolPres = 1.0f;     // hPa

struct Reading {
    int64_t ts = 0;      // heure de mesure (epoch s)
    float   t = 0.0f;
    float   h = 0.0f;    // 0 = non mesurée
    float   p = 0.0f;    // 0 = non mesurée
};

inline bool conformsToPrevious(bool havePrevious, const Reading& prev, const Reading& cur) {
    if (!havePrevious) return false;                 // rien pour en juger
    const int64_t age = cur.ts - prev.ts;
    if (age <= 0 || age > kWindowS) return false;    // trop ancienne (ou incohérente)
    if (fabsf(cur.t - prev.t) > kTolTemp) return false;
    // Humidité / pression : jugées seulement si mesurées des deux côtés.
    if (prev.h > 0.0f && cur.h > 0.0f && fabsf(cur.h - prev.h) > kTolHum) return false;
    if (prev.p > 0.0f && cur.p > 0.0f && fabsf(cur.p - prev.p) > kTolPres) return false;
    return true;
}

}  // namespace mhcold
