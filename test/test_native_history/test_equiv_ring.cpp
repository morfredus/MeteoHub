// Equivalence des tampons recents (RecentRing IN / OUT) avec le code de reference : memes
// fichiers LittleFS, memes compactions, memes rechargements.
//
// Sur l'hote time_t fait 64 bits (HistoryRecord de reference = 24 octets) alors que le firmware
// ecrit 16 octets : le fichier IN est donc compare champ a champ, le format 16 octets etant
// fige par un test d'octets dedie et par des static_assert sur le firmware.
#include <cmath>
#include <mutex>
#include "test_util.h"
#include "mem_fs.h"
#include "history/recent_ring.h"

static const time_t T0 = mkUtc(2026, 11, 5, 12, 0, 0);
static const size_t CAP = 1440;
static const size_t THR = 2 * 1440;
static const char* IN_FILE = "/history/indoor_recent.dat";
static const char* OUT_FILE = "/history/outdoor_recent.dat";

struct RingRig {
    MemVolume lfs;
    MemFs fs;
    std::mutex m;
    mhhist::InRecentRing in;
    mhhist::OutRecentRing out;
    RingRig()
        : fs(lfs),
          in(fs, m, IN_FILE, "/history/indoor_recent.tmp", CAP, THR),
          out(fs, m, OUT_FILE, "/history/outdoor_recent.tmp", CAP, THR) {
        lfs.renameReplaces = true;
        lfs.dirs.insert("/history");
    }
};

static bool sameFloat(float a, float b) {
    return (std::isnan(a) && std::isnan(b)) || memcmp(&a, &b, sizeof(float)) == 0;
}

// --- Format IN fige octet par octet ---------------------------------------------------------
void test_eqring_in_bytes_are_the_historic_16_byte_struct() {
    RingRig r;
    HistoryRecord a;
    a.timestamp = (time_t)0x65432100; a.t = 1.5f; a.h = 2.5f; a.p = 3.5f;
    r.in.push(a);
    r.in.persist(a);
    const auto& b = bytesOf(r.lfs, IN_FILE);
    TEST_ASSERT_EQUAL(16, b.size());
    // int32 little-endian (time_t 32 bits du firmware) puis trois float IEEE 754.
    const uint8_t expect[16] = {0x00, 0x21, 0x43, 0x65,
                                0x00, 0x00, 0xC0, 0x3F,    // 1.5f
                                0x00, 0x00, 0x20, 0x40,    // 2.5f
                                0x00, 0x00, 0x60, 0x40};   // 3.5f
    TEST_ASSERT_EQUAL_MEMORY(expect, b.data(), 16);
}

void test_eqring_in_load_ignores_partial_trailing_record() {
    RingRig r;
    std::vector<uint8_t> f;
    for (int i = 0; i < 3; i++) {
        mhhist::InRecentDisk d{(int32_t)(1000 + i), 1.f * i, 2.f, 3.f};
        f.insert(f.end(), (uint8_t*)&d, (uint8_t*)&d + sizeof(d));
    }
    f.insert(f.end(), 7, 0xEE);                       // coupure en plein enregistrement
    r.lfs.files[IN_FILE] = f;
    r.in.purgeTmp();
    r.in.load();
    TEST_ASSERT_EQUAL(3, r.in.items().size());
    TEST_ASSERT_EQUAL(3, r.in.fileRecords());
    TEST_ASSERT_EQUAL((long)1002, (long)r.in.items().back().timestamp);
}

// --- Scenarios comparatifs --------------------------------------------------------------------
struct Rng {
    uint32_t s;
    explicit Rng(uint32_t seed) : s(seed ? seed : 1) {}
    uint32_t next() { s = s * 1664525u + 1013904223u; return s >> 8; }
    uint32_t below(uint32_t n) { return next() % n; }
};

static std::vector<legacy::HistoryRecord> legacyInFile(const MemVolume& v) {
    std::vector<legacy::HistoryRecord> out;
    const auto& b = bytesOf(v, IN_FILE);
    for (size_t off = 0; off + sizeof(legacy::HistoryRecord) <= b.size(); off += sizeof(legacy::HistoryRecord)) {
        legacy::HistoryRecord r;
        memcpy(&r, b.data() + off, sizeof(r));
        out.push_back(r);
    }
    return out;
}
static std::vector<mhhist::InRecentDisk> newInFile(const MemVolume& v) {
    std::vector<mhhist::InRecentDisk> out;
    const auto& b = bytesOf(v, IN_FILE);
    for (size_t off = 0; off + sizeof(mhhist::InRecentDisk) <= b.size(); off += sizeof(mhhist::InRecentDisk)) {
        mhhist::InRecentDisk r;
        memcpy(&r, b.data() + off, sizeof(r));
        out.push_back(r);
    }
    return out;
}

