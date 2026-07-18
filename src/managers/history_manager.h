#pragma once
#include <vector>
#include <string>
#include <functional>
#include <Arduino.h>
#include "sd_manager.h"
 
struct HistoryRecord {
    time_t timestamp;
    float t; // Température
    float h; // Humidité
    float p; // Pression
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
    void add(float t, float h, float p);
    
    // Récupère l'historique récent (RAM)
    const std::vector<HistoryRecord>& getRecentHistory() const;
    Stats24h getRecentStats() const;

    // Agrège les mesures sur une plage temporelle absolue [from, to] (secondes Unix),
    // en tranches de interval_s secondes. Les données sont lues en priorité depuis les
    // fichiers CSV journaliers de la carte SD (couvrant potentiellement plusieurs jours),
    // complétées par l'historique RAM pour la portion la plus récente non encore écrite
    // sur SD (ou l'intégralité si aucune carte SD n'est disponible).
    std::vector<HistoryPoint> queryRange(time_t from, time_t to, long interval_s) const;

    // Synthèse pré-calculée d'une plage [from, to], agrégée depuis les fichiers
    // .stats journaliers (aucune relecture des mesures). Renvoie valid=false si
    // aucun .stats n'est disponible pour la plage.
    RangeSynthesis querySynthesis(time_t from, time_t to) const;

    // Export CSV en flux : émet une ligne CSV par mesure de la plage [from, to]
    // (en-tête inclus), lues depuis les fichiers binaires journaliers. Le callback
    // reçoit chaque ligne prête à écrire (permet un streaming HTTP sans tout garder
    // en mémoire). Le CSV reste ainsi le format d'export, adapté à Excel/LibreOffice.
    void exportCsv(time_t from, time_t to, const std::function<void(const char*)>& emit) const;

    // Gestion LittleFS
    void clearHistory();
    MeteoTrend getTrend() const;
private:
    std::vector<HistoryRecord> _recentHistory;
    SdManager* _sd = nullptr;
    unsigned long _lastSave = 0;

    // Cache RAM des statistiques du jour courant (évite de relire le .stats à
    // chaque acquisition). _currentDayKey vaut AAAAMMJJ (0 = non initialisé).
    DayStats _currentDayStats;
    uint32_t _currentDayKey = 0;

    void loadRecent();
    void saveRecent(const HistoryRecord& record);

    // Stockage binaire journalier (/history/AAAA/MM/AAAA-MM-JJ.bin + .stats)
    void saveToSdBinary(const HistoryRecord& record);
    bool ensureDayDirs(const struct tm& tinfo) const;
    void buildDayPaths(const struct tm& tinfo, char* binPath, char* statsPath, size_t sz) const;
    void updateDayStats(const HistoryRecord& record, const struct tm& tinfo);
    bool readDayStats(time_t day_ts, DayStats& out) const;
    bool readBinSampleNear(time_t target_ts, float& t_out, float& h_out, float& p_out) const;

    // Migration unique des anciens CSV plats (/history/AAAA-MM-JJ.csv) au démarrage.
    void migrateCsvToBinary();
    void removeDirRecursive(const char* path) const;

    // Helpers SD (legacy CSV, conservés en repli de lecture)
    void createSdStructure();
    bool readSdSampleNear(time_t target_ts, float& t_out, float& h_out, float& p_out) const;
};