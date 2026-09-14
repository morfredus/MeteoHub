#include "history_manager.h"
#include <LittleFS.h>
#include <SD.h>
#include "../utils/logs.h"
#include "../utils/cooperative_yield.h" // INCLUSION AJOUTÉE
#include "../modules/meteo_context.h"
#include <time.h>
#include <inttypes.h>
#include <climits>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <Arduino.h>

// Arborescence symétrique IN/OUT (voir header) :
//   SPIFFS : /history/indoor_recent.dat  +  /history/outdoor_recent.dat
//   SD     : /history/indoor/AAAA/MM/...  +  /history/outdoor/AAAA/MM/...
#define HISTORY_FILE "/history/indoor_recent.dat"
// Ancien nom (avant symétrie IN/OUT), supprimé par clearHistory() pour ne pas
// laisser un orphelin sur la partition SPIFFS après reflash.
#define HISTORY_FILE_LEGACY "/history/recent.dat"
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

// --- Nouvelles méthodes explicites IN/OUT (ÉTAPE 1) ---
// Pour l'instant, ces méthodes délèguent aux méthodes existantes (implicitement IN)
// Cela permet une transition progressive sans casser l'existant

void HistoryManager::addIndoor(const IndoorData& data) {
    // Décision legacy = IN : l'historique historique EST le flux intérieur.
    // addIndoor est le point d'entrée explicite ; add() en reste l'écriture
    // concrète (conservée pour les appels existants et la compatibilité disque).
    add(data.temperature, data.humidity, data.pressure);
}

void HistoryManager::addOutdoor(const OutdoorData& data) {
    // RÈGLE STRICTE : seule une trame OUT réellement REÇUE et VALIDE entre dans
    // l'historique OUT. Jamais une valeur intérieure, jamais la valeur
    // « effective » (présentation), jamais une valeur seedée depuis le disque au
    // boot. L'historique répond à « qu'a réellement mesuré le capteur OUT ? »,
    // pas à « que montre-t-on maintenant ? » : ce sont deux chemins distincts.
    // Une trame invalide (implausible / mal formée) ne met à jour NI le live NI
    // l'historique. Ce garde protège l'invariant même si un futur appelant
    // oubliait de filtrer en amont.
    if (!data.valid) {
        LOG_WARNING("History: Outdoor frame invalid, ignored (no live, no archive)");
        return;
    }

    // Trame OUT réelle et valide : elle devient le dernier relevé « live » et
    // fait référence pour la fraîcheur (contrairement au seed disque, qui reste
    // volontairement « périmé »). _hasOutdoorRadioMs = « au moins une vraie trame
    // reçue depuis le boot » : avant lui, l'OUT est en attente de 1re réception.
    _lastOutdoorLive = data;
    _hasLiveOutdoor = true;
    _lastOutdoorRadioMs = millis();
    _hasOutdoorRadioMs = true;

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        LOG_WARNING("History: Time not synced, outdoor live kept, archive skipped");
        return;
    }

    OutdoorHistoryRecord record;
    record.timestamp = time(NULL);
    record.t = data.temperature;
    record.h = data.humidity;
    record.p = data.pressure;
    
    // Extensions futures (vent, pluie, UV)
    record.wind_speed = data.wind_speed;
    record.wind_gust = data.wind_gust;
    record.wind_direction_deg = data.wind_direction_deg;
    record.rain_rate = data.rain_rate;
    record.rain_accumulated = data.rain_accumulated;
    record.solar_lux = data.solar_lux;
    record.uv_index = data.uv_index;

    _outdoorHistory.push_back(record);
    if (_outdoorHistory.size() > MAX_RECENT_RECORDS) {
        _outdoorHistory.erase(_outdoorHistory.begin());
    }

    saveOutdoorRecent(record);
    updateOutdoorDayStats(record, timeinfo);

    if (_sd && _sd->isAvailable()) {
        saveOutdoorToSdBinary(record);
    }
    
    LOG_INFO("History: Outdoor data added (T=" + std::to_string(data.temperature) + "°C, H=" + std::to_string(data.humidity) + "%)");
}

const std::vector<IndoorHistoryRecord>& HistoryManager::getIndoorHistory() const {
    // legacy = IN : l'historique récent est le flux intérieur. On l'expose sous
    // le type IndoorHistoryRecord, converti à la volée dans un cache statique.
    static std::vector<IndoorHistoryRecord> _indoorCache;
    _indoorCache.clear();
    for (const auto& rec : _recentHistory) {
        IndoorHistoryRecord irec;
        irec.timestamp = rec.timestamp;
        irec.t = rec.t;
        irec.h = rec.h;
        irec.p = rec.p;
        _indoorCache.push_back(irec);
    }
    return _indoorCache;
}

const std::vector<OutdoorHistoryRecord>& HistoryManager::getOutdoorHistory() const {
    // ÉTAPE 3: Renvoie l'historique OUT récent
    return _outdoorHistory;
}

