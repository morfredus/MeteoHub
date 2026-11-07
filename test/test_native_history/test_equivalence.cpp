// Tests d'EQUIVALENCE : le code refondu (src/history/) doit produire exactement les memes
// octets et les memes resultats que le code de reference de la 1.62.0 (legacy_ref.h), sur
// les memes entrees. Chaque test rejoue un scenario des deux cotes et compare.
#include "test_util.h"
#include "history/day_file_format.h"

static const time_t T0 = mkUtc(2026, 11, 5, 12, 0, 0);
static const char* IN_BIN = "/history/indoor/2026/11/2026-11-05.bin";
static const char* OUT_BIN = "/history/outdoor/2026/11/2026-11-05.bin";

// Comparaison octet pour octet de deux volumes (memes fichiers, memes dossiers).
static void assertSameVolume(const MemVolume& a, const MemVolume& b) {
    TEST_ASSERT_TRUE(a.dirs == b.dirs);
    TEST_ASSERT_EQUAL(a.files.size(), b.files.size());
    for (const auto& kv : a.files) {
        auto it = b.files.find(kv.first);
        TEST_ASSERT_TRUE_MESSAGE(it != b.files.end(), kv.first.c_str());
        TEST_ASSERT_TRUE_MESSAGE(kv.second == it->second, kv.first.c_str());
    }
}

// --- Etape 1 : day_file_format ------------------------------------------------------
void test_eq_header_bytes() {
    for (bool flags : {false, true}) {
        uint8_t ref[32];
        legacy_probe::header(flags, ref);
        const mhhist::FileHeader h = mhhist::makeHeader(flags);
        TEST_ASSERT_EQUAL_MEMORY(ref, &h, 32);
    }
}

void test_eq_pad_grid() {
    for (size_t fsz = 0; fsz < 200; fsz++)
        for (size_t off : {0, 32})
            for (size_t rs : {0, 16, 20, 28})
                TEST_ASSERT_EQUAL(legacy_probe::pad(fsz, off, rs), mhhist::padToRecordBoundary(fsz, off, rs));
}

// Volume de fichiers varies : en-tete v1/v2, sans en-tete, corrompu, tronque, vide...
static void buildFormatZoo(LegacyRig& r) {
    r.createTree();
    for (int i = 0; i < 1000; i++) r.hm.saveToSdBinary(inRec(T0 + i * 10, 15.f + (i % 7), 50.f, 1000.f));
    // OUT v2 : desordre, marques, NaN.
    const int order[] = {5, 3, 4, 9, 1, 2, 8, 7, 6, 0};
    for (int k : order)
        r.hm.saveOutdoorToSdBinary(outRec(T0 + k * 30, 8.f + k, k % 3 ? 70.f : NAN, 1005.f), k == 4 ? 1 : 0);
    // OUT longue (plusieurs blocs de 1 Ko en v2 : 64 enregistrements par bloc).
    for (int i = 0; i < 200; i++)
        r.hm.saveOutdoorToSdBinary(outRec(T0 + 86400 + (i * 37) % 3000, 1.f, 50.f, 1000.f), 0);
    r.sd.files["/zoo/empty.bin"] = {};
    r.sd.files["/zoo/headeronly.bin"] = std::vector<uint8_t>(32, 0);
    r.sd.files["/zoo/legacy.bin"] = std::vector<uint8_t>(16 * 5 + 3, 7);
    std::vector<uint8_t> bad(100, 1);
    memcpy(bad.data(), "XXXX", 4);
    r.sd.files["/zoo/bad.bin"] = bad;
    // En-tete valide mais fichier tronque en plein milieu d'un enregistrement.
    r.sd.files["/zoo/trunc.bin"] = r.sd.files[OUT_BIN];
    r.sd.files["/zoo/trunc.bin"].resize(32 + 20 * 4 + 9);
    // En-tete avec headerSize nul et recordSize inferieur a 16.
    uint8_t h[32];
    legacy_probe::header(false, h);
    memset(h + 6, 0, 2);
    uint16_t small = 4;
    memcpy(h + 8, &small, 2);
    std::vector<uint8_t> odd(h, h + 32);
    odd.resize(32 + 48);
    r.sd.files["/zoo/odd.bin"] = odd;
    r.sd.dirs.insert("/zoo");
}

