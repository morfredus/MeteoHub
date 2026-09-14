#pragma once
#include <vector>
#include <string>
#include <functional>
#include <Arduino.h>
#include "sd_manager.h"
#include "../modules/meteo_context.h"
 
// HistoryRecord devient explicitement IN (mesures intérieures)
// Maintenu pour compatibilité avec l'historique existant
struct HistoryRecord {
    time_t timestamp;
    float t; // Température
    float h; // Humidité
    float p; // Pression
};

// Version explicite pour mesures intérieures (IN)
struct IndoorHistoryRecord {
    time_t timestamp;
    float t; // Température
    float h; // Humidité
    float p; // Pression
};

// Version explicite pour mesures extérieures (OUT)
struct OutdoorHistoryRecord {
    time_t timestamp;
    float t; // Température
    float h; // Humidité
    float p; // Pression
    
    // Extensions futures (vent, pluie, UV)
    float wind_speed;
    float wind_gust;
    uint16_t wind_direction_deg;
    float rain_rate;
    float rain_accumulated;
    float solar_lux;
    float uv_index;
};

// Point agrégé renvoyé par une requête sur une plage arbitraire (queryRange).
// valid=false signale une tranche temporelle sans donnée (trou dans l'historique) :
// elle est tout de même émise pour conserver l'alignement des index entre deux
// périodes comparées.
struct HistoryPoint {
    time_t t;
    float temp;
    float hum;
    float pres;
    bool tvalid = false; // validité par grandeur (une tranche peut être valide pour
    bool hvalid = false; // certaines grandeurs et vide pour d'autres, p. ex. si une
    bool pvalid = false; // mesure aberrante a été écartée)
    bool valid = false;  // vrai si au moins une grandeur est valide
};

// Version explicite pour points IN (intérieur)
struct IndoorHistoryPoint {
    time_t t;
    float temp;
    float hum;
    float pres;
    bool tvalid = false;
    bool hvalid = false;
    bool pvalid = false;
    bool valid = false;
};

// Version explicite pour points OUT (extérieur)
struct OutdoorHistoryPoint {
    time_t t;
    float temp;
    float hum;
    float pres;
    bool tvalid = false;
    bool hvalid = false;
    bool pvalid = false;
    bool valid = false;
    
    // Extensions futures (vent, pluie, UV)
    float wind_speed;
    float wind_gust;
    uint16_t wind_direction_deg;
    float rain_rate;
    float rain_accumulated;
    float solar_lux;
    float uv_index;
};

// Une journée disponible dans l'historique binaire, telle que la voit un
// collecteur externe (morfAnalytics). `nrec` est le nombre d'enregistrements
// déjà écrits : un collecteur ayant importé `index` mesures sait qu'il lui en
// reste `nrec - index` à lire.
struct DayIndexEntry {
    uint32_t day_key;   // AAAAMMJJ
    uint32_t nrec;
    uint32_t first_ts;
    uint32_t last_ts;
};

// Une mesure brute, telle qu'elle est stockée (aucune agrégation, aucun filtre).
struct RawRecord {
    uint32_t ts;
    float t, h, p;
};

// Un instantané de PRÉVISION archivé (étape 9 : prévu vs observé). On fige, pour
// chaque jour cible, la prévision « day-ahead » (celle émise la veille), afin de la
// comparer plus tard aux mesures OUT réellement observées ce jour-là. Un fichier
// plat par jour cible : /history/forecast/AAAA-MM-JJ.json (volume faible, 1/jour).
struct ForecastRecord {
    uint32_t target_day;      // AAAAMMJJ : le jour PRÉVU
    uint32_t issued_ts;       // horodatage Unix d'émission de cette prévision
    float temp_min;
    float temp_max;
    std::string description;
};

struct StatMetric {
    float min = 10000.0f;
    float max = -10000.0f;
    double sum = 0;
    int count = 0;
    void add(float v) {
        if (v < min) min = v;
        if (v > max) max = v;
        sum += v;
        count++;
    }
    float avg() const { return count > 0 ? (float)(sum / count) : 0.0f; }
};


