#pragma once
// Requetes sur l'historique, en fonctions PURES au-dessus de deux sources : les fichiers
// journaliers de la carte SD (DayStore) et les derniers releves en RAM (RamSeries). Une meme
// fonction sert l'IN et l'OUT : seul le couple (magasin, serie RAM) change.
#include <cstdint>
#include <ctime>
#include <functional>
#include <mutex>
#include <vector>

#include "day_store.h"
#include "history_types.h"

namespace mhhist {

// Derniers releves en RAM d'un flux, dans l'ordre d'arrivee. L'implementation prend le verrou
// des tampons (loop() les alimente, async_tcp les lit) pendant toute la duree du parcours.
class RamSeries {
public:
    virtual ~RamSeries() {}
    virtual void forEach(const std::function<void(int64_t ts, float t, float h, float p)>& cb) const = 0;
};

// Serie RAM adossee a un RecentRing (IN ou OUT) et au verrou partage du gestionnaire.
template <typename Ring>
class RingSeries : public RamSeries {
public:
    RingSeries(const Ring& ring, std::mutex& ramMutex) : _ring(ring), _mutex(ramMutex) {}
    void forEach(const std::function<void(int64_t, float, float, float)>& cb) const override {
        std::lock_guard<std::mutex> lock(_mutex);
        for (const auto& r : _ring.items()) cb(static_cast<int64_t>(r.timestamp), r.t, r.h, r.p);
    }
private:
    const Ring& _ring;
    std::mutex& _mutex;
};

// Agrege les mesures sur la plage absolue [from, to] (secondes Unix) en tranches de interval_s
// secondes (au plus 800 tranches : l'intervalle est elargi sinon). Les fichiers SD sont lus en
// priorite, completes par la RAM pour la portion pas encore ecrite sur SD (ou toute la plage
// sans carte). Une tranche sans donnee est tout de meme emise (valid=false), pour conserver
// l'alignement des index entre deux periodes comparees. Un filtre anti-aberrations (pic ou creux
// isole entre deux voisins coherents) ecarte les mesures ponctuelles.
std::vector<HistoryPoint> queryRange(const DayStore& store, const RamSeries& ram,
                                     time_t from, time_t to, long interval_s);

// Statistiques sur les dernieres 24 h de la RAM (fenetre bornee a now-24h des que l'horloge est
// fiable). `now` est injecte pour rendre le calcul testable.
Stats24h computeStats24h(const RamSeries& ram, time_t now);

// Tendances 1 h / 12 h / 24 h (RAM, puis SD si la RAM ne remonte pas assez loin) et 48 h (SD
// seulement, available_48h dit si une mesure proche de J-48h a ete retrouvee).
MeteoTrend computeTrend(const DayStore& store, const RamSeries& ram);

// Synthese d'une plage a partir des .stats journaliers (aucune relecture des mesures). Pour un
// jour a un bord partiellement couvert, les statistiques du jour entier sont utilisees
// (approximation assumee au profit d'un affichage quasi instantane).
RangeSynthesis querySynthesis(const DayStore& store, time_t from, time_t to);

} // namespace mhhist
