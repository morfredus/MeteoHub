#include "outdoor_quarantine.h"
#include "../managers/sd_manager.h"
#include "../utils/logs.h"
#include <SD.h>
#include <LittleFS.h>

OutdoorQuarantine outdoorQuarantine;

static const char* kSdPath = "/history/outdoor_quarantine.csv";
static const char* kFsPath = "/outdoor_quarantine.csv";
// Repli LittleFS borné : au-delà, on repart d'un fichier neuf (et on le dit).
static const size_t kFsMaxBytes = 32 * 1024;
static const char* kHeader =
    "ts,mac,seq,reason,reset_reason,wake_count,temperature,humidity,pressure,battery_v\n";

static bool appendLine(fs::FS& fs, const char* path, const char* line, bool bounded) {
    bool fresh = !fs.exists(path);
    if (!fresh && bounded) {
        File probe = fs.open(path, FILE_READ);
        const size_t sz = probe ? probe.size() : 0;
        if (probe) probe.close();
        if (sz > kFsMaxBytes) {
            fs.remove(path);
            fresh = true;
            LOG_WARNING("Quarantaine: repli LittleFS plein, fichier recommence");
        }
    }
    File f = fs.open(path, FILE_APPEND);
    if (!f) return false;
    if (fresh) f.print(kHeader);
    const size_t w = f.print(line);
    f.close();
    return w > 0;
}

bool OutdoorQuarantine::add(const OutdoorData& o, time_t measurementTs, const char* reason) {
    char line[200];
    snprintf(line, sizeof(line),
             "%ld,%02X:%02X:%02X:%02X:%02X:%02X,%u,%s,%u,%u,%.2f,%.2f,%.2f,%.2f\n",
             (long)measurementTs,
             o.src_mac[0], o.src_mac[1], o.src_mac[2], o.src_mac[3], o.src_mac[4], o.src_mac[5],
             (unsigned)o.sequence, reason, (unsigned)o.reset_reason, (unsigned)o.wake_count,
             o.temperature, o.humidity, o.pressure,
             o.has_battery ? o.battery_voltage : 0.0f);

    bool ok = false;
    if (_sd && _sd->isAvailable()) ok = appendLine(SD, kSdPath, line, /*bounded=*/false);
    if (!ok) ok = appendLine(LittleFS, kFsPath, line, /*bounded=*/true);
    if (ok) _count++;
    return ok;
}

const char* OutdoorQuarantine::currentPath(bool& onSd) {
    if (_sd && _sd->isAvailable() && SD.exists(kSdPath)) { onSd = true; return kSdPath; }
    if (LittleFS.exists(kFsPath)) { onSd = false; return kFsPath; }
    onSd = false;
    return "";
}