void test_eq_layout_reads_and_walks_match() {
    LegacyRig r;
    buildFormatZoo(r);
    std::vector<std::string> paths;
    for (const auto& kv : r.sd.files)
        if (kv.first.size() > 4 && kv.first.compare(kv.first.size() - 4, 4, ".bin") == 0)
            paths.push_back(kv.first);
    TEST_ASSERT_TRUE(paths.size() >= 8);
    for (const std::string& path : paths) {
        const auto L = legacy_probe::probe(path.c_str());
        File f = SD.open(path.c_str(), FILE_READ);
        const mhhist::BinLayout N = mhhist::probeBin(f);
        TEST_ASSERT_EQUAL_MESSAGE(L.dataOffset, N.dataOffset, path.c_str());
        TEST_ASSERT_EQUAL_MESSAGE(L.recordSize, N.recordSize, path.c_str());
        TEST_ASSERT_EQUAL_MESSAGE(L.nrec, N.nrec, path.c_str());
        // Lecture a chaque index, y compris hors du fichier.
        for (size_t i = 0; i <= L.nrec + 1; i += (L.nrec > 50 ? 7 : 1)) {
            uint32_t ts; float t, h, p;
            const bool okRef = legacy_probe::readAt(path.c_str(), i, ts, t, h, p);
            mhhist::BinRecord br{};
            const bool okNew = mhhist::readBinRecordAt(f, N, i, br);
            TEST_ASSERT_EQUAL_MESSAGE(okRef, okNew, path.c_str());
            if (okRef) {
                TEST_ASSERT_EQUAL_UINT32(ts, br.ts);
                TEST_ASSERT_EQUAL_MEMORY(&t, &br.t, 4);
                TEST_ASSERT_EQUAL_MEMORY(&h, &br.h, 4);
            }
        }
        // Parcours sequentiel depuis plusieurs index : memes enregistrements, memes marques,
        // MEME nombre de cessions de la main.
        for (size_t start : {size_t(0), size_t(1), L.nrec / 2, L.nrec ? L.nrec - 1 : 0, L.nrec, L.nrec + 3}) {
            g_legacyYieldCalls = 0;
            const auto ref = legacy_probe::walk(path.c_str(), start);
            const unsigned long yRef = g_legacyYieldCalls;
            mhhist::testYieldCalls() = 0;
            std::vector<std::pair<uint32_t, uint32_t>> got;
            mhhist::forEachBinRecordFrom(f, N, start, [&](const mhhist::BinRecord& rr, uint32_t fl) {
                got.emplace_back(rr.ts, fl);
                return true;
            });
            TEST_ASSERT_TRUE_MESSAGE(ref == got, path.c_str());
            TEST_ASSERT_EQUAL_MESSAGE(yRef, mhhist::testYieldCalls(), path.c_str());
        }
        // Jour trie, plage etroite et plage large.
        for (long span : {200L, 1000000L}) {
            const auto ref = legacy_probe::sortedDay(path.c_str(), (long)T0 + 30, (long)T0 + span);
            std::vector<mhhist::BinRecord> got;
            mhhist::collectSortedDay(f, N, (long)T0 + 30, (long)T0 + span, got);
            TEST_ASSERT_EQUAL_MESSAGE(ref.size(), got.size(), path.c_str());
            for (size_t i = 0; i < ref.size(); i++) {
                TEST_ASSERT_EQUAL_UINT32(ref[i].ts, got[i].ts);
                TEST_ASSERT_EQUAL_MEMORY(&ref[i].t, &got[i].t, 4);
            }
        }
        f.close();
    }
}

void test_eq_walk_stops_when_callback_says_so() {
    LegacyRig r;
    buildFormatZoo(r);
    File f = SD.open(IN_BIN, FILE_READ);
    const mhhist::BinLayout N = mhhist::probeBin(f);
    size_t seen = 0;
    mhhist::forEachBinRecordFrom(f, N, 10, [&](const mhhist::BinRecord&, uint32_t) { return ++seen < 5; });
    TEST_ASSERT_EQUAL(5, seen);
}

// appendRecord : meme fichier final que le code de reference, fichier neuf (OUT v2) puis
// fichier desaligne (bourrage), v1 et v2.
static std::vector<uint8_t> v2Record(time_t ts, float t, float h, float p, uint32_t flags) {
    mhhist::BinRecordV2 rec{};
    rec.base.ts = (uint32_t)ts; rec.base.t = t; rec.base.h = h; rec.base.p = p; rec.flags = flags;
    return std::vector<uint8_t>((uint8_t*)&rec, (uint8_t*)&rec + sizeof(rec));
}

