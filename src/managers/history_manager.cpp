#include "history_manager.h"
#include <LittleFS.h>
#include <SD.h>
#include <time.h>
#include "../utils/logs.h"
#include "measure_range.h"
#include "config.h"
#include "../history/forecast_archive.h"

// Arborescence symétrique IN/OUT :
//   LittleFS : /history/indoor_recent.dat  +  /history/outdoor_recent.dat
//   SD       : /history/indoor/AAAA/MM/...  +  /history/outdoor/AAAA/MM/...
#define HISTORY_FILE "/history/indoor_recent.dat"
// Ancien nom (avant symétrie IN/OUT), supprimé par clearRam() pour ne pas
// laisser un orphelin sur la partition LittleFS après reflash.
#define HISTORY_FILE_LEGACY "/history/recent.dat"
#define OUTDOOR_HISTORY_FILE "/history/outdoor_recent.dat"
#define MAX_RECENT_RECORDS 1440
// Rotation des fichiers récents LittleFS : ils sont écrits en ajout seul, mais
// seules les MAX_RECENT_RECORDS dernières mesures servent (l'archive longue vit
// sur SD). Au-delà de ce seuil, le fichier est réécrit avec les seuls
// MAX_RECENT_RECORDS derniers. Le facteur 2 amortit le coût (une réécriture
// toutes les MAX_RECENT_RECORDS mesures) et borne l'occupation de la partition.
#define RECENT_COMPACT_THRESHOLD (2 * MAX_RECENT_RECORDS)
// Temporaires de compaction : le fichier définitif n'est remplacé que par un
// rename atomique (LittleFS) une fois le temporaire complet et fermé. Une coupure
// pendant l'écriture laisse l'original intact ; loadRecent() purge l'orphelin.
#define HISTORY_FILE_TMP "/history/indoor_recent.tmp"
#define OUTDOOR_HISTORY_FILE_TMP "/history/outdoor_recent.tmp"

// --- Construction et démarrage ----------------------------------------------------------

HistoryManager::HistoryManager()
    : _sdFs(SD), _lfs(LittleFS),
      _in(_sdFs, mhhist::kIndoorPolicy), _out(_sdFs, mhhist::kOutdoorPolicy),
      _inRing(_lfs, _ramMutex, HISTORY_FILE, HISTORY_FILE_TMP, MAX_RECENT_RECORDS, RECENT_COMPACT_THRESHOLD),
      _outRing(_lfs, _ramMutex, OUTDOOR_HISTORY_FILE, OUTDOOR_HISTORY_FILE_TMP, MAX_RECENT_RECORDS,
               RECENT_COMPACT_THRESHOLD),
      _inSeries(_inRing, _ramMutex), _outSeries(_outRing, _ramMutex) {}

void HistoryManager::begin(SdManager* sd) {
    _sd = sd;
    _sdFs.attach(sd);
    loadRecent();
    if (_sd && _sd->isAvailable()) {
        createSdStructure();
        migrateCsvToBinary(); // conversion unique des anciens CSV plats -> binaire
    }
}

void HistoryManager::loadRecent() {
    // Temporaires de compaction orphelins (coupure avant le rename) : jetés avant tout
    // chargement. Pas de return anticipé : un fichier IN absent ne doit pas empêcher le
    // chargement du fichier OUT.
    _inRing.purgeTmp();
    _outRing.purgeTmp();
    _inRing.load();
    _outRing.load();

    // Reprise d'affichage après reboot : dernier point archivé = dernier OUT connu.
    if (!_hasLiveOutdoor && !_outRing.items().empty()) {
        const OutdoorHistoryRecord& last = _outRing.items().back();
        _lastOutdoorLive.temperature = last.t;
        _lastOutdoorLive.humidity = last.h;
        _lastOutdoorLive.pressure = last.p;
        _lastOutdoorLive.has_humidity = mhrange::storedHumidityOk(last.h);
        _lastOutdoorLive.has_pressure = mhrange::storedPressureOk(last.p);
        _lastOutdoorLive.valid = true;
        _hasLiveOutdoor = true;
    }
}

// --- Points d'entrée IN -----------------------------------------------------------------

void HistoryManager::add(float t, float h, float p) {
    struct tm timeinfo;
    // Delai 0 : getLocalTime() sans argument attend jusqu'a 5 s, dans loop().
    if (!getLocalTime(&timeinfo, 0)) {
        LOG_WARNING("History: Time not synced, skipping record");
        return;
    }

    HistoryRecord record;
    record.timestamp = time(NULL);
    record.t = t;
    record.h = h;
    record.p = p;

    // Les pages web (async_tcp) lisent ce tampon : push sous verrou, puis fichier recent.
    _inRing.push(record);
    _inRing.persist(record);

    if (_sd && _sd->isAvailable()) {
        saveToSdBinary(record);
    }
}

