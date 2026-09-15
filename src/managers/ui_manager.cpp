#include "managers/ui_manager.h"
#include "utils/logs.h"
#include <Arduino.h>
#include "board_config.h"
#include "config.h"
#include <LittleFS.h> // Nécessaire pour l'effacement des logs

#if defined(ESP32_S3_OLED)
#include "modules/pages_oled.h"
#endif

namespace {
    constexpr unsigned long UI_MESSAGE_SHORT_MS = 1000;
    constexpr unsigned long UI_MESSAGE_LONG_MS = 2000;
}

// Implémentation de la méthode clearLogs
void UiManager::clearLogs() {
    LOG_INFO("User requested log clear");
    
    // Si vos logs sont stockés dans un fichier sur LittleFS
    if (LittleFS.exists("/logs.txt")) {
        LittleFS.remove("/logs.txt");
        LOG_INFO("File /logs.txt removed");
    }
    
    // Si votre système de logs global (utils/logs.h) a une fonction de reset en RAM, appelez-la ici.
    // Exemple hypothétique : resetLogBuffer(); 
    // Pour l'instant, on se contente de supprimer le fichier persistant.
}

void UiManager::showTransientMessage(UiTransientMessageType messageType, unsigned long durationMs) {
    transientMessage = messageType;
    transientMessageUntilMs = millis() + durationMs;
    drawPage();
}

bool UiManager::processTransientMessage() {
    if (transientMessage == UI_MESSAGE_NONE) {
        return false;
    }

    if (millis() < transientMessageUntilMs) {
        return true;
    }

    // Gestion de la fin du message avec résultat en attente (ex: formatage)
    if (pendingFormatResult) {
        pendingFormatResult = false;
        showTransientMessage(pendingFormatResultSuccess ? UI_MESSAGE_FORMAT_SUCCESS : UI_MESSAGE_FORMAT_FAIL, UI_MESSAGE_LONG_MS);
        return true;
    }

    transientMessage = UI_MESSAGE_NONE;
    transientMessageUntilMs = 0;
    drawPage();
    return false;
}

void UiManager::begin(DisplayInterface& display, WifiManager& wifiMgr, SensorManager& sensorMgr, ForecastManager& forecastMgr, HistoryManager& historyMgr, SdManager& sdMgr) {
    d = &display;
    wifi = &wifiMgr;
    sensors = &sensorMgr;
    forecast = &forecastMgr;
    history = &historyMgr;
    sd = &sdMgr;

    enc.begin();
    
    pinMode(BUTTON_BACK_PIN, INPUT_PULLUP);
    pinMode(BUTTON_CONFIRM_PIN, INPUT_PULLUP);

    page = 0; 
    last_rendered_page = -1;
    last_rendered_menu_mode = false;
    last_rendered_confirm_mode = false;
    menuMode = false;
    confirmFormatMode = false;
    confirmClearLogsMode = false;
    confirmClearHistMode = false;
}

void UiManager::update() {
    enc.update();
    int diff = enc.getStepCount();
    
    if (diff != 0) {
        enc.clearQueue();
        
#if defined(ESP32_S3_OLED)
        diff = -diff; // Inversion pour OLED
#endif

        if (menuMode) {
            menuIndex -= diff;
            if (menuIndex < 0) menuIndex = MENU_COUNT - 1;
            if (menuIndex >= MENU_COUNT) menuIndex = 0;

            #if defined(ESP32_S3_OLED)
            const int MENU_VISIBLE_ITEMS = 4;
            if (menuIndex < menuScrollOffset) {
                menuScrollOffset = menuIndex;
            } else if (menuIndex >= menuScrollOffset + MENU_VISIBLE_ITEMS) {
                menuScrollOffset = menuIndex - MENU_VISIBLE_ITEMS + 1;
            }
            #endif
        } else {
            page += diff;
            if (page < 0) page = PAGE_COUNT - 1;
            if (page >= PAGE_COUNT) page = 0;
            logScrollLine = 0;
        }
        drawPage();
        lastRefresh = millis();
    }

    if (processTransientMessage()) {
        return;
    }

    handleButtons();

    if (millis() - lastRefresh > DASHBOARD_REFRESH_MS) {
        drawPage();
        lastRefresh = millis();
    }
    
    if (page == PAGE_FORECAST && !menuMode) {
        if (millis() - lastForecastViewSwitch > 5000) {
            forecastViewIndex = (forecastViewIndex + 1) % 3;
            lastForecastViewSwitch = millis();
            drawPage();
        }
    }
}

