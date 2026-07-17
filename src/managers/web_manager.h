#ifndef WEB_MANAGER_H
#define WEB_MANAGER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "config.h"
#include "managers/history_manager.h"
#include "managers/forecast_manager.h"
#include "modules/sensors.h"
 
class WebManager {
public:
    WebManager();
    void begin(HistoryManager& history, SdManager& sd, ForecastManager& forecast, SensorManager& sensors);
    void handle();

private:
    AsyncWebServer _server;
    
    void _setupRoutes();
    void _setupApi();
    HistoryManager* _history = nullptr;
    SdManager* _sd = nullptr;
    ForecastManager* _forecast = nullptr;
    SensorManager* _sensors = nullptr;

    bool _ota_upload_error = false;
    unsigned long _ota_restart_at_ms = 0;
};

#endif // WEB_MANAGER_H