void test_eq_append_fresh_and_padded() {
    // Cote reference : fichier neuf, deux enregistrements, puis desalignement de 3 octets.
    LegacyRig ref;
    ref.createTree();
    ref.hm.saveOutdoorToSdBinary(outRec(T0, 8.f, 80.f, 1005.f), 0);
    const std::vector<uint8_t> afterFirst = bytesOf(ref.sd, OUT_BIN);
    ref.hm.saveOutdoorToSdBinary(outRec(T0 + 30, 9.f, NAN, 1004.f), 1);
    const std::vector<uint8_t> afterSecond = bytesOf(ref.sd, OUT_BIN);
    ref.sd.files[OUT_BIN].insert(ref.sd.files[OUT_BIN].end(), 3, 0xCD);
    ref.hm.saveOutdoorToSdBinary(outRec(T0 + 60, 10.f, 80.f, 1003.f), 0);
    const std::vector<uint8_t> afterPad = bytesOf(ref.sd, OUT_BIN);

    // Cote refondu : memes operations avec mhhist::appendRecord.
    MemVolume nv;
    nv.dirs.insert("/history"); nv.dirs.insert("/history/outdoor");
    nv.dirs.insert("/history/outdoor/2026"); nv.dirs.insert("/history/outdoor/2026/11");
    ShimFs fs;
    fs.vol = &nv;
    {
        File f = fs.open(OUT_BIN, "a");
        const mhhist::FileHeader h = mhhist::makeHeader(true);
        const auto rec = v2Record(T0, 8.f, 80.f, 1005.f, 0);
        auto out = mhhist::appendRecord(f, &h, 0, 0, rec.data(), rec.size());
        TEST_ASSERT_TRUE(out.ok);
        TEST_ASSERT_EQUAL(0, out.padSeen);
    }
    TEST_ASSERT_TRUE(afterFirst == bytesOf(nv, OUT_BIN));
    {
        File f = fs.open(OUT_BIN, "a");
        const auto rec = v2Record(T0 + 30, 9.f, NAN, 1004.f, 1);
        auto out = mhhist::appendRecord(f, nullptr, 32, 20, rec.data(), rec.size());
        TEST_ASSERT_TRUE(out.ok);
    }
    TEST_ASSERT_TRUE(afterSecond == bytesOf(nv, OUT_BIN));
    nv.files[OUT_BIN].insert(nv.files[OUT_BIN].end(), 3, 0xCD);
    {
        File f = fs.open(OUT_BIN, "a");
        const auto rec = v2Record(T0 + 60, 10.f, 80.f, 1003.f, 0);
        auto out = mhhist::appendRecord(f, nullptr, 32, 20, rec.data(), rec.size());
        TEST_ASSERT_TRUE(out.ok);
        TEST_ASSERT_EQUAL(17, out.padSeen);
        TEST_ASSERT_EQUAL(17, out.padWritten);
    }
    TEST_ASSERT_TRUE(afterPad == bytesOf(nv, OUT_BIN));
}

void test_eq_append_pad_guard_and_short_write() {
    MemVolume nv;
    nv.dirs.insert("/d");
    ShimFs fs;
    fs.vol = &nv;
    // Desalignement de 70 octets (> 32) avec un recordSize anormal : pas de bourrage ecrit.
    nv.files["/d/x.bin"] = std::vector<uint8_t>(32 + 1, 1);
    {
        File f = fs.open("/d/x.bin", "a");
        uint8_t rec[16] = {0};
        auto out = mhhist::appendRecord(f, nullptr, 32, 100, rec, sizeof(rec));
        TEST_ASSERT_EQUAL(99, out.padSeen);
        TEST_ASSERT_EQUAL(0, out.padWritten);
        TEST_ASSERT_EQUAL(32 + 1 + 16, nv.files["/d/x.bin"].size());
    }
    // Ecriture refusee a moitie (carte pleine) : ok = false.
    nv.quota = 5;
    {
        File f = fs.open("/d/x.bin", "a");
        uint8_t rec[16] = {0};
        auto out = mhhist::appendRecord(f, nullptr, 0, 16, rec, sizeof(rec));
        TEST_ASSERT_FALSE(out.ok);
    }
}

void run_equivalence() {
    RUN_TEST(test_eq_header_bytes);
    RUN_TEST(test_eq_pad_grid);
    RUN_TEST(test_eq_layout_reads_and_walks_match);
    RUN_TEST(test_eq_walk_stops_when_callback_says_so);
    RUN_TEST(test_eq_append_fresh_and_padded);
    RUN_TEST(test_eq_append_pad_guard_and_short_write);
}
