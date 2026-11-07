// Equivalence des requetes (queryRange, Stats24h, tendances, synthese) avec le code de
// reference, sur des historiques SD + RAM pseudo-aleatoires (pics isoles, mesures absentes,
// anciens 0.0, retransmissions hors ordre, repli CSV legacy).
#include <cmath>
#include <mutex>
#include "test_util.h"
#include "test_scenarios.h"
#include "history/history_query.h"
#include "history/history_export.h"
#include "history/recent_ring.h"

static const time_t T0 = mkUtc(2026, 11, 5, 12, 0, 0);

struct QueryRig {
    LegacyRig L;
    NewRig N;
    std::mutex m;
    mhhist::InRecentRing inRing;
    mhhist::OutRecentRing outRing;
    mhhist::RingSeries<mhhist::InRecentRing> inS;
    mhhist::RingSeries<mhhist::OutRecentRing> outS;
    time_t inEnd = T0, outEnd = T0;
    QueryRig()
        : inRing(N.fs, m, "/x/in", "/x/in.tmp", 1440, 2880),
          outRing(N.fs, m, "/x/out", "/x/out.tmp", 1440, 2880),
          inS(inRing, m), outS(outRing, m) {}
};

static float noisy(Rng& r, float base, float amp) {
    return base + amp * (float)((int)r.below(2001) - 1000) / 1000.f;
}

// Remplit SD (deux jours et demi) puis RAM, des deux cotes de la meme facon.
static void populate(QueryRig& q, uint32_t seed) {
    Rng r(seed);
    seedLegacyFiles(q.L, q.N);
    time_t ts = T0 - 2 * 86400;
    while (ts < T0 + 3600 * 6) {
        ts += 30 + r.below(120);
        if (r.below(60) == 0) ts += 3600;                 // trou dans l'historique
        float t = noisy(r, 14.f, 4.f), h = noisy(r, 50.f, 15.f), p = noisy(r, 1005.f, 8.f);
        if (r.below(40) == 0) t += 25.f;                  // pic isole
        if (r.below(60) == 0) h = NAN;                    // humidite absente
        if (r.below(80) == 0) p = 0.f;                    // ancien 0.0 deja archive
        q.L.hm.saveToSdBinary(inRec(ts, t, h, p));
        q.N.in.append(ts, mhhist::BinRecord{(uint32_t)ts, t, h, p}, 0);
    }
    q.inEnd = ts;
    ts = T0 - 2 * 86400;
    time_t latest = ts;
    while (ts < T0 + 3600 * 5) {
        ts += 20 + r.below(90);
        time_t when = ts;
        if (r.below(8) == 0) when = latest - (time_t)r.below(900);   // retransmission
        if (r.below(200) == 0) when = latest - 86400 * 2;            // tres ancienne
        if (when > latest) latest = when;
        float t = noisy(r, 9.f, 6.f), h = noisy(r, 70.f, 20.f), p = noisy(r, 1010.f, 9.f);
        if (r.below(50) == 0) t -= 20.f;
        if (r.below(45) == 0) h = NAN;
        if (r.below(45) == 0) p = NAN;
        const uint32_t flags = r.below(25) == 0 ? 1 : 0;
        q.L.hm.saveOutdoorToSdBinary(outRec(when, t, h, p), flags);
        q.N.out.append(when, mhhist::BinRecord{(uint32_t)when, t, h, p}, flags);
    }
    q.outEnd = latest;
    // Un jour d'IN au format CSV legacy, sans .bin.
    const time_t csvDay = T0 - 6 * 86400;
    std::string csv = "Timestamp,Temperature,Humidity,Pressure\n";
    for (int i = 0; i < 40; i++) {
        char b[64];
        snprintf(b, sizeof(b), "%ld,%.1f,%.1f,%.1f\n", (long)(csvDay + i * 600), 12.f + (i % 5), 55.f, 1001.f);
        csv += b;
    }
    char path[48];
    struct tm tm1;
    localtime_r(&csvDay, &tm1);
    strftime(path, sizeof(path), "/history/%Y-%m-%d.csv", &tm1);
    q.L.sd.files[path] = q.N.sd.files[path] = std::vector<uint8_t>(csv.begin(), csv.end());

    // RAM : la fin de l'historique + des mesures deja presentes sur SD (a ignorer) + du vide.
    for (time_t t = q.inEnd - 3 * 3600; t <= q.inEnd + 1800; t += 60 + r.below(60)) {
        const float tv = noisy(r, 14.f, 4.f);
        const float hv = r.below(30) == 0 ? NAN : noisy(r, 50.f, 15.f), pv = noisy(r, 1005.f, 8.f);
        q.L.hm._recentHistory.push_back(inRec(t, tv, hv, pv));
        HistoryRecord nr;
        nr.timestamp = t; nr.t = tv; nr.h = hv; nr.p = pv;
        q.inRing.items().push_back(nr);
    }
    for (time_t t = q.outEnd - 2 * 3600; t <= q.outEnd + 1200; t += 30 + r.below(60)) {
        const float tv = noisy(r, 9.f, 6.f);
        const float hv = r.below(30) == 0 ? NAN : noisy(r, 70.f, 20.f), pv = noisy(r, 1010.f, 9.f);
        q.L.hm._outdoorHistory.push_back(outRec(t, tv, hv, pv));
        OutdoorHistoryRecord nr{};
        nr.timestamp = t; nr.t = tv; nr.h = hv; nr.p = pv;
        q.outRing.items().push_back(nr);
    }
}