struct Stats24h {
    int count = 0;
    StatMetric temp;
    StatMetric hum;
    StatMetric pres;
};

// Structures pour la tendance météo
struct TrendMetric {
    float delta_1h = 0;
    float delta_12h = 0;
    float delta_24h = 0;
    float delta_48h = 0;
    std::string direction_1h;
    std::string direction_12h;
    std::string direction_24h;
    std::string direction_48h;
};

struct MeteoTrend {
    TrendMetric temp;
    TrendMetric hum;
    TrendMetric pres;
    bool available_48h = false; // true si une mesure proche de J-48h a été retrouvée sur la carte SD
};

// Statistiques d'une journée, pré-calculées au fil des acquisitions et persistées
// dans un fichier .stats à côté du .bin du jour. Elles permettent d'afficher la
// synthèse d'une période sans relire les mesures (philosophie « ne pas recalculer
// ce qui peut être construit progressivement »).
struct DayStats {
    uint32_t magic = 0;
    uint32_t count = 0;
    uint32_t first_ts = 0;
    uint32_t last_ts = 0;
    float t_min = 0, t_max = 0; double t_sum = 0;
    float h_min = 0, h_max = 0; double h_sum = 0;
    float p_min = 0, p_max = 0; double p_sum = 0;
    float t_first = 0, h_first = 0, p_first = 0; // première mesure de la journée
    float t_last = 0, h_last = 0, p_last = 0;    // dernière mesure de la journée
};

// Synthèse agrégée d'une plage de jours (min/max/moyenne + première/dernière
// valeur pour la variation), reconstruite à partir des fichiers .stats.
struct RangeSynthesis {
    bool valid = false;
    uint32_t count = 0;
    float t_min = 0, t_max = 0, t_avg = 0, t_first = 0, t_last = 0;
    float h_min = 0, h_max = 0, h_avg = 0, h_first = 0, h_last = 0;
    float p_min = 0, p_max = 0, p_avg = 0, p_first = 0, p_last = 0;
};

class HistoryManager {
public:
    void begin(SdManager* sd = nullptr);
    void update();
    
    // Méthodes existantes (maintenues pour compatibilité, traitées comme IN)
    void add(float t, float h, float p);
    const std::vector<HistoryRecord>& getRecentHistory() const;
    Stats24h getRecentStats() const;
    
    // Nouvelles méthodes explicites IN/OUT
    void addIndoor(const IndoorData& data);
    void addOutdoor(const OutdoorData& data);

    // Dernier relevé OUT vu (RAM), même si l'archivage a été sauté (NTP absent).
    bool hasLiveOutdoor() const { return _hasLiveOutdoor; }
    const OutdoorData& lastOutdoorLive() const { return _lastOutdoorLive; }

    // Vrai dès qu'au moins UNE vraie trame OUT a été reçue par radio depuis le
    // boot (jamais vrai pour une valeur seedée depuis le disque). Tant qu'il est
    // faux, l'OUT est « en attente de première réception » : rien n'est archivé
    // côté OUT, et l'affichage bascule sur l'absence (fallback IN de présentation).
    bool hasReceivedOutdoorSinceBoot() const { return _hasOutdoorRadioMs; }

    // Âge (ms) de la dernière trame OUT REÇUE par radio. Sert au résolveur de
    // lecture effective (fraîcheur/fallback). Renvoie une valeur volontairement
    // énorme si aucune trame n'a encore été reçue depuis le boot (une valeur
    // seedée depuis l'historique disque n'est donc jamais considérée fraîche).
    unsigned long outdoorAgeMs() const {
        if (!_hasOutdoorRadioMs) return 0xFFFFFFFFUL;
        return millis() - _lastOutdoorRadioMs;
    }
    
    const std::vector<IndoorHistoryRecord>& getIndoorHistory() const;
    const std::vector<OutdoorHistoryRecord>& getOutdoorHistory() const;
    
    Stats24h getIndoorStats() const;
    Stats24h getOutdoorStats() const;

