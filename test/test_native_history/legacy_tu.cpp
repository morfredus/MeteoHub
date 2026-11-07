// Unite de compilation du code de reference (voir legacy_ref.h).
#include "legacy_ref.h"

unsigned long g_legacyYieldCalls = 0;

#define private public
namespace legacy {
#include "legacy/managers/history_manager_impl.inc"
}
#undef private

namespace legacy_probe {
Layout probe(const char* path) {
    File f = SD.open(path, FILE_READ);
    Layout r{0, 0, 0};
    if (!f) return r;
    const legacy::BinLayout L = legacy::probeBin(f);
    f.close();
    r.dataOffset = L.dataOffset; r.recordSize = L.recordSize; r.nrec = L.nrec;
    return r;
}
bool readAt(const char* path, size_t idx, uint32_t& ts, float& t, float& h, float& p) {
    File f = SD.open(path, FILE_READ);
    if (!f) return false;
    const legacy::BinLayout L = legacy::probeBin(f);
    legacy::BinRecord br;
    const bool ok = legacy::readBinRecordAt(f, L, idx, br);
    f.close();
    if (ok) { ts = br.ts; t = br.t; h = br.h; p = br.p; }
    return ok;
}
size_t pad(size_t fs, size_t off, size_t rs) { return legacy::padToRecordBoundary(fs, off, rs); }
void header(bool withFlags, uint8_t out[32]) {
    const legacy::FileHeader h = legacy::makeHeader(withFlags);
    memcpy(out, &h, sizeof(h));
}
bool upgrade(const char* path) {
    legacy::FileHeader h;
    return legacy::upgradeLegacyBin(path, h);
}
bool writeReplacing(const char* path, const void* data, size_t len) {
    return legacy::writeFileReplacing(path, data, len);
}
std::vector<Rec> sortedDay(const char* path, long from, long to) {
    std::vector<Rec> out;
    File f = SD.open(path, FILE_READ);
    if (!f) return out;
    const legacy::BinLayout L = legacy::probeBin(f);
    std::vector<legacy::BinRecord> v;
    legacy::collectSortedDay(f, L, from, to, v);
    f.close();
    for (const auto& r : v) out.push_back(Rec{r.ts, r.t, r.h, r.p});
    return out;
}
std::vector<std::pair<uint32_t, uint32_t>> walk(const char* path, size_t startIdx) {
    std::vector<std::pair<uint32_t, uint32_t>> out;
    File f = SD.open(path, FILE_READ);
    if (!f) return out;
    const legacy::BinLayout L = legacy::probeBin(f);
    legacy::forEachBinRecordFrom(f, L, startIdx, [&](const legacy::BinRecord& r, uint32_t fl) {
        out.emplace_back(r.ts, fl);
        return true;
    });
    f.close();
    return out;
}
bool tsPlausible(time_t ts) { return legacy::outdoorTsPlausible(ts); }
}

// Globaux attendus par le shim et par le code de reference.
ShimFs SD;
ShimFs LittleFS;
std::vector<std::string> g_logLines;
void addLog(const std::string& msg) { g_logLines.push_back(msg); }