static void assertSamePoints(const std::vector<legacy::HistoryPoint>& a, const std::vector<HistoryPoint>& b) {
    TEST_ASSERT_EQUAL(a.size(), b.size());
    for (size_t i = 0; i < a.size(); i++) {
        TEST_ASSERT_TRUE((long)a[i].t == (long)b[i].t);
        TEST_ASSERT_TRUE(sameFloat(a[i].temp, b[i].temp) && sameFloat(a[i].hum, b[i].hum) && sameFloat(a[i].pres, b[i].pres));
        TEST_ASSERT_EQUAL(a[i].tvalid, b[i].tvalid);
        TEST_ASSERT_EQUAL(a[i].hvalid, b[i].hvalid);
        TEST_ASSERT_EQUAL(a[i].pvalid, b[i].pvalid);
        TEST_ASSERT_EQUAL(a[i].valid, b[i].valid);
    }
}

static void assertSameTrend(const legacy::MeteoTrend& a, const MeteoTrend& b) {
    TEST_ASSERT_EQUAL(a.available_48h, b.available_48h);
    const legacy::TrendMetric* la[3] = {&a.temp, &a.hum, &a.pres};
    const TrendMetric* nb[3] = {&b.temp, &b.hum, &b.pres};
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_TRUE(sameFloat(la[i]->delta_1h, nb[i]->delta_1h));
        TEST_ASSERT_TRUE(sameFloat(la[i]->delta_12h, nb[i]->delta_12h));
        TEST_ASSERT_TRUE(sameFloat(la[i]->delta_24h, nb[i]->delta_24h));
        TEST_ASSERT_TRUE(sameFloat(la[i]->delta_48h, nb[i]->delta_48h));
        TEST_ASSERT_EQUAL_STRING(la[i]->direction_1h.c_str(), nb[i]->direction_1h.c_str());
        TEST_ASSERT_EQUAL_STRING(la[i]->direction_12h.c_str(), nb[i]->direction_12h.c_str());
        TEST_ASSERT_EQUAL_STRING(la[i]->direction_24h.c_str(), nb[i]->direction_24h.c_str());
        TEST_ASSERT_EQUAL_STRING(la[i]->direction_48h.c_str(), nb[i]->direction_48h.c_str());
    }
}

void test_eqquery_range_matches_reference() {
    for (uint32_t seed : {1u, 2u, 3u}) {
        QueryRig q;
        populate(q, seed);
        assertSameVolume(q.L.sd, q.N.sd);
        Rng r(seed * 77);
        const long intervals[] = {1, 30, 60, 300, 900, 3600, 21600};
        for (int k = 0; k < 60; k++) {
            const time_t from = T0 - 7 * 86400 + (time_t)r.below(9 * 86400);
            const time_t to = from + (time_t)(r.below(4 * 86400) + (r.below(10) == 0 ? 0 : 1));
            const long interval = intervals[r.below(7)];
            for (int outdoor = 0; outdoor < 2; outdoor++) {
                g_legacyYieldCalls = 0;
                mhhist::testYieldCalls() = 0;
                const auto a = q.L.hm.queryRangeImpl(from, to, interval, outdoor == 1);
                const auto b = mhhist::queryRange(outdoor ? q.N.out : q.N.in,
                                                  outdoor ? (const mhhist::RamSeries&)q.outS : q.inS,
                                                  from, to, interval);
                assertSamePoints(a, b);
                TEST_ASSERT_EQUAL(g_legacyYieldCalls, mhhist::testYieldCalls());
            }
        }
        // Carte retiree : lecture RAM seule, des deux cotes.
        q.L.mgr.mounted = false;
        q.N.fs.mounted = false;
        assertSamePoints(q.L.hm.queryRangeImpl(T0 - 3600, T0 + 3600, 300, false),
                         mhhist::queryRange(q.N.in, q.inS, T0 - 3600, T0 + 3600, 300));
        assertSamePoints(q.L.hm.queryRangeImpl(T0 - 3600, T0 + 3600, 300, true),
                         mhhist::queryRange(q.N.out, q.outS, T0 - 3600, T0 + 3600, 300));
    }
}

