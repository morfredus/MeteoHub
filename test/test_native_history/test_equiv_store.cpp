// Equivalence du magasin de journees (DayStore, IN et OUT) avec le code de reference : memes
// octets sur la carte, memes retours, memes lectures, pour des suites d'operations
// pseudo-aleatoires (dont ecritures interrompues, fichiers desalignes, ecritures refusees).
#include <cmath>
#include "test_util.h"
#include "mem_fs.h"
#include "history/day_store.h"
#include "test_scenarios.h"

static const time_t T0 = mkUtc(2026, 11, 5, 12, 0, 0);

static float pickValue(Rng& r, float base) {
    const uint32_t k = r.below(20);
    if (k == 0) return NAN;                    // mesure absente
    return base + (float)r.below(400) / 10.f;
}

static void compareReads(LegacyRig& L, NewRig& N, Rng& r, time_t lo, time_t hi) {
    // Liste des jours, IN et OUT (le cache a pu etre construit en cours de route).
    for (int ctx = 0; ctx < 2; ctx++) {
        const auto a = ctx ? L.hm.listDaysOutdoor() : L.hm.listDays();
        const auto b = (ctx ? N.out : N.in).listDays();
        TEST_ASSERT_EQUAL(a.size(), b.size());
        for (size_t i = 0; i < a.size(); i++) {
            TEST_ASSERT_EQUAL_UINT32(a[i].day_key, b[i].day_key);
            TEST_ASSERT_EQUAL_UINT32(a[i].nrec, b[i].nrec);
            TEST_ASSERT_EQUAL_UINT32(a[i].first_ts, b[i].first_ts);
            TEST_ASSERT_EQUAL_UINT32(a[i].last_ts, b[i].last_ts);
        }
        // Export brut de chaque jour, en entier et par fenetre.
        for (const auto& d : a) {
            for (uint32_t from : {0u, 3u})
                for (uint32_t limit : {5u, 100000u}) {
                    std::vector<legacy::RawRecord> ra;
                    std::vector<RawRecord> rb;
                    const uint32_t ta = ctx ? L.hm.exportRawOutdoor(d.day_key, from, limit, [&](const legacy::RawRecord& x) { ra.push_back(x); })
                                            : L.hm.exportRaw(d.day_key, from, limit, [&](const legacy::RawRecord& x) { ra.push_back(x); });
                    const uint32_t tb = (ctx ? N.out : N.in).exportRaw(d.day_key, from, limit, [&](const RawRecord& x) { rb.push_back(x); });
                    TEST_ASSERT_EQUAL_UINT32(ta, tb);
                    TEST_ASSERT_EQUAL(ra.size(), rb.size());
                    for (size_t i = 0; i < ra.size(); i++) {
                        TEST_ASSERT_EQUAL_UINT32(ra[i].ts, rb[i].ts);
                        TEST_ASSERT_TRUE(sameFloat(ra[i].t, rb[i].t) && sameFloat(ra[i].h, rb[i].h) && sameFloat(ra[i].p, rb[i].p));
                        TEST_ASSERT_EQUAL_UINT32(ra[i].flags, rb[i].flags);
                    }
                }
        }
    }
    // Jour inexistant.
    TEST_ASSERT_EQUAL_UINT32(L.hm.exportRaw(20200101, 0, 10, [](const legacy::RawRecord&) {}),
                             N.in.exportRaw(20200101, 0, 10, [](const RawRecord&) {}));
    // Mesure la plus proche d'un instant : IN (dichotomie + CSV legacy) et OUT (parcours).
    for (int k = 0; k < 40; k++) {
        const time_t target = lo - 4000 + (time_t)r.below((uint32_t)(hi - lo + 8000));
        for (int ctx = 0; ctx < 2; ctx++) {
            float ta = -1, ha = -1, pa = -1, tb = -1, hb = -1, pb = -1;
            const bool a = L.hm.readSdSampleNear(target, ta, ha, pa, ctx == 1);
            const bool b = (ctx ? N.out : N.in).sampleNear(target, tb, hb, pb);
            TEST_ASSERT_EQUAL(a, b);
            if (a) TEST_ASSERT_TRUE(sameFloat(ta, tb) && sameFloat(ha, hb) && sameFloat(pa, pb));
        }
    }
    // Statistiques du jour (IN).
    for (const auto& d : L.hm.listDays()) {
        struct tm tm0{};
        tm0.tm_year = (int)(d.day_key / 10000) - 1900;
        tm0.tm_mon = (int)((d.day_key / 100) % 100) - 1;
        tm0.tm_mday = (int)(d.day_key % 100);
        tm0.tm_hour = 12;
        const time_t dayTs = mktime(&tm0);
        legacy::DayStats sa;
        DayStats sb;
        const bool a = L.hm.readDayStats(dayTs, sa);
        const bool b = N.in.readDayStats(dayTs, sb);
        TEST_ASSERT_EQUAL(a, b);
        if (a) TEST_ASSERT_EQUAL_MEMORY(&sa, &sb, sizeof(DayStats));
    }
}

