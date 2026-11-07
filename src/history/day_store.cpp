#include "day_store.h"

#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace mhhist {

// Tolerance de recherche autour de l'horodatage cible (30 min).
static const long kSampleToleranceS = 1800;
// Garde-fou d'un listage de dossier : si l'iteration se mettait malgre tout a boucler, on
// s'arrete plutot que de bloquer indefiniment la requete HTTP.
static const size_t kMaxDirEntries = 512;

// L'ordre des champs suit DayStorePolicy ; chaque ligne documente l'ecart IN / OUT.
const DayStorePolicy kIndoorPolicy = {
    "/history", "/history/indoor", "Bin Save",
    /*freshWithFlags*/ false, /*upgradeLegacy*/ true, /*maintainHeader*/ true,
    /*maintainStats*/ true, /*retryOpenOnFresh*/ true, /*logDirErrors*/ false,
    /*okMessage*/ nullptr, /*okFlaggedMessage*/ nullptr, /*failuresAreErrors*/ true,
    /*chronological*/ true, /*legacyCsvDir*/ "/history"};

const DayStorePolicy kOutdoorPolicy = {
    "/history", "/history/outdoor", "Bin Save OUT",
    /*freshWithFlags*/ true, /*upgradeLegacy*/ false, /*maintainHeader*/ false,
    /*maintainStats*/ false, /*retryOpenOnFresh*/ false, /*logDirErrors*/ true,
    "History: Outdoor data saved to SD", "History: Outdoor FLAGGED data saved to SD",
    /*failuresAreErrors*/ false, /*chronological*/ false, /*legacyCsvDir*/ nullptr};

// --- Outils de fichier ---------------------------------------------------------------------

bool writeFileReplacing(Fs& fs, const char* path, const void* data, size_t len) {
    const std::string tmp = std::string(path) + ".tmp";
    FilePtr f = fs.open(tmp.c_str(), mhfs::Mode::Write);
    if (!f) return false;
    const bool ok = f->write(static_cast<const uint8_t*>(data), len) == len;
    f->flush();
    f->close();
    if (!ok) { fs.remove(tmp.c_str()); return false; }
    fs.remove(path);
    return fs.rename(tmp.c_str(), path);
}

bool upgradeLegacyBin(Fs& fs, const char* binPath, FileHeader& outHdr) {
    FilePtr rf = fs.open(binPath, mhfs::Mode::Read);
    if (!rf) return false;
    std::string tmp = std::string(binPath) + ".tmp";
    FilePtr wf = fs.open(tmp.c_str(), mhfs::Mode::Write);
    if (!wf) { rf->close(); return false; }

    FileHeader h = makeHeader();
    bool ok = wf->write(reinterpret_cast<const uint8_t*>(&h), sizeof(h)) == sizeof(h); // en-tete provisoire

    BinRecord br;
    uint32_t count = 0; uint64_t firstTs = 0, lastTs = 0;
    size_t it = 0;
    while (ok && rf->read(reinterpret_cast<uint8_t*>(&br), sizeof(br)) == (int)sizeof(br)) {
        MH_YIELD_EVERY(it, 64); it++;
        ok = wf->write(reinterpret_cast<const uint8_t*>(&br), sizeof(br)) == sizeof(br);
        if (count == 0) firstTs = br.ts;
        lastTs = br.ts;
        count++;
    }
    rf->close();

    h.recordCount = count; h.firstTimestamp = firstTs; h.lastTimestamp = lastTs;
    if (ok) {
        wf->seek(0);
        ok = wf->write(reinterpret_cast<const uint8_t*>(&h), sizeof(h)) == sizeof(h);
    }
    wf->flush();
    wf->close();
    if (!ok) {
        fs.remove(tmp.c_str());
        MH_LOG_ERROR("Upgrade legacy bin failed (write): " + std::string(binPath));
        return false;
    }

    // Substitution sans fenetre de perte : l'original est mis de cote (.old), le nouveau
    // fichier prend sa place, puis .old est supprime. Si le rename echoue, l'original est
    // remis en place.
    const std::string old = std::string(binPath) + ".old";
    fs.remove(old.c_str());
    if (!fs.rename(binPath, old.c_str())) { fs.remove(tmp.c_str()); return false; }
    if (!fs.rename(tmp.c_str(), binPath)) {
        fs.rename(old.c_str(), binPath);
        fs.remove(tmp.c_str());
        MH_LOG_ERROR("Upgrade legacy bin failed (rename): " + std::string(binPath));
        return false;
    }
    fs.remove(old.c_str());
    outHdr = h;
    MH_LOG_INFO("Upgraded legacy bin: " + std::string(binPath));
    return true;
}

