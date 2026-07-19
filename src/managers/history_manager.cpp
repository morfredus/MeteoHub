#include "history_manager.h"
#include <LittleFS.h>
#include "../utils/logs.h"
#include "../utils/cooperative_yield.h" // INCLUSION AJOUTÉE
#include <time.h>
#include <inttypes.h>
#include <climits>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <Arduino.h>

#define HISTORY_FILE "/history/recent.dat"
#define MAX_RECENT_RECORDS 1440
#define SD_SAMPLE_TOLERANCE_S 1800 // tolérance de recherche autour de l'horodatage cible (30 min)

// Détection de valeurs aberrantes par cohérence temporelle (mêmes seuils que le
// client) : un point s'écartant fortement de SES DEUX voisins alors que ceux-ci
// restent cohérents entre eux est considéré comme un pic/creux ponctuel. Les
// planchers de bruit évitent d'écarter les micro-variations normales.
#define OUTLIER_FLOOR_T 1.0f
#define OUTLIER_FLOOR_H 6.0f
#define OUTLIER_FLOOR_P 1.0f

static bool isTemporalOutlier(float prev, float cur, float next, float floor) {
    const float jump = fminf(fabsf(cur - prev), fabsf(cur - next)); // amplitude aller-retour
    const float neighborGap = fabsf(prev - next);                    // cohérence des voisins
    return jump > floor && jump > 3.0f * neighborGap;
}

// --- Stockage binaire journalier -------------------------------------------
// Chaque mesure est enregistrée comme une structure fixe de 16 octets (contre
// ~35 en CSV), ce qui rend les fichiers plus compacts et permet l'accès direct
// à une mesure par sa position (recherche dichotomique), sans relire ce qui
// précède. Les fichiers sont rangés par jour : /history/AAAA/MM/AAAA-MM-JJ.bin
// avec, à côté, un fichier .stats contenant les statistiques déjà calculées.
#define BIN_STATS_MAGIC 0x53544231u // "1BTS" — identifie/versionne un fichier .stats

struct __attribute__((packed)) BinRecord {
    uint32_t ts; // horodatage Unix (secondes)
    float t;     // température
    float h;     // humidité
    float p;     // pression
};
static const size_t BIN_RECORD_SIZE = sizeof(BinRecord); // = 16

// En-tête présent au début de chaque fichier .bin. Il rend le format pérenne :
// MeteoHub identifie le format (magic + version) et connaît la structure exacte
// (taille d'en-tête, taille d'enregistrement, capteurs présents) avant de lire.
// De nouveaux capteurs pourront agrandir l'enregistrement dans une version
// ultérieure sans imposer de migrer les anciens fichiers : la lecture s'appuie
// sur recordSize pour se déplacer et n'exploite que les champs qu'elle connaît.
#define BIN_FORMAT_VERSION 1
#define BIN_SENSOR_TEMP  0x0001
#define BIN_SENSOR_HUM   0x0002
#define BIN_SENSOR_PRES  0x0004

struct __attribute__((packed)) FileHeader {
    char     magic[4];        // "MTHB"
    uint16_t version;         // format du fichier
    uint16_t headerSize;      // taille de l'en-tête
    uint16_t recordSize;      // taille d'un enregistrement
    uint16_t sensorFlags;     // capteurs présents (bits)
    uint32_t recordCount;     // nombre d'enregistrements
    uint64_t firstTimestamp;  // premier relevé
    uint64_t lastTimestamp;   // dernier relevé
};

static bool hdrMagicOk(const FileHeader& h) {
    return h.magic[0] == 'M' && h.magic[1] == 'T' && h.magic[2] == 'H' && h.magic[3] == 'B';
}

static FileHeader makeHeader() {
    FileHeader h{};
    h.magic[0] = 'M'; h.magic[1] = 'T'; h.magic[2] = 'H'; h.magic[3] = 'B';
    h.version = BIN_FORMAT_VERSION;
    h.headerSize = sizeof(FileHeader);
    h.recordSize = sizeof(BinRecord);
    h.sensorFlags = BIN_SENSOR_TEMP | BIN_SENSOR_HUM | BIN_SENSOR_PRES;
    h.recordCount = 0; h.firstTimestamp = 0; h.lastTimestamp = 0;
    return h;
}

// Disposition d'un fichier .bin ouvert : où commencent les enregistrements, leur
// taille, et combien il y en a. Gère les fichiers avec en-tête ET les anciens
// fichiers sans en-tête (créés par la v1.4.0 : enregistrements dès l'offset 0).
struct BinLayout { uint32_t dataOffset; uint16_t recordSize; size_t nrec; };

static BinLayout probeBin(File& f) {
    BinLayout L; L.dataOffset = 0; L.recordSize = sizeof(BinRecord); L.nrec = 0;
    const size_t fsize = f.size();
    if (fsize >= sizeof(FileHeader)) {
        FileHeader h;
        f.seek(0);
        if (f.read(reinterpret_cast<uint8_t*>(&h), sizeof(h)) == (int)sizeof(h) && hdrMagicOk(h)) {
            L.dataOffset = h.headerSize ? h.headerSize : sizeof(FileHeader);
            L.recordSize = h.recordSize >= sizeof(BinRecord) ? h.recordSize : sizeof(BinRecord);
            const size_t avail = fsize > L.dataOffset ? (fsize - L.dataOffset) : 0;
            L.nrec = avail / L.recordSize; // recalculé depuis la taille (robuste)
            return L;
        }
    }
    L.nrec = fsize / sizeof(BinRecord); // ancien format sans en-tête
    return L;
}

// Lit l'enregistrement d'index idx (on n'exploite que les 16 premiers octets :
// ts/t/h/p ; le pas de déplacement suit recordSize, d'où la compatibilité avec
// de futurs enregistrements plus grands).
static bool readBinRecordAt(File& f, const BinLayout& L, size_t idx, BinRecord& br) {
    if (!f.seek(L.dataOffset + static_cast<uint32_t>(idx) * L.recordSize)) return false;
    return f.read(reinterpret_cast<uint8_t*>(&br), sizeof(BinRecord)) == (int)sizeof(BinRecord);
}