    // Agrège les mesures sur une plage temporelle absolue [from, to] (secondes Unix),
    // en tranches de interval_s secondes. Les données sont lues en priorité depuis les
    // fichiers CSV journaliers de la carte SD (couvrant potentiellement plusieurs jours),
    // complétées par l'historique RAM pour la portion la plus récente non encore écrite
    // sur SD (ou l'intégralité si aucune carte SD n'est disponible).
    std::vector<HistoryPoint> queryRange(time_t from, time_t to, long interval_s) const;
    
    // Versions explicites IN/OUT de queryRange
    std::vector<IndoorHistoryPoint> queryIndoorRange(time_t from, time_t to, long interval_s) const;
    std::vector<OutdoorHistoryPoint> queryOutdoorRange(time_t from, time_t to, long interval_s) const;

    // Synthèse pré-calculée d'une plage [from, to], agrégée depuis les fichiers
    // .stats journaliers (aucune relecture des mesures). Renvoie valid=false si
    // aucun .stats n'est disponible pour la plage.
    RangeSynthesis querySynthesis(time_t from, time_t to) const;
    
    // Versions explicites IN/OUT de querySynthesis
    RangeSynthesis queryIndoorSynthesis(time_t from, time_t to) const;
    RangeSynthesis queryOutdoorSynthesis(time_t from, time_t to) const;

    // Export CSV en flux : émet une ligne CSV par mesure de la plage [from, to]
    // (en-tête inclus), lues depuis les fichiers binaires journaliers. Le callback
    // reçoit chaque ligne prête à écrire (permet un streaming HTTP sans tout garder
    // en mémoire). Le CSV reste ainsi le format d'export, adapté à Excel/LibreOffice.
    void exportCsv(time_t from, time_t to, const std::function<void(const char*)>& emit) const;
    
    // Versions explicites IN/OUT de exportCsv
    void exportIndoorCsv(time_t from, time_t to, const std::function<void(const char*)>& emit) const;
    void exportOutdoorCsv(time_t from, time_t to, const std::function<void(const char*)>& emit) const;

    // --- Collecte incrémentale externe (morfAnalytics) -----------------------
    // Les fichiers journaliers sont écrits en AJOUT SEUL : l'index d'un
    // enregistrement dans son fichier ne change jamais. Le couple
    // (jour, index) forme donc un curseur strictement monotone, contrairement à
    // un horodatage, qui peut reculer ou se répéter lors d'un changement d'heure
    // ou d'un recalage NTP. Un collecteur mémorise (jour, index) et ne demande
    // que les enregistrements suivants — jamais de doublon, jamais de trou.

    // Journées présentes sur la carte SD, triées par date croissante.
    // Version par défaut = flux IN (legacy) ; version OUT = flux extérieur.
    std::vector<DayIndexEntry> listDays() const;
    std::vector<DayIndexEntry> listDaysOutdoor() const;

    // Émet les enregistrements [from_index, from_index + limit) du jour donné
    // (AAAAMMJJ). Renvoie le nombre total d'enregistrements du jour, ce qui
    // permet à l'appelant de savoir s'il reste des données à lire.
    // Version par défaut = IN (legacy) ; version OUT = flux extérieur.
    uint32_t exportRaw(uint32_t day_key, uint32_t from_index, uint32_t limit,
                       const std::function<void(const RawRecord&)>& emit) const;
    uint32_t exportRawOutdoor(uint32_t day_key, uint32_t from_index, uint32_t limit,
                              const std::function<void(const RawRecord&)>& emit) const;

    // --- Prévisions archivées (étape 9 : prévu vs observé) -------------------
    // Fige la prévision « day-ahead » sous son jour cible (fichier plat JSON).
    // Écrasé au fil de J-1 : le fichier du jour J finit avec la dernière prévision
    // émise en J-1 pour J, la comparaison de référence.
    void addForecast(const ForecastRecord& rec);
    // Émet le JSON brut de chaque prévision dont le jour cible tombe dans
    // [from, to] (streaming HTTP, sans tout charger en mémoire). Trié par jour.
    void forecastHistoryRaw(time_t from, time_t to,
                            const std::function<void(const char*)>& emit) const;