void UiManager::handleButtons() {
    bool btnBack = (digitalRead(BUTTON_BACK_PIN) == LOW);
    bool btnConfirm = (digitalRead(BUTTON_CONFIRM_PIN) == LOW);
    bool btnEncoder = enc.clicked();

    if (millis() < ignoreButtonsUntilMs) return;

    // --- Gestion des confirmations ---
    
    if (confirmFormatMode) {
        if (btnEncoder || btnConfirm) {
            ignoreButtonsUntilMs = millis() + BUTTON_GUARD_MS;
            d->clear();
            int h = 64; // Supposition hauteur écran
            d->center(h / 2 - 10, "Formatage SD...");
            d->center(h / 2 + 10, "Veuillez patienter.");
            d->show();

            bool success = sd->format();
            confirmFormatMode = false;
            pendingFormatResult = true;
            pendingFormatResultSuccess = success;
            // Message court pour laisser place au résultat
            showTransientMessage(UI_MESSAGE_FORMAT_IN_PROGRESS, 50); 
            return;
        }
        if (btnBack) {
            confirmFormatMode = false;
            drawPage();
            ignoreButtonsUntilMs = millis() + BUTTON_GUARD_MS;
            return;
        }
        return;
    }

    if (confirmClearLogsMode) {
        if (btnEncoder || btnConfirm) {
            clearLogs(); // Appel de la méthode maintenant définie
            confirmClearLogsMode = false;
            showTransientMessage(UI_MESSAGE_LOGS_CLEARED, UI_MESSAGE_SHORT_MS);
        } else if (btnBack) {
            confirmClearLogsMode = false;
            drawPage();
        }
        if (btnEncoder || btnConfirm || btnBack) ignoreButtonsUntilMs = millis() + BUTTON_GUARD_MS;
        return;
    }

    if (confirmClearHistMode) {
        if (btnEncoder || btnConfirm) {
            history->clearHistory();
            confirmClearHistMode = false;
            showTransientMessage(UI_MESSAGE_HISTORY_CLEARED, UI_MESSAGE_SHORT_MS);
        } else if (btnBack) {
            confirmClearHistMode = false;
            drawPage();
        }
        if (btnEncoder || btnConfirm || btnBack) ignoreButtonsUntilMs = millis() + BUTTON_GUARD_MS;
        return;
    }

    // --- Navigation normale ---

    if (btnBack) {
        if (menuMode) {
            menuMode = false;
        }
        ignoreButtonsUntilMs = millis() + BUTTON_GUARD_MS;
        drawPage();
        return;
    }

    if (menuMode) {
        if (btnConfirm || btnEncoder) {
            switch(menuIndex) {
                case MENU_EXIT: 
                    menuMode = false; 
                    break;
                case MENU_REBOOT: 
                    ESP.restart(); 
                    break;
                case MENU_CLEAR_LOGS: 
                    confirmClearLogsMode = true;
                    drawPage();
                    return;
                case MENU_CLEAR_HISTORY: 
                    confirmClearHistMode = true;
                    drawPage();
                    return;
                case MENU_FORMAT_SD:
                    confirmFormatMode = true;
                    ignoreButtonsUntilMs = millis() + BUTTON_GUARD_MS;
                    drawPage();
                    return;
            }
            ignoreButtonsUntilMs = millis() + BUTTON_GUARD_MS;
            drawPage();
        }
    } else {
        if (btnEncoder) {
            menuMode = true;
            menuIndex = 0;
            ignoreButtonsUntilMs = millis() + BUTTON_GUARD_MS;
            drawPage();
        } else if (btnConfirm) {
            if (page == PAGE_FORECAST) {
                forecastViewIndex = (forecastViewIndex + 1) % 3;
                lastForecastViewSwitch = millis();
                drawPage();
            } else if (page == PAGE_LOGS) {
                logScrollLine++;
                // Limite à ajuster selon getLogCount() si disponible
                drawPage();
            }
            ignoreButtonsUntilMs = millis() + BUTTON_GUARD_MS;
        }
    }
}

