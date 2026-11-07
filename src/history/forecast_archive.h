#pragma once
// Archive des previsions (etape 9 « prevu vs observe »). Sans lien avec les mesures : un
// fichier JSON plat par jour cible, /history/forecast/AAAA-MM-JJ.json (~1/jour, donc pas
// d'arborescence AAAA/MM). On fige, pour chaque jour cible, la prevision « day-ahead » (celle
// emise la veille) afin de la comparer plus tard aux mesures OUT reellement observees.
#include <ctime>
#include <functional>

#include "fs_port.h"
#include "history_types.h"

namespace mhhist {

#define MH_FORECAST_DIR "/history/forecast"

// Reecrit le fichier du jour cible : il reflete la DERNIERE prevision emise pour ce jour (au fil
// de J-1, on converge vers la prevision day-ahead). L'appelant s'assure que la carte est utilisable.
void addForecast(mhfs::Fs& fs, const ForecastRecord& rec);

// Emet le JSON brut de chaque prevision dont le jour cible tombe dans [from, to] (streaming HTTP,
// sans tout charger en memoire), trie par jour.
void forecastHistoryRaw(mhfs::Fs& fs, time_t from, time_t to, const std::function<void(const char*)>& emit);

} // namespace mhhist