// --- Chemins et dossiers -------------------------------------------------------------------

uint32_t DayStore::dayKey(const struct tm& tinfo) {
    return static_cast<uint32_t>((tinfo.tm_year + 1900) * 10000 + (tinfo.tm_mon + 1) * 100 + tinfo.tm_mday);
}

void DayStore::dayPaths(const struct tm& tinfo, char* binPath, char* statsPath, size_t sz) const {
    // <root>/AAAA/MM/AAAA-MM-JJ.bin (+ .stats)
    snprintf(binPath, sz, "%s/%04d/%02d/%04d-%02d-%02d.bin", _p.root,
             tinfo.tm_year + 1900, tinfo.tm_mon + 1,
             tinfo.tm_year + 1900, tinfo.tm_mon + 1, tinfo.tm_mday);
    snprintf(statsPath, sz, "%s/%04d/%02d/%04d-%02d-%02d.stats", _p.root,
             tinfo.tm_year + 1900, tinfo.tm_mon + 1,
             tinfo.tm_year + 1900, tinfo.tm_mon + 1, tinfo.tm_mday);
}

bool DayStore::ensureDirs(const struct tm& tinfo) const {
    if (!_fs.exists(_p.baseDir) && !_fs.mkdir(_p.baseDir)) {
        if (_p.logDirErrors) MH_LOG_WARNING(std::string("History: Failed to create ") + _p.baseDir);
        return false;
    }
    if (!_fs.exists(_p.root) && !_fs.mkdir(_p.root)) {
        if (_p.logDirErrors) MH_LOG_WARNING(std::string("History: Failed to create root directory ") + _p.root);
        return false;
    }
    char dirPath[64];
    snprintf(dirPath, sizeof(dirPath), "%s/%04d", _p.root, tinfo.tm_year + 1900);
    if (!_fs.exists(dirPath) && !_fs.mkdir(dirPath)) {
        if (_p.logDirErrors) MH_LOG_WARNING(std::string("History: Failed to create year directory ") + dirPath);
        return false;
    }
    snprintf(dirPath, sizeof(dirPath), "%s/%04d/%02d", _p.root, tinfo.tm_year + 1900, tinfo.tm_mon + 1);
    if (!_fs.exists(dirPath) && !_fs.mkdir(dirPath)) {
        if (_p.logDirErrors) MH_LOG_WARNING(std::string("History: Failed to create month directory ") + dirPath);
        return false;
    }
    return true;
}

// --- Ecriture ------------------------------------------------------------------------------