static void assertSameIn(const std::vector<legacy::HistoryRecord>& a, const std::vector<HistoryRecord>& b) {
    TEST_ASSERT_EQUAL(a.size(), b.size());
    for (size_t i = 0; i < a.size(); i++) {
        TEST_ASSERT_TRUE((long)a[i].timestamp == (long)b[i].timestamp);
        TEST_ASSERT_TRUE(sameFloat(a[i].t, b[i].t) && sameFloat(a[i].h, b[i].h) && sameFloat(a[i].p, b[i].p));
    }
}
static void assertSameOut(const std::vector<legacy::OutdoorHistoryRecord>& a, const std::vector<OutdoorHistoryRecord>& b) {
    TEST_ASSERT_EQUAL(a.size(), b.size());
    for (size_t i = 0; i < a.size(); i++) {
        TEST_ASSERT_TRUE((long)a[i].timestamp == (long)b[i].timestamp);
        TEST_ASSERT_TRUE(sameFloat(a[i].t, b[i].t) && sameFloat(a[i].h, b[i].h) && sameFloat(a[i].p, b[i].p));
    }
}

static float val(Rng& r, float base) {
    return r.below(25) == 0 ? NAN : base + (float)r.below(500) / 10.f;
}

void test_eqring_in_scenarios() {
    for (uint32_t seed : {1u, 2u, 3u}) {
        LegacyRig L;
        RingRig N;
        Rng r(seed);
        for (int i = 0; i < 3300; i++) {
            const bool fault = r.below(40) == 0;
            L.lfs.failOpenForWrite = N.lfs.failOpenForWrite = fault;
            const time_t ts = T0 + i * 7;
            const float t = val(r, 5.f), h = val(r, 30.f), p = val(r, 980.f);
            // Reference : meme enchainement que add().
            legacy::HistoryRecord lr = inRec(ts, t, h, p);
            L.hm._recentHistory.push_back(lr);
            if (L.hm._recentHistory.size() > CAP) L.hm._recentHistory.erase(L.hm._recentHistory.begin());
            L.hm.saveRecent(lr);
            HistoryRecord nr;
            nr.timestamp = ts; nr.t = t; nr.h = h; nr.p = p;
            N.in.push(nr);
            N.in.persist(nr);
        }
        L.lfs.failOpenForWrite = N.lfs.failOpenForWrite = false;
        assertSameIn(L.hm._recentHistory, N.in.items());
        TEST_ASSERT_EQUAL(L.hm._inRecentFileRecords, N.in.fileRecords());
        // Fichier : memes enregistrements, dans le meme ordre.
        const auto la = legacyInFile(L.lfs);
        const auto nb = newInFile(N.lfs);
        TEST_ASSERT_EQUAL(la.size(), nb.size());
        for (size_t i = 0; i < la.size(); i++) {
            TEST_ASSERT_TRUE((long)la[i].timestamp == (long)nb[i].ts);
            TEST_ASSERT_TRUE(sameFloat(la[i].t, nb[i].t) && sameFloat(la[i].h, nb[i].h) && sameFloat(la[i].p, nb[i].p));
        }
        TEST_ASSERT_EQUAL(L.lfs.files.count("/history/indoor_recent.tmp"), N.lfs.files.count("/history/indoor_recent.tmp"));
        // Rechargement apres redemarrage.
        legacy::HistoryManager h2;
        h2._sd = &L.mgr;
        h2.loadRecent();
        RingRig N2;
        N2.lfs.files = N.lfs.files;
        N2.in.purgeTmp();
        N2.in.load();
        assertSameIn(h2._recentHistory, N2.in.items());
        TEST_ASSERT_EQUAL(h2._inRecentFileRecords, N2.in.fileRecords());
    }
}

