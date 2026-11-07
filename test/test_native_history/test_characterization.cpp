// Tests de CARACTERISATION : figent le comportement et le format disque du code de la
// 1.62.0 (reference compilee dans legacy_tu.cpp). Ils doivent passer AVANT la refonte ;
// les tests d'equivalence (test_equivalence.cpp) prouvent ensuite que le code refondu
// produit les memes octets et les memes resultats.
#include "test_util.h"

static const char* IN_BIN = "/history/indoor/2026/11/2026-11-05.bin";
static const char* IN_STATS = "/history/indoor/2026/11/2026-11-05.stats";
static const char* OUT_BIN = "/history/outdoor/2026/11/2026-11-05.bin";
static const char* OUT_STATS = "/history/outdoor/2026/11/2026-11-05.stats";
static const time_t T0 = mkUtc(2026, 11, 5, 12, 0, 0);
static const size_t MAX_RECENT = 1440;

struct RawRec { uint32_t ts; float t; uint32_t flags; };

// --- En-tete et fonctions pures de format ------------------------------------------
void test_char_header_layout() {
    uint8_t h[32];
    legacy_probe::header(false, h);
    TEST_ASSERT_EQUAL_MEMORY("MTHB", h, 4);
    uint16_t v; memcpy(&v, h + 4, 2); TEST_ASSERT_EQUAL_UINT16(1, v);
    memcpy(&v, h + 6, 2); TEST_ASSERT_EQUAL_UINT16(32, v);   // headerSize
    memcpy(&v, h + 8, 2); TEST_ASSERT_EQUAL_UINT16(16, v);   // recordSize
    memcpy(&v, h + 10, 2); TEST_ASSERT_EQUAL_UINT16(7, v);   // temp|hum|pres
    for (int i = 12; i < 32; i++) TEST_ASSERT_EQUAL_UINT8(0, h[i]); // compteurs a zero
    legacy_probe::header(true, h);
    memcpy(&v, h + 4, 2); TEST_ASSERT_EQUAL_UINT16(2, v);
    memcpy(&v, h + 8, 2); TEST_ASSERT_EQUAL_UINT16(20, v);
}

void test_char_pad_to_boundary() {
    TEST_ASSERT_EQUAL(0, legacy_probe::pad(32, 32, 16));        // juste l'en-tete
    TEST_ASSERT_EQUAL(0, legacy_probe::pad(32 + 32, 32, 16));   // aligne
    TEST_ASSERT_EQUAL(11, legacy_probe::pad(32 + 16 + 5, 32, 16));
    TEST_ASSERT_EQUAL(15, legacy_probe::pad(32 + 1, 32, 16));
    TEST_ASSERT_EQUAL(19, legacy_probe::pad(32 + 1, 32, 20));
    TEST_ASSERT_EQUAL(0, legacy_probe::pad(10, 32, 16));        // fichier plus court que l'en-tete
    TEST_ASSERT_EQUAL(0, legacy_probe::pad(100, 0, 0));         // recordSize nul : pas de bourrage
}

void test_char_probe_variants() {
    LegacyRig r;
    r.createTree();
    // 1. fichier absent
    TEST_ASSERT_EQUAL(0, legacy_probe::probe(IN_BIN).nrec);
    // 2. avec en-tete v1, 3 enregistrements
    for (int i = 0; i < 3; i++) r.hm.saveToSdBinary(inRec(T0 + i * 60, 20.f + i, 50.f, 1000.f));
    auto L = legacy_probe::probe(IN_BIN);
    TEST_ASSERT_EQUAL(32, L.dataOffset);
    TEST_ASSERT_EQUAL(16, L.recordSize);
    TEST_ASSERT_EQUAL(3, L.nrec);
    // 3. sans en-tete (v1.4.0) : enregistrements des l'offset 0
    r.sd.files["/legacy.bin"] = std::vector<uint8_t>(16 * 4 + 7, 0);
    L = legacy_probe::probe("/legacy.bin");
    TEST_ASSERT_EQUAL(0, L.dataOffset);
    TEST_ASSERT_EQUAL(16, L.recordSize);
    TEST_ASSERT_EQUAL(4, L.nrec);
    // 4. en-tete corrompu (magic faux) : traite comme sans en-tete
    std::vector<uint8_t> bad(64, 0);
    memcpy(bad.data(), "XXXX", 4);
    r.sd.files["/bad.bin"] = bad;
    L = legacy_probe::probe("/bad.bin");
    TEST_ASSERT_EQUAL(0, L.dataOffset);
    TEST_ASSERT_EQUAL(4, L.nrec);
    // 5. recordSize du fichier plus petit que 16 : ramene a 16
    uint8_t h[32];
    legacy_probe::header(false, h);
    uint16_t small = 8;
    memcpy(h + 8, &small, 2);
    std::vector<uint8_t> f(h, h + 32);
    f.resize(32 + 64);
    r.sd.files["/small.bin"] = f;
    L = legacy_probe::probe("/small.bin");
    TEST_ASSERT_EQUAL(16, L.recordSize);
    TEST_ASSERT_EQUAL(4, L.nrec);
    // 6. lecture a un index donne, dans et hors du fichier
    uint32_t ts; float t, hh, p;
    TEST_ASSERT_TRUE(legacy_probe::readAt(IN_BIN, 1, ts, t, hh, p));
    TEST_ASSERT_EQUAL_UINT32((uint32_t)(T0 + 60), ts);
    TEST_ASSERT_EQUAL_FLOAT(21.f, t);
    TEST_ASSERT_FALSE(legacy_probe::readAt(IN_BIN, 3, ts, t, hh, p));
}