static void runScenario(uint32_t seed, int nops, bool faults) {
    LegacyRig L;
    NewRig N;
    seedLegacyFiles(L, N);
    Rng r(seed);
    time_t inTs = T0, outTs = T0;
    for (int i = 0; i < nops; i++) {
        const uint32_t kind = r.below(100);
        // Pannes simulees sur l'operation suivante, identiques des deux cotes.
        bool failOpen = false;
        long quota = -1;
        if (faults && r.below(12) == 0) {
            if (r.below(2)) failOpen = true; else quota = (long)r.below(60);
        }
        L.sd.failOpenForWrite = N.sd.failOpenForWrite = failOpen;
        L.sd.quota = N.sd.quota = quota;

        if (kind < 45) {
            inTs += 20 + r.below(100) + (r.below(40) == 0 ? 86400 : 0);
            const float t = pickValue(r, 10.f), h = pickValue(r, 30.f), p = pickValue(r, 980.f);
            const bool a = L.hm.saveToSdBinary(inRec(inTs, t, h, p));
            const mhhist::BinRecord br{(uint32_t)inTs, t, h, p};
            const bool b = N.in.append(inTs, br, 0);
            TEST_ASSERT_EQUAL(a, b);
        } else if (kind < 80) {
            // OUT : parfois une mesure retransmise (plus ancienne), parfois marquee, parfois sur
            // la journee v1 d'avant la 1.54.0.
            outTs += 15 + r.below(60) + (r.below(50) == 0 ? 86400 : 0);
            time_t ts = outTs;
            if (r.below(5) == 0) ts -= 30 + r.below(600);
            if (r.below(15) == 0) ts = mkUtc(2026, 11, 3, 9, 0, 0) + r.below(3600);
            const uint32_t flags = r.below(8) == 0 ? 1 : 0;
            const float t = pickValue(r, 0.f), h = pickValue(r, 30.f), p = pickValue(r, 980.f);
            const bool a = L.hm.saveOutdoorToSdBinary(outRec(ts, t, h, p), flags);
            const mhhist::BinRecord br{(uint32_t)ts, t, h, p};
            const bool b = N.out.append(ts, br, flags);
            TEST_ASSERT_EQUAL(a, b);
        } else if (kind < 85) {
            // Ecriture interrompue : quelques octets parasites en fin d'un .bin existant.
            std::vector<std::string> bins;
            for (const auto& kv : L.sd.files)
                if (kv.first.size() > 4 && kv.first.compare(kv.first.size() - 4, 4, ".bin") == 0)
                    bins.push_back(kv.first);
            if (!bins.empty()) {
                const std::string& victim = bins[r.below((uint32_t)bins.size())];
                const uint32_t n = 1 + r.below(31);
                for (uint32_t k = 0; k < n; k++) {
                    const uint8_t junk = (uint8_t)r.below(256);
                    L.sd.files[victim].push_back(junk);
                    N.sd.files[victim].push_back(junk);
                }
            }
        } else if (kind < 92) {
            // Lecture intermediaire : construit le cache de la liste des jours (la suite le
            // tient a jour incrementalement ou l'invalide).
            const auto a = L.hm.listDays();
            const auto b = N.in.listDays();
            TEST_ASSERT_EQUAL(a.size(), b.size());
            const auto ao = L.hm.listDaysOutdoor();
            const auto bo = N.out.listDays();
            TEST_ASSERT_EQUAL(ao.size(), bo.size());
        } else if (kind < 94) {
            // Redemarrage : caches vides (nouveau gestionnaire cote reference).
            L.hm._currentDayKey = 0;
            N.in.resetStatsCache();
        }
    }
    L.sd.failOpenForWrite = N.sd.failOpenForWrite = false;
    L.sd.quota = N.sd.quota = -1;

    assertSameVolume(L.sd, N.sd);
    compareReads(L, N, r, T0 - 86400 * 3, inTs > outTs ? inTs : outTs);

    // Les listes en cache doivent aussi egaler un rescan complet.
    L.hm.invalidateDaysCache();
    N.in.invalidateDaysCache();
    N.out.invalidateDaysCache();
    compareReads(L, N, r, T0 - 86400 * 3, inTs > outTs ? inTs : outTs);
}

void test_eqstore_scenarios_without_faults() {
    for (uint32_t seed : {1u, 2u, 3u, 4u, 5u, 6u}) runScenario(seed, 400, false);
}

void test_eqstore_scenarios_with_faults() {
    for (uint32_t seed : {11u, 12u, 13u, 14u, 15u, 16u}) runScenario(seed, 400, true);
}