// Décision legacy = IN : l'historique historique EST le flux intérieur. addIndoor est le
// point d'entrée explicite ; add() en reste l'écriture concrète (conservée pour les appels
// existants et la compatibilité disque). Le flux OUT a ses propres méthodes (addOutdoor*).
void HistoryManager::addIndoor(const IndoorData& data) {
    add(data.temperature, data.humidity, data.pressure);
}

const std::vector<HistoryRecord>& HistoryManager::getRecentHistory() const {
    return _inRing.items();
}

bool HistoryManager::saveToSdBinary(const HistoryRecord& record) {
    const mhhist::BinRecord br { static_cast<uint32_t>(record.timestamp), record.t, record.h, record.p };
    return _in.append(record.timestamp, br, 0);
}

// --- Points d'entrée OUT ----------------------------------------------------------------

// Horodatage plancher de plausibilité : en deçà (avant ~2020), l'heure est
// bidon (hub pas encore synchronisé NTP, ou ancre absente). On garde alors le
// live mais on n'archive pas une mesure mal datée.
static bool outdoorTsPlausible(time_t ts) { return ts > 1600000000; }

// Construit l'enregistrement OUT à partir de la donnée + son heure de mesure.
static OutdoorHistoryRecord makeOutdoorRecord(const OutdoorData& data, time_t ts) {
    OutdoorHistoryRecord record;
    record.timestamp = ts;
    record.t = data.temperature;
    // Humidite / pression : la valeur n'est archivee que si la sonde l'a MESUREE et
    // qu'elle est plausible ; sinon NaN (jamais un 0.0 pris pour une vraie mesure).
    record.h = mhrange::humidityToStore(data.has_humidity, data.humidity);
    record.p = mhrange::pressureToStore(data.has_pressure, data.pressure);
    record.wind_speed = data.wind_speed;
    record.wind_gust = data.wind_gust;
    record.wind_direction_deg = data.wind_direction_deg;
    record.rain_rate = data.rain_rate;
    record.rain_accumulated = data.rain_accumulated;
    record.solar_lux = data.solar_lux;
    record.uv_index = data.uv_index;
    return record;
}

// Ecriture d'une mesure OUT. false si elle n'a pas pu etre archivee TELLE QUELLE - notamment
// une mesure MARQUEE dans un fichier v1, qui ne sait pas porter la marque : l'appelant la met
// alors en quarantaine plutot que de la perdre ou de l'archiver comme une mesure normale.
bool HistoryManager::saveOutdoorToSdBinary(const OutdoorHistoryRecord& record, uint32_t flags) {
    const mhhist::BinRecord br { static_cast<uint32_t>(record.timestamp), record.t, record.h, record.p };
    return _out.append(record.timestamp, br, flags);
}

// Enregistre une mesure OUT live. Renvoie true quand la mesure est ARCHIVEE (ou qu'il n'y a
// rien a archiver : trame invalide) ; false quand l'archivage a echoue (heure implausible,
// carte SD absente ou ecriture refusee). L'appelant n'accuse la mesure a la sonde que sur
// true : un accuse efface la mesure du buffer de la sonde.
bool HistoryManager::addOutdoorLive(const OutdoorData& data, time_t measurementTs) {
    // RÈGLE STRICTE : seule une trame OUT réellement REÇUE et VALIDE entre dans
    // l'historique OUT (jamais une valeur intérieure, ni la valeur « effective »
    // de présentation, ni un seed disque). Une trame invalide ne met à jour NI le
    // live NI l'historique.
    if (!data.valid) {
        LOG_WARNING("History: Outdoor frame invalid, ignored (no live, no archive)");
        return true; // rien a archiver : ne pas bloquer l'accuse
    }

    // Trame OUT réelle et valide : elle devient le dernier relevé « live » et fait
    // référence pour la fraîcheur.
    setOutdoorLive(data);

    if (!outdoorTsPlausible(measurementTs)) {
        LOG_WARNING("History: Outdoor live kept, archive skipped (implausible ts)");
        return false;
    }

    OutdoorHistoryRecord record = makeOutdoorRecord(data, measurementTs);

    // SD D'ABORD : si l'ecriture echoue, ni le tampon RAM ni le fichier recent ne
    // recoivent la mesure, sinon sa retransmission (meme seq) la dupliquerait.
    if (_sd && _sd->isAvailable()) {
        if (!saveOutdoorToSdBinary(record)) return false;
    } else {
#if SD_CARD_ENABLED
        return false; // SD attendue mais absente : rien n'est archive
#endif
    }

    _outRing.push(record);
    _outRing.persist(record);

    LOG_INFO("History: Outdoor live added (T=" + std::to_string(data.temperature)
             + "°C, seq=" + std::to_string(data.sequence) + ")");
    return true;
}