// --- Ecriture IN ---------------------------------------------------------------
void test_char_in_append_creates_header_and_updates_counters() {
    LegacyRig r;
    r.createTree();
    TEST_ASSERT_TRUE(r.hm.saveToSdBinary(inRec(T0, 20.5f, 55.f, 1013.25f)));
    auto b = bytesOf(r.sd, IN_BIN);
    TEST_ASSERT_EQUAL(48, b.size());
    TEST_ASSERT_EQUAL_MEMORY("MTHB", b.data(), 4);
    TEST_ASSERT_EQUAL_UINT16(1, rd<uint16_t>(b, 4));
    TEST_ASSERT_EQUAL_UINT32(1, rd<uint32_t>(b, 12));            // recordCount mis a jour
    TEST_ASSERT_TRUE((uint64_t)T0 == rd<uint64_t>(b, 16));       // firstTimestamp
    TEST_ASSERT_TRUE((uint64_t)T0 == rd<uint64_t>(b, 24));       // lastTimestamp
    TEST_ASSERT_EQUAL_UINT32((uint32_t)T0, rd<uint32_t>(b, 32));
    TEST_ASSERT_EQUAL_FLOAT(20.5f, rd<float>(b, 36));
    TEST_ASSERT_EQUAL_FLOAT(55.f, rd<float>(b, 40));
    TEST_ASSERT_EQUAL_FLOAT(1013.25f, rd<float>(b, 44));
    TEST_ASSERT_TRUE(r.hm.saveToSdBinary(inRec(T0 + 60, 21.f, 54.f, 1013.f)));
    b = bytesOf(r.sd, IN_BIN);
    TEST_ASSERT_EQUAL(64, b.size());
    TEST_ASSERT_EQUAL_UINT32(2, rd<uint32_t>(b, 12));
    TEST_ASSERT_TRUE((uint64_t)T0 == rd<uint64_t>(b, 16));          // first inchange
    TEST_ASSERT_TRUE((uint64_t)(T0 + 60) == rd<uint64_t>(b, 24));   // last avance
    TEST_ASSERT_FALSE(r.sd.exists(std::string(IN_BIN) + ".tmp"));
    TEST_ASSERT_FALSE(r.sd.exists(OUT_STATS));
}

void test_char_in_stats_file() {
    LegacyRig r;
    r.createTree();
    r.hm.saveToSdBinary(inRec(T0, 20.f, 50.f, 1000.f));
    r.hm.saveToSdBinary(inRec(T0 + 60, 24.f, 40.f, 1010.f));
    r.hm.saveToSdBinary(inRec(T0 + 120, 22.f, 45.f, 1005.f));
    auto b = bytesOf(r.sd, IN_STATS);
    TEST_ASSERT_EQUAL(88, b.size());                         // sizeof(DayStats) fige
    TEST_ASSERT_EQUAL_UINT32(0x53544231u, rd<uint32_t>(b, 0));
    TEST_ASSERT_EQUAL_UINT32(3, rd<uint32_t>(b, 4));
    TEST_ASSERT_EQUAL_UINT32((uint32_t)T0, rd<uint32_t>(b, 8));
    TEST_ASSERT_EQUAL_UINT32((uint32_t)(T0 + 120), rd<uint32_t>(b, 12));
    TEST_ASSERT_EQUAL_FLOAT(20.f, rd<float>(b, 16));   // t_min
    TEST_ASSERT_EQUAL_FLOAT(24.f, rd<float>(b, 20));   // t_max
    TEST_ASSERT_TRUE(std::fabs(66.0 - rd<double>(b, 24)) < 1e-9); // t_sum
    TEST_ASSERT_EQUAL_FLOAT(40.f, rd<float>(b, 32));   // h_min
    TEST_ASSERT_EQUAL_FLOAT(20.f, rd<float>(b, 64));   // t_first
    TEST_ASSERT_EQUAL_FLOAT(22.f, rd<float>(b, 76));   // t_last
    TEST_ASSERT_FALSE(r.sd.exists(std::string(IN_STATS) + ".tmp"));
}

void test_char_in_stats_resume_after_reboot() {
    LegacyRig r;
    r.createTree();
    r.hm.saveToSdBinary(inRec(T0, 20.f, 50.f, 1000.f));
    r.hm.saveToSdBinary(inRec(T0 + 60, 22.f, 50.f, 1000.f));
    // Nouveau gestionnaire (cache vide) : il relit le .stats du jour et poursuit.
    legacy::HistoryManager hm2;
    hm2._sd = &r.mgr;
    hm2.saveToSdBinary(inRec(T0 + 120, 30.f, 50.f, 1000.f));
    auto b = bytesOf(r.sd, IN_STATS);
    TEST_ASSERT_EQUAL_UINT32(3, rd<uint32_t>(b, 4));
    TEST_ASSERT_EQUAL_FLOAT(30.f, rd<float>(b, 20));
    TEST_ASSERT_TRUE(std::fabs(72.0 - rd<double>(b, 24)) < 1e-9);
}

void test_char_in_legacy_headerless_is_upgraded() {
    LegacyRig r;
    r.createTree();
    r.sd.dirs.insert("/history/indoor/2026");
    r.sd.dirs.insert("/history/indoor/2026/11");
    std::vector<uint8_t> raw;
    for (int i = 0; i < 3; i++) {
        uint32_t ts = (uint32_t)(T0 + i * 60);
        float t = 10.f + i, h = 50.f, p = 1000.f;
        raw.insert(raw.end(), (uint8_t*)&ts, (uint8_t*)&ts + 4);
        raw.insert(raw.end(), (uint8_t*)&t, (uint8_t*)&t + 4);
        raw.insert(raw.end(), (uint8_t*)&h, (uint8_t*)&h + 4);
        raw.insert(raw.end(), (uint8_t*)&p, (uint8_t*)&p + 4);
    }
    r.sd.files[IN_BIN] = raw;
    TEST_ASSERT_TRUE(r.hm.saveToSdBinary(inRec(T0 + 180, 13.f, 50.f, 1000.f)));
    auto b = bytesOf(r.sd, IN_BIN);
    TEST_ASSERT_EQUAL(32 + 4 * 16, b.size());
    TEST_ASSERT_EQUAL_MEMORY("MTHB", b.data(), 4);
    TEST_ASSERT_EQUAL_UINT32(4, rd<uint32_t>(b, 12));            // 3 convertis + 1 ajoute
    TEST_ASSERT_TRUE((uint64_t)T0 == rd<uint64_t>(b, 16));
    TEST_ASSERT_EQUAL_UINT32((uint32_t)T0, rd<uint32_t>(b, 32)); // premier enregistrement conserve
    TEST_ASSERT_FALSE(r.sd.exists(std::string(IN_BIN) + ".old"));
    TEST_ASSERT_FALSE(r.sd.exists(std::string(IN_BIN) + ".tmp"));
}

void test_char_in_misaligned_file_is_padded() {
    LegacyRig r;
    r.createTree();
    r.hm.saveToSdBinary(inRec(T0, 20.f, 50.f, 1000.f));
    r.sd.files[IN_BIN].insert(r.sd.files[IN_BIN].end(), 5, 0xAB); // ecriture interrompue
    TEST_ASSERT_TRUE(r.hm.saveToSdBinary(inRec(T0 + 60, 21.f, 50.f, 1000.f)));
    auto b = bytesOf(r.sd, IN_BIN);
    TEST_ASSERT_EQUAL(32 + 16 + 5 + 11 + 16, b.size());
    for (int i = 0; i < 11; i++) TEST_ASSERT_EQUAL_UINT8(0, b[32 + 16 + 5 + i]);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)(T0 + 60), rd<uint32_t>(b, 32 + 16 + 5 + 11));
}