// Parcours SÉQUENTIEL des enregistrements [startIdx, nrec) d'un .bin. Pour le
// format courant (recordSize == 16 octets), les enregistrements sont contigus :
// on lit par blocs de plusieurs Ko (un seul seek initial, aucun seek par mesure),
// ce qui accélère nettement la lecture SD par rapport à un accès mesure par mesure.
// Le callback reçoit chaque enregistrement et renvoie false pour arrêter (p. ex.
// horodatage au-delà de la plage). Repli index par index si recordSize > 16.
template <typename Fn>
static void forEachBinRecordFrom(File& f, const BinLayout& L, size_t startIdx, Fn&& cb) {
    if (startIdx >= L.nrec) return;

    if (L.recordSize == sizeof(BinRecord)) {
        f.seek(L.dataOffset + static_cast<uint32_t>(startIdx) * L.recordSize);
        const size_t CHUNK = 64; // 64 * 16 = 1 Ko par lecture
        BinRecord buf[CHUNK];
        size_t remaining = L.nrec - startIdx;
        size_t processed = 0;
        while (remaining > 0) {
            const size_t want = remaining < CHUNK ? remaining : CHUNK;
            const int got = f.read(reinterpret_cast<uint8_t*>(buf), want * sizeof(BinRecord));
            if (got <= 0) break;
            const size_t nread = static_cast<size_t>(got) / sizeof(BinRecord);
            for (size_t k = 0; k < nread; ++k) {
                if (!cb(buf[k])) return;
            }
            COOPERATIVE_YIELD_EVERY(processed, 256);
            processed += nread;
            if (nread < want) break; // fin de fichier
            remaining -= nread;
        }
    } else {
        BinRecord br;
        for (size_t i = startIdx; i < L.nrec; ++i) {
            COOPERATIVE_YIELD_EVERY(i, 64);
            if (!readBinRecordAt(f, L, i, br)) return;
            if (!cb(br)) return;
        }
    }
}

// Convertit un ancien fichier .bin sans en-tête (v1.4.0) en fichier avec en-tête,
// une seule fois (réécriture complète via un fichier temporaire).
static void upgradeLegacyBin(const char* binPath, FileHeader& outHdr) {
    File rf = SD.open(binPath, FILE_READ);
    if (!rf) return;
    std::string tmp = std::string(binPath) + ".tmp";
    File wf = SD.open(tmp.c_str(), FILE_WRITE);
    if (!wf) { rf.close(); return; }

    FileHeader h = makeHeader();
    wf.write(reinterpret_cast<const uint8_t*>(&h), sizeof(h)); // en-tête provisoire

    BinRecord br;
    uint32_t count = 0; uint64_t firstTs = 0, lastTs = 0;
    size_t it = 0;
    while (rf.read(reinterpret_cast<uint8_t*>(&br), sizeof(br)) == (int)sizeof(br)) {
        COOPERATIVE_YIELD_EVERY(it, 64); it++;
        wf.write(reinterpret_cast<const uint8_t*>(&br), sizeof(br));
        if (count == 0) firstTs = br.ts;
        lastTs = br.ts;
        count++;
    }
    rf.close();

    h.recordCount = count; h.firstTimestamp = firstTs; h.lastTimestamp = lastTs;
    wf.seek(0);
    wf.write(reinterpret_cast<const uint8_t*>(&h), sizeof(h));
    wf.flush();
    wf.close();

    SD.remove(binPath);
    SD.rename(tmp.c_str(), binPath);
    outHdr = h;
    LOG_INFO("Upgraded legacy bin: " + std::string(binPath));
}

void HistoryManager::begin(SdManager* sd) {
    _sd = sd;
    loadRecent();
    if (_sd && _sd->isAvailable()) {
        createSdStructure();
        migrateCsvToBinary(); // conversion unique des anciens CSV plats -> binaire
    }
}

void HistoryManager::update() {
    // Tâches de fond si nécessaire
}

void HistoryManager::add(float t, float h, float p) {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        LOG_WARNING("History: Time not synced, skipping record");
        return;
    }

    HistoryRecord record;
    record.timestamp = time(NULL);
    record.t = t;
    record.h = h;
    record.p = p;

    _recentHistory.push_back(record);
    if (_recentHistory.size() > MAX_RECENT_RECORDS) {
        _recentHistory.erase(_recentHistory.begin());
    }

    saveRecent(record);

    if (_sd && _sd->isAvailable()) {
        saveToSdBinary(record);
    }
}

const std::vector<HistoryRecord>& HistoryManager::getRecentHistory() const {
    return _recentHistory;
}

// Statistiques robustes par grandeur : écarte les valeurs aberrantes via la
// médiane et l'écart absolu médian (MAD). Contrairement au filtre temporel (qui
// ne repère qu'un pic d'un seul point), cette approche gère aussi les SÉRIES de
// valeurs aberrantes (ex. plusieurs mesures à 0 d'affilée lors d'échecs I2C). Le
// seuil est piloté par les données (pas fixe) ; un plancher évite d'écarter les
// variations normales quand le MAD est très faible (données très stables).
static void robustMetric(const std::vector<float>& v, float floor, StatMetric& out) {
    const size_t n = v.size();
    if (n == 0) return;
    std::vector<float> tmp(v);
    std::nth_element(tmp.begin(), tmp.begin() + n / 2, tmp.end());
    const float median = tmp[n / 2];
    for (auto& x : tmp) x = fabsf(x - median);
    std::nth_element(tmp.begin(), tmp.begin() + n / 2, tmp.end());
    const float mad = tmp[n / 2];
    const float thr = fmaxf(floor, 5.0f * 1.4826f * mad); // 1.4826 : MAD -> écart-type
    for (float x : v) {
        if (fabsf(x - median) <= thr) out.add(x);
    }
}