Stats24h HistoryManager::getIndoorStats() const {
    // Statistiques IN sur les dernières 24 h (legacy = IN). L'historique RAM peut
    // couvrir bien plus (chargé depuis LittleFS au boot) : sans cette fenêtre, la
    // synthèse « 24 h » agrégerait des semaines de mesures et gonflerait les
    // amplitudes. On borne donc à now-24h dès que l'horloge est fiable.
    Stats24h stats;
    stats.count = 0;

    const time_t now = time(NULL);
    const time_t cutoff = (now > 86400) ? (now - 86400) : 0;

    std::vector<float> temps, hums, pres;
    for (const auto& rec : _recentHistory) {
        if (rec.timestamp < cutoff) continue;
        temps.push_back(rec.t);
        hums.push_back(rec.h);
        pres.push_back(rec.p);
        stats.count++;
    }
    
    // Utilisation du même moteur statistique robuste que pour l'existant
    // Copie locale de robustMetric pour l'ÉTAPE 4 (sera refactorisée plus tard)
    auto robustMetricLocal = [](const std::vector<float>& v, float floor, StatMetric& out) {
        const size_t n = v.size();
        if (n == 0) return;
        std::vector<float> tmp(v);
        std::nth_element(tmp.begin(), tmp.begin() + n / 2, tmp.end());
        const float median = tmp[n / 2];
        for (auto& x : tmp) x = fabsf(x - median);
        std::nth_element(tmp.begin(), tmp.begin() + n / 2, tmp.end());
        const float mad = tmp[n / 2];
        const float thr = fmaxf(floor, 5.0f * 1.4826f * mad);
        for (float x : v) {
            if (fabsf(x - median) <= thr) out.add(x);
        }
    };
    
    robustMetricLocal(temps, OUTLIER_FLOOR_T, stats.temp);
    robustMetricLocal(hums, OUTLIER_FLOOR_H, stats.hum);
    robustMetricLocal(pres, OUTLIER_FLOOR_P, stats.pres);
    
    return stats;
}

Stats24h HistoryManager::getOutdoorStats() const {
    // Statistiques OUT sur les dernières 24 h (même fenêtre que l'IN, pour que les
    // deux résumés soient comparables sur la même période).
    Stats24h stats;
    stats.count = 0;

    const time_t now = time(NULL);
    const time_t cutoff = (now > 86400) ? (now - 86400) : 0;

    std::vector<float> temps, hums, pres;
    for (const auto& rec : _outdoorHistory) {
        if (rec.timestamp < cutoff) continue;
        temps.push_back(rec.t);
        hums.push_back(rec.h);
        pres.push_back(rec.p);
        stats.count++;
    }
    
    // Utilisation du même moteur statistique robuste que pour IN
    auto robustMetricLocal = [](const std::vector<float>& v, float floor, StatMetric& out) {
        const size_t n = v.size();
        if (n == 0) return;
        std::vector<float> tmp(v);
        std::nth_element(tmp.begin(), tmp.begin() + n / 2, tmp.end());
        const float median = tmp[n / 2];
        for (auto& x : tmp) x = fabsf(x - median);
        std::nth_element(tmp.begin(), tmp.begin() + n / 2, tmp.end());
        const float mad = tmp[n / 2];
        const float thr = fmaxf(floor, 5.0f * 1.4826f * mad);
        for (float x : v) {
            if (fabsf(x - median) <= thr) out.add(x);
        }
    };
    
    robustMetricLocal(temps, OUTLIER_FLOOR_T, stats.temp);
    robustMetricLocal(hums, OUTLIER_FLOOR_H, stats.hum);
    robustMetricLocal(pres, OUTLIER_FLOOR_P, stats.pres);
    
    return stats;
}

// Requêtes de plage IN/OUT.
std::vector<IndoorHistoryPoint> HistoryManager::queryIndoorRange(time_t from, time_t to, long interval_s) const {
    // legacy = IN : on agrège la plage via queryRange puis on la typifie IN.
    std::vector<HistoryPoint> points = queryRange(from, to, interval_s);
    std::vector<IndoorHistoryPoint> indoorPoints;
    for (const auto& p : points) {
        IndoorHistoryPoint ip;
        ip.t = p.t;
        ip.temp = p.temp;
        ip.hum = p.hum;
        ip.pres = p.pres;
        ip.tvalid = p.tvalid;
        ip.hvalid = p.hvalid;
        ip.pvalid = p.pvalid;
        ip.valid = p.valid;
        indoorPoints.push_back(ip);
    }
    return indoorPoints;
}

std::vector<OutdoorHistoryPoint> HistoryManager::queryOutdoorRange(time_t from, time_t to, long interval_s) const {
    // Agrège la plage OUT via le coeur commun (racine /history/outdoor + RAM OUT),
    // puis typifie le résultat en points OUT.
    std::vector<HistoryPoint> points = queryRangeImpl(from, to, interval_s, true);
    std::vector<OutdoorHistoryPoint> outPoints;
    outPoints.reserve(points.size());
    for (const auto& p : points) {
        OutdoorHistoryPoint op;
        op.t = p.t;
        op.temp = p.temp;
        op.hum = p.hum;
        op.pres = p.pres;
        op.tvalid = p.tvalid;
        op.hvalid = p.hvalid;
        op.pvalid = p.pvalid;
        op.valid = p.valid;
        outPoints.push_back(op);
    }
    return outPoints;
}

// Méthodes querySynthesis IN/OUT (placeholder pour ÉTAPE 1)
RangeSynthesis HistoryManager::queryIndoorSynthesis(time_t from, time_t to) const {
    // legacy = IN : la synthèse existante est la synthèse intérieure.
    return querySynthesis(from, to);
}

RangeSynthesis HistoryManager::queryOutdoorSynthesis(time_t from, time_t to) const {
    // ÉTAPE 1: Pas encore de synthèse OUT
    RangeSynthesis empty;
    return empty;
}