void test_char_in_save_fails_without_card() {
    LegacyRig r;
    // Pas d'arborescence et carte non montable : echec propre, rien d'ecrit.
    r.mgr.mounted = false;
    r.sd.failOpenForWrite = true;
    TEST_ASSERT_FALSE(r.hm.saveToSdBinary(inRec(T0, 20.f, 50.f, 1000.f)));
    TEST_ASSERT_EQUAL(0, r.sd.files.size());
}

// --- Ecriture OUT --------------------------------------------------------------
void test_char_out_fresh_file_is_v2_with_zero_counters() {
    LegacyRig r;
    r.createTree();
    TEST_ASSERT_TRUE(r.hm.saveOutdoorToSdBinary(outRec(T0, 8.f, 80.f, 1005.f), 0));
    auto b = bytesOf(r.sd, OUT_BIN);
    TEST_ASSERT_EQUAL(32 + 20, b.size());
    TEST_ASSERT_EQUAL_UINT16(2, rd<uint16_t>(b, 4));
    TEST_ASSERT_EQUAL_UINT16(20, rd<uint16_t>(b, 8));
    TEST_ASSERT_EQUAL_UINT32(0, rd<uint32_t>(b, 12));   // recordCount jamais mis a jour cote OUT
    TEST_ASSERT_TRUE(0 == rd<uint64_t>(b, 16));
    TEST_ASSERT_TRUE(0 == rd<uint64_t>(b, 24));
    TEST_ASSERT_EQUAL_UINT32((uint32_t)T0, rd<uint32_t>(b, 32));
    TEST_ASSERT_EQUAL_UINT32(0, rd<uint32_t>(b, 48));   // marques
    TEST_ASSERT_TRUE(r.hm.saveOutdoorToSdBinary(outRec(T0 + 30, 8.5f, NAN, NAN), 1));
    b = bytesOf(r.sd, OUT_BIN);
    TEST_ASSERT_EQUAL(32 + 40, b.size());
    TEST_ASSERT_EQUAL_UINT32(1, rd<uint32_t>(b, 32 + 20 + 16)); // marque archivee
    TEST_ASSERT_TRUE(std::isnan(rd<float>(b, 32 + 20 + 8)));    // humidite absente = NaN
    TEST_ASSERT_FALSE(r.sd.exists(OUT_STATS));                  // pas de .stats cote OUT
}

void test_char_out_v1_file_keeps_its_format() {
    LegacyRig r;
    r.createTree();
    r.sd.dirs.insert("/history/outdoor/2026");
    r.sd.dirs.insert("/history/outdoor/2026/11");
    // Journee anterieure a la 1.54.0 : v1 sans en-tete.
    r.sd.files[OUT_BIN] = std::vector<uint8_t>(16 * 2, 0);
    TEST_ASSERT_FALSE(r.hm.saveOutdoorToSdBinary(outRec(T0, 8.f, 80.f, 1005.f), 1)); // marque impossible
    TEST_ASSERT_EQUAL(32, bytesOf(r.sd, OUT_BIN).size());
    TEST_ASSERT_TRUE(r.hm.saveOutdoorToSdBinary(outRec(T0, 8.f, 80.f, 1005.f), 0));
    TEST_ASSERT_EQUAL(48, bytesOf(r.sd, OUT_BIN).size());                 // 16 octets, sans en-tete
}

void test_char_out_misaligned_v2_is_padded() {
    LegacyRig r;
    r.createTree();
    r.hm.saveOutdoorToSdBinary(outRec(T0, 8.f, 80.f, 1005.f), 0);
    r.sd.files[OUT_BIN].insert(r.sd.files[OUT_BIN].end(), 3, 0xCD);
    TEST_ASSERT_TRUE(r.hm.saveOutdoorToSdBinary(outRec(T0 + 30, 9.f, 80.f, 1005.f), 0));
    auto b = bytesOf(r.sd, OUT_BIN);
    TEST_ASSERT_EQUAL(32 + 20 + 3 + 17 + 20, b.size());
    TEST_ASSERT_EQUAL_UINT32((uint32_t)(T0 + 30), rd<uint32_t>(b, 32 + 20 + 3 + 17));
}

// --- Lecture / requetes -----------------------------------------------------------
static void fillIn(LegacyRig& r, int n, float (*tf)(int), float h = 50.f, float p = 1000.f) {
    for (int i = 0; i < n; i++) r.hm.saveToSdBinary(inRec(T0 + i * 60, tf(i), h, p));
}

void test_char_query_range_buckets_and_spike_filter() {
    LegacyRig r;
    r.createTree();
    fillIn(r, 7, [](int i) { return i == 3 ? 35.f : 20.f; });   // pic isole au milieu
    auto pts = r.hm.queryRange(T0, T0 + 420, 420);
    TEST_ASSERT_EQUAL(1, pts.size());
    TEST_ASSERT_TRUE(pts[0].valid && pts[0].tvalid && pts[0].hvalid && pts[0].pvalid);
    TEST_ASSERT_EQUAL_FLOAT(20.f, pts[0].temp);                  // pic ecarte
    TEST_ASSERT_EQUAL_FLOAT(50.f, pts[0].hum);
    TEST_ASSERT_EQUAL_FLOAT(1000.f, pts[0].pres);
    TEST_ASSERT_EQUAL((long)(T0 + 210), (long)pts[0].t);         // centre de tranche
}

void test_char_query_range_gaps_are_emitted_invalid() {
    LegacyRig r;
    r.createTree();
    fillIn(r, 3, [](int i) { return 20.f + i; });
    auto pts = r.hm.queryRange(T0, T0 + 600, 60);
    TEST_ASSERT_EQUAL(10, pts.size());
    TEST_ASSERT_TRUE(pts[0].valid);
    TEST_ASSERT_TRUE(pts[2].valid);
    TEST_ASSERT_FALSE(pts[3].valid);
    TEST_ASSERT_EQUAL_FLOAT(0.f, pts[9].temp);
    TEST_ASSERT_EQUAL(0, r.hm.queryRange(T0 + 10, T0, 60).size());   // to <= from
}

