#pragma once
#include <string>
 
struct Forecast {
    // Daily
    float temp_day;
    float temp_min;
    float temp_max;
    std::string description;
    int weather_id;
};

struct Alert {
    std::string sender;
    std::string event;
    std::string description;
    int severity; // 1=Yellow, 2=Orange, 3=Red
    long start_unix = 0;
    long end_unix = 0;
};

class ForecastManager {
public:
    void begin();
    void update();

    Forecast today;
    Forecast tomorrow;
    Alert alert;
    bool alert_active = false;

private:
    unsigned long lastUpdate = 0;
    void parseResponse(const std::string& payload);
};