bool DayStore::append(time_t ts, const BinRecord& br, uint32_t flags) {
    const std::string tag = _p.tag;
    auto fail = [&](const std::string& msg) {
        if (_p.failuresAreErrors) MH_LOG_ERROR(msg);
        else MH_LOG_WARNING(msg);
    };

    struct tm tinfo;
    if (!localtime_r(&ts, &tinfo)) {
        MH_LOG_WARNING(tag + ": localtime failed");
        return false;
    }

    if (!ensureDirs(tinfo) && _fs.ensureMounted()) {
        ensureDirs(tinfo);
    }

    char binPath[64], statsPath[64];
    dayPaths(tinfo, binPath, statsPath, sizeof(binPath));

    // Inspecte le fichier du jour : absent/vide (neuf), avec en-tete, ou ancien format sans
    // en-tete.
    FileHeader hdr = makeHeader(_p.freshWithFlags);
    bool exists = false, hasHeader = false;
    {
        FilePtr rf = _fs.open(binPath, mhfs::Mode::Read);
        if (rf) {
            if (rf->size() > 0) {
                exists = true;
                FileHeader h;
                probeBin(*rf, &h, &hasHeader);
                if (hasHeader) hdr = h;
            }
            rf->close();
        }
    }

    if (exists && !hasHeader && _p.upgradeLegacy) {
        if (!upgradeLegacyBin(_fs, binPath, hdr)) return false;
        hasHeader = true;
    }

    // Disposition du fichier existant (en-tete valide, sinon enregistrements v1 des l'offset 0).
    const bool fresh = !exists;
    size_t dataOffset = 0;
    uint16_t recordSize = sizeof(BinRecord);
    if (hasHeader) {
        dataOffset = hdr.headerSize ? hdr.headerSize : sizeof(FileHeader);
        recordSize = hdr.recordSize >= sizeof(BinRecord) ? hdr.recordSize : sizeof(BinRecord);
    } else if (fresh) {
        recordSize = hdr.recordSize;
    }
    if (flags && recordSize < sizeof(BinRecordV2)) return false; // v1 : marque impossible

    BinRecordV2 rec{};
    rec.base = br;
    rec.flags = flags;
    // L'IN n'ecrit jamais que des enregistrements de 16 octets ; l'OUT suit le format du fichier.
    const size_t recLen = (_p.freshWithFlags && recordSize >= sizeof(BinRecordV2)) ? sizeof(BinRecordV2)
                                                                                  : sizeof(BinRecord);

    // En-tete (fichier neuf) / bourrage de realignement + enregistrement : UNE seule ecriture,
    // dont le nombre d'octets est verifie.
    FilePtr af = _fs.open(binPath, mhfs::Mode::Append);
    if (!af && fresh && _p.retryOpenOnFresh && _fs.ensureMounted()) {
        ensureDirs(tinfo);
        af = _fs.open(binPath, mhfs::Mode::Append);
    }
    if (!af) {
        fail(tag + ": cannot append " + std::string(binPath));
        return false;
    }
    const AppendOutcome wr = appendRecord(*af, fresh ? &hdr : nullptr, dataOffset, recordSize, &rec, recLen);
    if (wr.padWritten > 0) {
        MH_LOG_WARNING(tag + ": fichier desaligne, bourrage de " + std::to_string(wr.padWritten)
                       + " octets (" + std::string(binPath) + ")");
    }
    if (!wr.ok) {
        fail(tag + ": write failed " + std::string(binPath));
        return false;
    }
    if (flags ? _p.okFlaggedMessage != nullptr : _p.okMessage != nullptr) {
        MH_LOG_INFO(flags ? _p.okFlaggedMessage : _p.okMessage);
    }

    if (_p.maintainHeader) {
        // Met a jour l'en-tete (compteur + horodatages) - best effort, sans blocage si le mode
        // "r+" n'est pas supporte (les lecteurs recalculent nrec via la taille).
        hdr.recordCount += 1;
        hdr.lastTimestamp = static_cast<uint64_t>(ts);
        if (hdr.firstTimestamp == 0) hdr.firstTimestamp = static_cast<uint64_t>(ts);
        FilePtr uf = _fs.open(binPath, mhfs::Mode::ReadWrite);
        if (uf) {
            uf->seek(0);
            uf->write(reinterpret_cast<const uint8_t*>(&hdr), sizeof(hdr));
            uf->flush();
            uf->close();
        }
    }

    if (_p.maintainStats) updateDayStats(ts, br, tinfo);
    // Cache de la liste des jours : un bourrage a decale le compte, on le reconstruira.
    noteDayWritten(tinfo, static_cast<uint32_t>(ts), wr.padSeen > 0);
    return true;
}

void DayStore::updateDayStats(time_t ts, const BinRecord& record, const struct tm& tinfo) {
    const uint32_t key = dayKey(tinfo);

    if (key != _currentDayKey) {
        // Changement de jour (ou premier enregistrement depuis le demarrage) : recharge le
        // .stats existant du jour s'il y en a un (reprise apres reboot), sinon repart d'une
        // structure vierge.
        DayStats existing;
        if (readDayStats(ts, existing) && existing.count > 0) {
            _currentDayStats = existing;
        } else {
            _currentDayStats = DayStats();
        }
        _currentDayStats.magic = kDayStatsMagic;
        _currentDayKey = key;
    }

    DayStats& s = _currentDayStats;
    if (s.count == 0) {
        s.first_ts = record.ts;
        s.t_min = s.t_max = s.t_first = record.t;
        s.h_min = s.h_max = s.h_first = record.h;
        s.p_min = s.p_max = s.p_first = record.p;
        s.t_sum = s.h_sum = s.p_sum = 0;
    }
    if (record.t < s.t_min) s.t_min = record.t;
    if (record.t > s.t_max) s.t_max = record.t;
    if (record.h < s.h_min) s.h_min = record.h;
    if (record.h > s.h_max) s.h_max = record.h;
    if (record.p < s.p_min) s.p_min = record.p;
    if (record.p > s.p_max) s.p_max = record.p;
    s.t_sum += record.t; s.h_sum += record.h; s.p_sum += record.p;
    s.t_last = record.t; s.h_last = record.h; s.p_last = record.p;
    s.last_ts = record.ts;
    s.count++;

    // Persiste le .stats (petit fichier, reecrit integralement a chaque mesure), par
    // temporaire + rename : une coupure en plein ecrasement ne laisse plus un .stats tronque.
    char binPath[64], statsPath[64];
    dayPaths(tinfo, binPath, statsPath, sizeof(binPath));
    if (!writeFileReplacing(_fs, statsPath, &s, sizeof(s))) {
        MH_LOG_WARNING("Stats write failed: " + std::string(statsPath));
    }
}

