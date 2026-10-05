#pragma once
// Serie temporelle de la batterie de la sonde : lecture d'une ligne CSV et
// agregation en un nombre BORNE de points (logique pure, testee sur l'hote).
//
// Pourquoi agreger : sur 90 jours la sonde produit ~26 000 mesures. La page n'en
// affiche qu'une courbe de quelques centaines de points, et l'ESP32 ne doit ni
// charger tout cela en RAM ni le renvoyer au navigateur. Chaque mesure tombe dans
// un « seau » de duree fixe (moyenne du seau) : l'ordre des lignes dans le fichier
// n'a aucune importance, ce qui compte car une retransmission est ecrite apres des
// mesures plus recentes.
#include <stdint.h>
#include <stdio.h>
#include <vector>

namespace mhbat {

struct Point {
    uint32_t ts;   // heure moyenne des mesures du seau
    float volts;
    float pct;
};

// « ts,tension,pourcentage ». false si la ligne n'est pas une mesure (en-tete,
// ligne tronquee par une coupure, vide) : elle est simplement ignoree.
inline bool parseLine(const char* line, uint32_t& ts, float& volts, float& pct) {
    unsigned long t = 0;
    float v = 0, p = 0;
    if (sscanf(line, "%lu,%f,%f", &t, &v, &p) != 3) return false;
    ts = (uint32_t)t;
    volts = v;
    pct = p;
    return true;
}

class Aggregator {
public:
    // [from, to] en secondes Unix, au plus `maxPoints` points en sortie. Un seau ne
    // descend pas sous 300 s (le rythme des mesures) : sur 24 h on garde chaque mesure.
    Aggregator(uint32_t from, uint32_t to, uint32_t maxPoints = 400)
        : _from(from), _to(to) {
        const uint32_t span = (to > from) ? (to - from) : 1;
        uint32_t sec = (span + maxPoints - 1) / maxPoints;
        _bucketSec = (sec < 300) ? 300 : sec;
        _buckets.resize(span / _bucketSec + 1);
    }

    void add(uint32_t ts, float volts, float pct) {
        if (ts < _from || ts > _to) return;
        Bucket& b = _buckets[(ts - _from) / _bucketSec];
        b.tsSum += ts;
        b.vSum += volts;
        b.pSum += pct;
        b.n++;
    }

    uint32_t bucketSeconds() const { return _bucketSec; }

    // Points non vides, par ordre chronologique.
    std::vector<Point> points() const {
        std::vector<Point> out;
        for (const Bucket& b : _buckets) {
            if (b.n == 0) continue;
            out.push_back({(uint32_t)(b.tsSum / b.n + 0.5), (float)(b.vSum / b.n),
                           (float)(b.pSum / b.n)});
        }
        return out;
    }

private:
    struct Bucket { double tsSum = 0, vSum = 0, pSum = 0; uint32_t n = 0; };
    uint32_t _from, _to, _bucketSec;
    std::vector<Bucket> _buckets;
};

} // namespace mhbat
