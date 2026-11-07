#include "history_query.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include "measure_range.h"
#include "temporal_filter.h"

namespace mhhist {

using mhfilter::isTemporalOutlier; // definition unique : include/temporal_filter.h

// Detection de valeurs aberrantes par coherence temporelle (memes seuils que le client) : un
// point s'ecartant fortement de SES DEUX voisins alors que ceux-ci restent coherents entre eux
// est considere comme un pic/creux ponctuel. Les planchers de bruit evitent d'ecarter les
// micro-variations normales.
static const float kOutlierFloorT = 1.0f;
static const float kOutlierFloorH = 6.0f;
static const float kOutlierFloorP = 1.0f;

Stats24h computeStats24h(const RamSeries& ram, time_t now) {
    Stats24h stats;
    stats.count = 0;

    const time_t cutoff = (now > 86400) ? (now - 86400) : 0;

    // Series HORODATEES : le filtre juge chaque point contre ses voisins dans le temps (voir
    // temporal_filter.h). Seul un aller-retour isole est ecarte ; une vraie variation rapide
    // qui persiste reste dans min / max / moyenne.
    std::vector<std::pair<int64_t, float>> temps, hums, pres;
    ram.forEach([&](int64_t ts, float t, float h, float p) {
        if (ts < static_cast<int64_t>(cutoff)) return;
        temps.push_back({ts, t});
        hums.push_back({ts, h});
        pres.push_back({ts, p});
        stats.count++;
    });
    for (float v : mhfilter::keptValues(temps, kOutlierFloorT, false)) stats.temp.add(v);
    for (float v : mhfilter::keptValues(hums, kOutlierFloorH, true)) stats.hum.add(v);
    for (float v : mhfilter::keptValues(pres, kOutlierFloorP, true)) stats.pres.add(v);

    return stats;
}

std::vector<HistoryPoint> queryRange(const DayStore& store, const RamSeries& ram,
                                     time_t from, time_t to, long interval_s) {
    std::vector<HistoryPoint> out;
    if (to <= from) return out;
    if (interval_s < 1) interval_s = 1;

    // Nombre de tranches, borne pour limiter la consommation memoire sur l'ESP32. Si la plage
    // est trop large pour l'intervalle demande, on elargit l'intervalle.
    const long MAX_BUCKETS = 800;
    long span = static_cast<long>(to - from);
    long nbuckets = (span + interval_s - 1) / interval_s;
    if (nbuckets > MAX_BUCKETS) {
        interval_s = (span + MAX_BUCKETS - 1) / MAX_BUCKETS;
        nbuckets = (span + interval_s - 1) / interval_s;
    }
    if (nbuckets < 1) nbuckets = 1;

    // Accumulateurs par grandeur : une grandeur peut etre ecartee d'une mesure (valeur
    // aberrante) sans invalider les deux autres.
    struct Acc { double t = 0, h = 0, p = 0; int nt = 0, nh = 0, np = 0; };
    std::vector<Acc> accs(static_cast<size_t>(nbuckets));

    const long from_l = static_cast<long>(from);
    const long to_l = static_cast<long>(to);
    auto accT = [&](long ts, float v) {
        long idx = (ts - from_l) / interval_s;
        if (ts < from_l || ts > to_l || idx < 0 || idx >= nbuckets) return;
        accs[static_cast<size_t>(idx)].t += v; accs[static_cast<size_t>(idx)].nt++;
    };
    auto accH = [&](long ts, float v) {
        if (!mhrange::storedHumidityOk(v)) return; // NaN ou ancien 0.0 : pas une mesure
        long idx = (ts - from_l) / interval_s;
        if (ts < from_l || ts > to_l || idx < 0 || idx >= nbuckets) return;
        accs[static_cast<size_t>(idx)].h += v; accs[static_cast<size_t>(idx)].nh++;
    };
    auto accP = [&](long ts, float v) {
        if (!mhrange::storedPressureOk(v)) return; // NaN ou ancien 0.0 : pas une mesure
        long idx = (ts - from_l) / interval_s;
        if (ts < from_l || ts > to_l || idx < 0 || idx >= nbuckets) return;
        accs[static_cast<size_t>(idx)].p += v; accs[static_cast<size_t>(idx)].np++;
    };

    // Filtre anti-aberrations en flux : fenetre glissante de 3 mesures. La mesure centrale est
    // testee par grandeur contre ses voisines avant d'etre agregee ; les mesures de bord
    // (premiere/derniere, sans deux voisines) sont conservees.
    int wn = 0; long wts[3]; float wt[3], wh[3], wp[3];
    auto flushMiddle = [&]() {
        if (!isTemporalOutlier(wt[0], wt[1], wt[2], kOutlierFloorT)) accT(wts[1], wt[1]);
        if (!isTemporalOutlier(wh[0], wh[1], wh[2], kOutlierFloorH)) accH(wts[1], wh[1]);
        if (!isTemporalOutlier(wp[0], wp[1], wp[2], kOutlierFloorP)) accP(wts[1], wp[1]);
    };
    auto feed = [&](long ts, float t, float h, float p) {
        if (wn == 0) {
            wts[0] = ts; wt[0] = t; wh[0] = h; wp[0] = p; wn = 1;
            accT(ts, t); accH(ts, h); accP(ts, p); // bord gauche : conserve tel quel
        } else if (wn == 1) {
            wts[1] = ts; wt[1] = t; wh[1] = h; wp[1] = p; wn = 2;
        } else if (wn == 2) {
            wts[2] = ts; wt[2] = t; wh[2] = h; wp[2] = p; wn = 3;
            flushMiddle();
        } else {
            wts[0] = wts[1]; wt[0] = wt[1]; wh[0] = wh[1]; wp[0] = wp[1];
            wts[1] = wts[2]; wt[1] = wt[2]; wh[1] = wh[2]; wp[1] = wp[2];
            wts[2] = ts; wt[2] = t; wh[2] = h; wp[2] = p;
            flushMiddle();
        }
    };
    auto flushEnd = [&]() {
        // La derniere mesure (bord droit) n'a jamais ete centrale : agregee telle quelle.
        if (wn >= 2) { int last = wn - 1; accT(wts[last], wt[last]); accH(wts[last], wh[last]); accP(wts[last], wp[last]); }
    };

    long lastSdTs = LONG_MIN; // horodatage le plus recent trouve sur SD

    // 1) Source SD : lit les fichiers binaires journaliers couvrant [from, to]. Grace aux
    //    enregistrements de taille fixe et chronologiques (IN), on saute directement
    //    (dichotomie) au premier enregistrement >= from, puis on lit sequentiellement jusqu'a
    //    depasser to - sans relire tout le fichier. Repli sur l'ancien CSV plat (IN seulement)
    //    si un .bin n'existe pas encore.
    //
    // On monte via ensureMounted() (comme /api/history/raw et /days) et NON via isAvailable() :
    // cette derniere sonde SD.cardType() et, sur un faux CARD_NONE transitoire (frequent sous
    // charge web), demonte la carte et reste en cooldown de reconnexion. La lecture retombait
    // alors sur la RAM seule -> historique IN vide et OUT reduit a quelques trames recentes,
    // alors que la donnee est bien sur la carte (l'export brut, lui, la lit sans souci).
    Fs& fs = store.fs();
    const DayStorePolicy& pol = store.policy();
    if (fs.ensureMounted()) {
        char prevBin[64] = {0};
        size_t day_iter = 0;
        for (time_t cursor = from; cursor <= to + 86400; cursor += 86400) {
            MH_YIELD_EVERY(day_iter, 8);
            day_iter++;
            struct tm tinfo;
            if (!localtime_r(&cursor, &tinfo)) continue;
            char binPath[64], statsPath[64];
            store.dayPaths(tinfo, binPath, statsPath, sizeof(binPath));
            if (strcmp(binPath, prevBin) == 0) continue; // evite un double parcours (bord DST)
            strncpy(prevBin, binPath, sizeof(prevBin));
            prevBin[sizeof(prevBin) - 1] = '\0';

            if (fs.exists(binPath)) {
                FilePtr f = fs.open(binPath, mhfs::Mode::Read);
                if (!f) continue;
                const BinLayout L = probeBin(*f);
                BinRecord br;

                if (!pol.chronological) {
                    // OUT : fichier non chronologique (retransmissions en fin) -> lecture
                    // complete du jour puis tri, avant la fenetre glissante du filtre.
                    std::vector<BinRecord> day;
                    collectSortedDay(*f, L, from_l, to_l, day);
                    f->close();
                    for (const BinRecord& rec : day) {
                        feed(static_cast<long>(rec.ts), rec.t, rec.h, rec.p);
                        if (static_cast<long>(rec.ts) > lastSdTs) lastSdTs = static_cast<long>(rec.ts);
                    }
                    continue;
                }

                // Dichotomie : premier index dont ts >= from.
                size_t lo = 0, hi = L.nrec;
                while (lo < hi) {
                    size_t mid = (lo + hi) / 2;
                    if (!readBinRecordAt(*f, L, mid, br)) break;
                    if (static_cast<long>(br.ts) < from_l) lo = mid + 1; else hi = mid;
                }

                forEachBinRecordFrom(*f, L, lo, [&](const BinRecord& rec, uint32_t flags) -> bool {
                    const long ts = static_cast<long>(rec.ts);
                    if (ts > to_l) return false; // tries : les suivants aussi
                    // Point marque (demarrage a froid non conforme) : archive pour
                    // morfAnalytics, jamais affiche par le hub.
                    if (flags) return true;
                    feed(ts, rec.t, rec.h, rec.p);
                    if (ts > lastSdTs) lastSdTs = ts;
                    return true;
                });
                f->close();
                continue;
            }

            // Le repli CSV plat n'existe que pour l'historique IN (legacy).
            if (!pol.legacyCsvDir) continue;

            // Repli : ancien fichier CSV plat <dir>/AAAA-MM-JJ.csv
            char csvPath[48];
            const std::string fmt = std::string(pol.legacyCsvDir) + "/%Y-%m-%d.csv";
            strftime(csvPath, sizeof(csvPath), fmt.c_str(), &tinfo);
            if (!fs.exists(csvPath)) continue;
            FilePtr f = fs.open(csvPath, mhfs::Mode::Read);
            if (!f) continue;
            bool first_line = true;
            size_t line_count = 0;
            mhfs::LineReader lines(*f);
            std::string line;
            while (lines.next(line)) {
                MH_YIELD_EVERY(line_count, 32);
                line_count++;
                if (first_line) { first_line = false; continue; }
                if (line.empty()) continue;
                long ts = 0; float t = 0, h = 0, p = 0;
                if (sscanf(line.c_str(), "%ld,%f,%f,%f", &ts, &t, &h, &p) == 4) {
                    feed(ts, t, h, p);
                    if (ts > lastSdTs) lastSdTs = ts;
                }
            }
            f->close();
        }
    }

    // 2) Complete avec l'historique RAM pour la portion non encore ecrite sur SD (ou la
    //    totalite de la plage si aucune carte SD n'est disponible).
    size_t ram_iteration = 0;
    ram.forEach([&](int64_t ts64, float t, float h, float p) {
        MH_YIELD_EVERY(ram_iteration, 256);
        ram_iteration++;
        long ts = static_cast<long>(ts64);
        if (ts <= lastSdTs) return;
        feed(ts, t, h, p);
    });
    flushEnd(); // agrege la derniere mesure retenue dans la fenetre glissante

    // 3) Serialise les tranches (moyenne par grandeur ; grandeur vide => null).
    out.reserve(static_cast<size_t>(nbuckets));
    for (long i = 0; i < nbuckets; ++i) {
        const Acc& a = accs[static_cast<size_t>(i)];
        HistoryPoint pt;
        pt.t = from + static_cast<time_t>(i * interval_s + interval_s / 2); // centre de tranche
        pt.tvalid = a.nt > 0; pt.hvalid = a.nh > 0; pt.pvalid = a.np > 0;
        pt.temp = pt.tvalid ? static_cast<float>(a.t / a.nt) : 0.0f;
        pt.hum  = pt.hvalid ? static_cast<float>(a.h / a.nh) : 0.0f;
        pt.pres = pt.pvalid ? static_cast<float>(a.p / a.np) : 0.0f;
        pt.valid = pt.tvalid || pt.hvalid || pt.pvalid;
        if (!pt.valid) {
            pt.temp = pt.hum = pt.pres = 0.0f;
            pt.valid = false;
        }
        out.push_back(pt);
    }
    return out;
}

RangeSynthesis querySynthesis(const DayStore& store, time_t from, time_t to) {
    RangeSynthesis r;
    if (to <= from || !store.fs().ensureMounted()) return r; // lecture : ne pas sonder cardType()

    double t_sum = 0, h_sum = 0, p_sum = 0;
    char prevBin[64] = {0};
    size_t day_iter = 0;

    // Agrege les .stats journaliers de la plage (aucune relecture des mesures).
    for (time_t cursor = from; cursor <= to + 86400; cursor += 86400) {
        MH_YIELD_EVERY(day_iter, 8);
        day_iter++;
        struct tm tinfo;
        if (!localtime_r(&cursor, &tinfo)) continue;
        char binPath[64], statsPath[64];
        store.dayPaths(tinfo, binPath, statsPath, sizeof(binPath));
        if (strcmp(binPath, prevBin) == 0) continue;
        strncpy(prevBin, binPath, sizeof(prevBin));
        prevBin[sizeof(prevBin) - 1] = '\0';

        DayStats s;
        if (!store.readDayStats(cursor, s) || s.count == 0) continue;

        if (!r.valid) {
            r.t_min = s.t_min; r.t_max = s.t_max;
            r.h_min = s.h_min; r.h_max = s.h_max;
            r.p_min = s.p_min; r.p_max = s.p_max;
            r.t_first = s.t_first; r.h_first = s.h_first; r.p_first = s.p_first;
            r.valid = true;
        } else {
            if (s.t_min < r.t_min) r.t_min = s.t_min;
            if (s.t_max > r.t_max) r.t_max = s.t_max;
            if (s.h_min < r.h_min) r.h_min = s.h_min;
            if (s.h_max > r.h_max) r.h_max = s.h_max;
            if (s.p_min < r.p_min) r.p_min = s.p_min;
            if (s.p_max > r.p_max) r.p_max = s.p_max;
        }
        // La derniere valeur correspond au dernier jour rencontre dans la plage.
        r.t_last = s.t_last; r.h_last = s.h_last; r.p_last = s.p_last;
        r.count += s.count;
        t_sum += s.t_sum; h_sum += s.h_sum; p_sum += s.p_sum;
    }

    if (r.valid && r.count > 0) {
        r.t_avg = static_cast<float>(t_sum / r.count);
        r.h_avg = static_cast<float>(h_sum / r.count);
        r.p_avg = static_cast<float>(p_sum / r.count);
    }
    return r;
}

MeteoTrend computeTrend(const DayStore& store, const RamSeries& ramSeries) {
    MeteoTrend trend;

    // On copie (ts,t,h,p) de la source RAM dans une vue uniforme pour partager la meme logique
    // quel que soit le flux (IN ~24h a 1/min, OUT ~12h a 1/30s). Humidite / pression non
    // mesurees (NaN, ou ancien 0.0 deja archive) -> NaN : la variation correspondante est alors
    // neutralisee au lieu d'un faux -1000 hPa.
    auto hOk = [](float v) { return mhrange::storedHumidityOk(v) ? v : NAN; };
    auto pOk = [](float v) { return mhrange::storedPressureOk(v) ? v : NAN; };
    std::vector<RawRecord> ram;
    ramSeries.forEach([&](int64_t ts, float t, float h, float p) {
        ram.push_back(RawRecord{static_cast<uint32_t>(ts), t, hOk(h), pOk(p), 0});
    });
    if (ram.empty()) return trend;

    const time_t now = static_cast<time_t>(ram.back().ts);
    float t_now = ram.back().t;
    float h_now = ram.back().h;
    float p_now = ram.back().p;

    float t_1h = t_now, h_1h = h_now, p_1h = p_now;
    float t_12h = t_now, h_12h = h_now, p_12h = p_now;
    float t_24h = t_now, h_24h = h_now, p_24h = p_now;
    bool found_1h = false, found_12h = false, found_24h = false;
    size_t trend_iteration = 0;

    for (auto it = ram.rbegin(); it != ram.rend(); ++it) {
        MH_YIELD_EVERY(trend_iteration, 256);
        trend_iteration++;

        time_t dt = now - static_cast<time_t>(it->ts);
        if (!found_1h && dt >= 3600) {
            t_1h = it->t; h_1h = it->h; p_1h = it->p; found_1h = true;
        }
        if (!found_12h && dt >= 43200) {
            t_12h = it->t; h_12h = it->h; p_12h = it->p; found_12h = true;
        }
        if (!found_24h && dt >= 86400) {
            t_24h = it->t; h_24h = it->h; p_24h = it->p; found_24h = true;
            break;
        }
    }

    // Points hors de portee de la RAM (l'OUT ne couvre que ~12h) : on les cherche sur la carte
    // SD, dans l'arborescence du bon flux. Aucun repli entre flux : une tendance OUT
    // n'emprunte jamais de point IN.
    float tmp_t, tmp_h, tmp_p;
    if (!found_12h && store.sampleNear(now - 43200, tmp_t, tmp_h, tmp_p)) {
        t_12h = tmp_t; h_12h = hOk(tmp_h); p_12h = pOk(tmp_p); found_12h = true;
    }
    if (!found_24h && store.sampleNear(now - 86400, tmp_t, tmp_h, tmp_p)) {
        t_24h = tmp_t; h_24h = hOk(tmp_h); p_24h = pOk(tmp_p); found_24h = true;
    }

    // Point a J-48h : jamais en RAM, lu sur SD si disponible.
    float t_48h = t_now, h_48h = h_now, p_48h = p_now;
    trend.available_48h = store.sampleNear(now - 172800, t_48h, h_48h, p_48h);
    h_48h = hOk(h_48h); p_48h = pOk(p_48h);

    trend.temp.delta_1h = t_now - t_1h;
    trend.temp.delta_12h = t_now - t_12h;
    trend.temp.delta_24h = t_now - t_24h;
    trend.temp.delta_48h = trend.available_48h ? (t_now - t_48h) : 0.0f;
    trend.hum.delta_1h = h_now - h_1h;
    trend.hum.delta_12h = h_now - h_12h;
    trend.hum.delta_24h = h_now - h_24h;
    trend.hum.delta_48h = trend.available_48h ? (h_now - h_48h) : 0.0f;
    trend.pres.delta_1h = p_now - p_1h;
    trend.pres.delta_12h = p_now - p_12h;
    trend.pres.delta_24h = p_now - p_24h;
    trend.pres.delta_48h = trend.available_48h ? (p_now - p_48h) : 0.0f;

    // Une variation non finie (mesure absente d'un des deux points) est neutralisee.
    for (TrendMetric* m : {&trend.hum, &trend.pres}) {
        if (!std::isfinite(m->delta_1h)) m->delta_1h = 0.0f;
        if (!std::isfinite(m->delta_12h)) m->delta_12h = 0.0f;
        if (!std::isfinite(m->delta_24h)) m->delta_24h = 0.0f;
        if (!std::isfinite(m->delta_48h)) m->delta_48h = 0.0f;
    }
    auto dir = [](float d) {
        if (d > 0.2) return std::string("hausse");
        if (d < -0.2) return std::string("baisse");
        return std::string("stable");
    };
    trend.temp.direction_1h = dir(trend.temp.delta_1h);
    trend.temp.direction_12h = dir(trend.temp.delta_12h);
    trend.temp.direction_24h = dir(trend.temp.delta_24h);
    trend.temp.direction_48h = trend.available_48h ? dir(trend.temp.delta_48h) : "indisponible";
    trend.hum.direction_1h = dir(trend.hum.delta_1h);
    trend.hum.direction_12h = dir(trend.hum.delta_12h);
    trend.hum.direction_24h = dir(trend.hum.delta_24h);
    trend.hum.direction_48h = trend.available_48h ? dir(trend.hum.delta_48h) : "indisponible";
    trend.pres.direction_1h = dir(trend.pres.delta_1h);
    trend.pres.direction_12h = dir(trend.pres.delta_12h);
    trend.pres.direction_24h = dir(trend.pres.delta_24h);
    trend.pres.direction_48h = trend.available_48h ? dir(trend.pres.delta_48h) : "indisponible";

    return trend;
}

} // namespace mhhist