Stats24h HistoryManager::getRecentStats() const {
    Stats24h stats;
    const auto& H = _recentHistory;
    const size_t n = H.size();
    stats.count = n;
    if (n == 0) return stats;

    std::vector<float> vt, vh, vp;
    vt.reserve(n); vh.reserve(n); vp.reserve(n);
    size_t it = 0;
    for (const auto& r : H) {
        COOPERATIVE_YIELD_EVERY(it, 256); it++;
        vt.push_back(r.t); vh.push_back(r.h); vp.push_back(r.p);
    }

    robustMetric(vt, 2.0f, stats.temp);
    robustMetric(vh, 8.0f, stats.hum);
    robustMetric(vp, 3.0f, stats.pres);
    return stats;
}

void HistoryManager::loadRecent() {
    if (!LittleFS.exists(HISTORY_FILE)) return;

    File f = LittleFS.open(HISTORY_FILE, "r");
    if (!f) return;

    size_t loaded_records = 0;
    while (f.available()) {
        HistoryRecord r;
        if (f.read((uint8_t*)&r, sizeof(HistoryRecord)) == sizeof(HistoryRecord)) {
            _recentHistory.push_back(r);
            loaded_records++;
            COOPERATIVE_YIELD_EVERY(loaded_records, 256);
        }
    }
    f.close();
    
    if (_recentHistory.size() > MAX_RECENT_RECORDS) {
        const size_t overflow = _recentHistory.size() - MAX_RECENT_RECORDS;
        _recentHistory.erase(_recentHistory.begin(), _recentHistory.begin() + overflow);
    }
    
    LOG_INFO("History loaded: " + std::to_string(_recentHistory.size()) + " points");
}

void HistoryManager::saveRecent(const HistoryRecord& record) {
    File f = LittleFS.open(HISTORY_FILE, "a");
    if (f) {
        f.write((uint8_t*)&record, sizeof(HistoryRecord));
        f.flush(); // Appel simple, pas de test de retour (void sur certains cores)
        f.close();
    } else {
        LOG_ERROR("Failed to append history to LittleFS");
    }
}

void HistoryManager::buildDayPaths(const struct tm& tinfo, char* binPath, char* statsPath, size_t sz) const {
    char date[16];
    strftime(date, sizeof(date), "%Y-%m-%d", &tinfo);       // AAAA-MM-JJ
    char dir[24];
    strftime(dir, sizeof(dir), "/history/%Y/%m", &tinfo);   // /history/AAAA/MM
    snprintf(binPath, sz, "%s/%s.bin", dir, date);
    snprintf(statsPath, sz, "%s/%s.stats", dir, date);
}

bool HistoryManager::ensureDayDirs(const struct tm& tinfo) const {
    char p[24];
    if (!SD.exists("/history") && !SD.mkdir("/history")) return false;
    strftime(p, sizeof(p), "/history/%Y", &tinfo);
    if (!SD.exists(p) && !SD.mkdir(p)) return false;
    strftime(p, sizeof(p), "/history/%Y/%m", &tinfo);
    if (!SD.exists(p) && !SD.mkdir(p)) return false;
    return true;
}

void HistoryManager::saveToSdBinary(const HistoryRecord& record) {
    time_t ts = record.timestamp;
    struct tm tinfo;
    if (!localtime_r(&ts, &tinfo)) {
        LOG_WARNING("Bin Save: localtime failed");
        return;
    }

    if (!ensureDayDirs(tinfo) && _sd && _sd->ensureMounted()) {
        ensureDayDirs(tinfo);
    }

    char binPath[48], statsPath[48];
    buildDayPaths(tinfo, binPath, statsPath, sizeof(binPath));

    BinRecord br { static_cast<uint32_t>(record.timestamp), record.t, record.h, record.p };

    // Inspecte le fichier : absent, avec en-tête, ou ancien format sans en-tête.
    FileHeader hdr = makeHeader();
    bool haveHeader = false, legacy = false;
    {
        File rf = SD.open(binPath, FILE_READ);
        if (rf) {
            const size_t sz = rf.size();
            if (sz >= sizeof(FileHeader)) {
                FileHeader h;
                rf.seek(0);
                if (rf.read(reinterpret_cast<uint8_t*>(&h), sizeof(h)) == (int)sizeof(h) && hdrMagicOk(h)) {
                    hdr = h; haveHeader = true;
                }
            }
            if (!haveHeader && sz > 0) legacy = true;
            rf.close();
        }
    }

    if (legacy) { upgradeLegacyBin(binPath, hdr); haveHeader = true; }

    if (!haveHeader) {
        // Nouveau fichier : écrit l'en-tête initial.
        File wf = SD.open(binPath, FILE_WRITE);
        if (!wf && _sd && _sd->ensureMounted()) { ensureDayDirs(tinfo); wf = SD.open(binPath, FILE_WRITE); }
        if (!wf) { LOG_ERROR("Bin Save: cannot create " + std::string(binPath)); return; }
        wf.write(reinterpret_cast<const uint8_t*>(&hdr), sizeof(hdr));
        wf.flush(); wf.close();
    }

    // Ajoute l'enregistrement en fin de fichier.
    File af = SD.open(binPath, FILE_APPEND);
    if (!af) { LOG_ERROR("Bin Save: cannot append " + std::string(binPath)); return; }
    af.write(reinterpret_cast<const uint8_t*>(&br), sizeof(br));
    af.flush(); af.close();

    // Met à jour l'en-tête (compteur + horodatages) — best effort, sans blocage
    // si le mode "r+" n'est pas supporté (les lecteurs recalculent nrec via la taille).
    hdr.recordCount += 1;
    hdr.lastTimestamp = static_cast<uint64_t>(record.timestamp);
    if (hdr.firstTimestamp == 0) hdr.firstTimestamp = static_cast<uint64_t>(record.timestamp);
    File uf = SD.open(binPath, "r+");
    if (uf) {
        uf.seek(0);
        uf.write(reinterpret_cast<const uint8_t*>(&hdr), sizeof(hdr));
        uf.flush(); uf.close();
    }

    updateDayStats(record, tinfo);
}