void DayStore::resetStatsCache() {
    _currentDayKey = 0;
    _currentDayStats = DayStats();
}

// --- Lecture -------------------------------------------------------------------------------

bool DayStore::readDayStats(time_t day_ts, DayStats& out) const {
    if (!_fs.ensureMounted()) return false; // lecture : ne pas sonder cardType()
    struct tm tinfo;
    if (!localtime_r(&day_ts, &tinfo)) return false;

    char binPath[64], statsPath[64];
    dayPaths(tinfo, binPath, statsPath, sizeof(binPath));
    // Si la coupure est tombee entre le remove et le rename de writeFileReplacing, le
    // temporaire complet est la version la plus recente.
    std::string readPath = statsPath;
    if (!_fs.exists(statsPath)) {
        readPath += ".tmp";
        if (!_fs.exists(readPath.c_str())) return false;
    }

    FilePtr f = _fs.open(readPath.c_str(), mhfs::Mode::Read);
    if (!f) return false;
    DayStats tmp;
    int n = f->read(reinterpret_cast<uint8_t*>(&tmp), sizeof(tmp));
    f->close();
    if (n != (int)sizeof(tmp) || tmp.magic != kDayStatsMagic) return false;
    out = tmp;
    return true;
}

bool DayStore::sampleNear(time_t target_ts, float& t_out, float& h_out, float& p_out) const {
    if (!_fs.ensureMounted()) return false; // lecture : ne pas sonder cardType()
    // Priorite au format binaire ; repli sur l'ancien CSV plat si absent.
    if (binSampleNear(target_ts, t_out, h_out, p_out)) return true;
    if (!_p.legacyCsvDir) return false;
    return csvSampleNear(target_ts, t_out, h_out, p_out);
}

bool DayStore::binSampleNear(time_t target_ts, float& t_out, float& h_out, float& p_out) const {
    struct tm tinfo;
    if (!localtime_r(&target_ts, &tinfo)) return false;

    char binPath[64], statsPath[64];
    dayPaths(tinfo, binPath, statsPath, sizeof(binPath));
    if (!_fs.exists(binPath)) return false;

    FilePtr f = _fs.open(binPath, mhfs::Mode::Read);
    if (!f) return false;

    const BinLayout L = probeBin(*f);
    if (L.nrec == 0) { f->close(); return false; }

    if (!_p.chronological) {
        // Fichier non chronologique (retransmissions ajoutees en fin) : la dichotomie ne
        // s'applique pas, parcours sequentiel du jour (<= quelques centaines d'enregistrements).
        long best = LONG_MAX;
        bool hit = false;
        forEachBinRecordFrom(*f, L, 0, [&](const BinRecord& rec, uint32_t flags) -> bool {
            if (flags) return true;
            const long diff = labs(static_cast<long>(rec.ts) - static_cast<long>(target_ts));
            if (diff < best) { best = diff; t_out = rec.t; h_out = rec.h; p_out = rec.p; hit = true; }
            return true;
        });
        f->close();
        return hit && best <= kSampleToleranceS;
    }

    // Dichotomie : premier index dont ts >= target.
    size_t lo = 0, hi = L.nrec;
    BinRecord br;
    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        if (!readBinRecordAt(*f, L, mid, br)) break;
        if (static_cast<long>(br.ts) < static_cast<long>(target_ts)) lo = mid + 1;
        else hi = mid;
    }

    long best_diff = LONG_MAX;
    bool found = false;
    // Examine les voisins immediats de l'index trouve (avant/apres).
    for (long k = static_cast<long>(lo) - 1; k <= static_cast<long>(lo); ++k) {
        if (k < 0 || k >= static_cast<long>(L.nrec)) continue;
        if (!readBinRecordAt(*f, L, static_cast<size_t>(k), br)) continue;
        long diff = labs(static_cast<long>(br.ts) - static_cast<long>(target_ts));
        if (diff < best_diff) {
            best_diff = diff;
            t_out = br.t; h_out = br.h; p_out = br.p;
            found = true;
        }
    }
    f->close();
    return found && best_diff <= kSampleToleranceS;
}