// Export CSV IN/OUT.
void HistoryManager::exportIndoorCsv(time_t from, time_t to, const std::function<void(const char*)>& emit) const {
    // legacy = IN : l'export existant est l'export intérieur.
    exportCsv(from, to, emit);
}

void HistoryManager::exportOutdoorCsv(time_t from, time_t to, const std::function<void(const char*)>& emit) const {
    exportCsvImpl(from, to, emit, true);
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
    // Compatibilité : le flux « recent » est le flux IN (legacy = IN).
    return getIndoorStats();
}

void HistoryManager::loadRecent() {
    // Chargement de l'historique IN (existant)
    if (!LittleFS.exists(HISTORY_FILE)) return;

    File f = LittleFS.open(HISTORY_FILE, "r");
    if (!f) return;

    size_t loaded_records = 0;
    while (f.available()) {
        HistoryRecord r;
        if (f.read((uint8_t*)&r, sizeof(HistoryRecord)) == sizeof(HistoryRecord)) {
            _recentHistory.push_back(r);
            loaded_records++;
        }
    }
    f.close();

    // L'historique RAM est un tampon récent (~24 h) ; l'archive longue vit sur la
    // carte SD. Le fichier LittleFS peut avoir accumulé des semaines de mesures :
    // on ne garde en RAM que les dernières MAX_RECENT_RECORDS (comme le fait add()
    // au fil de l'eau), pour ne pas immobiliser des Mo ni ralentir chaque calcul.
    if (_recentHistory.size() > MAX_RECENT_RECORDS) {
        _recentHistory.erase(_recentHistory.begin(),
                             _recentHistory.end() - MAX_RECENT_RECORDS);
    }

    LOG_INFO("History: Loaded " + std::to_string(loaded_records)
             + " IN records (kept " + std::to_string(_recentHistory.size()) + " in RAM)");
    
    // ÉTAPE 4: Chargement de l'historique OUT (CSV simplifié)
    const char* outdoorHistoryFile = "/history/outdoor_recent.dat";
    if (LittleFS.exists(outdoorHistoryFile)) {
        File f_out = LittleFS.open(outdoorHistoryFile, "r");
        if (f_out) {
            size_t loaded_outdoor = 0;
            while (f_out.available()) {
                String line = f_out.readStringUntil('\n');
                if (line.length() > 0) {
                    // Format CSV: timestamp,t,h,p
                    int comma1 = line.indexOf(',');
                    int comma2 = line.indexOf(',', comma1 + 1);
                    int comma3 = line.indexOf(',', comma2 + 1);
                    
                    if (comma1 > 0 && comma2 > 0 && comma3 > 0) {
                        OutdoorHistoryRecord r;
                        r.timestamp = line.substring(0, comma1).toInt();
                        r.t = line.substring(comma1 + 1, comma2).toFloat();
                        r.h = line.substring(comma2 + 1, comma3).toFloat();
                        r.p = line.substring(comma3 + 1).toFloat();
                        
                        // Extensions futures à 0 pour l'instant
                        r.wind_speed = 0; r.wind_gust = 0; r.wind_direction_deg = 0;
                        r.rain_rate = 0; r.rain_accumulated = 0;
                        r.solar_lux = 0; r.uv_index = 0;
                        
                        _outdoorHistory.push_back(r);
                        loaded_outdoor++;
                    }
                }
            }
            f_out.close();
            LOG_INFO("History: Loaded " + std::to_string(loaded_outdoor) + " OUT records");
        }
    }

    // Reprise d'affichage après reboot : dernier point archivé = dernier OUT connu.
    if (!_hasLiveOutdoor && !_outdoorHistory.empty()) {
        const OutdoorHistoryRecord& last = _outdoorHistory.back();
        _lastOutdoorLive.temperature = last.t;
        _lastOutdoorLive.humidity = last.h;
        _lastOutdoorLive.pressure = last.p;
        _lastOutdoorLive.valid = true;
        _hasLiveOutdoor = true;
    }
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
    strftime(date, sizeof(date), "%Y-%m-%d", &tinfo);              // AAAA-MM-JJ
    char dir[32];
    strftime(dir, sizeof(dir), "/history/indoor/%Y/%m", &tinfo);   // /history/indoor/AAAA/MM
    snprintf(binPath, sz, "%s/%s.bin", dir, date);
    snprintf(statsPath, sz, "%s/%s.stats", dir, date);
}

