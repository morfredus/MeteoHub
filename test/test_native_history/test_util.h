#pragma once
// Outils communs aux tests de l'historique : volume en memoire, dates, lecture d'octets.
#include <unity.h>
#include <cmath>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
#include "legacy_ref.h"
#include "mem_volume.h"

extern std::vector<std::string> g_logLines;

// Secondes Unix d'une date UTC (le fuseau des tests est fixe sur UTC, voir test_main.cpp).
inline time_t mkUtc(int y, int mo, int d, int h = 0, int mi = 0, int s = 0) {
    // Algorithme "days from civil" (H. Hinnant).
    y -= mo <= 2;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = (unsigned)(y - era * 400);
    const unsigned doy = (153 * (mo + (mo > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const long days = (long)era * 146097 + (long)doe - 719468;
    return (time_t)(days * 86400L + h * 3600L + mi * 60L + s);
}

inline const std::vector<uint8_t>& bytesOf(const MemVolume& v, const char* path) {
    static const std::vector<uint8_t> empty;
    auto it = v.files.find(path);
    return it == v.files.end() ? empty : it->second;
}

template <typename T>
inline T rd(const std::vector<uint8_t>& b, size_t off) {
    T x;
    memcpy(&x, b.data() + off, sizeof(T));
    return x;
}

// Banc de reference : un volume SD (FAT : rename ne remplace pas) et un volume LittleFS
// (rename remplace), relies au shim et au HistoryManager de reference.
struct LegacyRig {
    MemVolume sd, lfs;
    legacy::SdManager mgr;
    legacy::HistoryManager hm;
    LegacyRig() {
        sd.renameReplaces = false;
        lfs.renameReplaces = true;
        lfs.dirs.insert("/history");
        SD.vol = &sd;
        LittleFS.vol = &lfs;
        hm._sd = &mgr;
        g_logLines.clear();
    }
    void createTree() { hm.createSdStructure(); }
};

inline legacy::HistoryRecord inRec(time_t ts, float t, float h, float p) {
    legacy::HistoryRecord r;
    r.timestamp = ts; r.t = t; r.h = h; r.p = p;
    return r;
}
inline legacy::OutdoorHistoryRecord outRec(time_t ts, float t, float h, float p) {
    legacy::OutdoorHistoryRecord r{};
    r.timestamp = ts; r.t = t; r.h = h; r.p = p;
    return r;
}
