#pragma once
#include <vector>
#include <string>
#include <Arduino.h>
#include "sd_manager.h"
 
struct HistoryRecord {
    time_t timestamp;
    float t; // Température
    float h; // Humidité
    float p; // Pression
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

class HistoryManager {
public:
    void begin(SdManager* sd = nullptr);
    void update();
    void add(float t, float h, float p);
    
    // Récupère l'historique récent (RAM)
    const std::vector<HistoryRecord>& getRecentHistory() const;
    Stats24h getRecentStats() const;
    
    // Gestion LittleFS
    void clearHistory();
    MeteoTrend getTrend() const;
private:
    std::vector<HistoryRecord> _recentHistory;
    SdManager* _sd = nullptr;
    unsigned long _lastSave = 0;

    void loadRecent();
    void saveRecent(const HistoryRecord& record);

    // Helpers SD
    void saveToSd(const HistoryRecord& record);
    void createSdStructure();
    bool readSdSampleNear(time_t target_ts, float& t_out, float& h_out, float& p_out) const;
};