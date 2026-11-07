#include "forecast_archive.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "history_platform.h"

namespace mhhist {

void addForecast(mhfs::Fs& fs, const ForecastRecord& rec) {
    if (!fs.exists("/history") && !fs.mkdir("/history")) return;
    if (!fs.exists(MH_FORECAST_DIR) && !fs.mkdir(MH_FORECAST_DIR)) return;

    const uint32_t d = rec.target_day;
    char path[64];
    snprintf(path, sizeof(path), "%s/%04u-%02u-%02u.json",
             MH_FORECAST_DIR, d / 10000u, (d / 100u) % 100u, d % 100u);

    // Echappe la description pour un JSON valide (guillemets, backslash, controle).
    std::string esc;
    esc.reserve(rec.description.size() + 8);
    for (char c : rec.description) {
        switch (c) {
            case '"':  esc += "\\\""; break;
            case '\\': esc += "\\\\"; break;
            case '\n': case '\r': case '\t': esc += ' '; break;
            default:   esc += c; break;
        }
    }

    // Reecriture complete : le fichier du jour cible reflete la DERNIERE prevision emise pour ce
    // jour (au fil de J-1, on converge vers la prevision day-ahead).
    fs.remove(path);
    mhfs::FilePtr f = fs.open(path, mhfs::Mode::Write);
    if (!f) { MH_LOG_WARNING("Forecast: open failed"); return; }
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"target_day\":%u,\"issued_ts\":%u,\"temp_min\":%.1f,"
             "\"temp_max\":%.1f,\"description\":\"%s\"}",
             rec.target_day, rec.issued_ts,
             (double)rec.temp_min, (double)rec.temp_max, esc.c_str());
    f->write(reinterpret_cast<const uint8_t*>(buf), strlen(buf));
    f->close();
}

void forecastHistoryRaw(mhfs::Fs& fs, time_t from, time_t to, const std::function<void(const char*)>& emit) {
    if (!fs.ensureMounted()) return;

    struct tm tf, tt;
    if (!localtime_r(&from, &tf) || !localtime_r(&to, &tt)) return;
    const uint32_t dayFrom = (tf.tm_year + 1900) * 10000u + (tf.tm_mon + 1) * 100u + tf.tm_mday;
    const uint32_t dayTo   = (tt.tm_year + 1900) * 10000u + (tt.tm_mon + 1) * 100u + tt.tm_mday;

    std::vector<mhfs::DirEntry> entries;
    fs.listDir(MH_FORECAST_DIR, mhfs::Want::Files, 512, entries);
    std::vector<std::string> files;
    for (const auto& e : entries) files.push_back(e.name);
    std::sort(files.begin(), files.end()); // AAAA-MM-JJ : tri lexical = chronologique

    size_t it = 0;
    for (const std::string& name : files) {
        MH_YIELD_EVERY(it, 8);
        it++;
        unsigned y = 0, mo = 0, dd = 0;
        if (sscanf(name.c_str(), "%4u-%2u-%2u.json", &y, &mo, &dd) != 3) continue;
        if (name.size() < 5 || name.compare(name.size() - 5, 5, ".json") != 0) continue;
        const uint32_t dayKey = y * 10000u + mo * 100u + dd;
        if (dayKey < dayFrom || dayKey > dayTo) continue;

        const std::string path = std::string(MH_FORECAST_DIR) + "/" + name;
        mhfs::FilePtr f = fs.open(path.c_str(), mhfs::Mode::Read);
        if (!f) continue;
        std::string content;
        uint8_t chunk[128];
        for (int n; (n = f->read(chunk, sizeof(chunk))) > 0;) content.append(reinterpret_cast<char*>(chunk), (size_t)n);
        f->close();
        // trim() d'Arduino : espaces de tete et de queue.
        size_t a = 0, b = content.size();
        while (a < b && isspace(static_cast<unsigned char>(content[a]))) a++;
        while (b > a && isspace(static_cast<unsigned char>(content[b - 1]))) b--;
        content = content.substr(a, b - a);
        if (!content.empty()) emit(content.c_str());
    }
}

} // namespace mhhist