void HistoryManager::updateDayStats(const HistoryRecord& record, const struct tm& tinfo) {
    const uint32_t key = static_cast<uint32_t>(
        (tinfo.tm_year + 1900) * 10000 + (tinfo.tm_mon + 1) * 100 + tinfo.tm_mday);

    if (key != _currentDayKey) {
        // Changement de jour (ou premier enregistrement depuis le démarrage) :
        // recharge le .stats existant du jour s'il y en a un (reprise après reboot),
        // sinon repart d'une structure vierge.
        DayStats existing;
        if (readDayStats(record.timestamp, existing) && existing.count > 0) {
            _currentDayStats = existing;
        } else {
            _currentDayStats = DayStats();
        }
        _currentDayStats.magic = BIN_STATS_MAGIC;
        _currentDayKey = key;
    }

    DayStats& s = _currentDayStats;
    if (s.count == 0) {
        s.first_ts = static_cast<uint32_t>(record.timestamp);
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
    s.last_ts = static_cast<uint32_t>(record.timestamp);
    s.count++;

    // Persiste le .stats (petit fichier, réécrit intégralement à chaque mesure).
    char binPath[48], statsPath[48];
    buildDayPaths(tinfo, binPath, statsPath, sizeof(binPath));
    File sf = SD.open(statsPath, FILE_WRITE); // FILE_WRITE ("w") : tronque puis écrit
    if (sf) {
        sf.write(reinterpret_cast<const uint8_t*>(&s), sizeof(s));
        sf.flush();
        sf.close();
    }
}

bool HistoryManager::readDayStats(time_t day_ts, DayStats& out) const {
    if (!_sd || !_sd->isAvailable()) return false;
    struct tm tinfo;
    if (!localtime_r(&day_ts, &tinfo)) return false;

    char binPath[48], statsPath[48];
    buildDayPaths(tinfo, binPath, statsPath, sizeof(binPath));
    if (!SD.exists(statsPath)) return false;

    File f = SD.open(statsPath, FILE_READ);
    if (!f) return false;
    DayStats tmp;
    size_t n = f.read(reinterpret_cast<uint8_t*>(&tmp), sizeof(tmp));
    f.close();
    if (n != sizeof(tmp) || tmp.magic != BIN_STATS_MAGIC) return false;
    out = tmp;
    return true;
}

// Recherche la mesure la plus proche de target_ts dans le .bin du jour, par
// dichotomie (les enregistrements sont chronologiques et de taille fixe).
bool HistoryManager::readBinSampleNear(time_t target_ts, float& t_out, float& h_out, float& p_out) const {
    if (!_sd || !_sd->isAvailable()) return false;
    struct tm tinfo;
    if (!localtime_r(&target_ts, &tinfo)) return false;

    char binPath[48], statsPath[48];
    buildDayPaths(tinfo, binPath, statsPath, sizeof(binPath));
    if (!SD.exists(binPath)) return false;

    File f = SD.open(binPath, FILE_READ);
    if (!f) return false;

    const BinLayout L = probeBin(f);
    if (L.nrec == 0) { f.close(); return false; }

    // Dichotomie : premier index dont ts >= target.
    size_t lo = 0, hi = L.nrec;
    BinRecord br;
    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        if (!readBinRecordAt(f, L, mid, br)) break;
        if (static_cast<long>(br.ts) < static_cast<long>(target_ts)) lo = mid + 1;
        else hi = mid;
    }

    long best_diff = LONG_MAX;
    bool found = false;
    // Examine les voisins immédiats de l'index trouvé (avant/après).
    for (long k = static_cast<long>(lo) - 1; k <= static_cast<long>(lo); ++k) {
        if (k < 0 || k >= static_cast<long>(L.nrec)) continue;
        if (!readBinRecordAt(f, L, static_cast<size_t>(k), br)) continue;
        long diff = labs(static_cast<long>(br.ts) - static_cast<long>(target_ts));
        if (diff < best_diff) {
            best_diff = diff;
            t_out = br.t; h_out = br.h; p_out = br.p;
            found = true;
        }
    }
    f.close();
    return found && best_diff <= SD_SAMPLE_TOLERANCE_S;
}