bool HistoryManager::ensureDayDirs(const struct tm& tinfo) const {
    char p[32];
    if (!SD.exists("/history") && !SD.mkdir("/history")) return false;
    if (!SD.exists("/history/indoor") && !SD.mkdir("/history/indoor")) return false;
    strftime(p, sizeof(p), "/history/indoor/%Y", &tinfo);
    if (!SD.exists(p) && !SD.mkdir(p)) return false;
    strftime(p, sizeof(p), "/history/indoor/%Y/%m", &tinfo);
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
    if (!_sd || !_sd->ensureMounted()) return false; // lecture : ne pas sonder cardType()
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
bool HistoryManager::readBinSampleNear(time_t target_ts, float& t_out, float& h_out, float& p_out,
                                       bool outdoor) const {
    if (!_sd || !_sd->ensureMounted()) return false; // lecture : ne pas sonder cardType()
    struct tm tinfo;
    if (!localtime_r(&target_ts, &tinfo)) return false;

    char binPath[64], statsPath[64];
    if (outdoor) buildOutdoorDayPaths(tinfo, binPath, statsPath, sizeof(binPath));
    else         buildDayPaths(tinfo, binPath, statsPath, sizeof(binPath));
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
    return queryRangeImpl(from, to, interval_s, false);
}

std::vector<HistoryPoint> HistoryManager::queryRangeImpl(time_t from, time_t to, long interval_s,
                                                         bool outdoor) const {
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
    //
    // On monte via ensureMounted() (comme /api/history/raw et /days) et NON via
    // isAvailable() : cette dernière sonde SD.cardType() et, sur un faux CARD_NONE
    // transitoire (fréquent sous charge web), démonte la carte et reste en cooldown
    // de reconnexion. La lecture retombait alors sur la RAM seule -> historique IN
    // vide et OUT réduit à quelques trames récentes, alors que la donnée est bien
    // sur la carte (l'export brut, lui, la lit sans souci).
    if (_sd && _sd->ensureMounted()) {
        char prevBin[64] = {0};
        size_t day_iter = 0;
        for (time_t cursor = from; cursor <= to + 86400; cursor += 86400) {
            COOPERATIVE_YIELD_EVERY(day_iter, 8);
            day_iter++;
            struct tm tinfo;
            if (!localtime_r(&cursor, &tinfo)) continue;
            char binPath[64], statsPath[64];
            if (outdoor) buildOutdoorDayPaths(tinfo, binPath, statsPath, sizeof(binPath));
            else         buildDayPaths(tinfo, binPath, statsPath, sizeof(binPath));
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

            // Le repli CSV plat n'existe que pour l'historique IN (legacy).
            if (outdoor) continue;

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
    if (outdoor) {
        for (const auto& r : _outdoorHistory) {
            COOPERATIVE_YIELD_EVERY(ram_iteration, 256);
            ram_iteration++;
            long ts = static_cast<long>(r.timestamp);
            if (ts <= lastSdTs) continue;
            feed(ts, r.t, r.h, r.p);
        }
    } else {
        for (const auto& r : _recentHistory) {
            COOPERATIVE_YIELD_EVERY(ram_iteration, 256);
            ram_iteration++;
            long ts = static_cast<long>(r.timestamp);
            if (ts <= lastSdTs) continue;
            feed(ts, r.t, r.h, r.p);
        }
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

bool HistoryManager::readSdSampleNear(time_t target_ts, float& t_out, float& h_out, float& p_out,
                                      bool outdoor) const {
    if (!_sd || !_sd->ensureMounted()) return false; // lecture : ne pas sonder cardType()

    // Priorité au format binaire ; repli sur l'ancien CSV plat si absent.
    if (readBinSampleNear(target_ts, t_out, h_out, p_out, outdoor)) return true;

    // Le CSV plat legacy n'existe que pour l'IN ; l'OUT n'a jamais eu ce format.
    if (outdoor) return false;

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
    // Arborescence symétrique : /history/indoor et /history/outdoor côte à côte.
    if (!SD.exists("/history") && !SD.mkdir("/history")) {
        LOG_ERROR("Failed to create /history directory on SD card.");
        return;
    }
    if (!SD.exists("/history/indoor")) SD.mkdir("/history/indoor");
    if (!SD.exists("/history/outdoor")) SD.mkdir("/history/outdoor");
    LOG_INFO("SD history structure ready (/history/indoor + /history/outdoor).");
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
    // Efface TOUT l'historique, IN comme OUT, pour repartir sur des mesures
    // propres : états RAM, fichiers récents SPIFFS, et toute l'arborescence SD.
    _recentHistory.clear();
    _outdoorHistory.clear();
    _currentDayKey = 0;
    _currentDayStats = DayStats();
    _outdoorDayStats = DayStats();

    // SPIFFS : les deux fichiers récents + l'ancien nom (avant symétrie IN/OUT),
    // pour ne pas laisser d'orphelin sur la partition qui survit au reflash.
    LittleFS.remove(HISTORY_FILE);                    // /history/indoor_recent.dat
    LittleFS.remove("/history/outdoor_recent.dat");
    LittleFS.remove(HISTORY_FILE_LEGACY);            // /history/recent.dat (ancien)

    if (_sd && _sd->isAvailable()) {
        LOG_INFO("Clearing history from SD card...");
        // Supprime récursivement toute l'arborescence puis recrée la structure
        // symétrique vide (/history/indoor + /history/outdoor).
        removeDirRecursive("/history");
        createSdStructure();
    }
    LOG_INFO("History cleared (IN + OUT)");
}

RangeSynthesis HistoryManager::querySynthesis(time_t from, time_t to) const {
    RangeSynthesis r;
    if (to <= from || !_sd || !_sd->ensureMounted()) return r; // lecture : ne pas sonder cardType()

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

// --- Collecte incrémentale externe -------------------------------------------
// Voir le commentaire de listDays()/exportRaw() dans l'en-tête : le curseur d'un
// collecteur est (jour, index), pas un horodatage.

// Liste les entrées d'UN répertoire, puis le referme aussitôt.
//
// Ce découpage n'est pas cosmétique : imbriquer les parcours (garder ouvert
// l'itérateur de /history pendant qu'on parcourt /history/AAAA, puis
// /history/AAAA/MM) bloque la lecture de la carte SD sur ESP32. La première
// version de listDays() procédait ainsi et ne répondait jamais. Chaque niveau
// est donc entièrement lu et refermé avant de descendre au suivant.
//
// `wantDirs` sélectionne les sous-répertoires ; sinon les fichiers.
static void listEntries(const char* path, bool wantDirs,
                        std::vector<std::string>& out) {
    File dir = SD.open(path);
    if (!dir || !dir.isDirectory()) { if (dir) dir.close(); return; }

    // Garde-fou : si l'itération se mettait malgré tout à boucler, on s'arrête
    // plutôt que de bloquer indéfiniment la requête HTTP.
    const size_t kMaxEntries = 512;
    for (File e = dir.openNextFile(); e && out.size() < kMaxEntries; e = dir.openNextFile()) {
        if (e.isDirectory() == wantDirs) {
            const char* nm = e.name();
            const char* base = strrchr(nm, '/');
            out.push_back(base ? base + 1 : nm);
        }
        e.close();
    }
    dir.close();
}

// Prévisions archivées : fichiers plats /history/forecast/AAAA-MM-JJ.json, un par
// jour cible (volume faible, ~1/jour), donc pas d'arborescence AAAA/MM ici.
#define FORECAST_DIR "/history/forecast"

void HistoryManager::addForecast(const ForecastRecord& rec) {
    if (!_sd || !_sd->isAvailable()) return;
    if (!SD.exists("/history") && !SD.mkdir("/history")) return;
    if (!SD.exists(FORECAST_DIR) && !SD.mkdir(FORECAST_DIR)) return;

    const uint32_t d = rec.target_day;
    char path[64];
    snprintf(path, sizeof(path), "%s/%04u-%02u-%02u.json",
             FORECAST_DIR, d / 10000u, (d / 100u) % 100u, d % 100u);

    // Échappe la description pour un JSON valide (guillemets, backslash, contrôle).
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

    // Réécriture complète : le fichier du jour cible reflète la DERNIÈRE prévision
    // émise pour ce jour (au fil de J-1, on converge vers la prévision day-ahead).
    SD.remove(path);
    File f = SD.open(path, FILE_WRITE);
    if (!f) { LOG_WARNING("Forecast: open failed"); return; }
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"target_day\":%u,\"issued_ts\":%u,\"temp_min\":%.1f,"
             "\"temp_max\":%.1f,\"description\":\"%s\"}",
             rec.target_day, rec.issued_ts,
             (double)rec.temp_min, (double)rec.temp_max, esc.c_str());
    f.print(buf);
    f.close();
}

void HistoryManager::forecastHistoryRaw(
        time_t from, time_t to,
        const std::function<void(const char*)>& emit) const {
    if (!_sd || !_sd->ensureMounted()) return;

    struct tm tf, tt;
    if (!localtime_r(&from, &tf) || !localtime_r(&to, &tt)) return;
    const uint32_t dayFrom = (tf.tm_year + 1900) * 10000u + (tf.tm_mon + 1) * 100u + tf.tm_mday;
    const uint32_t dayTo   = (tt.tm_year + 1900) * 10000u + (tt.tm_mon + 1) * 100u + tt.tm_mday;

    std::vector<std::string> files;
    listEntries(FORECAST_DIR, false, files);
    std::sort(files.begin(), files.end()); // AAAA-MM-JJ : tri lexical = chronologique

    size_t it = 0;
    for (const std::string& name : files) {
        COOPERATIVE_YIELD_EVERY(it, 8);
        it++;
        unsigned y = 0, mo = 0, dd = 0;
        if (sscanf(name.c_str(), "%4u-%2u-%2u.json", &y, &mo, &dd) != 3) continue;
        if (name.size() < 5 || name.compare(name.size() - 5, 5, ".json") != 0) continue;
        const uint32_t dayKey = y * 10000u + mo * 100u + dd;
        if (dayKey < dayFrom || dayKey > dayTo) continue;

        const std::string path = std::string(FORECAST_DIR) + "/" + name;
        File f = SD.open(path.c_str(), FILE_READ);
        if (!f) continue;
        String content = f.readString();
        f.close();
        content.trim();
        if (content.length() > 0) emit(content.c_str());
    }
}

std::vector<DayIndexEntry> HistoryManager::listDays() const {
    return listDaysFromRoot("/history/indoor");
}

std::vector<DayIndexEntry> HistoryManager::listDaysOutdoor() const {
    return listDaysFromRoot("/history/outdoor");
}

std::vector<DayIndexEntry> HistoryManager::listDaysFromRoot(const char* root) const {
    std::vector<DayIndexEntry> out;
    if (!_sd || !_sd->ensureMounted()) return out;

    // <root>/AAAA/MM/AAAA-MM-JJ.bin : on aplatit les deux niveaux de
    // répertoires avant d'ouvrir le moindre fichier de mesures.
    std::vector<std::string> months; // chemins complets des dossiers de mois
    {
        std::vector<std::string> years;
        listEntries(root, true, years);
        for (const std::string& y : years) {
            const std::string yearPath = std::string(root) + "/" + y;
            std::vector<std::string> monthNames;
            listEntries(yearPath.c_str(), true, monthNames);
            for (const std::string& m : monthNames)
                months.push_back(yearPath + "/" + m);
        }
    }

    for (const std::string& monthPath : months) {
        std::vector<std::string> files;
        listEntries(monthPath.c_str(), false, files);

        for (const std::string& name : files) {
            // L'extension est vérifiée SÉPARÉMENT de la date. sscanf renvoie le
            // nombre de conversions AFFECTÉES : avec "%4u-%2u-%2u.bin", un
            // fichier "AAAA-MM-JJ.stats" renvoie quand même 3, l'échec du
            // littéral ".bin" final n'étant pas compté. Les fichiers .stats
            // étaient ainsi pris pour des fichiers de mesures et publiaient des
            // journées fantômes aux horodatages aberrants.
            const size_t len = name.size();
            if (len < 14 || name.compare(len - 4, 4, ".bin") != 0)
                continue;

            unsigned y = 0, mo = 0, d = 0;
            if (sscanf(name.c_str(), "%4u-%2u-%2u", &y, &mo, &d) != 3)
                continue;
            if (mo < 1 || mo > 12 || d < 1 || d > 31)
                continue;

            const std::string full = monthPath + "/" + name;
            File f = SD.open(full.c_str(), FILE_READ);
            if (!f) continue;

            BinLayout L = probeBin(f);
            if (L.nrec > 0) {
                DayIndexEntry e{};
                e.day_key = y * 10000u + mo * 100u + d;
                e.nrec = static_cast<uint32_t>(L.nrec);

                // Les horodatages extrêmes sont lus dans l'en-tête quand il est
                // présent : chaque accès à la carte SD coûte cher, et deux
                // positionnements par journée sont évitables.
                FileHeader h;
                f.seek(0);
                const bool headerOk =
                    f.read(reinterpret_cast<uint8_t*>(&h), sizeof(h)) == (int)sizeof(h)
                    && hdrMagicOk(h) && h.firstTimestamp != 0;
                if (headerOk) {
                    e.first_ts = static_cast<uint32_t>(h.firstTimestamp);
                    e.last_ts  = static_cast<uint32_t>(h.lastTimestamp);
                } else {
                    BinRecord br;
                    if (readBinRecordAt(f, L, 0, br)) e.first_ts = br.ts;
                    if (readBinRecordAt(f, L, L.nrec - 1, br)) e.last_ts = br.ts;
                }
                out.push_back(e);
            }
            f.close();
        }
    }

    std::sort(out.begin(), out.end(),
              [](const DayIndexEntry& a, const DayIndexEntry& b) { return a.day_key < b.day_key; });
    return out;
}

uint32_t HistoryManager::exportRaw(uint32_t day_key, uint32_t from_index, uint32_t limit,
                                   const std::function<void(const RawRecord&)>& emit) const {
    return exportRawFromRoot("/history/indoor", day_key, from_index, limit, emit);
}

uint32_t HistoryManager::exportRawOutdoor(uint32_t day_key, uint32_t from_index, uint32_t limit,
                                          const std::function<void(const RawRecord&)>& emit) const {
    return exportRawFromRoot("/history/outdoor", day_key, from_index, limit, emit);
}

uint32_t HistoryManager::exportRawFromRoot(const char* root, uint32_t day_key, uint32_t from_index,
                                           uint32_t limit,
                                           const std::function<void(const RawRecord&)>& emit) const {
    if (!_sd || !_sd->ensureMounted()) return 0;

    const unsigned y = day_key / 10000u;
    const unsigned mo = (day_key / 100u) % 100u;
    const unsigned d = day_key % 100u;

    char binPath[64];
    snprintf(binPath, sizeof(binPath), "%s/%04u/%02u/%04u-%02u-%02u.bin", root, y, mo, y, mo, d);

    File f = SD.open(binPath, FILE_READ);
    if (!f) return 0;

    BinLayout L = probeBin(f);
    const uint32_t total = static_cast<uint32_t>(L.nrec);

    uint32_t sent = 0;
    forEachBinRecordFrom(f, L, from_index, [&](const BinRecord& br) {
        if (sent >= limit) return false;
        emit(RawRecord{br.ts, br.t, br.h, br.p});
        sent++;
        return true;
    });

    f.close();
    return total;
}

void HistoryManager::exportCsv(time_t from, time_t to, const std::function<void(const char*)>& emit) const {
    exportCsvImpl(from, to, emit, false);
}

void HistoryManager::exportCsvImpl(time_t from, time_t to,
                                   const std::function<void(const char*)>& emit, bool outdoor) const {
    emit("Timestamp,Temperature,Humidity,Pressure\n");
    if (to <= from || !_sd || !_sd->ensureMounted()) return; // lecture : ne pas sonder cardType()

    const long from_l = static_cast<long>(from);
    const long to_l = static_cast<long>(to);
    char prevBin[64] = {0};
    size_t day_iter = 0;

    for (time_t cursor = from; cursor <= to + 86400; cursor += 86400) {
        COOPERATIVE_YIELD_EVERY(day_iter, 8); // export « Tout » : la plage peut couvrir des années
        day_iter++;
        struct tm tinfo;
        if (!localtime_r(&cursor, &tinfo)) continue;
        char binPath[64], statsPath[64];
        if (outdoor) buildOutdoorDayPaths(tinfo, binPath, statsPath, sizeof(binPath));
        else         buildDayPaths(tinfo, binPath, statsPath, sizeof(binPath));
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

MeteoTrend HistoryManager::getTrend() const { return getTrendImpl(false); }
MeteoTrend HistoryManager::getTrendOutdoor() const { return getTrendImpl(true); }

MeteoTrend HistoryManager::getTrendImpl(bool outdoor) const {
    MeteoTrend trend;

    // Source RAM selon le contexte : IN = _recentHistory (~24h à 1/min),
    // OUT = _outdoorHistory (~12h à 1/30s). On copie (ts,t,h,p) dans une vue
    // uniforme pour partager la même logique quelle que soit la source.
    std::vector<RawRecord> ram;
    if (outdoor) {
        ram.reserve(_outdoorHistory.size());
        for (const auto& r : _outdoorHistory)
            ram.push_back(RawRecord{static_cast<uint32_t>(r.timestamp), r.t, r.h, r.p});
    } else {
        ram.reserve(_recentHistory.size());
        for (const auto& r : _recentHistory)
            ram.push_back(RawRecord{static_cast<uint32_t>(r.timestamp), r.t, r.h, r.p});
    }
    if (ram.empty()) return trend;

    const time_t now = static_cast<time_t>(ram.back().ts);
    float t_now = ram.back().t;
    float h_now = ram.back().h;
    float p_now = ram.back().p;

    float t_1h = t_now, h_1h = h_now, p_1h = p_now;
    float t_12h = t_now, h_12h = h_now, p_12h = p_now;
    float t_24h = t_now, h_24h = h_now, p_24h = p_now;
    bool found_1h = false, found_12h = false, found_24h = false;
    size_t trend_iteration = 0;

    for (auto it = ram.rbegin(); it != ram.rend(); ++it) {
        COOPERATIVE_YIELD_EVERY(trend_iteration, 256);
        trend_iteration++;

        time_t dt = now - static_cast<time_t>(it->ts);
        if (!found_1h && dt >= 3600) {
            t_1h = it->t; h_1h = it->h; p_1h = it->p; found_1h = true;
        }
        if (!found_12h && dt >= 43200) {
            t_12h = it->t; h_12h = it->h; p_12h = it->p; found_12h = true;
        }
        if (!found_24h && dt >= 86400) {
            t_24h = it->t; h_24h = it->h; p_24h = it->p; found_24h = true;
            break;
        }
    }

    // Points hors de portée de la RAM (l'OUT ne couvre que ~12h) : on les cherche
    // sur la carte SD, dans l'arborescence du bon contexte. Aucun repli entre
    // contextes : une tendance OUT n'emprunte jamais de point IN.
    float tmp_t, tmp_h, tmp_p;
    if (!found_12h && readSdSampleNear(now - 43200, tmp_t, tmp_h, tmp_p, outdoor)) {
        t_12h = tmp_t; h_12h = tmp_h; p_12h = tmp_p; found_12h = true;
    }
    if (!found_24h && readSdSampleNear(now - 86400, tmp_t, tmp_h, tmp_p, outdoor)) {
        t_24h = tmp_t; h_24h = tmp_h; p_24h = tmp_p; found_24h = true;
    }

    // Point à J-48h : jamais en RAM, lu sur SD si disponible.
    float t_48h = t_now, h_48h = h_now, p_48h = p_now;
    trend.available_48h = readSdSampleNear(now - 172800, t_48h, h_48h, p_48h, outdoor);

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

// --- Méthodes privées de stockage OUT ---
// (Le stockage IN passe par les méthodes legacy : saveRecent, saveToSdBinary,
//  updateDayStats... — décision legacy = IN, pas de chemin indoor parallèle.)
void HistoryManager::saveOutdoorRecent(const OutdoorHistoryRecord& record) {
    // Sauvegarde des données OUT récentes dans LittleFS
    // Similaire à saveRecent mais pour OUT
    char path[64];
    snprintf(path, sizeof(path), "/history/outdoor_recent.dat");
    
    File f = LittleFS.open(path, "a");
    if (!f) {
        LOG_WARNING("History: Failed to open outdoor recent file");
        return;
    }
    
    // Format : timestamp,t,h,p (pour l'instant format simple)
    char buf[128];
    snprintf(buf, sizeof(buf), "%lu,%.2f,%.2f,%.2f\n", 
             (unsigned long)record.timestamp, record.t, record.h, record.p);
    f.write((const uint8_t*)buf, strlen(buf));
    f.close();
}

void HistoryManager::saveOutdoorToSdBinary(const OutdoorHistoryRecord& record) {
    // ÉTAPE 3: Stockage OUT sur SD dans /history/outdoor/AAAA/MM/AAAA-MM-JD.bin
    // Pour l'instant, utilise le même format binaire que IN (extensions futures à prévoir)
    
    time_t ts = record.timestamp;
    struct tm tinfo;
    if (!localtime_r(&ts, &tinfo)) {
        LOG_WARNING("Bin Save OUT: localtime failed");
        return;
    }

    if (!ensureOutdoorDayDirs(tinfo) && _sd && _sd->ensureMounted()) {
        ensureOutdoorDayDirs(tinfo);
    }
    
    char binPath[64], statsPath[64];
    buildOutdoorDayPaths(tinfo, binPath, statsPath, sizeof(binPath));
    
    // Pour l'ÉTAPE 3, on utilise temporairement le même format que IN
    // Conversion vers HistoryRecord pour réutiliser la logique existante
    HistoryRecord legacy;
    legacy.timestamp = record.timestamp;
    legacy.t = record.t;
    legacy.h = record.h;
    legacy.p = record.p;
    
    // Logique simplifiée pour l'ÉTAPE 3 : ouverture, écriture, fermeture
    File f = SD.open(binPath, FILE_APPEND);
    if (!f) {
        LOG_WARNING("Bin Save OUT: Failed to open " + std::string(binPath));
        return;
    }
    
    BinRecord bin;
    bin.ts = record.timestamp;
    bin.t = record.t;
    bin.h = record.h;
    bin.p = record.p;
    
    if (f.write((const uint8_t*)&bin, sizeof(bin)) != sizeof(bin)) {
        LOG_WARNING("Bin Save OUT: Write failed");
    }
    f.close();
    
    LOG_INFO("History: Outdoor data saved to SD");
}

bool HistoryManager::ensureOutdoorDayDirs(const struct tm& tinfo) const {
    // ÉTAPE 3: Création des répertoires pour /history/outdoor/AAAA/MM/
    // Utilisation directe de SD (pas via SdManager qui n'a pas exists/mkdir)
    if (!SD.exists("/history") && !SD.mkdir("/history")) {
        LOG_WARNING("History: Failed to create /history for outdoor");
        return false;
    }
    if (!SD.exists("/history/outdoor") && !SD.mkdir("/history/outdoor")) {
        LOG_WARNING("History: Failed to create outdoor root directory");
        return false;
    }
    char dirPath[64];
    snprintf(dirPath, sizeof(dirPath), "/history/outdoor/%04d", tinfo.tm_year + 1900);
    if (!SD.exists(dirPath)) {
        if (!SD.mkdir(dirPath)) {
            LOG_WARNING("History: Failed to create outdoor year directory");
            return false;
        }
    }
    
    snprintf(dirPath, sizeof(dirPath), "/history/outdoor/%04d/%02d", tinfo.tm_year + 1900, tinfo.tm_mon + 1);
    if (!SD.exists(dirPath)) {
        if (!SD.mkdir(dirPath)) {
            LOG_WARNING("History: Failed to create outdoor month directory");
            return false;
        }
    }
    
    return true;
}

void HistoryManager::buildOutdoorDayPaths(const struct tm& tinfo, char* binPath, char* statsPath, size_t sz) const {
    // ÉTAPE 3: Chemins pour /history/outdoor/AAAA/MM/AAAA-MM-JD.bin
    snprintf(binPath, sz, "/history/outdoor/%04d/%02d/%04d-%02d-%02d.bin",
             tinfo.tm_year + 1900, tinfo.tm_mon + 1,
             tinfo.tm_year + 1900, tinfo.tm_mon + 1, tinfo.tm_mday);
    snprintf(statsPath, sz, "/history/outdoor/%04d/%02d/%04d-%02d-%02d.stats",
             tinfo.tm_year + 1900, tinfo.tm_mon + 1,
             tinfo.tm_year + 1900, tinfo.tm_mon + 1, tinfo.tm_mday);
}

void HistoryManager::updateOutdoorDayStats(const OutdoorHistoryRecord& record, const struct tm& tinfo) {
    // ÉTAPE 3: Mise à jour des statistiques OUT du jour courant
    // Pour l'instant, utilise la même logique que IN mais pour OUT
    uint32_t dayKey = (tinfo.tm_year + 1900) * 10000 + (tinfo.tm_mon + 1) * 100 + tinfo.tm_mday;
    
    if (_currentDayKey != dayKey) {
        // Nouveau jour : sauvegarder les stats précédentes et réinitialiser
        // Pour l'ÉTAPE 3, on réinitialise simplement
        _outdoorDayStats.magic = BIN_STATS_MAGIC;
        _outdoorDayStats.count = 0;
        _outdoorDayStats.first_ts = record.timestamp;
        _outdoorDayStats.last_ts = record.timestamp;
        _outdoorDayStats.t_min = record.t; _outdoorDayStats.t_max = record.t; _outdoorDayStats.t_sum = record.t;
        _outdoorDayStats.h_min = record.h; _outdoorDayStats.h_max = record.h; _outdoorDayStats.h_sum = record.h;
        _outdoorDayStats.p_min = record.p; _outdoorDayStats.p_max = record.p; _outdoorDayStats.p_sum = record.p;
        _outdoorDayStats.t_first = record.t; _outdoorDayStats.h_first = record.h; _outdoorDayStats.p_first = record.p;
        _outdoorDayStats.t_last = record.t; _outdoorDayStats.h_last = record.h; _outdoorDayStats.p_last = record.p;
        _currentDayKey = dayKey;
    } else {
        // Mise à jour des stats existantes
        _outdoorDayStats.count++;
        _outdoorDayStats.last_ts = record.timestamp;
        
        if (record.t < _outdoorDayStats.t_min) _outdoorDayStats.t_min = record.t;
        if (record.t > _outdoorDayStats.t_max) _outdoorDayStats.t_max = record.t;
        _outdoorDayStats.t_sum += record.t;
        _outdoorDayStats.t_last = record.t;
        
        if (record.h < _outdoorDayStats.h_min) _outdoorDayStats.h_min = record.h;
        if (record.h > _outdoorDayStats.h_max) _outdoorDayStats.h_max = record.h;
        _outdoorDayStats.h_sum += record.h;
        _outdoorDayStats.h_last = record.h;
        
        if (record.p < _outdoorDayStats.p_min) _outdoorDayStats.p_min = record.p;
        if (record.p > _outdoorDayStats.p_max) _outdoorDayStats.p_max = record.p;
        _outdoorDayStats.p_sum += record.p;
        _outdoorDayStats.p_last = record.p;
    }
}

bool HistoryManager::readOutdoorDayStats(time_t day_ts, DayStats& out) const {
    // ÉTAPE 1: Pas encore de stats OUT
    return false;
}