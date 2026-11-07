#pragma once
// Types de donnees PORTABLES de l'historique (sans Arduino) : enregistrements, points
// agreges, statistiques, tendances. Partages par le gestionnaire (history_manager.h) et par
// les unites de src/history/ ; testables sur l'hote.
#include <cstdint>
#include <ctime>
#include <string>

// HistoryRecord devient explicitement IN (mesures intérieures)
// Maintenu pour compatibilité avec l'historique existant
struct HistoryRecord {
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
    uint32_t flags; // marques de l'enregistrement (0 = mesure normale ; format v2)
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

// Le fichier .stats EST cette structure, écrite telle quelle : sa taille fait partie du format.
// Magic d'un .stats valide ("1BTS") : identifie et versionne le fichier.
constexpr uint32_t kDayStatsMagic = 0x53544231u;
static_assert(sizeof(DayStats) == 88, "DayStats : 88 octets sur disque");

// Synthèse agrégée d'une plage de jours (min/max/moyenne + première/dernière
// valeur pour la variation), reconstruite à partir des fichiers .stats.
struct RangeSynthesis {
    bool valid = false;
    uint32_t count = 0;
    float t_min = 0, t_max = 0, t_avg = 0, t_first = 0, t_last = 0;
    float h_min = 0, h_max = 0, h_avg = 0, h_first = 0, h_last = 0;
    float p_min = 0, p_max = 0, p_avg = 0, p_first = 0, p_last = 0;
};
