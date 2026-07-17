#include <string>
#include <time.h>
#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoOTA.h>

#include "managers/forecast_manager.h"
#include "managers/ui_manager.h"
#include "managers/wifi_manager.h"
#include "managers/web_manager.h"
#include "managers/history_manager.h"
#include "managers/sd_manager.h"
#include "modules/neopixel_status.h"
#include "modules/sensors.h"
#include "config.h"
#if defined(ESP32_S3_OLED)
#include "modules/oled_display.h"
#include "modules/pages_oled.h"
#endif
#include "utils/logs.h"

DisplayInterface* display = nullptr;
WifiManager wifi;
UiManager ui;
SensorManager sensors;
ForecastManager forecast;
WebManager webManager;
HistoryManager history;
SdManager sdCard;
bool ota_started = false;

void setup() {
    Serial.begin(115200);

#if defined(ESP32_S3_OLED)
    static OledDisplay oled;
    display = &oled;
#endif
    display->begin();
    neoInit();

    ArduinoOTA.setHostname(WEB_MDNS_HOSTNAME);
    ArduinoOTA.onStart([]() {
        LOG_INFO("OTA update start");
    });
    ArduinoOTA.onEnd([]() {
        LOG_INFO("OTA update end");
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        if (total == 0) {
            return;
        }
        int percent = static_cast<int>((progress * 100U) / total);
        LOG_DEBUG("OTA progress: " + std::to_string(percent) + "%");
    });
    ArduinoOTA.onError([](ota_error_t error) {
        LOG_ERROR("OTA error code: " + std::to_string(static_cast<int>(error)));
    });

    // --- Maintenance : Formatage LittleFS si BOOT maintenu ---
    pinMode(0, INPUT_PULLUP);
    if (digitalRead(0) == LOW) {
        delay(100); // Debounce
        if (digitalRead(0) == LOW) {
            display->clear();
            // Utilisation de la méthode center de base
            display->center(30, "MAINTENANCE");
            display->center(50, "Maintenir BOOT");
            display->center(70, "pour Formater");
            display->show();
            
            delay(3000);
            
            if (digitalRead(0) == LOW) {
                display->clear();
                display->center(50, "Formatage...");
                display->show();
                
                LittleFS.begin(true);
                LittleFS.format();
                
                display->clear();
                display->center(50, "Redemarrage...");
                display->show();
                delay(1000);
                ESP.restart();
            }
        }
    }

    // Montage du système de fichiers
    bool fsMounted = true;
    if (!LittleFS.begin(false)) { // Essai sans formatage d'abord
        if (!LittleFS.begin(true)) { // Formatage si échec
            LOG_ERROR("LittleFS Mount Failed");
            fsMounted = false;
        }
    }
    
    if (fsMounted) {
        if (!LittleFS.exists("/history")) {
            LittleFS.mkdir("/history");
        }
    }

    // Initialisation Carte SD (Optionnel)
    sdCard.begin();

    // Etape 1 : Splash Screen (MORFREDUS + Projet)
#if defined(ESP32_S3_OLED)
    drawSplashScreen_oled(*display);
#endif

    LOG_INFO("System Boot");

    // Etape 2 : Capteurs
#if defined(ESP32_S3_OLED)
    drawBootProgress_oled(*display, 1, 5, "Init Capteurs...");
#endif
    sensors.begin();
    delay(200); // Petit delai visuel

    // Etape 3 : WiFi
#if defined(ESP32_S3_OLED)
    drawBootProgress_oled(*display, 2, 5, "Connexion WiFi...");
#endif
    wifi.begin();

    // Boucle d'attente WiFi (Max ~10s)
    int w = 0;
    while (wifi.ip() == "0.0.0.0" && w < 100) {
        wifi.update();
        delay(100);
        w++;
    }

    if (wifi.ip() != "0.0.0.0") {
        ArduinoOTA.begin();
        ota_started = true;
        LOG_INFO(std::string("OTA ready: ") + WEB_MDNS_HOSTNAME + ".local");
    } else {
        LOG_WARNING("OTA not started (WiFi unavailable at boot)");
    }

    // Etape 4 : Heure
#if defined(ESP32_S3_OLED)
    drawBootProgress_oled(*display, 3, 5, "Sync Heure...");
#endif
    configTime(3600, 3600, "pool.ntp.org");

    // Boucle d'attente NTP (Max 10s)
    struct tm timeinfo;
    int t = 0;
    while (!getLocalTime(&timeinfo, 0) && t < 100) {
        delay(100);
        t++;
    }
    
    if (getLocalTime(&timeinfo, 0)) {
        LOG_INFO("NTP Sync OK");
    } else {
        LOG_WARNING("NTP Sync Fail");
    }

    // Etape 5 : Pret
#if defined(ESP32_S3_OLED)
    drawBootProgress_oled(*display, 4, 5, "Chargement Historique...");
#endif
    
    // Etape 6 : Lancement
#if defined(ESP32_S3_OLED)
    drawBootProgress_oled(*display, 5, 5, "Systeme Pret");
#endif
    delay(800);
    
    history.begin(&sdCard); // Injection de la dépendance SD
    
    // Lancement des modules principaux
    forecast.begin();
    webManager.begin(history, sdCard, forecast, sensors);

    ui.begin(*display, wifi, sensors, forecast, history, sdCard);
}

void loop() {
    static unsigned long lastHistoryUpdate = 0;
    static unsigned long lastLedUpdate = 0;

    wifi.update();

    if (!ota_started && wifi.ip() != "0.0.0.0") {
        ArduinoOTA.begin();
        ota_started = true;
        LOG_INFO(std::string("OTA ready (late): ") + WEB_MDNS_HOSTNAME + ".local");
    }
    if (ota_started) {
        ArduinoOTA.handle();
    }

    forecast.update();
    history.update(); 
    ui.update();
    webManager.handle();

    // Gestion LED Status (toutes les 500ms)
    if (millis() - lastLedUpdate >= 500) {
        lastLedUpdate = millis();
        static bool blink = false;
        blink = !blink;

        if (forecast.alert_active) {
            if (forecast.alert.severity >= 3) { if (blink) neoAlertRed(); else neoOff(); }
            else if (forecast.alert.severity == 2) neoAlertOrange();
            else neoAlertYellow();
        } else {
            if (wifi.ip() != "0.0.0.0") neoWifiOK();
            else { if (blink) neoWifiLost(); else neoOff(); }
        }
    }

    // Enregistrement historique toutes les minutes
    if (millis() - lastHistoryUpdate >= 60000) {
        lastHistoryUpdate = millis();
        SensorData data = sensors.read();
        if (data.valid) {
            // Filtrage des valeurs aberrantes (bruit capteur)
            bool valuesOk = true;
            if (data.temperature < -40.0f || data.temperature > 85.0f) valuesOk = false;
            if (data.humidity < 0.0f || data.humidity > 100.0f) valuesOk = false;
            if (data.pressure < 800.0f || data.pressure > 1200.0f) valuesOk = false;

            if (valuesOk) {
                history.add(data.temperature, data.humidity, data.pressure);
            } else {
                LOG_WARNING("Valeurs capteurs hors limites ignorees");
            }
        }
    }
}