void test_char_query_range_bucket_cap_widens_interval() {
    LegacyRig r;
    r.createTree();
    auto pts = r.hm.queryRange(T0, T0 + 86400 * 2, 1);   // 172800 tranches demandees
    TEST_ASSERT_EQUAL(800, pts.size());                   // plafonne a 800
}

void test_char_query_range_nan_humidity_ignored() {
    LegacyRig r;
    r.createTree();
    for (int i = 0; i < 5; i++) r.hm.saveToSdBinary(inRec(T0 + i * 60, 20.f, NAN, 1000.f));
    auto pts = r.hm.queryRange(T0, T0 + 300, 300);
    TEST_ASSERT_TRUE(pts[0].tvalid);
    TEST_ASSERT_FALSE(pts[0].hvalid);
    TEST_ASSERT_TRUE(pts[0].pvalid);
}

void test_char_query_range_merges_ram_after_sd() {
    LegacyRig r;
    r.createTree();
    fillIn(r, 3, [](int) { return 20.f; });
    // RAM : un point deja sur SD (ignore) et un plus recent (pris).
    r.hm._recentHistory.push_back(inRec(T0 + 60, 99.f, 50.f, 1000.f));
    r.hm._recentHistory.push_back(inRec(T0 + 200, 22.f, 50.f, 1000.f));
    auto pts = r.hm.queryRange(T0, T0 + 240, 240);
    TEST_ASSERT_EQUAL(1, pts.size());
    TEST_ASSERT_EQUAL_FLOAT((20.f * 3 + 22.f) / 4, pts[0].temp);
}

void test_char_query_outdoor_sorted_and_flag_excluded() {
    LegacyRig r;
    r.createTree();
    // Ordre d'arrivee desordonne (retransmission en fin) + une mesure marquee.
    r.hm.saveOutdoorToSdBinary(outRec(T0 + 120, 12.f, 50.f, 1000.f), 0);
    r.hm.saveOutdoorToSdBinary(outRec(T0 + 180, 12.f, 50.f, 1000.f), 0);
    r.hm.saveOutdoorToSdBinary(outRec(T0 + 60, 12.f, 50.f, 1000.f), 0);   // retransmise
    r.hm.saveOutdoorToSdBinary(outRec(T0 + 90, 99.f, 50.f, 1000.f), 1);   // marquee
    auto s = legacy_probe::sortedDay(OUT_BIN, (long)T0, (long)(T0 + 600));
    TEST_ASSERT_EQUAL(3, s.size());
    TEST_ASSERT_EQUAL_UINT32((uint32_t)(T0 + 60), s[0].ts);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)(T0 + 120), s[1].ts);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)(T0 + 180), s[2].ts);
    auto w = legacy_probe::walk(OUT_BIN, 1);
    TEST_ASSERT_EQUAL(3, w.size());
    TEST_ASSERT_EQUAL_UINT32(1, w[2].second);                 // marque exposee par le parcours
    auto pts = r.hm.queryOutdoorRange(T0, T0 + 240, 240);
    TEST_ASSERT_EQUAL(1, pts.size());
    TEST_ASSERT_EQUAL_FLOAT(12.f, pts[0].temp);               // la mesure marquee n'entre pas
}

void test_char_stats24h_window() {
    LegacyRig r;
    const time_t now = time(nullptr);
    for (int k = 0; k < 6; k++)
        r.hm._recentHistory.push_back(inRec(now - 600 - k * 1200, 20.f + k, 50.f, 1000.f));
    r.hm._recentHistory.push_back(inRec(now - 2 * 86400, 5.f, 50.f, 1000.f));   // hors fenetre
    auto s = r.hm.getIndoorStats();
    TEST_ASSERT_EQUAL(6, s.count);
    TEST_ASSERT_EQUAL_FLOAT(20.f, s.temp.min);
    TEST_ASSERT_EQUAL_FLOAT(25.f, s.temp.max);
    TEST_ASSERT_EQUAL(6, s.temp.count);
    TEST_ASSERT_EQUAL(0, r.hm.getOutdoorStats().count);
}

void test_char_trend_from_ram_and_sd() {
    LegacyRig r;
    r.createTree();
    // 30 h de points horaires en RAM : t(k) = 20 - 0.5k (k = heures avant le dernier).
    for (int k = 30; k >= 0; k--)
        r.hm._recentHistory.push_back(inRec(T0 - k * 3600, 20.f - 0.5f * k, 50.f, 1000.f));
    auto tr = r.hm.getTrend();
    TEST_ASSERT_FALSE(tr.available_48h);
    TEST_ASSERT_EQUAL_FLOAT(0.5f, tr.temp.delta_1h);
    TEST_ASSERT_EQUAL_FLOAT(6.f, tr.temp.delta_12h);
    TEST_ASSERT_EQUAL_FLOAT(12.f, tr.temp.delta_24h);
    TEST_ASSERT_EQUAL_STRING("hausse", tr.temp.direction_1h.c_str());
    TEST_ASSERT_EQUAL_STRING("stable", tr.hum.direction_1h.c_str());
    TEST_ASSERT_EQUAL_STRING("indisponible", tr.temp.direction_48h.c_str());
    // Un point sur SD a J-48h : l'indicateur passe a vrai.
    r.hm.saveToSdBinary(inRec(T0 - 172800, 5.f, 40.f, 990.f));
    tr = r.hm.getTrend();
    TEST_ASSERT_TRUE(tr.available_48h);
    TEST_ASSERT_EQUAL_FLOAT(15.f, tr.temp.delta_48h);
    TEST_ASSERT_EQUAL_FLOAT(10.f, tr.hum.delta_48h);
    TEST_ASSERT_EQUAL_FLOAT(10.f, tr.pres.delta_48h);
    // Humidite absente (NaN) : variation neutralisee, pas de faux saut.
    r.hm._recentHistory.back().h = NAN;
    tr = r.hm.getTrend();
    TEST_ASSERT_EQUAL_FLOAT(0.f, tr.hum.delta_1h);
    TEST_ASSERT_EQUAL(0, r.hm.getTrendOutdoor().temp.direction_1h.size());   // RAM OUT vide
}