    // Gestion LittleFS
    void clearHistory();
    // Tendance IN (confort) et OUT (météo). La tendance météo doit s'appuyer sur
    // l'extérieur : getTrendOutdoor() en est la source dédiée.
    MeteoTrend getTrend() const;
    MeteoTrend getTrendOutdoor() const;
private:
    // Historiques séparés IN et OUT
    std::vector<HistoryRecord> _recentHistory; // legacy = flux IN (intérieur)
    std::vector<OutdoorHistoryRecord> _outdoorHistory;
    OutdoorData _lastOutdoorLive;
    bool _hasLiveOutdoor = false;
    unsigned long _lastOutdoorRadioMs = 0; // millis() de la dernière trame OUT reçue
    bool _hasOutdoorRadioMs = false;       // true dès la 1re trame radio (pas le seed disque)
    
    SdManager* _sd = nullptr;
    unsigned long _lastSave = 0;

    // Cache RAM des statistiques du jour courant (évite de relire le .stats à
    // chaque acquisition). _currentDayKey vaut AAAAMMJJ (0 = non initialisé).
    DayStats _currentDayStats; // = jour courant IN (legacy)
    DayStats _outdoorDayStats;
    uint32_t _currentDayKey = 0;

    void loadRecent();
    void saveRecent(const HistoryRecord& record);
    void saveOutdoorRecent(const OutdoorHistoryRecord& record);

    // Stockage binaire journalier, arborescence SYMÉTRIQUE IN/OUT :
    // IN  : /history/indoor/AAAA/MM/AAAA-MM-JJ.bin + .stats
    // OUT : /history/outdoor/AAAA/MM/AAAA-MM-JJ.bin + .stats
    void saveToSdBinary(const HistoryRecord& record);
    void saveOutdoorToSdBinary(const OutdoorHistoryRecord& record);
    bool ensureDayDirs(const struct tm& tinfo) const;
    bool ensureOutdoorDayDirs(const struct tm& tinfo) const;
    void buildDayPaths(const struct tm& tinfo, char* binPath, char* statsPath, size_t sz) const;
    void buildOutdoorDayPaths(const struct tm& tinfo, char* binPath, char* statsPath, size_t sz) const;
    void updateDayStats(const HistoryRecord& record, const struct tm& tinfo);
    void updateOutdoorDayStats(const OutdoorHistoryRecord& record, const struct tm& tinfo);
    bool readDayStats(time_t day_ts, DayStats& out) const;
    bool readOutdoorDayStats(time_t day_ts, DayStats& out) const;
    bool readBinSampleNear(time_t target_ts, float& t_out, float& h_out, float& p_out,
                           bool outdoor = false) const;
    // Cœur commun de tendance, paramétré IN (confort) / OUT (météo).
    MeteoTrend getTrendImpl(bool outdoor) const;

    // Coeur commun des routes de collecte, paramétré par la racine de stockage
    // ("/history/indoor" pour IN, "/history/outdoor" pour OUT). Les deux racines
    // sont désormais des sœurs sous /history : arborescences identiques, aucun
    // recouvrement possible entre les deux flux.
    std::vector<DayIndexEntry> listDaysFromRoot(const char* root) const;

    // Coeur commun de queryRange, paramétré IN (legacy) / OUT. Agrège les .bin
    // journaliers de la racine correspondante + l'historique RAM du même contexte.
    std::vector<HistoryPoint> queryRangeImpl(time_t from, time_t to, long interval_s,
                                             bool outdoor) const;

    // Coeur commun d'export CSV, paramétré IN (legacy) / OUT.
    void exportCsvImpl(time_t from, time_t to,
                       const std::function<void(const char*)>& emit, bool outdoor) const;
    uint32_t exportRawFromRoot(const char* root, uint32_t day_key, uint32_t from_index,
                               uint32_t limit,
                               const std::function<void(const RawRecord&)>& emit) const;

    // Migration unique des anciens CSV plats (/history/AAAA-MM-JJ.csv) au démarrage.
    void migrateCsvToBinary();
    void removeDirRecursive(const char* path) const;

    // Helpers SD (legacy CSV, conservés en repli de lecture)
    void createSdStructure();
    bool readSdSampleNear(time_t target_ts, float& t_out, float& h_out, float& p_out,
                          bool outdoor = false) const;
};