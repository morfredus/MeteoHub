#pragma once

#include "modules/display_interface.h"
#include "managers/wifi_manager.h"
#include "modules/sensors.h"
#include "managers/forecast_manager.h"
#include "managers/history_manager.h"
#include "managers/sd_manager.h"
#include "modules/encoder.h"
#include <string>
 
enum Page {
    PAGE_WEATHER,
    PAGE_FORECAST,
    PAGE_GRAPH_IN_TEMP,
    PAGE_GRAPH_IN_HUM,
    PAGE_GRAPH_IN_PRES,
    PAGE_GRAPH_OUT_TEMP,
    PAGE_GRAPH_OUT_HUM,
    PAGE_GRAPH_OUT_PRES,
    PAGE_NETWORK,
    PAGE_SYSTEM,
    PAGE_SENSOR,
    PAGE_LOGS,
    PAGE_COUNT
};

enum MenuItem {
    MENU_EXIT,
    MENU_REBOOT,
    MENU_CLEAR_LOGS,
    MENU_CLEAR_HISTORY,
    MENU_FORMAT_SD,
    MENU_COUNT
};

class UiManager {
public:
    void begin(DisplayInterface& display, WifiManager& wifiMgr, SensorManager& sensorMgr, ForecastManager& forecastMgr, HistoryManager& historyMgr, SdManager& sdMgr);
    void update();

private:
    DisplayInterface* d = nullptr;
    WifiManager* wifi = nullptr;
    SensorManager* sensors = nullptr;
    ForecastManager* forecast = nullptr;
    HistoryManager* history = nullptr;
    SdManager* sd = nullptr;
    Encoder enc;
    
    int page = 0;
    bool menuMode = false;
    int menuIndex = 0;
    int forecastViewIndex = 0;
    unsigned long lastRefresh = 0;
    unsigned long lastForecastViewSwitch = 0;
    unsigned long ignoreButtonsUntilMs = 0;
    int menuScrollOffset = 0;
    int logScrollLine = 0;
    
    // États de confirmation
    bool confirmFormatMode = false;
    bool confirmClearLogsMode = false;
    bool confirmClearHistMode = false;
    
    // États de rendu pour optimisation
    int last_rendered_page = -1;
    bool last_rendered_menu_mode = false;
    bool last_rendered_confirm_mode = false;

    // Messages transitoires
    enum UiTransientMessageType {
        UI_MESSAGE_NONE,
        UI_MESSAGE_FORMAT_IN_PROGRESS,
        UI_MESSAGE_FORMAT_SUCCESS,
        UI_MESSAGE_FORMAT_FAIL,
        UI_MESSAGE_LOGS_CLEARED,
        UI_MESSAGE_HISTORY_CLEARED
    };

    UiTransientMessageType transientMessage = UI_MESSAGE_NONE;
    unsigned long transientMessageUntilMs = 0;
    bool pendingFormatResult = false;
    bool pendingFormatResultSuccess = false;

    // Méthodes privées
    void showTransientMessage(UiTransientMessageType messageType, unsigned long durationMs);
    bool processTransientMessage();
    void handleButtons();
    void drawPage();
    
    // NOUVEAU : Déclaration de la méthode pour effacer les logs
    void clearLogs();
};