#pragma once
// Elements communs aux tests d'equivalence : banc "refondu" (volume + magasins IN/OUT),
// generateur pseudo-aleatoire, comparaison de volumes, fichiers d'avant les evolutions du format.
#include <cmath>
#include <cstring>
#include "test_util.h"
#include "mem_fs.h"
#include "history/day_store.h"

struct NewRig {
    MemVolume sd;
    MemFs fs;
    mhhist::DayStore in;
    mhhist::DayStore out;
    NewRig() : fs(sd), in(fs, mhhist::kIndoorPolicy), out(fs, mhhist::kOutdoorPolicy) {}
};

// Generateur pseudo-aleatoire deterministe (reproductible d'une machine a l'autre).
struct Rng {
    uint32_t s;
    explicit Rng(uint32_t seed) : s(seed ? seed : 1) {}
    uint32_t next() { s = s * 1664525u + 1013904223u; return s >> 8; }
    uint32_t below(uint32_t n) { return next() % n; }
};

inline void assertSameVolume(const MemVolume& a, const MemVolume& b) {
    TEST_ASSERT_TRUE(a.dirs == b.dirs);
    TEST_ASSERT_EQUAL(a.files.size(), b.files.size());
    for (const auto& kv : a.files) {
        auto it = b.files.find(kv.first);
        TEST_ASSERT_TRUE_MESSAGE(it != b.files.end(), kv.first.c_str());
        TEST_ASSERT_TRUE_MESSAGE(kv.second == it->second, kv.first.c_str());
    }
}

inline bool sameFloat(float a, float b) {
    return (std::isnan(a) && std::isnan(b)) || memcmp(&a, &b, sizeof(float)) == 0;
}


inline void seedLegacyFiles(LegacyRig& L, NewRig& N) {
    // Journees d'avant les evolutions du format : IN sans en-tete (v1.4.0), OUT en v1 brut.
    auto raw = [](time_t ts0, int n) {
        std::vector<uint8_t> v;
        for (int i = 0; i < n; i++) {
            uint32_t ts = (uint32_t)(ts0 + i * 60);
            float t = 10.f + i, h = 50.f, p = 1000.f;
            v.insert(v.end(), (uint8_t*)&ts, (uint8_t*)&ts + 4);
            v.insert(v.end(), (uint8_t*)&t, (uint8_t*)&t + 4);
            v.insert(v.end(), (uint8_t*)&h, (uint8_t*)&h + 4);
            v.insert(v.end(), (uint8_t*)&p, (uint8_t*)&p + 4);
        }
        return v;
    };
    const time_t d0 = mkUtc(2026, 11, 3, 8, 0, 0);
    for (MemVolume* v : {&L.sd, &N.sd}) {
        for (const char* d : {"/history", "/history/indoor", "/history/indoor/2026", "/history/indoor/2026/11",
                              "/history/outdoor", "/history/outdoor/2026", "/history/outdoor/2026/11"})
            v->dirs.insert(d);
        v->files["/history/indoor/2026/11/2026-11-03.bin"] = raw(d0, 4);
        v->files["/history/outdoor/2026/11/2026-11-03.bin"] = raw(d0, 3);
    }
}