void test_char_sample_near_tolerance() {
    LegacyRig r;
    r.createTree();
    r.hm.saveToSdBinary(inRec(T0, 20.f, 50.f, 1000.f));
    r.hm.saveToSdBinary(inRec(T0 + 3600, 22.f, 51.f, 1001.f));
    float t, h, p;
    TEST_ASSERT_TRUE(r.hm.readSdSampleNear(T0 + 1000, t, h, p, false));       // 1000 s : dans 30 min
    TEST_ASSERT_EQUAL_FLOAT(20.f, t);
    TEST_ASSERT_TRUE(r.hm.readSdSampleNear(T0 + 3000, t, h, p, false));
    TEST_ASSERT_EQUAL_FLOAT(22.f, t);
    TEST_ASSERT_FALSE(r.hm.readSdSampleNear(T0 + 3600 + 1801, t, h, p, false));  // trop loin
    TEST_ASSERT_TRUE(r.hm.readSdSampleNear(T0 + 3600 + 1800, t, h, p, false));   // borne incluse
    TEST_ASSERT_FALSE(r.hm.readSdSampleNear(T0, t, h, p, true));              // OUT : fichier absent
    r.mgr.mounted = false;
    TEST_ASSERT_FALSE(r.hm.readSdSampleNear(T0, t, h, p, false));
}

void test_char_sample_near_csv_fallback_is_in_only() {
    LegacyRig r;
    r.createTree();
    const std::string csv = "Timestamp,Temperature,Humidity,Pressure\n" + std::to_string((long)T0)
                            + ",18.5,60.0,1010.5\n" + std::to_string((long)T0 + 60) + ",18.6,60.5,1010.6\n";
    r.sd.files["/history/2026-11-05.csv"] = std::vector<uint8_t>(csv.begin(), csv.end());
    float t, h, p;
    TEST_ASSERT_TRUE(r.hm.readSdSampleNear(T0 + 10, t, h, p, false));
    TEST_ASSERT_EQUAL_FLOAT(18.5f, t);
    TEST_ASSERT_FALSE(r.hm.readSdSampleNear(T0 + 10, t, h, p, true));
    auto pts = r.hm.queryRange(T0, T0 + 120, 120);
    TEST_ASSERT_TRUE(pts[0].valid);
}

void test_char_synthesis_from_stats() {
    LegacyRig r;
    r.createTree();
    r.hm.saveToSdBinary(inRec(T0, 20.f, 50.f, 1000.f));
    r.hm.saveToSdBinary(inRec(T0 + 60, 24.f, 40.f, 1010.f));
    r.hm.saveToSdBinary(inRec(T0 + 86400, 10.f, 70.f, 990.f));
    r.hm.saveToSdBinary(inRec(T0 + 86400 + 60, 12.f, 60.f, 995.f));
    auto s = r.hm.querySynthesis(T0 - 100, T0 + 86400 + 1000);
    TEST_ASSERT_TRUE(s.valid);
    TEST_ASSERT_EQUAL(4, s.count);
    TEST_ASSERT_EQUAL_FLOAT(10.f, s.t_min);
    TEST_ASSERT_EQUAL_FLOAT(24.f, s.t_max);
    TEST_ASSERT_EQUAL_FLOAT(16.5f, s.t_avg);
    TEST_ASSERT_EQUAL_FLOAT(20.f, s.t_first);
    TEST_ASSERT_EQUAL_FLOAT(12.f, s.t_last);
    TEST_ASSERT_FALSE(r.hm.querySynthesis(T0 + 10, T0).valid);
}

void test_char_list_days_cache_and_ignored_files() {
    LegacyRig r;
    r.createTree();
    r.hm.saveToSdBinary(inRec(T0, 20.f, 50.f, 1000.f));
    r.hm.saveToSdBinary(inRec(T0 + 60, 21.f, 50.f, 1000.f));
    r.hm.saveToSdBinary(inRec(T0 + 86400, 22.f, 50.f, 1000.f));
    auto d = r.hm.listDays();
    TEST_ASSERT_EQUAL(2, d.size());                           // les .stats ne sont pas des journees
    TEST_ASSERT_EQUAL_UINT32(20261105, d[0].day_key);
    TEST_ASSERT_EQUAL_UINT32(2, d[0].nrec);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)T0, d[0].first_ts);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)(T0 + 60), d[0].last_ts);
    TEST_ASSERT_EQUAL_UINT32(20261106, d[1].day_key);
    // Liste en cache : un nouvel appel n'ouvre plus aucun fichier, une ecriture la tient a jour.
    const unsigned long opens = r.sd.opens;
    r.hm.listDays();
    TEST_ASSERT_EQUAL(opens, r.sd.opens);
    r.hm.saveToSdBinary(inRec(T0 + 120, 23.f, 50.f, 1000.f));
    d = r.hm.listDays();
    TEST_ASSERT_EQUAL_UINT32(3, d[0].nrec);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)(T0 + 120), d[0].last_ts);
    TEST_ASSERT_EQUAL(0, r.hm.listDaysOutdoor().size());
}

void test_char_list_days_outdoor_reads_first_last_from_records() {
    LegacyRig r;
    r.createTree();
    r.hm.saveOutdoorToSdBinary(outRec(T0 + 100, 1.f, 50.f, 1000.f), 0);
    r.hm.saveOutdoorToSdBinary(outRec(T0 + 40, 1.f, 50.f, 1000.f), 0);   // retransmise
    auto d = r.hm.listDaysOutdoor();
    TEST_ASSERT_EQUAL(1, d.size());
    TEST_ASSERT_EQUAL_UINT32(2, d[0].nrec);
    // En-tete OUT a 0 : premier/dernier lus dans les enregistrements, dans l'ordre d'ajout.
    TEST_ASSERT_EQUAL_UINT32((uint32_t)(T0 + 100), d[0].first_ts);
    TEST_ASSERT_EQUAL_UINT32((uint32_t)(T0 + 40), d[0].last_ts);
}

void test_char_export_raw_window_and_flags() {
    LegacyRig r;
    r.createTree();
    for (int i = 0; i < 5; i++)
        r.hm.saveOutdoorToSdBinary(outRec(T0 + i * 30, 10.f + i, 50.f, 1000.f), i == 2 ? 1 : 0);
    std::vector<RawRec> got;
    uint32_t total = r.hm.exportRawOutdoor(20261105, 1, 3, [&](const legacy::RawRecord& rr) {
        got.push_back({rr.ts, rr.t, rr.flags});
    });
    TEST_ASSERT_EQUAL_UINT32(5, total);                      // total du jour, pas du lot
    TEST_ASSERT_EQUAL(3, got.size());
    TEST_ASSERT_EQUAL_UINT32((uint32_t)(T0 + 30), got[0].ts);
    TEST_ASSERT_EQUAL_UINT32(1, got[1].flags);
    TEST_ASSERT_EQUAL_UINT32(0, r.hm.exportRaw(20261105, 0, 10, [](const legacy::RawRecord&) {}));
    TEST_ASSERT_EQUAL_UINT32(0, r.hm.exportRawOutdoor(20260101, 0, 10, [](const legacy::RawRecord&) {}));
}