void test_eqring_out_scenarios() {
    for (uint32_t seed : {4u, 5u, 6u}) {
        LegacyRig L;
        RingRig N;
        Rng r(seed);
        for (int i = 0; i < 3300; i++) {
            // Pannes : ouverture refusee ou partition pleine pendant l'ajout / la compaction.
            L.lfs.failOpenForWrite = N.lfs.failOpenForWrite = false;
            L.lfs.quota = N.lfs.quota = -1;
            const uint32_t f = r.below(60);
            if (f == 0) L.lfs.failOpenForWrite = N.lfs.failOpenForWrite = true;
            else if (f == 1) L.lfs.quota = N.lfs.quota = (long)r.below(200);
            const time_t ts = T0 + i * 11;
            const float t = val(r, 5.f), h = val(r, 30.f), p = val(r, 980.f);
            legacy::OutdoorHistoryRecord lr = outRec(ts, t, h, p);
            L.hm._outdoorHistory.push_back(lr);
            if (L.hm._outdoorHistory.size() > CAP) L.hm._outdoorHistory.erase(L.hm._outdoorHistory.begin());
            L.hm.saveOutdoorRecent(lr);
            OutdoorHistoryRecord nr{};
            nr.timestamp = ts; nr.t = t; nr.h = h; nr.p = p;
            N.out.push(nr);
            N.out.persist(nr);
        }
        L.lfs.failOpenForWrite = N.lfs.failOpenForWrite = false;
        L.lfs.quota = N.lfs.quota = -1;
        assertSameOut(L.hm._outdoorHistory, N.out.items());
        TEST_ASSERT_EQUAL(L.hm._outRecentFileRecords, N.out.fileRecords());
        // Fichier texte : IDENTIQUE octet pour octet.
        TEST_ASSERT_TRUE(bytesOf(L.lfs, OUT_FILE) == bytesOf(N.lfs, OUT_FILE));
        TEST_ASSERT_EQUAL(L.lfs.files.size(), N.lfs.files.size());
        // Rechargement.
        legacy::HistoryManager h2;
        h2._sd = &L.mgr;
        h2.loadRecent();
        RingRig N2;
        N2.lfs.files = N.lfs.files;
        N2.out.purgeTmp();
        N2.out.load();
        assertSameOut(h2._outdoorHistory, N2.out.items());
        TEST_ASSERT_EQUAL(h2._outRecentFileRecords, N2.out.fileRecords());
    }
}

void test_eqring_out_load_handles_odd_files() {
    // Lignes illisibles, vides, sans fin de ligne finale, avec \r, champs manquants.
    const std::string content =
        "1000,1.50,2.50,3.50\n\nn'importe quoi\n,1,2,3\n1,2,3\n2000,4.00,nan,6.00\r\n"
        "3000,7.00,8.00,9.00";
    LegacyRig L;
    RingRig N;
    L.lfs.files[OUT_FILE] = N.lfs.files[OUT_FILE] = std::vector<uint8_t>(content.begin(), content.end());
    legacy::HistoryManager h2;
    h2._sd = &L.mgr;
    h2.loadRecent();
    N.out.purgeTmp();
    N.out.load();
    assertSameOut(h2._outdoorHistory, N.out.items());
    TEST_ASSERT_EQUAL(h2._outRecentFileRecords, N.out.fileRecords());
    TEST_ASSERT_EQUAL(3, N.out.items().size());
}

void test_eqring_load_compacts_oversized_files_and_purges_tmp() {
    // Fichiers ayant grossi sans borne (firmware anterieur a la rotation) + temporaire orphelin.
    LegacyRig L;
    RingRig N;
    std::string out;
    for (int i = 0; i < 4000; i++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%ld,%.2f,%.2f,%.2f\n", (long)(T0 + i), 1.f + i % 9, 50.f, 1000.f);
        out += buf;
    }
    L.lfs.files[OUT_FILE] = N.lfs.files[OUT_FILE] = std::vector<uint8_t>(out.begin(), out.end());
    L.lfs.files["/history/outdoor_recent.tmp"] = N.lfs.files["/history/outdoor_recent.tmp"] = {1, 2, 3};
    legacy::HistoryManager h2;
    h2._sd = &L.mgr;
    h2.loadRecent();
    N.out.purgeTmp();
    N.out.load();
    TEST_ASSERT_EQUAL(1440, N.out.items().size());
    TEST_ASSERT_EQUAL(1440, N.out.fileRecords());
    TEST_ASSERT_TRUE(bytesOf(L.lfs, OUT_FILE) == bytesOf(N.lfs, OUT_FILE));
    TEST_ASSERT_EQUAL(L.lfs.files.size(), N.lfs.files.size());
    TEST_ASSERT_EQUAL(0, N.lfs.files.count("/history/outdoor_recent.tmp"));
}

void test_eqring_clear_removes_ram_and_files() {
    RingRig N;
    OutdoorHistoryRecord nr{};
    nr.timestamp = T0;
    N.out.push(nr);
    N.out.persist(nr);
    N.lfs.files["/history/outdoor_recent.tmp"] = {1};
    TEST_ASSERT_EQUAL(1, N.out.fileRecords());
    N.out.clear();
    TEST_ASSERT_EQUAL(0, N.out.items().size());
    TEST_ASSERT_EQUAL(0, N.out.fileRecords());
    TEST_ASSERT_EQUAL(0, N.lfs.files.size());
}

void run_equiv_ring() {
    RUN_TEST(test_eqring_in_bytes_are_the_historic_16_byte_struct);
    RUN_TEST(test_eqring_in_load_ignores_partial_trailing_record);
    RUN_TEST(test_eqring_in_scenarios);
    RUN_TEST(test_eqring_out_scenarios);
    RUN_TEST(test_eqring_out_load_handles_odd_files);
    RUN_TEST(test_eqring_load_compacts_oversized_files_and_purges_tmp);
    RUN_TEST(test_eqring_clear_removes_ram_and_files);
}