std::vector<HistoryPoint> HistoryManager::queryRange(time_t from, time_t to, long interval_s) const {
    std::vector<HistoryPoint> out;
    if (to <= from) return out;
    if (interval_s < 1) interval_s = 1;

    // Nombre de tranches, borné pour limiter la consommation mémoire sur l'ESP32.
    // Si la plage est trop large pour l'intervalle demandé, on élargit l'intervalle.
    const long MAX_BUCKETS = 800;
    long span = static_cast<long>(to - from);
    long nbuckets = (span + interval_s - 1) / interval_s;
    if (nbuckets > MAX_BUCKETS) {
        interval_s = (span + MAX_BUCKETS - 1) / MAX_BUCKETS;
        nbuckets = (span + interval_s - 1) / interval_s;
    }
    if (nbuckets < 1) nbuckets = 1;

    // Accumulateurs par grandeur : une grandeur peut être écartée d'une mesure
    // (valeur aberrante) sans invalider les deux autres.
    struct Acc { double t = 0, h = 0, p = 0; int nt = 0, nh = 0, np = 0; };
    std::vector<Acc> accs(static_cast<size_t>(nbuckets));

    const long from_l = static_cast<long>(from);
    const long to_l = static_cast<long>(to);
    auto accT = [&](long ts, float v) {
        long idx = (ts - from_l) / interval_s;
        if (ts < from_l || ts > to_l || idx < 0 || idx >= nbuckets) return;
        accs[static_cast<size_t>(idx)].t += v; accs[static_cast<size_t>(idx)].nt++;
    };
    auto accH = [&](long ts, float v) {
        long idx = (ts - from_l) / interval_s;
        if (ts < from_l || ts > to_l || idx < 0 || idx >= nbuckets) return;
        accs[static_cast<size_t>(idx)].h += v; accs[static_cast<size_t>(idx)].nh++;
    };
    auto accP = [&](long ts, float v) {
        long idx = (ts - from_l) / interval_s;
        if (ts < from_l || ts > to_l || idx < 0 || idx >= nbuckets) return;
        accs[static_cast<size_t>(idx)].p += v; accs[static_cast<size_t>(idx)].np++;
    };

    // Filtre anti-aberrations en flux : fenêtre glissante de 3 mesures. La mesure
    // centrale est testée par grandeur contre ses voisines avant d'être agrégée ;
    // les mesures de bord (première/dernière, sans deux voisines) sont conservées.
    int wn = 0; long wts[3]; float wt[3], wh[3], wp[3];
    auto flushMiddle = [&]() {
        if (!isTemporalOutlier(wt[0], wt[1], wt[2], OUTLIER_FLOOR_T)) accT(wts[1], wt[1]);
        if (!isTemporalOutlier(wh[0], wh[1], wh[2], OUTLIER_FLOOR_H)) accH(wts[1], wh[1]);
        if (!isTemporalOutlier(wp[0], wp[1], wp[2], OUTLIER_FLOOR_P)) accP(wts[1], wp[1]);
    };
    auto feed = [&](long ts, float t, float h, float p) {
        if (wn == 0) {
            wts[0] = ts; wt[0] = t; wh[0] = h; wp[0] = p; wn = 1;
            accT(ts, t); accH(ts, h); accP(ts, p); // bord gauche : conservé tel quel
        } else if (wn == 1) {
            wts[1] = ts; wt[1] = t; wh[1] = h; wp[1] = p; wn = 2;
        } else if (wn == 2) {
            wts[2] = ts; wt[2] = t; wh[2] = h; wp[2] = p; wn = 3;
            flushMiddle();
        } else {
            wts[0] = wts[1]; wt[0] = wt[1]; wh[0] = wh[1]; wp[0] = wp[1];
            wts[1] = wts[2]; wt[1] = wt[2]; wh[1] = wh[2]; wp[1] = wp[2];
            wts[2] = ts; wt[2] = t; wh[2] = h; wp[2] = p;
            flushMiddle();
        }
    };
    auto flushEnd = [&]() {
        // La dernière mesure (bord droit) n'a jamais été centrale : agrégée telle quelle.
        if (wn >= 2) { int last = wn - 1; accT(wts[last], wt[last]); accH(wts[last], wh[last]); accP(wts[last], wp[last]); }
    };

    long lastSdTs = LONG_MIN; // horodatage le plus récent trouvé sur SD

    // 1) Source SD : lit les fichiers binaires journaliers couvrant [from, to].
    //    Grâce aux enregistrements de taille fixe et chronologiques, on saute
    //    directement (dichotomie) au premier enregistrement >= from, puis on lit
    //    séquentiellement jusqu'à dépasser to — sans relire tout le fichier.
    //    Repli sur l'ancien CSV plat si un .bin n'existe pas encore.
    if (_sd && _sd->isAvailable()) {
        char prevBin[48] = {0};
        size_t day_iter = 0;
        for (time_t cursor = from; cursor <= to + 86400; cursor += 86400) {
            COOPERATIVE_YIELD_EVERY(day_iter, 8);
            day_iter++;
            struct tm tinfo;
            if (!localtime_r(&cursor, &tinfo)) continue;
            char binPath[48], statsPath[48];
            buildDayPaths(tinfo, binPath, statsPath, sizeof(binPath));
            if (strcmp(binPath, prevBin) == 0) continue; // évite un double parcours (bord DST)
            strncpy(prevBin, binPath, sizeof(prevBin));
            prevBin[sizeof(prevBin) - 1] = '\0';

            if (SD.exists(binPath)) {
                File f = SD.open(binPath, FILE_READ);
                if (!f) continue;
                const BinLayout L = probeBin(f);
                BinRecord br;

                // Dichotomie : premier index dont ts >= from.
                size_t lo = 0, hi = L.nrec;
                while (lo < hi) {
                    size_t mid = (lo + hi) / 2;
                    if (!readBinRecordAt(f, L, mid, br)) break;
                    if (static_cast<long>(br.ts) < from_l) lo = mid + 1; else hi = mid;
                }

                forEachBinRecordFrom(f, L, lo, [&](const BinRecord& rec) -> bool {
                    const long ts = static_cast<long>(rec.ts);
                    if (ts > to_l) return false; // triés : les suivants aussi
                    feed(ts, rec.t, rec.h, rec.p);
                    if (ts > lastSdTs) lastSdTs = ts;
                    return true;
                });
                f.close();
                continue;
            }

            // Repli : ancien fichier CSV plat /history/AAAA-MM-JJ.csv
            char csvPath[32];
            strftime(csvPath, sizeof(csvPath), "/history/%Y-%m-%d.csv", &tinfo);
            if (!SD.exists(csvPath)) continue;
            File f = SD.open(csvPath, FILE_READ);
            if (!f) continue;
            bool first_line = true;
            size_t line_count = 0;
            while (f.available()) {
                String line = f.readStringUntil('\n');
                COOPERATIVE_YIELD_EVERY(line_count, 32);
                line_count++;
                if (first_line) { first_line = false; continue; }
                if (line.length() == 0) continue;
                long ts = 0; float t = 0, h = 0, p = 0;
                if (sscanf(line.c_str(), "%ld,%f,%f,%f", &ts, &t, &h, &p) == 4) {
                    feed(ts, t, h, p);
                    if (ts > lastSdTs) lastSdTs = ts;
                }
            }
            f.close();
        }
    }

    // 2) Complète avec l'historique RAM pour la portion non encore écrite sur SD
    //    (ou la totalité de la plage si aucune carte SD n'est disponible).
    size_t ram_iteration = 0;
    for (const auto& r : _recentHistory) {
        COOPERATIVE_YIELD_EVERY(ram_iteration, 256);
        ram_iteration++;
        long ts = static_cast<long>(r.timestamp);
        if (ts <= lastSdTs) continue;
        feed(ts, r.t, r.h, r.p);
    }
    flushEnd(); // agrège la dernière mesure retenue dans la fenêtre glissante

    // 3) Sérialise les tranches (moyenne par grandeur ; grandeur vide => null).
    out.reserve(static_cast<size_t>(nbuckets));
    for (long i = 0; i < nbuckets; ++i) {
        const Acc& a = accs[static_cast<size_t>(i)];
        HistoryPoint pt;
        pt.t = from + static_cast<time_t>(i * interval_s + interval_s / 2); // centre de tranche
        pt.tvalid = a.nt > 0; pt.hvalid = a.nh > 0; pt.pvalid = a.np > 0;
        pt.temp = pt.tvalid ? static_cast<float>(a.t / a.nt) : 0.0f;
        pt.hum  = pt.hvalid ? static_cast<float>(a.h / a.nh) : 0.0f;
        pt.pres = pt.pvalid ? static_cast<float>(a.p / a.np) : 0.0f;
        pt.valid = pt.tvalid || pt.hvalid || pt.pvalid;
        if (!pt.valid) {
            pt.temp = pt.hum = pt.pres = 0.0f;
            pt.valid = false;
        }
        out.push_back(pt);
    }
    return out;
}