static std::string csvAll(legacy::HistoryManager& hm, bool outdoor, time_t from, time_t to, size_t chunk) {
    legacy::HistoryManager::CsvCursor c;
    c.outdoor = outdoor; c.from = from; c.to = to; c.day = from;
    std::string out;
    std::vector<char> buf(chunk);
    for (int guard = 0; guard < 100000; guard++) {
        const size_t n = hm.exportCsvChunk(c, buf.data(), chunk);
        if (n == 0) break;
        out.append(buf.data(), n);
    }
    return out;
}

void test_char_export_csv_text_and_chunking() {
    LegacyRig r;
    r.createTree();
    r.hm.saveToSdBinary(inRec(T0, 20.5f, 55.f, 1013.25f));
    r.hm.saveToSdBinary(inRec(T0 + 60, 21.f, NAN, NAN));
    const std::string all = csvAll(r.hm, false, T0 - 10, T0 + 600, 4096);
    std::string expect = "Timestamp,Temperature,Humidity,Pressure\n";
    expect += std::to_string((unsigned long)T0) + ",20.50,55.0,1013.2\n";
    expect += std::to_string((unsigned long)(T0 + 60)) + ",21.00,,\n";
    TEST_ASSERT_EQUAL_STRING(expect.c_str(), all.c_str());
    // Tampon minuscule : le meme texte, jamais une ligne coupee.
    TEST_ASSERT_EQUAL_STRING(expect.c_str(), csvAll(r.hm, false, T0 - 10, T0 + 600, 42).c_str());
    TEST_ASSERT_EQUAL_STRING("Timestamp,Temperature,Humidity,Pressure\n",
                             csvAll(r.hm, true, T0 - 10, T0 + 600, 4096).c_str());   // OUT vide
    TEST_ASSERT_EQUAL_STRING("Timestamp,Temperature,Humidity,Pressure\n",
                             csvAll(r.hm, false, T0 + 10, T0, 4096).c_str());        // plage vide
}

// --- Fichiers recents LittleFS ----------------------------------------------------------
// Sur l'hote time_t fait 64 bits (24 octets par HistoryRecord) alors qu'il en fait 32 sur
// l'ESP32 (16 octets) : on compare donc en nombre d'enregistrements ; la taille de 16 octets
// du firmware est verifiee par un static_assert dans le code refondu.
void test_char_recent_in_roundtrip_and_compaction() {
    LegacyRig r;
    for (int i = 0; i < 3000; i++) {
        legacy::HistoryRecord rec = inRec(T0 + i, 10.f + (i % 10), 50.f, 1000.f);
        r.hm._recentHistory.push_back(rec);
        if (r.hm._recentHistory.size() > MAX_RECENT) r.hm._recentHistory.erase(r.hm._recentHistory.begin());
        r.hm.saveRecent(rec);
    }
    // Seuil de rotation : 2 * 1440 = 2880 enregistrements ; compaction a 2881, puis 119 ajouts.
    TEST_ASSERT_EQUAL(1440 + 119, r.hm._inRecentFileRecords);
    TEST_ASSERT_EQUAL((1440 + 119) * sizeof(legacy::HistoryRecord),
                      bytesOf(r.lfs, "/history/indoor_recent.dat").size());
    TEST_ASSERT_FALSE(r.lfs.exists("/history/indoor_recent.tmp"));
    legacy::HistoryManager h2;
    h2._sd = &r.mgr;
    h2.loadRecent();
    TEST_ASSERT_EQUAL(1440, h2._recentHistory.size());
    TEST_ASSERT_EQUAL((long)(T0 + 3000 - 1), (long)h2._recentHistory.back().timestamp);
    TEST_ASSERT_EQUAL((long)(T0 + 3000 - 1440), (long)h2._recentHistory.front().timestamp);
}

void test_char_recent_out_text_format_and_load() {
    LegacyRig r;
    legacy::OutdoorHistoryRecord rec = outRec(T0, 8.456f, 80.f, NAN);
    r.hm._outdoorHistory.push_back(rec);
    r.hm.saveOutdoorRecent(rec);
    const auto& b = bytesOf(r.lfs, "/history/outdoor_recent.dat");
    const std::string line((const char*)b.data(), b.size());
    char exp[96];
    snprintf(exp, sizeof(exp), "%lu,%.2f,%.2f,%.2f\n", (unsigned long)T0, 8.456f, 80.f, (double)NAN);
    TEST_ASSERT_EQUAL_STRING(exp, line.c_str());
    // Lecture : lignes valides + une ligne illisible comptee dans la base de rotation.
    std::string content = "1000,1.50,2.50,3.50\nn'importe quoi\n2000,4.00,5.00,6.00\n\n";
    r.lfs.files["/history/outdoor_recent.dat"] = std::vector<uint8_t>(content.begin(), content.end());
    legacy::HistoryManager h2;
    h2._sd = &r.mgr;
    h2.loadRecent();
    TEST_ASSERT_EQUAL(2, h2._outdoorHistory.size());
    TEST_ASSERT_EQUAL(3, h2._outRecentFileRecords);
    TEST_ASSERT_EQUAL((long)2000, (long)h2._outdoorHistory[1].timestamp);
    TEST_ASSERT_EQUAL_FLOAT(5.f, h2._outdoorHistory[1].h);
    // Reprise d'affichage apres reboot : dernier point = dernier OUT connu.
    TEST_ASSERT_TRUE(h2._hasLiveOutdoor);
    TEST_ASSERT_EQUAL_FLOAT(4.f, h2._lastOutdoorLive.temperature);
}

void test_char_recent_out_compaction_threshold() {
    LegacyRig r;
    for (int i = 0; i < 2900; i++) {
        legacy::OutdoorHistoryRecord rec = outRec(T0 + i, 10.f, 50.f, 1000.f);
        r.hm._outdoorHistory.push_back(rec);
        if (r.hm._outdoorHistory.size() > MAX_RECENT) r.hm._outdoorHistory.erase(r.hm._outdoorHistory.begin());
        r.hm.saveOutdoorRecent(rec);
    }
    TEST_ASSERT_EQUAL(1440 + 19, r.hm._outRecentFileRecords);
    TEST_ASSERT_FALSE(r.lfs.exists("/history/outdoor_recent.tmp"));
}

