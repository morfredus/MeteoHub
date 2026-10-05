#include "battery_log.h"
#include "../managers/sd_manager.h"
#include "../utils/logs.h"
#include <SD.h>

BatteryLog batteryLog;

static const char* kDir = "/history/battery";
static const char* kHeader = "ts,volts,pct\n";

// /history/battery/AAAA-MM.csv pour le mois (heure locale) de `ts`.
static void monthPath(time_t ts, char* out, size_t sz) {
    struct tm t;
    localtime_r(&ts, &t);
    snprintf(out, sz, "%s/%04d-%02d.csv", kDir, t.tm_year + 1900, t.tm_mon + 1);
}

bool BatteryLog::add(time_t ts, float volts, uint8_t pct) {
    // Heure de mesure plausible exigee : sans NTP, un ts relatif rangerait la ligne
    // dans un fichier de 1970.
    if (ts < 1600000000) return false;
    if (!_sd || !_sd->isAvailable()) {
        if (!_warnedNoSd) {
            _warnedNoSd = true;
            LOG_WARNING("Batterie: pas de SD, serie non archivee");
        }
        return false;
    }
    if (!SD.exists("/history")) SD.mkdir("/history");
    if (!SD.exists(kDir) && !SD.mkdir(kDir)) {
        LOG_WARNING("Batterie: creation de /history/battery impossible");
        return false;
    }

    char path[40];
    monthPath(ts, path, sizeof(path));
    const bool fresh = !SD.exists(path);
    File f = SD.open(path, FILE_APPEND);
    if (!f) {
        LOG_WARNING(std::string("Batterie: ouverture impossible ") + path);
        return false;
    }
    if (fresh) f.print(kHeader);
    char line[48];
    snprintf(line, sizeof(line), "%lu,%.3f,%u\n", (unsigned long)ts, volts, (unsigned)pct);
    const bool ok = f.print(line) > 0;
    f.close();
    return ok;
}

std::vector<mhbat::Point> BatteryLog::query(time_t from, time_t to, uint32_t& bucketSeconds) {
    mhbat::Aggregator agg((uint32_t)from, (uint32_t)to);
    bucketSeconds = agg.bucketSeconds();
    if (!_sd || !_sd->isAvailable()) return {};

    // Mois couverts par la plage : du mois de `from` a celui de `to`.
    struct tm a, b;
    localtime_r(&from, &a);
    localtime_r(&to, &b);
    int y = a.tm_year + 1900, m = a.tm_mon + 1;
    const int yEnd = b.tm_year + 1900, mEnd = b.tm_mon + 1;
    char path[40];
    while (y < yEnd || (y == yEnd && m <= mEnd)) {
        snprintf(path, sizeof(path), "%s/%04d-%02d.csv", kDir, y, m);
        File f = SD.open(path, FILE_READ);
        if (f) {
            char line[64];
            while (f.available()) {
                const size_t n = f.readBytesUntil('\n', line, sizeof(line) - 1);
                line[n] = 0;
                uint32_t ts; float v, p;
                if (mhbat::parseLine(line, ts, v, p)) agg.add(ts, v, p);
            }
            f.close();
        }
        if (++m > 12) { m = 1; y++; }
    }
    return agg.points();
}