bool DayStore::csvSampleNear(time_t target_ts, float& t_out, float& h_out, float& p_out) const {
    struct tm timeinfo;
    if (!localtime_r(&target_ts, &timeinfo)) return false;

    char filename[48];
    const std::string fmt = std::string(_p.legacyCsvDir) + "/%Y-%m-%d.csv";
    strftime(filename, sizeof(filename), fmt.c_str(), &timeinfo);

    if (!_fs.exists(filename)) return false;

    FilePtr f = _fs.open(filename, mhfs::Mode::Read);
    if (!f) return false;

    bool found = false;
    long best_diff = LONG_MAX;
    bool first_line = true;
    size_t line_count = 0;

    mhfs::LineReader lines(*f);
    std::string line;
    while (lines.next(line)) {
        MH_YIELD_EVERY(line_count, 256);
        line_count++;

        if (first_line) {
            first_line = false;
            continue; // en-tete CSV "Timestamp,Temperature,Humidity,Pressure"
        }
        if (line.empty()) continue;

        long ts = 0;
        float t = 0, h = 0, p = 0;
        if (sscanf(line.c_str(), "%ld,%f,%f,%f", &ts, &t, &h, &p) == 4) {
            long diff = labs(static_cast<long>(ts - static_cast<long>(target_ts)));
            if (diff < best_diff) {
                best_diff = diff;
                t_out = t;
                h_out = h;
                p_out = p;
                found = true;
            }
        }
    }
    f->close();

    return found && best_diff <= kSampleToleranceS;
}

// --- Liste des jours -----------------------------------------------------------------------

std::vector<DayIndexEntry> DayStore::listDays() const {
    uint32_t gen = 0;
    {
        std::lock_guard<std::mutex> lock(_daysMutex);
        if (_days.valid) return _days.list;
        gen = _daysGen;
    }
    std::vector<DayIndexEntry> list = scanDays(); // long, rend la main par jour
    std::lock_guard<std::mutex> lock(_daysMutex);
    // Une ecriture survenue pendant le parcours peut ne pas y figurer : ne pas memoriser.
    if (gen == _daysGen) { _days.list = list; _days.valid = true; }
    return list;
}

void DayStore::noteDayWritten(const struct tm& tinfo, uint32_t ts, bool invalidate) {
    const uint32_t key = dayKey(tinfo);
    std::lock_guard<std::mutex> lock(_daysMutex);
    _daysGen++;
    DaysCache& c = _days;
    if (!c.valid) return;
    if (invalidate) { c.valid = false; c.list.clear(); return; }
    for (DayIndexEntry& e : c.list) {
        if (e.day_key != key) continue;
        e.nrec++;
        if (ts < e.first_ts || e.first_ts == 0) e.first_ts = ts;
        if (ts > e.last_ts) e.last_ts = ts;
        return;
    }
    DayIndexEntry e{};
    e.day_key = key; e.nrec = 1; e.first_ts = ts; e.last_ts = ts;
    c.list.insert(std::upper_bound(c.list.begin(), c.list.end(), e,
                  [](const DayIndexEntry& a, const DayIndexEntry& b) { return a.day_key < b.day_key; }), e);
}

void DayStore::invalidateDaysCache() {
    std::lock_guard<std::mutex> lock(_daysMutex);
    _daysGen++;
    _days.valid = false;
    _days.list.clear();
}