void test_char_recent_compaction_failure_keeps_original() {
    LegacyRig r;
    for (int i = 0; i < 10; i++) {
        legacy::OutdoorHistoryRecord rec = outRec(T0 + i, 10.f, 50.f, 1000.f);
        r.hm._outdoorHistory.push_back(rec);
        r.hm.saveOutdoorRecent(rec);
    }
    const auto before = bytesOf(r.lfs, "/history/outdoor_recent.dat");
    r.lfs.quota = 20;                       // partition pleine en cours de compaction
    r.hm.compactOutdoorRecent();
    TEST_ASSERT_TRUE(before == bytesOf(r.lfs, "/history/outdoor_recent.dat"));
    TEST_ASSERT_FALSE(r.lfs.exists("/history/outdoor_recent.tmp"));
}

// --- Effacement par lots ----------------------------------------------------------------
void test_char_clear_in_batches() {
    LegacyRig r;
    r.createTree();
    for (int d = 0; d < 30; d++) {
        r.hm.saveToSdBinary(inRec(T0 + d * 86400, 20.f, 50.f, 1000.f));
        r.hm.saveOutdoorToSdBinary(outRec(T0 + d * 86400, 10.f, 50.f, 1000.f), 0);
    }
    r.lfs.files["/history/indoor_recent.dat"] = {1, 2, 3};
    r.lfs.files["/history/recent.dat"] = {1};
    r.hm.requestClear();
    TEST_ASSERT_TRUE(r.hm.isClearing());
    int passes = 0;
    while (r.hm.isClearing() && passes < 500) { r.hm.service(); passes++; }
    TEST_ASSERT_FALSE(r.hm.isClearing());
    TEST_ASSERT_TRUE(passes > 1);                       // reparti sur plusieurs tours de loop()
    TEST_ASSERT_EQUAL(0, r.sd.files.size());
    TEST_ASSERT_TRUE(r.sd.dirs.count("/history/indoor") && r.sd.dirs.count("/history/outdoor"));
    TEST_ASSERT_EQUAL(0, r.sd.dirs.count("/history/indoor/2026"));
    TEST_ASSERT_EQUAL(0, r.lfs.files.size());           // recents + ancien nom supprimes
}

void test_char_clear_keeps_trying_when_removals_fail() {
    LegacyRig r;
    r.createTree();
    r.hm.saveToSdBinary(inRec(T0, 20.f, 50.f, 1000.f));
    r.sd.removeFails = true;
    r.hm.requestClear();
    int passes = 0;
    while (r.hm.isClearing() && passes < 100) { r.hm.service(); passes++; }
    // Particularite figee (pas un comportement voulu) : le budget baisse meme quand remove()
    // echoue, donc le garde-fou "aucun progres" ne se declenche jamais sur ce scenario et
    // l'effacement continue a chaque tour de loop() tant que la carte refuse les suppressions.
    TEST_ASSERT_TRUE(r.hm.isClearing());
    TEST_ASSERT_EQUAL(100, passes);
}

void test_char_clear_without_sd() {
    LegacyRig r;
    r.mgr.available = false;
    r.hm._recentHistory.push_back(inRec(T0, 1, 1, 1));
    r.hm.requestClear();
    r.hm.service();
    TEST_ASSERT_FALSE(r.hm.isClearing());
    TEST_ASSERT_EQUAL(0, r.hm._recentHistory.size());
}

// --- Migration des anciens CSV --------------------------------------------------------------
void test_char_migrate_csv() {
    LegacyRig r;
    r.createTree();
    const std::string csv = "Timestamp,Temperature,Humidity,Pressure\n" + std::to_string((long)T0)
                            + ",18.5,60.0,1010.5\n" + std::to_string((long)T0 + 60)
                            + ",19.5,61.0,1011.5\nligne foireuse\n";
    r.sd.files["/history/2026-11-05.csv"] = std::vector<uint8_t>(csv.begin(), csv.end());
    // Un 2e CSV dont la journee a deja un .bin : conserve tel quel.
    r.hm.saveToSdBinary(inRec(mkUtc(2026, 11, 4, 8), 1.f, 1.f, 1.f));
    const std::string csv2 = "Timestamp,Temperature,Humidity,Pressure\n"
                             + std::to_string((long)mkUtc(2026, 11, 4, 9)) + ",5.0,5.0,5.0\n";
    r.sd.files["/history/2026-11-04.csv"] = std::vector<uint8_t>(csv2.begin(), csv2.end());
    r.hm.migrateCsvToBinary();
    auto b = bytesOf(r.sd, IN_BIN);
    TEST_ASSERT_EQUAL(32 + 2 * 16, b.size());
    TEST_ASSERT_EQUAL_UINT32(2, rd<uint32_t>(b, 12));
    TEST_ASSERT_EQUAL_UINT32((uint32_t)T0, rd<uint32_t>(b, 32));
    TEST_ASSERT_FALSE(r.sd.exists("/history/2026-11-05.csv"));
    TEST_ASSERT_TRUE(r.sd.exists("/history/2026-11-05.csv.bak"));
    TEST_ASSERT_EQUAL(88, bytesOf(r.sd, IN_STATS).size());
    TEST_ASSERT_TRUE(r.sd.exists("/history/2026-11-04.csv"));        // CSV conserve
    TEST_ASSERT_FALSE(r.sd.exists("/history/2026-11-04.csv.bak"));
}

// --- Previsions archivees -----------------------------------------------------------------------
void test_char_forecast_archive() {
    LegacyRig r;
    r.createTree();
    legacy::ForecastRecord rec;
    rec.target_day = 20261106u;
    rec.issued_ts = 1793793600u;
    rec.temp_min = 3.25f;
    rec.temp_max = 11.5f;
    rec.description = "Pluie \"fine\"\nlegere\\";
    r.hm.addForecast(rec);
    const auto& b = bytesOf(r.sd, "/history/forecast/2026-11-06.json");
    const std::string js((const char*)b.data(), b.size());
    TEST_ASSERT_EQUAL_STRING(
        "{\"target_day\":20261106,\"issued_ts\":1793793600,\"temp_min\":3.2,\"temp_max\":11.5,"
        "\"description\":\"Pluie \\\"fine\\\" legere\\\\\"}", js.c_str());
    rec.target_day = 20261107u; rec.description = "Soleil";
    r.hm.addForecast(rec);
    rec.target_day = 20261201u;
    r.hm.addForecast(rec);
    std::vector<std::string> out;
    r.hm.forecastHistoryRaw(mkUtc(2026, 11, 6), mkUtc(2026, 11, 8), [&](const char* s) { out.push_back(s); });
    TEST_ASSERT_EQUAL(2, out.size());
    TEST_ASSERT_NOT_NULL(strstr(out[0].c_str(), "20261106"));
    TEST_ASSERT_NOT_NULL(strstr(out[1].c_str(), "20261107"));
    // Reecriture : la derniere prevision emise pour un jour remplace la precedente.
    rec.target_day = 20261106u; rec.description = "Neige";
    r.hm.addForecast(rec);
    const auto& b2 = bytesOf(r.sd, "/history/forecast/2026-11-06.json");
    TEST_ASSERT_NOT_NULL(strstr(std::string((const char*)b2.data(), b2.size()).c_str(), "Neige"));
}

