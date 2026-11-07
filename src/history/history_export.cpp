#include "history_export.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace mhhist {

// Remplit `buf` (au plus maxLen octets) avec la suite de l'export decrite par le curseur, puis
// rend la main. La route web l'appelle depuis un callback de reponse chunked : chaque appel ne
// lit qu'une portion (jamais le CSV entier en RAM, ni toute la plage de dates d'un seul tenant
// dans async_tcp).
size_t exportCsvChunk(const DayStore& store, CsvCursor& c, char* buf, size_t maxLen) {
    size_t n = 0;
    size_t lines = 0;
    while (!c.done) {
        char line[64];
        size_t len = 0;

        if (!c.headerDone) {
            len = snprintf(line, sizeof(line), "Timestamp,Temperature,Humidity,Pressure\n");
            if (c.to <= c.from || !store.fs().ensureMounted()) c.done = true; // en-tete seul
        } else {
            if (!c.dayLoaded) {
                if (c.day > c.to + 86400) { c.done = true; break; }
                c.dayRecs.clear();
                c.index = 0;
                c.dayLoaded = true;
                struct tm tinfo;
                if (localtime_r(&c.day, &tinfo)) {
                    char binPath[64], statsPath[64];
                    store.dayPaths(tinfo, binPath, statsPath, sizeof(binPath));
                    // Evite un double parcours d'un meme fichier (bord d'heure d'ete).
                    if (c.prevBin != binPath) {
                        c.prevBin = binPath;
                        FilePtr f = store.fs().exists(binPath) ? store.fs().open(binPath, mhfs::Mode::Read)
                                                               : nullptr;
                        if (f) {
                            const BinLayout L = probeBin(*f);
                            // Jour trie : cote OUT le fichier n'est pas chronologique.
                            collectSortedDay(*f, L, static_cast<long>(c.from), static_cast<long>(c.to),
                                             c.dayRecs);
                            f->close();
                        }
                    }
                }
            }
            if (c.index >= c.dayRecs.size()) {
                c.dayLoaded = false;
                c.dayRecs.clear();
                c.day += 86400;
                continue;
            }
            const BinRecord& rec = c.dayRecs[c.index];
            // Humidite / pression absentes (NaN) : champ vide, jamais « nan » ni 0.
            char hb[16] = "", pb[16] = "";
            if (std::isfinite(rec.h)) snprintf(hb, sizeof(hb), "%.1f", rec.h);
            if (std::isfinite(rec.p)) snprintf(pb, sizeof(pb), "%.1f", rec.p);
            len = snprintf(line, sizeof(line), "%lu,%.2f,%s,%s\n",
                           static_cast<unsigned long>(rec.ts), rec.t, hb, pb);
        }

        if (n + len > maxLen) break; // pas la place : cette ligne ouvrira le prochain appel
        memcpy(buf + n, line, len);
        n += len;
        if (!c.headerDone) c.headerDone = true; else c.index++;
        MH_YIELD_EVERY(lines, 64);
        lines++;
    }
    return n;
}

} // namespace mhhist