void test_eqstore_in_legacy_upgrade_and_out_v1() {
    LegacyRig L;
    NewRig N;
    seedLegacyFiles(L, N);
    const time_t d3 = mkUtc(2026, 11, 3, 9, 0, 0);
    // IN : le .bin sans en-tete est converti a la premiere ecriture.
    TEST_ASSERT_EQUAL(L.hm.saveToSdBinary(inRec(d3, 1.f, 2.f, 3.f)),
                      N.in.append(d3, mhhist::BinRecord{(uint32_t)d3, 1.f, 2.f, 3.f}, 0));
    // OUT : le fichier v1 garde son format ; une marque est refusee.
    TEST_ASSERT_EQUAL(L.hm.saveOutdoorToSdBinary(outRec(d3, 1.f, 2.f, 3.f), 1),
                      N.out.append(d3, mhhist::BinRecord{(uint32_t)d3, 1.f, 2.f, 3.f}, 1));
    TEST_ASSERT_EQUAL(L.hm.saveOutdoorToSdBinary(outRec(d3, 1.f, 2.f, 3.f), 0),
                      N.out.append(d3, mhhist::BinRecord{(uint32_t)d3, 1.f, 2.f, 3.f}, 0));
    assertSameVolume(L.sd, N.sd);
    TEST_ASSERT_EQUAL(32 + 5 * 16, bytesOf(N.sd, "/history/indoor/2026/11/2026-11-03.bin").size());
    TEST_ASSERT_EQUAL(4 * 16, bytesOf(N.sd, "/history/outdoor/2026/11/2026-11-03.bin").size());
}

void test_eqstore_csv_fallback_and_unmounted_card() {
    LegacyRig L;
    NewRig N;
    const std::string csv = "Timestamp,Temperature,Humidity,Pressure\n" + std::to_string((long)T0) + ",18.5,60.0,1010.5\n"
                            + std::to_string((long)T0 + 60) + ",18.6,60.5,1010.6\n" + std::to_string((long)T0 + 120)
                            + ",18.7,61.0,1010.7";   // derniere ligne sans fin de ligne
    for (MemVolume* v : {&L.sd, &N.sd}) {
        v->dirs.insert("/history");
        v->files["/history/2026-11-05.csv"] = std::vector<uint8_t>(csv.begin(), csv.end());
    }
    for (time_t target : {T0 - 100, T0 + 10, T0 + 119, T0 + 125, T0 + 5000}) {
        float ta = 0, ha = 0, pa = 0, tb = 0, hb = 0, pb = 0;
        const bool a = L.hm.readSdSampleNear(target, ta, ha, pa, false);
        const bool b = N.in.sampleNear(target, tb, hb, pb);
        TEST_ASSERT_EQUAL(a, b);
        if (a) TEST_ASSERT_TRUE(sameFloat(ta, tb) && sameFloat(ha, hb) && sameFloat(pa, pb));
        // OUT : jamais de repli CSV.
        TEST_ASSERT_EQUAL(L.hm.readSdSampleNear(target, ta, ha, pa, true), N.out.sampleNear(target, tb, hb, pb));
    }
    L.mgr.mounted = false;
    N.fs.mounted = false;
    float t, h, p;
    TEST_ASSERT_FALSE(L.hm.readSdSampleNear(T0, t, h, p, false));
    TEST_ASSERT_FALSE(N.in.sampleNear(T0, t, h, p));
    TEST_ASSERT_EQUAL(0, N.in.listDays().size());
    TEST_ASSERT_EQUAL(0, L.hm.listDays().size());
}

void test_eqstore_list_days_yields_and_ignores_strays() {
    LegacyRig L;
    NewRig N;
    seedLegacyFiles(L, N);
    for (int d = 0; d < 40; d++) {
        const time_t ts = T0 + d * 86400;
        L.hm.saveToSdBinary(inRec(ts, 1.f, 2.f, 3.f));
        N.in.append(ts, mhhist::BinRecord{(uint32_t)ts, 1.f, 2.f, 3.f}, 0);
    }
    for (MemVolume* v : {&L.sd, &N.sd}) {
        v->files["/history/indoor/2026/11/notes.txt"] = {1, 2, 3};
        v->files["/history/indoor/2026/11/2026-13-40.bin"] = std::vector<uint8_t>(64, 0);   // date impossible
        v->files["/history/indoor/2026/11/a.bin"] = std::vector<uint8_t>(64, 0);            // nom trop court
        v->dirs.insert("/history/indoor/2026/11/sub");
    }
    L.hm.invalidateDaysCache();
    N.in.invalidateDaysCache();
    g_legacyYieldCalls = 0;
    mhhist::testYieldCalls() = 0;
    const auto a = L.hm.listDays();
    const auto b = N.in.listDays();
    TEST_ASSERT_EQUAL(a.size(), b.size());
    TEST_ASSERT_TRUE(a.size() > 30);
    TEST_ASSERT_EQUAL(g_legacyYieldCalls, mhhist::testYieldCalls());
    TEST_ASSERT_TRUE(mhhist::testYieldCalls() >= a.size());   // un yield par fichier examine
}

void run_equiv_store() {
    RUN_TEST(test_eqstore_scenarios_without_faults);
    RUN_TEST(test_eqstore_scenarios_with_faults);
    RUN_TEST(test_eqstore_in_legacy_upgrade_and_out_v1);
    RUN_TEST(test_eqstore_csv_fallback_and_unmounted_card);
    RUN_TEST(test_eqstore_list_days_yields_and_ignores_strays);
}