bool HistoryManager::readSdSampleNear(time_t target_ts, float& t_out, float& h_out, float& p_out) const {
    if (!_sd || !_sd->isAvailable()) return false;

    // Priorité au format binaire ; repli sur l'ancien CSV plat si absent.
    if (readBinSampleNear(target_ts, t_out, h_out, p_out)) return true;

    struct tm timeinfo;
    if (!localtime_r(&target_ts, &timeinfo)) return false;

    char filename[32];
    strftime(filename, sizeof(filename), "/history/%Y-%m-%d.csv", &timeinfo);

    if (!SD.exists(filename)) return false;

    File f = SD.open(filename, FILE_READ);
    if (!f) return false;

    bool found = false;
    long best_diff = LONG_MAX;
    bool first_line = true;
    size_t line_count = 0;

    while (f.available()) {
        String line = f.readStringUntil('\n');
        COOPERATIVE_YIELD_EVERY(line_count, 256);
        line_count++;

        if (first_line) {
            first_line = false;
            continue; // en-tête CSV "Timestamp,Temperature,Humidity,Pressure"
        }
        if (line.length() == 0) continue;

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
    f.close();

    return found && best_diff <= SD_SAMPLE_TOLERANCE_S;
}

void HistoryManager::createSdStructure() {
    if (!SD.exists("/history")) {
        if (SD.mkdir("/history")) {
            LOG_INFO("Created /history directory on SD card.");
        } else {
            LOG_ERROR("Failed to create /history directory on SD card.");
        }
    }
}

void HistoryManager::removeDirRecursive(const char* path) const {
    File dir = SD.open(path);
    if (!dir) return;
    if (!dir.isDirectory()) { dir.close(); SD.remove(path); return; }

    File entry = dir.openNextFile();
    while (entry) {
        // path() renvoie le chemin absolu (nécessaire pour SD.remove/rmdir).
        std::string child = entry.path();
        bool isDir = entry.isDirectory();
        entry.close();
        if (isDir) {
            removeDirRecursive(child.c_str());
        } else {
            SD.remove(child.c_str());
        }
        entry = dir.openNextFile();
    }
    dir.close();
    SD.rmdir(path);
}

void HistoryManager::clearHistory() {
    _recentHistory.clear();
    _currentDayKey = 0;
    _currentDayStats = DayStats();
    LittleFS.remove(HISTORY_FILE);

    if (_sd && _sd->isAvailable()) {
        LOG_INFO("Clearing history from SD card...");
        // Supprime récursivement toute l'arborescence puis la recrée vide.
        removeDirRecursive("/history");
        SD.mkdir("/history");
    }
    LOG_INFO("History cleared");
}

RangeSynthesis HistoryManager::querySynthesis(time_t from, time_t to) const {
    RangeSynthesis r;
    if (to <= from || !_sd || !_sd->isAvailable()) return r;

    double t_sum = 0, h_sum = 0, p_sum = 0;
    char prevBin[48] = {0};
    size_t day_iter = 0;

    // Agrège les .stats journaliers de la plage (aucune relecture des mesures).
    // Note : pour un jour situé à un bord de la plage et partiellement couvert,
    // les statistiques du jour entier sont utilisées (approximation assumée au
    // profit d'un affichage quasi instantané).
    for (time_t cursor = from; cursor <= to + 86400; cursor += 86400) {
        COOPERATIVE_YIELD_EVERY(day_iter, 8);
        day_iter++;
        struct tm tinfo;
        if (!localtime_r(&cursor, &tinfo)) continue;
        char binPath[48], statsPath[48];
        buildDayPaths(tinfo, binPath, statsPath, sizeof(binPath));
        if (strcmp(binPath, prevBin) == 0) continue;
        strncpy(prevBin, binPath, sizeof(prevBin));
        prevBin[sizeof(prevBin) - 1] = '\0';

        DayStats s;
        if (!readDayStats(cursor, s) || s.count == 0) continue;

        if (!r.valid) {
            r.t_min = s.t_min; r.t_max = s.t_max;
            r.h_min = s.h_min; r.h_max = s.h_max;
            r.p_min = s.p_min; r.p_max = s.p_max;
            r.t_first = s.t_first; r.h_first = s.h_first; r.p_first = s.p_first;
            r.valid = true;
        } else {
            if (s.t_min < r.t_min) r.t_min = s.t_min;
            if (s.t_max > r.t_max) r.t_max = s.t_max;
            if (s.h_min < r.h_min) r.h_min = s.h_min;
            if (s.h_max > r.h_max) r.h_max = s.h_max;
            if (s.p_min < r.p_min) r.p_min = s.p_min;
            if (s.p_max > r.p_max) r.p_max = s.p_max;
        }
        // La dernière valeur correspond au dernier jour rencontré dans la plage.
        r.t_last = s.t_last; r.h_last = s.h_last; r.p_last = s.p_last;
        r.count += s.count;
        t_sum += s.t_sum; h_sum += s.h_sum; p_sum += s.p_sum;
    }

    if (r.valid && r.count > 0) {
        r.t_avg = static_cast<float>(t_sum / r.count);
        r.h_avg = static_cast<float>(h_sum / r.count);
        r.p_avg = static_cast<float>(p_sum / r.count);
    }
    return r;
}

void HistoryManager::exportCsv(time_t from, time_t to, const std::function<void(const char*)>& emit) const {
    emit("Timestamp,Temperature,Humidity,Pressure\n");
    if (to <= from || !_sd || !_sd->isAvailable()) return;

    const long from_l = static_cast<long>(from);
    const long to_l = static_cast<long>(to);
    char prevBin[48] = {0};
    size_t day_iter = 0;

    for (time_t cursor = from; cursor <= to + 86400; cursor += 86400) {
        COOPERATIVE_YIELD_EVERY(day_iter, 8); // export « Tout » : la plage peut couvrir des années
        day_iter++;
        struct tm tinfo;
        if (!localtime_r(&cursor, &tinfo)) continue;
        char binPath[48], statsPath[48];
        buildDayPaths(tinfo, binPath, statsPath, sizeof(binPath));
        if (strcmp(binPath, prevBin) == 0) continue;
        strncpy(prevBin, binPath, sizeof(prevBin));
        prevBin[sizeof(prevBin) - 1] = '\0';

        if (!SD.exists(binPath)) continue;
        File f = SD.open(binPath, FILE_READ);
        if (!f) continue;
        const BinLayout L = probeBin(f);
        BinRecord br;

        // Dichotomie : premier enregistrement dont ts >= from.
        size_t lo = 0, hi = L.nrec;
        while (lo < hi) {
            size_t mid = (lo + hi) / 2;
            if (!readBinRecordAt(f, L, mid, br)) break;
            if (static_cast<long>(br.ts) < from_l) lo = mid + 1; else hi = mid;
        }

        char line[64];
        forEachBinRecordFrom(f, L, lo, [&](const BinRecord& rec) -> bool {
            if (static_cast<long>(rec.ts) > to_l) return false;
            snprintf(line, sizeof(line), "%lu,%.2f,%.1f,%.1f\n",
                     static_cast<unsigned long>(rec.ts), rec.t, rec.h, rec.p);
            emit(line);
            return true;
        });
        f.close();
    }
}

// Conversion unique des anciens fichiers CSV plats (/history/AAAA-MM-JJ.csv) vers
// le format binaire journalier (+ .stats). Chaque CSV migré est renommé en .bak
// pour ne pas être retraité au démarrage suivant.
void HistoryManager::migrateCsvToBinary() {
    if (!_sd || !_sd->isAvailable()) return;

    File root = SD.open("/history");
    if (!root || !root.isDirectory()) { if (root) root.close(); return; }

    // Collecte d'abord les CSV à migrer (on ne modifie pas le dossier en le parcourant).
    std::vector<std::string> csvFiles;
    File entry = root.openNextFile();
    while (entry) {
        std::string name = entry.name(); // nom de base
        bool isDir = entry.isDirectory();
        std::string full = entry.path();
        entry.close();
        if (!isDir && name.size() > 4 && name.compare(name.size() - 4, 4, ".csv") == 0) {
            csvFiles.push_back(full);
        }
        entry = root.openNextFile();
    }
    root.close();

    if (csvFiles.empty()) return;
    LOG_INFO("History migration: " + std::to_string(csvFiles.size()) + " CSV file(s) to convert");

    for (const auto& csvPath : csvFiles) {
        File in = SD.open(csvPath.c_str(), FILE_READ);
        if (!in) continue;

        DayStats stats;
        stats.magic = BIN_STATS_MAGIC;
        struct tm firstTm;
        bool haveDir = false;
        char binPath[48] = {0}, statsPath[48] = {0};
        File out;
        FileHeader mhdr = makeHeader();

        bool first_line = true;
        size_t line_count = 0;
        while (in.available()) {
            String line = in.readStringUntil('\n');
            COOPERATIVE_YIELD_EVERY(line_count, 32);
            line_count++;
            if (first_line) { first_line = false; continue; } // en-tête CSV
            if (line.length() == 0) continue;

            long ts = 0; float t = 0, h = 0, p = 0;
            if (sscanf(line.c_str(), "%ld,%f,%f,%f", &ts, &t, &h, &p) != 4) continue;

            // Crée l'arborescence et ouvre le .bin au premier échantillon valide.
            if (!haveDir) {
                time_t tt = static_cast<time_t>(ts);
                if (!localtime_r(&tt, &firstTm)) break;
                ensureDayDirs(firstTm);
                buildDayPaths(firstTm, binPath, statsPath, sizeof(binPath));
                out = SD.open(binPath, FILE_WRITE); // (re)crée le .bin
                if (!out) break;
                out.write(reinterpret_cast<const uint8_t*>(&mhdr), sizeof(mhdr)); // en-tête provisoire
                haveDir = true;
            }

            BinRecord br { static_cast<uint32_t>(ts), t, h, p };
            out.write(reinterpret_cast<const uint8_t*>(&br), sizeof(br));

            if (stats.count == 0) {
                stats.first_ts = static_cast<uint32_t>(ts);
                stats.t_min = stats.t_max = stats.t_first = t;
                stats.h_min = stats.h_max = stats.h_first = h;
                stats.p_min = stats.p_max = stats.p_first = p;
            }
            if (t < stats.t_min) stats.t_min = t;
            if (t > stats.t_max) stats.t_max = t;
            if (h < stats.h_min) stats.h_min = h;
            if (h > stats.h_max) stats.h_max = h;
            if (p < stats.p_min) stats.p_min = p;
            if (p > stats.p_max) stats.p_max = p;
            stats.t_sum += t; stats.h_sum += h; stats.p_sum += p;
            stats.t_last = t; stats.h_last = h; stats.p_last = p;
            stats.last_ts = static_cast<uint32_t>(ts);
            stats.count++;
        }
        in.close();

        if (haveDir && out) {
            // Finalise l'en-tête (compteur + horodatages) au début du .bin.
            mhdr.recordCount = stats.count;
            mhdr.firstTimestamp = stats.first_ts;
            mhdr.lastTimestamp = stats.last_ts;
            out.seek(0);
            out.write(reinterpret_cast<const uint8_t*>(&mhdr), sizeof(mhdr));
            out.flush();
            out.close();
            File sf = SD.open(statsPath, FILE_WRITE);
            if (sf) {
                sf.write(reinterpret_cast<const uint8_t*>(&stats), sizeof(stats));
                sf.flush();
                sf.close();
            }
        }

        // Renomme le CSV source pour ne pas le retraiter au prochain démarrage.
        std::string bak = csvPath + ".bak";
        SD.rename(csvPath.c_str(), bak.c_str());
        LOG_INFO("Migrated " + csvPath + " (" + std::to_string(stats.count) + " points)");
    }

    _currentDayKey = 0; // forcera un rechargement propre du .stats du jour courant
    LOG_INFO("History migration complete");
}

MeteoTrend HistoryManager::getTrend() const {
    MeteoTrend trend;
    if (_recentHistory.empty()) return trend;

    const time_t now = _recentHistory.back().timestamp;
    float t_now = _recentHistory.back().t;
    float h_now = _recentHistory.back().h;
    float p_now = _recentHistory.back().p;

    float t_1h = t_now, h_1h = h_now, p_1h = p_now;
    float t_12h = t_now, h_12h = h_now, p_12h = p_now;
    float t_24h = t_now, h_24h = h_now, p_24h = p_now;
    bool found_1h = false, found_12h = false, found_24h = false;
    size_t trend_iteration = 0;

    // L'historique RAM (_recentHistory) couvre au maximum 24h (échantillonnage ~1 min).
    for (auto it = _recentHistory.rbegin(); it != _recentHistory.rend(); ++it) {
        COOPERATIVE_YIELD_EVERY(trend_iteration, 256);
        trend_iteration++;

        time_t dt = now - it->timestamp;
        if (!found_1h && dt >= 3600) {
            t_1h = it->t;
            h_1h = it->h;
            p_1h = it->p;
            found_1h = true;
        }
        if (!found_12h && dt >= 43200) {
            t_12h = it->t;
            h_12h = it->h;
            p_12h = it->p;
            found_12h = true;
        }
        if (!found_24h && dt >= 86400) {
            t_24h = it->t;
            h_24h = it->h;
            p_24h = it->p;
            found_24h = true;
            break;
        }
    }

    // Le point à J-48h n'existe plus en RAM : on va le chercher dans le CSV journalier sur SD, si disponible.
    float t_48h = t_now, h_48h = h_now, p_48h = p_now;
    trend.available_48h = readSdSampleNear(now - 172800, t_48h, h_48h, p_48h);

    trend.temp.delta_1h = t_now - t_1h;
    trend.temp.delta_12h = t_now - t_12h;
    trend.temp.delta_24h = t_now - t_24h;
    trend.temp.delta_48h = trend.available_48h ? (t_now - t_48h) : 0.0f;
    trend.hum.delta_1h = h_now - h_1h;
    trend.hum.delta_12h = h_now - h_12h;
    trend.hum.delta_24h = h_now - h_24h;
    trend.hum.delta_48h = trend.available_48h ? (h_now - h_48h) : 0.0f;
    trend.pres.delta_1h = p_now - p_1h;
    trend.pres.delta_12h = p_now - p_12h;
    trend.pres.delta_24h = p_now - p_24h;
    trend.pres.delta_48h = trend.available_48h ? (p_now - p_48h) : 0.0f;

    auto dir = [](float d) {
        if (d > 0.2) return std::string("hausse");
        if (d < -0.2) return std::string("baisse");
        return std::string("stable");
    };
    trend.temp.direction_1h = dir(trend.temp.delta_1h);
    trend.temp.direction_12h = dir(trend.temp.delta_12h);
    trend.temp.direction_24h = dir(trend.temp.delta_24h);
    trend.temp.direction_48h = trend.available_48h ? dir(trend.temp.delta_48h) : "indisponible";
    trend.hum.direction_1h = dir(trend.hum.delta_1h);
    trend.hum.direction_12h = dir(trend.hum.delta_12h);
    trend.hum.direction_24h = dir(trend.hum.delta_24h);
    trend.hum.direction_48h = trend.available_48h ? dir(trend.hum.delta_48h) : "indisponible";
    trend.pres.direction_1h = dir(trend.pres.delta_1h);
    trend.pres.direction_12h = dir(trend.pres.delta_12h);
    trend.pres.direction_24h = dir(trend.pres.delta_24h);
    trend.pres.direction_48h = trend.available_48h ? dir(trend.pres.delta_48h) : "indisponible";

    return trend;
}