void UiManager::drawPage() {
    const bool current_confirm_mode = confirmClearLogsMode || confirmClearHistMode || confirmFormatMode;
    const bool screen_context_changed = (page != last_rendered_page) ||
        (menuMode != last_rendered_menu_mode) ||
        (current_confirm_mode != last_rendered_confirm_mode);

    if (screen_context_changed) {
        d->clear();
    }

    last_rendered_page = page;
    last_rendered_menu_mode = menuMode;
    last_rendered_confirm_mode = current_confirm_mode;

    // Priorité aux messages transitoires
    if (transientMessage != UI_MESSAGE_NONE) {
        d->clear();
        d->center(10, "INFO");
        switch (transientMessage) {
            case UI_MESSAGE_FORMAT_IN_PROGRESS:
                d->center(28, "Formatage...");
                d->center(44, "Patientez...");
                break;
            case UI_MESSAGE_FORMAT_SUCCESS:
                d->center(32, "Reussi !");
                break;
            case UI_MESSAGE_FORMAT_FAIL:
                d->center(32, "Echec !");
                break;
            case UI_MESSAGE_LOGS_CLEARED:
                d->center(32, "Logs effaces");
                break;
            case UI_MESSAGE_HISTORY_CLEARED:
                d->center(32, "Hist efface");
                break;
            default:
                break;
        }
        d->show();
        return;
    }

    // Écrans de confirmation
    if (current_confirm_mode) {
        d->center(10, "CONFIRMER");
        
        // CORRECTION ICI : Construction explicite du string pour éviter l'erreur de compilateur
        std::string msg;
        if (confirmFormatMode) {
            msg = "Formater SD ?";
        } else if (confirmClearLogsMode) {
            msg = "Effacer Logs ?";
        } else {
            msg = "Effacer Hist ?";
        }
        
        d->center(30, msg.c_str());
        d->text(0, 50, "Clic=OK, Back=Non");
        d->show();
        return;
    }

    if (menuMode) {
        const char* itemNames[] = { "Retour", "Reboot", "Clear Logs", "Clear Hist", "Format SD" };
        const int MENU_VISIBLE_ITEMS = 4;

        d->clear();
        d->center(0, "MENU");

        // Scroll simple
        int start = 0;
        if (menuIndex >= MENU_VISIBLE_ITEMS) {
            start = menuIndex - MENU_VISIBLE_ITEMS + 1;
        }

        for (int i = 0; i < MENU_VISIBLE_ITEMS; i++) {
            int itemIndex = start + i;
            if (itemIndex >= MENU_COUNT) break;

            std::string line = (itemIndex == menuIndex) ? "> " : "  ";
            line += itemNames[itemIndex];
            d->text(0, 16 + i * 12, line.c_str());
        }
        d->show();
        return;
    }

    // Rendu des pages
    int pCount = PAGE_COUNT;
#if defined(ESP32_S3_OLED)
    switch(page) {
        case PAGE_WEATHER: pageWeather_oled(*d, *sensors, *forecast, *history, page + 1, pCount); break;
        case PAGE_FORECAST: pageForecast_oled(*d, *forecast, forecastViewIndex, page + 1, pCount); break;
        case PAGE_GRAPH_IN_TEMP:  pageGraph_oled(*d, *history, 0, page + 1, pCount, false); break;
        case PAGE_GRAPH_IN_HUM:   pageGraph_oled(*d, *history, 1, page + 1, pCount, false); break;
        case PAGE_GRAPH_IN_PRES:  pageGraph_oled(*d, *history, 2, page + 1, pCount, false); break;
        case PAGE_GRAPH_OUT_TEMP: pageGraph_oled(*d, *history, 0, page + 1, pCount, true); break;
        case PAGE_GRAPH_OUT_HUM:  pageGraph_oled(*d, *history, 1, page + 1, pCount, true); break;
        case PAGE_GRAPH_OUT_PRES: pageGraph_oled(*d, *history, 2, page + 1, pCount, true); break;
        case PAGE_NETWORK: pageNetwork_oled(*d, *wifi, page + 1, pCount); break;
        case PAGE_LOGS: pageLogs_oled(*d, page + 1, pCount, logScrollLine); break;
        case PAGE_SYSTEM: pageSystem_oled(*d, page + 1, pCount); break;
        case PAGE_SENSOR: pageSensor_oled(*d, *history, page + 1, pCount); break;
    }
#endif
}