bool HistoryManager::addOutdoorHistorical(const OutdoorData& data, time_t measurementTs) {
    // Mesure RETRANSMISE (ancienne) : on archive SEULEMENT, à son heure d'origine,
    // sans toucher au live ni à la fraîcheur. Elle atterrit dans le bon fichier-jour
    // grâce à son horodatage ; morfAnalytics la relira via son curseur (jour,index)
    // et la remettra dans l'ordre chronologique (ORDER BY ts).
    // true = archivée (ou rien à archiver) ; false = à ne PAS accuser (voir addOutdoorLive).
    if (!data.valid) return true;
    if (!outdoorTsPlausible(measurementTs)) {
        LOG_WARNING("History: Outdoor historical dropped (implausible ts, seq="
                    + std::to_string(data.sequence) + ")");
        return false;
    }
    if (!(_sd && _sd->isAvailable())) return false; // sans SD, pas d'archive historique

    OutdoorHistoryRecord record = makeOutdoorRecord(data, measurementTs);
    if (!saveOutdoorToSdBinary(record)) return false; // append-only, chemin dérivé de l'heure de mesure
    LOG_INFO("History: Outdoor historical archived (seq=" + std::to_string(data.sequence)
             + ", ts=" + std::to_string((long)measurementTs) + ")");
    return true;
}

bool HistoryManager::addOutdoorFlagged(const OutdoorData& data, time_t measurementTs,
                                       uint32_t flags) {
    // Mesure SUSPECTE (voir cold_boot_rule.h) : archivée avec sa marque, pour
    // que morfAnalytics la voie et puisse la réintégrer ; mais ni valeur live,
    // ni historique RAM, ni stats du jour : le hub ne la présente pas comme une
    // mesure météo. false = non archivée (pas de SD, fichier du jour en v1...).
    if (!data.valid || !outdoorTsPlausible(measurementTs)) return false;
    if (!(_sd && _sd->isAvailable())) return false;
    return saveOutdoorToSdBinary(makeOutdoorRecord(data, measurementTs), flags);
}

// Dernier relevé « live » + fraîcheur, écrits par loop() et lus par async_tcp.
void HistoryManager::setOutdoorLive(const OutdoorData& data) {
    std::lock_guard<std::mutex> lock(_ramMutex);
    _lastOutdoorLive = data;
    _hasLiveOutdoor = true;
    _lastOutdoorRadioMs = millis();
    _hasOutdoorRadioMs = true;
}

void HistoryManager::refreshOutdoorLive(const OutdoorData& data) {
    if (!data.valid) return;
    setOutdoorLive(data);
}

OutdoorData HistoryManager::lastOutdoorLive() const {
    std::lock_guard<std::mutex> lock(_ramMutex);
    return _lastOutdoorLive;
}

bool HistoryManager::lastOutdoorRecord(OutdoorHistoryRecord& out) const {
    std::lock_guard<std::mutex> lock(_ramMutex);
    if (_outRing.items().empty()) return false;
    out = _outRing.items().back();
    return true;
}

// --- Requêtes : délégation aux fonctions pures de src/history/history_query.h -----------------

// Statistiques sur les dernieres 24 h. L'historique RAM peut couvrir bien plus (charge depuis
// LittleFS au boot) : la fenetre evite d'agreger des semaines de mesures. IN et OUT utilisent la
// meme fenetre pour que les deux resumes soient comparables.
Stats24h HistoryManager::getIndoorStats() const { return mhhist::computeStats24h(_inSeries, time(NULL)); }
Stats24h HistoryManager::getOutdoorStats() const { return mhhist::computeStats24h(_outSeries, time(NULL)); }
Stats24h HistoryManager::getRecentStats() const { return getIndoorStats(); } // compat : recent = IN

std::vector<HistoryPoint> HistoryManager::queryRangeImpl(time_t from, time_t to, long interval_s,
                                                         bool outdoor) const {
    return mhhist::queryRange(outdoor ? _out : _in,
                              outdoor ? static_cast<const mhhist::RamSeries&>(_outSeries) : _inSeries,
                              from, to, interval_s);
}

