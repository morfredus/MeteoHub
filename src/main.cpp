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
#include "modules/analytics_beacon.h"
#include "modules/battery_alert.h"
#include "modules/espnow_receiver.h"
#include "modules/meteo_sync_service.h"
#include "../third_party/morf/beacon-arduino/morfbeacon_emitter.h"
#include "config.h"
#include "board_config.h"
#include "../include/meteo_packet.h"
#if defined(ESP32_S3_OLED)
#include "modules/oled_display.h"
#include "modules/pages_oled.h"
#endif
#include "utils/logs.h"
#include "utils/udp_logger.h"

DisplayInterface* display = nullptr;
WifiManager wifi;
UiManager ui;
SensorManager sensors;
ForecastManager forecast;
WebManager webManager;
HistoryManager history;
SdManager sdCard;
AnalyticsBeacon analytics;
EspNowReceiver espNowReceiver;
mhsync::MeteoSyncService meteoSync; // suivi reception / dedup / horodatage / voie inverse (v3)

// Annonce de presence sur le LAN (protocole morfbeacon/1). MeteoHub ECOUTAIT
// deja ce protocole pour reperer un service d'analyse ; il l'EMET desormais, et
// devient donc decouvrable par le meme mecanisme que les services Linux et
// Windows du parc — sans qu'aucun consommateur ait a connaitre son adresse ni
// son nom mDNS a l'avance.
morfbeacon::Emitter presence;

bool ota_started = false;

// Traduit le code esp_reset_reason() rapporte par la sonde (paquet v2) en nom
// lisible. BROWNOUT/POWERON apres un trou = coupure d'alimentation ; DEEPSLEEP =
// reveil normal ; PANIC/WDT = plantage firmware.
static const char* resetReasonName(uint8_t r) {
    switch (r) {
        case 1:  return "POWERON";
        case 2:  return "EXT";
        case 3:  return "SW";
        case 4:  return "PANIC";
        case 5:  return "INT_WDT";
        case 6:  return "TASK_WDT";
        case 7:  return "WDT";
        case 8:  return "DEEPSLEEP";
        case 9:  return "BROWNOUT";
        case 10: return "SDIO";
        default: return "UNKNOWN";
    }
}