// --- Ecritures OUT : regles d'entree ------------------------------------------------------------
void test_char_out_entry_rules() {
    LegacyRig r;
    r.createTree();
    OutdoorData d;
    d.valid = true; d.temperature = 9.f; d.humidity = 0.f; d.pressure = 0.f;
    d.has_humidity = false; d.has_pressure = false;
    TEST_ASSERT_FALSE(r.hm.addOutdoorLive(d, 1000));                      // heure implausible
    TEST_ASSERT_TRUE(r.hm.hasLiveOutdoor());                              // le live est tout de meme pris
    TEST_ASSERT_TRUE(r.hm.addOutdoorLive(d, T0));
    auto b = bytesOf(r.sd, OUT_BIN);
    TEST_ASSERT_EQUAL(52, b.size());
    TEST_ASSERT_TRUE(std::isnan(rd<float>(b, 32 + 8)));                   // humidite non mesuree : NaN
    TEST_ASSERT_TRUE(std::isnan(rd<float>(b, 32 + 12)));
    TEST_ASSERT_EQUAL(1, r.hm._outdoorHistory.size());
    TEST_ASSERT_TRUE(r.lfs.exists("/history/outdoor_recent.dat"));
    OutdoorData bad;
    bad.valid = false;
    TEST_ASSERT_TRUE(r.hm.addOutdoorLive(bad, T0));                       // rien a archiver
    TEST_ASSERT_TRUE(r.hm.addOutdoorHistorical(d, T0 + 30));              // archive seule
    TEST_ASSERT_EQUAL(1, r.hm._outdoorHistory.size());                    // pas de RAM pour une ancienne mesure
    TEST_ASSERT_EQUAL(72, bytesOf(r.sd, OUT_BIN).size());
    r.sd.failOpenForWrite = true;
    TEST_ASSERT_FALSE(r.hm.addOutdoorLive(d, T0 + 60));                   // echec SD : ni RAM ni recent
    TEST_ASSERT_EQUAL(1, r.hm._outdoorHistory.size());
    r.sd.failOpenForWrite = false;
    TEST_ASSERT_TRUE(r.hm.addOutdoorFlagged(d, T0 + 90, 1));
    TEST_ASSERT_EQUAL(1, r.hm._outdoorHistory.size());                    // une mesure marquee n'est pas affichee
    r.mgr.available = false;
    TEST_ASSERT_FALSE(r.hm.addOutdoorHistorical(d, T0 + 120));
}

// --- Cessions de la main (anti TASK_WDT) -----------------------------------------------------------
void test_char_yields_are_present_in_long_loops() {
    LegacyRig r;
    r.createTree();
    for (int i = 0; i < 700; i++) r.hm.saveToSdBinary(inRec(T0 + i * 10, 20.f, 50.f, 1000.f));
    g_legacyYieldCalls = 0;
    r.hm.queryRange(T0, T0 + 7000, 100);
    TEST_ASSERT_TRUE(g_legacyYieldCalls >= 2);   // un par jour parcouru + un par bloc lu
    g_legacyYieldCalls = 0;
    r.hm._sd = &r.mgr;
    r.hm.invalidateDaysCache();
    r.hm.listDays();
    TEST_ASSERT_TRUE(g_legacyYieldCalls >= 1);   // un yield par jour liste
}

void run_characterization() {
    RUN_TEST(test_char_header_layout);
    RUN_TEST(test_char_pad_to_boundary);
    RUN_TEST(test_char_probe_variants);
    RUN_TEST(test_char_in_append_creates_header_and_updates_counters);
    RUN_TEST(test_char_in_stats_file);
    RUN_TEST(test_char_in_stats_resume_after_reboot);
    RUN_TEST(test_char_in_legacy_headerless_is_upgraded);
    RUN_TEST(test_char_in_misaligned_file_is_padded);
    RUN_TEST(test_char_in_save_fails_without_card);
    RUN_TEST(test_char_out_fresh_file_is_v2_with_zero_counters);
    RUN_TEST(test_char_out_v1_file_keeps_its_format);
    RUN_TEST(test_char_out_misaligned_v2_is_padded);
    RUN_TEST(test_char_query_range_buckets_and_spike_filter);
    RUN_TEST(test_char_query_range_gaps_are_emitted_invalid);
    RUN_TEST(test_char_query_range_bucket_cap_widens_interval);
    RUN_TEST(test_char_query_range_nan_humidity_ignored);
    RUN_TEST(test_char_query_range_merges_ram_after_sd);
    RUN_TEST(test_char_query_outdoor_sorted_and_flag_excluded);
    RUN_TEST(test_char_stats24h_window);
    RUN_TEST(test_char_trend_from_ram_and_sd);
    RUN_TEST(test_char_sample_near_tolerance);
    RUN_TEST(test_char_sample_near_csv_fallback_is_in_only);
    RUN_TEST(test_char_synthesis_from_stats);
    RUN_TEST(test_char_list_days_cache_and_ignored_files);
    RUN_TEST(test_char_list_days_outdoor_reads_first_last_from_records);
    RUN_TEST(test_char_export_raw_window_and_flags);
    RUN_TEST(test_char_export_csv_text_and_chunking);
    RUN_TEST(test_char_recent_in_roundtrip_and_compaction);
    RUN_TEST(test_char_recent_out_text_format_and_load);
    RUN_TEST(test_char_recent_out_compaction_threshold);
    RUN_TEST(test_char_recent_compaction_failure_keeps_original);
    RUN_TEST(test_char_clear_in_batches);
    RUN_TEST(test_char_clear_keeps_trying_when_removals_fail);
    RUN_TEST(test_char_clear_without_sd);
    RUN_TEST(test_char_migrate_csv);
    RUN_TEST(test_char_forecast_archive);
    RUN_TEST(test_char_out_entry_rules);
    RUN_TEST(test_char_yields_are_present_in_long_loops);
}