void test_eqquery_degenerate_ranges() {
    QueryRig q;
    populate(q, 9);
    for (time_t to : {T0 - 10, T0, T0 + 1}) {
        for (long interval : {-5L, 0L, 1L, 1000000000L}) {
            assertSamePoints(q.L.hm.queryRangeImpl(T0, to, interval, false),
                             mhhist::queryRange(q.N.in, q.inS, T0, to, interval));
        }
    }
}

void test_eqquery_trend_matches_reference() {
    for (uint32_t seed : {1u, 2u, 3u, 4u}) {
        QueryRig q;
        populate(q, seed);
        assertSameTrend(q.L.hm.getTrend(), mhhist::computeTrend(q.N.in, q.inS));
        assertSameTrend(q.L.hm.getTrendOutdoor(), mhhist::computeTrend(q.N.out, q.outS));
        // RAM vide : tendance vide, des deux cotes.
        q.L.hm._recentHistory.clear();
        q.inRing.items().clear();
        assertSameTrend(q.L.hm.getTrend(), mhhist::computeTrend(q.N.in, q.inS));
        // RAM courte : 12 h / 24 h / 48 h cherches sur SD.
        q.L.hm._outdoorHistory.erase(q.L.hm._outdoorHistory.begin(), q.L.hm._outdoorHistory.end() - 5);
        q.outRing.items().erase(q.outRing.items().begin(), q.outRing.items().end() - 5);
        assertSameTrend(q.L.hm.getTrendOutdoor(), mhhist::computeTrend(q.N.out, q.outS));
    }
}

void test_eqquery_stats24h_matches_reference() {
    QueryRig q;
    Rng r(5);
    const time_t now = time(nullptr);
    for (int i = 0; i < 400; i++) {
        // Hors des 10 s autour de la borne now-24h, pour ne pas dependre de la seconde en cours.
        long off = (long)r.below(2 * 86400);
        if (labs(off - 86400) < 10) off += 30;
        const time_t ts = now - off;
        const float t = noisy(r, 12.f, 5.f) + (r.below(50) == 0 ? 30.f : 0.f);
        const float h = r.below(30) == 0 ? NAN : noisy(r, 55.f, 20.f);
        const float p = r.below(30) == 0 ? 0.f : noisy(r, 1005.f, 6.f);
        q.L.hm._recentHistory.push_back(inRec(ts, t, h, p));
        HistoryRecord nr;
        nr.timestamp = ts; nr.t = t; nr.h = h; nr.p = p;
        q.inRing.items().push_back(nr);
    }
    // Les deux tampons sont lus dans l'ordre d'arrivee, comme en production.
    const auto a = q.L.hm.getIndoorStats();
    const auto b = mhhist::computeStats24h(q.inS, now);
    TEST_ASSERT_EQUAL(a.count, b.count);
    TEST_ASSERT_EQUAL(a.temp.count, b.temp.count);
    TEST_ASSERT_TRUE(sameFloat(a.temp.min, b.temp.min) && sameFloat(a.temp.max, b.temp.max));
    TEST_ASSERT_TRUE(std::fabs(a.temp.sum - b.temp.sum) < 1e-9);
    TEST_ASSERT_EQUAL(a.hum.count, b.hum.count);
    TEST_ASSERT_TRUE(sameFloat(a.hum.min, b.hum.min) && sameFloat(a.hum.max, b.hum.max));
    TEST_ASSERT_TRUE(std::fabs(a.hum.sum - b.hum.sum) < 1e-9);
    TEST_ASSERT_EQUAL(a.pres.count, b.pres.count);
    TEST_ASSERT_TRUE(std::fabs(a.pres.sum - b.pres.sum) < 1e-9);
    // Horloge pas encore fiable (now <= 24 h) : toute la RAM compte.
    const auto c = mhhist::computeStats24h(q.inS, 100);
    TEST_ASSERT_EQUAL(400, c.count);
}