std::vector<DayIndexEntry> DayStore::scanDays() const {
    std::vector<DayIndexEntry> out;
    if (!_fs.ensureMounted()) return out;

    // <root>/AAAA/MM/AAAA-MM-JJ.bin : on aplatit les deux niveaux de repertoires avant
    // d'ouvrir le moindre fichier de mesures (chaque niveau est lu PUIS referme).
    std::vector<std::string> months; // chemins complets des dossiers de mois
    {
        std::vector<mhfs::DirEntry> years;
        _fs.listDir(_p.root, mhfs::Want::Dirs, kMaxDirEntries, years);
        for (const mhfs::DirEntry& y : years) {
            const std::string yearPath = std::string(_p.root) + "/" + y.name;
            std::vector<mhfs::DirEntry> monthNames;
            _fs.listDir(yearPath.c_str(), mhfs::Want::Dirs, kMaxDirEntries, monthNames);
            for (const mhfs::DirEntry& m : monthNames)
                months.push_back(yearPath + "/" + m.name);
        }
    }

    size_t dayScan = 0;
    for (const std::string& monthPath : months) {
        std::vector<mhfs::DirEntry> files;
        _fs.listDir(monthPath.c_str(), mhfs::Want::Files, kMaxDirEntries, files);

        for (const mhfs::DirEntry& fe : files) {
            const std::string& name = fe.name;
            MH_YIELD_EVERY(dayScan, 1); // un open par jour : rearme le watchdog
            dayScan++;
            // L'extension est verifiee SEPAREMENT de la date. sscanf renvoie le nombre de
            // conversions AFFECTEES : avec "%4u-%2u-%2u.bin", un fichier "AAAA-MM-JJ.stats"
            // renvoie quand meme 3, l'echec du litteral ".bin" final n'etant pas compte. Les
            // fichiers .stats etaient ainsi pris pour des fichiers de mesures et publiaient
            // des journees fantomes aux horodatages aberrants.
            const size_t len = name.size();
            if (len < 14 || name.compare(len - 4, 4, ".bin") != 0)
                continue;

            unsigned y = 0, mo = 0, d = 0;
            if (sscanf(name.c_str(), "%4u-%2u-%2u", &y, &mo, &d) != 3)
                continue;
            if (mo < 1 || mo > 12 || d < 1 || d > 31)
                continue;

            const std::string full = monthPath + "/" + name;
            FilePtr f = _fs.open(full.c_str(), mhfs::Mode::Read);
            if (!f) continue;

            FileHeader h;
            bool headerValid = false;
            BinLayout L = probeBin(*f, &h, &headerValid);
            if (L.nrec > 0) {
                DayIndexEntry e{};
                e.day_key = y * 10000u + mo * 100u + d;
                e.nrec = static_cast<uint32_t>(L.nrec);

                // Les horodatages extremes sont lus dans l'en-tete quand il est present :
                // chaque acces a la carte SD coute cher, et deux positionnements par journee
                // sont evitables.
                if (headerValid && h.firstTimestamp != 0) {
                    e.first_ts = static_cast<uint32_t>(h.firstTimestamp);
                    e.last_ts  = static_cast<uint32_t>(h.lastTimestamp);
                } else {
                    BinRecord br;
                    if (readBinRecordAt(*f, L, 0, br)) e.first_ts = br.ts;
                    if (readBinRecordAt(*f, L, L.nrec - 1, br)) e.last_ts = br.ts;
                }
                out.push_back(e);
            }
            f->close();
        }
    }

    std::sort(out.begin(), out.end(),
              [](const DayIndexEntry& a, const DayIndexEntry& b) { return a.day_key < b.day_key; });
    return out;
}

// --- Export brut ---------------------------------------------------------------------------

uint32_t DayStore::exportRaw(uint32_t day_key, uint32_t from_index, uint32_t limit,
                             const std::function<void(const RawRecord&)>& emit) const {
    if (!_fs.ensureMounted()) return 0;

    const unsigned y = day_key / 10000u;
    const unsigned mo = (day_key / 100u) % 100u;
    const unsigned d = day_key % 100u;

    char binPath[64];
    snprintf(binPath, sizeof(binPath), "%s/%04u/%02u/%04u-%02u-%02u.bin", _p.root, y, mo, y, mo, d);

    FilePtr f = _fs.open(binPath, mhfs::Mode::Read);
    if (!f) return 0;

    BinLayout L = probeBin(*f);
    const uint32_t total = static_cast<uint32_t>(L.nrec);

    uint32_t sent = 0;
    forEachBinRecordFrom(*f, L, from_index, [&](const BinRecord& br, uint32_t flags) {
        if (sent >= limit) return false;
        // Export BRUT : tout, marques comprises. Les positions (index) restent celles du
        // fichier, dont depend la reprise de morfAnalytics.
        emit(RawRecord{br.ts, br.t, br.h, br.p, flags});
        sent++;
        return true;
    });

    f->close();
    return total;
}

} // namespace mhhist