std::vector<HistoryPoint> HistoryManager::queryRange(time_t from, time_t to, long interval_s) const {
    return queryRangeImpl(from, to, interval_s, false);
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

RangeSynthesis HistoryManager::querySynthesis(time_t from, time_t to) const {
    return mhhist::querySynthesis(_in, from, to);
}

MeteoTrend HistoryManager::getTrendImpl(bool outdoor) const {
    return mhhist::computeTrend(outdoor ? _out : _in,
                                outdoor ? static_cast<const mhhist::RamSeries&>(_outSeries) : _inSeries);
}
MeteoTrend HistoryManager::getTrend() const { return getTrendImpl(false); }
MeteoTrend HistoryManager::getTrendOutdoor() const { return getTrendImpl(true); }

// --- Export et collecte incrémentale ------------------------------------------------------
// Le curseur d'un collecteur est (jour, index), pas un horodatage (voir l'en-tête).

size_t HistoryManager::exportCsvChunk(CsvCursor& c, char* buf, size_t maxLen) const {
    return mhhist::exportCsvChunk(c.outdoor ? _out : _in, c, buf, maxLen);
}

std::vector<DayIndexEntry> HistoryManager::listDays() const { return _in.listDays(); }
std::vector<DayIndexEntry> HistoryManager::listDaysOutdoor() const { return _out.listDays(); }

uint32_t HistoryManager::exportRaw(uint32_t day_key, uint32_t from_index, uint32_t limit,
                                   const std::function<void(const RawRecord&)>& emit) const {
    return _in.exportRaw(day_key, from_index, limit, emit);
}

uint32_t HistoryManager::exportRawOutdoor(uint32_t day_key, uint32_t from_index, uint32_t limit,
                                          const std::function<void(const RawRecord&)>& emit) const {
    return _out.exportRaw(day_key, from_index, limit, emit);
}

// --- Prévisions archivées -----------------------------------------------------------------

void HistoryManager::addForecast(const ForecastRecord& rec) {
    if (!_sd || !_sd->isAvailable()) return;
    mhhist::addForecast(_sdFs, rec);
}

void HistoryManager::forecastHistoryRaw(time_t from, time_t to,
                                        const std::function<void(const char*)>& emit) const {
    mhhist::forecastHistoryRaw(_sdFs, from, to, emit);
}

// --- Effacement et travaux de fond --------------------------------------------------------

void HistoryManager::invalidateDaysCache() {
    _in.invalidateDaysCache();
    _out.invalidateDaysCache();
}

void HistoryManager::createSdStructure() {
    mhhist::createHistoryTree(_sdFs, mhhist::kIndoorPolicy.baseDir, mhhist::kIndoorPolicy.root,
                              mhhist::kOutdoorPolicy.root);
}

void HistoryManager::migrateCsvToBinary() {
    if (!_sd || !_sd->isAvailable()) return;
    mhhist::migrateCsvToBinary(_in);
}

void HistoryManager::clearRam() {
    invalidateDaysCache();
    std::lock_guard<std::mutex> lock(_ramMutex);
    // Les tampons effacent aussi leurs fichiers LittleFS (+ temporaires) ; reste l'ancien nom
    // (avant symétrie IN/OUT), pour ne pas laisser d'orphelin sur la partition qui survit au
    // reflash.
    _inRing.clear();
    _outRing.clear();
    _in.resetStatsCache();
    LittleFS.remove(HISTORY_FILE_LEGACY);
}

// Effacement synchrone : appelé depuis loop() (menu OLED), jamais depuis async_tcp.
void HistoryManager::clearHistory() {
    clearRam();
    if (_sd && _sd->isAvailable()) {
        LOG_INFO("Clearing history from SD card...");
        // Supprime récursivement toute l'arborescence puis recrée la structure
        // symétrique vide (/history/indoor + /history/outdoor).
        mhhist::removeDirRecursive(_sdFs, mhhist::kIndoorPolicy.baseDir);
        createSdStructure();
    }
    LOG_INFO("History cleared (IN + OUT)");
}

// Demande d'effacement posee par la page web : la route repond tout de suite (202) et service(),
// appele depuis loop(), supprime la carte par petits lots. Une suppression recursive complete
// dans async_tcp depassait le delai du watchdog de tache.
void HistoryManager::requestClear() { _clear.request(); }

void HistoryManager::service() {
    _clear.service(_sdFs, mhhist::kIndoorPolicy.baseDir, _sd && _sd->isAvailable(),
                   [this]() { clearRam(); },
                   [this]() {
                       createSdStructure();
                       invalidateDaysCache();
                   });
}