void setup() {
    Serial.begin(115200);

    // Monitoring des logs par UDP installé au plus tôt : capture dès le boot les
    // logs applicatifs ET ceux du cœur ESP (SD, capteurs, WiFi…). Les lignes sont
    // bufferisées jusqu'à la connexion WiFi, puis rejouées dans l'ordre.
    udpLogBegin();

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
    pinMode(BUTTON_BOOT_PIN, INPUT_PULLUP);
    if (digitalRead(BUTTON_BOOT_PIN) == LOW) {
        delay(100); // Debounce
        if (digitalRead(BUTTON_BOOT_PIN) == LOW) {
            display->clear();
            // Utilisation de la méthode center de base
            display->center(30, "MAINTENANCE");
            display->center(50, "Maintenir BOOT");
            display->center(70, "pour Formater");
            display->show();
            
            delay(3000);
            
            if (digitalRead(BUTTON_BOOT_PIN) == LOW) {
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

    // Détection optionnelle de morfAnalytics (écoute passive du beacon LAN).
    // No-op si MORF_ECOSYSTEM_ENABLED = 0 (banc de test, voir config.h).
    analytics.begin();

    // Alerte « accu à changer » : reprend le palier déjà notifié (NVS).
    batteryAlert.begin();

    // Etat de synchronisation fiable (v3) : charge l'accuse cumulatif + trous
    // persistes (reprise apres reboot hub, sans re-archiver ni redemander).
    meteoSync.begin();
    
    // Récepteur ESP-NOW : le callback est enregistré avant begin() pour que
    // les retries dans loop() n'oublient pas d'injecter les trames OUT.
    espNowReceiver.setOutdoorDataCallback([&](const OutdoorData& outdoor) {
        if (!outdoor.valid) return;

        // --- Synchronisation fiable (v3) -----------------------------------
        // 1) Le service decide : mesure nouvelle ou doublon, live ou historique,
        //    et l'HEURE DE MESURE reconstruite (jamais l'heure d'arrivee).
        const int64_t nowReal = (int64_t)time(NULL);
        const uint8_t nodeId = outdoor.node_id ? outdoor.node_id : 1;
        const mhsync::MeteoSyncService::Decision d = meteoSync.onPacket(
            nodeId, outdoor.sequence, outdoor.sensor_ts, outdoor.frame_type,
            outdoor.oldest_seq, nowReal);

        // 2) Voie inverse D'ABORD : la fenetre d'ecoute de la sonde est courte
        //    (~300 ms). On renvoie l'accuse cumulatif + le trou a combler AVANT
        //    l'archivage (une ecriture SD peut etre lente et ferait manquer la
        //    fenetre). Cet accuse elague le backlog de la sonde et recale son
        //    horloge ; la retransmission, elle, est PROACTIVE cote sonde.
        bool replySent = false;
        if (d.haveReply) {
            SyncControl reply = d.reply;
            replySent = espNowReceiver.sendControl(reply);
        }

        // 3) Archivage IDEMPOTENT : un doublon (retransmission deja connue) n'est
        //    pas ré-archivé. Une trame live met a jour l'affichage ; une
        //    retransmission n'archive que l'historique, a son heure d'origine.
        if (d.archive) {
            if (d.isLive) history.addOutdoorLive(outdoor, (time_t)d.measurementTs);
            else          history.addOutdoorHistorical(outdoor, (time_t)d.measurementTs);
        } else if (d.isLive) {
            // Doublon mais trame live : on rafraichit tout de meme la valeur
            // « live » (affichage/fraicheur) sans ré-archiver.
            history.refreshOutdoorLive(outdoor);
        }

        // 4) Surveillance de l'accu : mesures LIVE et NOUVELLES seulement. Une
        //    retransmission rejoue une tension passée ; un doublon live est la même
        //    mesure réémise, qui ne doit pas compter deux fois dans l'anti-rebond.
        //    L'envoi éventuel se fait plus tard, dans loop().
        if (d.isLive && d.archive && outdoor.has_battery) {
            batteryAlert.onLiveReading(outdoor.battery_voltage);
        }

        char buf[208];
        snprintf(buf, sizeof(buf),
                 "[OUT] %s seq=%u %s wake=%u reset=%s t=%.1f h=%.0f p=%.1f ack<=%u want=%u/%u reply=%d",
                 (outdoor.frame_type == FRAME_RETRANSMIT) ? "retx" : "live",
                 (unsigned)outdoor.sequence, d.archive ? "new" : "dup",
                 (unsigned)outdoor.wake_count, resetReasonName(outdoor.reset_reason),
                 outdoor.temperature, outdoor.humidity, outdoor.pressure,
                 (unsigned)d.reply.ack_seq, (unsigned)d.reply.want_from_seq,
                 (unsigned)d.reply.want_count, replySent ? 1 : 0);
        LOG_INFO(std::string(buf));
    });
    if (espNowReceiver.begin()) {
        LOG_INFO("ESP-NOW receiver initialized");
    } else {
        LOG_WARNING("ESP-NOW receiver init deferred (will retry)");
    }

    // Annonce de MeteoHub sur le LAN. La capacite « web_ui » est declaree : un
    // observateur peut alors proposer un lien vers l'interface sans rien
    // connaitre de MeteoHub. Le detail est servi par GET /status.
    // Les champs restent renseignés même si l'émission est coupée : GET /status
    // s'en sert pour décrire MeteoHub.
    presence.appName    = PROJECT_NAME;
    presence.version    = PROJECT_VERSION;
    presence.statusPort = 80;
    presence.webUi      = true;
#if MORF_ECOSYSTEM_ENABLED
    presence.begin();
#else
    LOG_WARNING("morfSystem neutralise (MORF_ECOSYSTEM_ENABLED=0) : pas de heartbeat morfBeacon");
#endif

    // Lancement des modules principaux
    forecast.begin();
    webManager.begin(history, sdCard, forecast, sensors, analytics);

    ui.begin(*display, wifi, sensors, forecast, history, sdCard);
}

// Redémarre proprement si la mémoire devient critique (voir config.h). Sépare
// l'auto-récupération de la boucle métier pour la garder lisible.
static void heapGuard() {
#if HEAP_GUARD_ENABLED
    static unsigned long lastCheck = 0;
    static unsigned long lowSince = 0;
    if (millis() - lastCheck < HEAP_CHECK_PERIOD_MS) {
        return;
    }
    lastCheck = millis();

    const size_t freeHeap = ESP.getFreeHeap();
    const size_t largest  = ESP.getMaxAllocHeap();  // plus gros bloc contigu allouable
    const bool critical   = (freeHeap < HEAP_MIN_FREE_BYTES) || (largest < HEAP_MIN_BLOCK_BYTES);

    if (!critical) {
        lowSince = 0;
        return;
    }
    if (lowSince == 0) {
        lowSince = millis();
        LOG_WARNING("Heap basse: libre=" + std::to_string(freeHeap) +
                    " bloc=" + std::to_string(largest) + " (surveillance)");
    }
    if (millis() - lowSince >= HEAP_LOW_GRACE_MS) {
        LOG_ERROR("Heap critique persistante: libre=" + std::to_string(freeHeap) +
                  " bloc=" + std::to_string(largest) + " -> redemarrage auto");
        delay(250);  // laisse la tache UDP emettre le dernier log
        ESP.restart();
    }
#endif
}

void loop() {
    static unsigned long lastHistoryUpdate = 0;
    static unsigned long lastLedUpdate = 0;
    static unsigned long lastForecastArchive = 0;

    heapGuard();

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
    analytics.update();
    espNowReceiver.update();
    // Notification « accu à changer » : vide si morfNotify n'est pas sur le réseau
    // (ou écosystème neutralisé), auquel cas rien ne part.
    batteryAlert.update(analytics.notifyUrl());
    // Persiste l'etat de synchro (accuse cumulatif + trous) modifie pendant le
    // traitement des trames, pour survivre a un reboot du hub sans re-archiver.
    meteoSync.persistDirty();
#if MORF_ECOSYSTEM_ENABLED
    presence.update();
#endif
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

    // Archivage « day-ahead » de la prévision (étape 9 : prévu vs observé). On fige
    // périodiquement la prévision du LENDEMAIN sous son jour cible ; en réécrivant au
    // fil de J-1, le fichier du jour J converge vers la dernière prévision J-1 pour J,
    // la référence à comparer aux mesures OUT observées. Cadence alignée sur celle de
    // la prévision (~30 min). On exige NTP (horodatage fiable) et une prévision
    // plausible (min < max) pour ne pas archiver une trame vide.
    if (millis() - lastForecastArchive >= (30UL * 60UL * 1000UL)) {
        struct tm tinfo;
        if (getLocalTime(&tinfo) && forecast.tomorrow.temp_max > forecast.tomorrow.temp_min) {
            const time_t nowt = time(NULL);
            const time_t tmr = nowt + 86400;
            struct tm tt;
            if (localtime_r(&tmr, &tt)) {
                ForecastRecord fr;
                fr.target_day = (tt.tm_year + 1900) * 10000u + (tt.tm_mon + 1) * 100u + tt.tm_mday;
                fr.issued_ts = static_cast<uint32_t>(nowt);
                fr.temp_min = forecast.tomorrow.temp_min;
                fr.temp_max = forecast.tomorrow.temp_max;
                fr.description = forecast.tomorrow.description;
                history.addForecast(fr);
                lastForecastArchive = millis(); // ne marque le succès qu'une fois archivé
            }
        }
    }

    // Cycle de mesure intérieure : acquisition + enregistrement à l'historique,
    // à la cadence configurée (INDOOR_MEASUREMENT_INTERVAL_SECONDS). C'est le SEUL
    // endroit qui acquiert ; l'UI, elle, lit la dernière mesure (sensors.last()).
    if (millis() - lastHistoryUpdate >= (INDOOR_MEASUREMENT_INTERVAL_SECONDS * 1000UL)) {
        lastHistoryUpdate = millis();
        SensorData data = sensors.read();
        if (data.valid) {
            // Filtrage des valeurs aberrantes (bruit capteur)
            bool valuesOk = true;
            if (data.temperature < -40.0f || data.temperature > 85.0f) valuesOk = false;
            if (data.humidity < 0.0f || data.humidity > 100.0f) valuesOk = false;
            if (data.pressure < 800.0f || data.pressure > 1200.0f) valuesOk = false;

            if (valuesOk) {
                // Chemin IN explicite : les capteurs locaux sont la source
                // intérieure (confort). addIndoor écrit dans l'historique legacy,
                // qui EST le flux IN (décision legacy = IN).
                IndoorData indoor;
                indoor.temperature = data.temperature;
                indoor.humidity = data.humidity;
                indoor.pressure = data.pressure;
                indoor.valid = true;
                history.addIndoor(indoor);
            } else {
                LOG_WARNING("Valeurs capteurs hors limites ignorees");
            }
        }
    }
}