void test_eqquery_synthesis_matches_reference() {
    for (uint32_t seed : {1u, 2u}) {
        QueryRig q;
        populate(q, seed);
        Rng r(seed + 100);
        for (int k = 0; k < 30; k++) {
            const time_t from = T0 - 5 * 86400 + (time_t)r.below(6 * 86400);
            const time_t to = from + (time_t)r.below(3 * 86400);
            g_legacyYieldCalls = 0;
            mhhist::testYieldCalls() = 0;
            const auto a = q.L.hm.querySynthesis(from, to);
            const auto b = mhhist::querySynthesis(q.N.in, from, to);
            TEST_ASSERT_EQUAL(a.valid, b.valid);
            TEST_ASSERT_EQUAL_UINT32(a.count, b.count);
            const float av[] = {a.t_min, a.t_max, a.t_avg, a.t_first, a.t_last, a.h_min, a.h_max, a.h_avg, a.h_first,
                                a.h_last, a.p_min, a.p_max, a.p_avg, a.p_first, a.p_last};
            const float bv[] = {b.t_min, b.t_max, b.t_avg, b.t_first, b.t_last, b.h_min, b.h_max, b.h_avg, b.h_first,
                                b.h_last, b.p_min, b.p_max, b.p_avg, b.p_first, b.p_last};
            for (int i = 0; i < 15; i++) TEST_ASSERT_TRUE(sameFloat(av[i], bv[i]));
            TEST_ASSERT_EQUAL(g_legacyYieldCalls, mhhist::testYieldCalls());
        }
    }
}

void test_eqquery_trend_direction_thresholds() {
    // Seuil de +/- 0.2 pour hausse / baisse / stable, y compris juste au-dela et en-deca.
    for (float delta : {0.f, 0.1f, 0.19f, 0.2f, 0.21f, 0.25f, 0.3f, -0.19f, -0.2f, -0.21f, -0.3f, 5.f, -5.f}) {
        QueryRig q;
        for (time_t t : {T0 - 3600, T0}) {
            const float tv = 20.f + (t == T0 ? delta : 0.f);
            q.L.hm._recentHistory.push_back(inRec(t, tv, 50.f + (t == T0 ? delta : 0.f), 1000.f));
            HistoryRecord nr;
            nr.timestamp = t; nr.t = tv; nr.h = 50.f + (t == T0 ? delta : 0.f); nr.p = 1000.f;
            q.inRing.items().push_back(nr);
        }
        assertSameTrend(q.L.hm.getTrend(), mhhist::computeTrend(q.N.in, q.inS));
    }
}

// Export CSV reprenable : MEME suite de morceaux (taille et contenu de chaque appel) que le
// code de reference, pour des tampons de toutes tailles, des plages variees et les deux flux.
void test_eqexport_csv_chunks_match_reference() {
    for (uint32_t seed : {1u, 2u}) {
        QueryRig q;
        populate(q, seed);
        Rng r(seed * 31);
        const size_t sizes[] = {1, 13, 41, 64, 100, 1024, 4096};
        for (int k = 0; k < 24; k++) {
            const time_t from = T0 - 3 * 86400 + (time_t)r.below(3 * 86400);
            const time_t to = from + (time_t)(r.below(3 * 86400) + (r.below(8) == 0 ? 0 : 1));
            const size_t chunk = sizes[r.below(7)];
            for (int outdoor = 0; outdoor < 2; outdoor++) {
                legacy::HistoryManager::CsvCursor a;
                a.outdoor = outdoor == 1; a.from = from; a.to = to; a.day = from;
                mhhist::CsvCursor b;
                b.outdoor = outdoor == 1; b.from = from; b.to = to; b.day = from;
                std::vector<char> ba(chunk), bb(chunk);
                g_legacyYieldCalls = 0;
                mhhist::testYieldCalls() = 0;
                size_t calls = 0;
                for (;;) {
                    const size_t na = q.L.hm.exportCsvChunk(a, ba.data(), chunk);
                    const size_t nb = mhhist::exportCsvChunk(outdoor ? q.N.out : q.N.in, b, bb.data(), chunk);
                    TEST_ASSERT_EQUAL(na, nb);
                    if (na) TEST_ASSERT_EQUAL_MEMORY(ba.data(), bb.data(), na);
                    if (na == 0) break;
                    TEST_ASSERT_TRUE(++calls < 1000000);
                }
                TEST_ASSERT_EQUAL(g_legacyYieldCalls, mhhist::testYieldCalls());
            }
        }
    }
}

void run_equiv_query() {
    RUN_TEST(test_eqquery_range_matches_reference);
    RUN_TEST(test_eqquery_degenerate_ranges);
    RUN_TEST(test_eqquery_trend_matches_reference);
    RUN_TEST(test_eqquery_trend_direction_thresholds);
    RUN_TEST(test_eqquery_stats24h_matches_reference);
    RUN_TEST(test_eqquery_synthesis_matches_reference);
    RUN_TEST(test_eqexport_csv_chunks_match_reference);
}
