#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

// -----------------------------------------------------------------------------
// Filtre des valeurs aberrantes par COHÉRENCE TEMPORELLE (logique pure, testée
// sur l'hôte : pio test -e native).
//
// Un point n'est écarté que s'il s'éloigne de SES DEUX voisins alors que ceux-ci
// restent cohérents entre eux : un aller-retour d'une seule mesure (capteur
// perturbé au réveil, trame aberrante). Une vraie variation rapide qui PERSISTE
// (front, orage, chute de pression) est un changement de niveau : ses points ont
// au moins un voisin proche, elle n'est jamais écartée, quelle que soit son
// amplitude. C'est la règle voulue pour la météo : mieux vaut garder un artefact
// que perdre un vrai changement de temps.
//
// Pourquoi pas le filtre médiane / écart médian (MAD) utilisé jusqu'en 1.53.0 :
// après une longue période stable, l'écart médian tombe presque à zéro et le
// seuil se réduit au plancher. Une chute réelle de 10 hPa pendant les 4 dernières
// heures d'une journée calme sortait alors ENTIÈREMENT du min/max 24 h.
// -----------------------------------------------------------------------------

namespace mhfilter {

// `floor` : amplitude en dessous de laquelle un écart n'est jamais aberrant
// (bruit normal du capteur).
inline bool isTemporalOutlier(float prev, float cur, float next, float floor) {
    const float jump = fminf(fabsf(cur - prev), fabsf(cur - next)); // amplitude aller-retour
    const float neighborGap = fabsf(prev - next);                    // cohérence des voisins
    return jump > floor && jump > 3.0f * neighborGap;
}

// Valeurs retenues d'une série (horodatage, valeur), dans l'ordre CHRONOLOGIQUE
// (la série est triée ici : une retransmission arrive après des mesures plus
// récentes). Une valeur absente est ignorée avant tout jugement : NaN, ou <= 0
// quand `positiveOnly` (humidité / pression : 0 = champ non mesuré, jamais une
// vraie mesure). Les deux points de bord, sans deux voisins, sont conservés.
inline std::vector<float> keptValues(std::vector<std::pair<int64_t, float>> series,
                                     float floor, bool positiveOnly) {
    series.erase(std::remove_if(series.begin(), series.end(),
                                [positiveOnly](const std::pair<int64_t, float>& s) {
                                    return !(s.second == s.second)
                                           || (positiveOnly && s.second <= 0.0f);
                                }),
                 series.end());
    std::stable_sort(series.begin(), series.end(),
                     [](const std::pair<int64_t, float>& a, const std::pair<int64_t, float>& b) {
                         return a.first < b.first;
                     });
    std::vector<float> out;
    out.reserve(series.size());
    const size_t n = series.size();
    for (size_t i = 0; i < n; ++i) {
        const bool edge = (i == 0 || i + 1 == n);
        if (edge || !isTemporalOutlier(series[i - 1].second, series[i].second,
                                       series[i + 1].second, floor))
            out.push_back(series[i].second);
    }
    return out;
}

}  // namespace mhfilter
