#include "managers/web_manager.h"
#include "managers/forecast_manager.h"
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <Update.h>
#include <cctype>
#include <string>
#include <cstring>
#include <Arduino.h>
#include "utils/logs.h"
#include "utils/system_info.h"
#include "utils/cooperative_yield.h"
#include "modules/neopixel_status.h"
#include "modules/analytics_beacon.h"
#include "project_config.h"
#include "config.h"
#include "web_pages.h"

#ifndef WEB_MDNS_HOSTNAME
#define WEB_MDNS_HOSTNAME "meteohub"
#endif

// --- UTILITAIRES EN C++ STANDARD (std::string) ---

static std::string toLowerCopy(const std::string& text) {
    std::string lower = text;
    for (auto& c : lower) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return lower;
}

static const char* getAlertLevelLabelFr(int severity) {
    if (severity >= 3) return "Rouge";
    if (severity == 2) return "Orange";
    if (severity == 1) return "Jaune";
    return "Aucune";
}

static bool isLikelyEnglishAlertText(const std::string& lower_text) {
    return lower_text.find("warning") != std::string::npos ||
        lower_text.find("watch") != std::string::npos ||
        lower_text.find("advisory") != std::string::npos ||
        lower_text.find("statement") != std::string::npos ||
        lower_text.find("storm") != std::string::npos;
}

// Logique métier en std::string
static std::string translateAlertToFrench(const std::string& event) {
    const std::string lower = toLowerCopy(event);

    if (lower.find("flood") != std::string::npos || lower.find("inond") != std::string::npos || lower.find("crue") != std::string::npos) {
        return "Risque d'inondation";
    }
    if (lower.find("thunder") != std::string::npos || lower.find("orage") != std::string::npos) {
        return "Orages";
    }
    if (lower.find("rain") != std::string::npos || lower.find("pluie") != std::string::npos) {
        return "Fortes pluies";
    }
    if (lower.find("wind") != std::string::npos || lower.find("vent") != std::string::npos) {
        return "Vent fort";
    }
    if (lower.find("snow") != std::string::npos || lower.find("neige") != std::string::npos) {
        return "Neige";
    }
    if (lower.find("heat") != std::string::npos || lower.find("chaleur") != std::string::npos) {
        return "Canicule";
    }
    if (lower.find("cold") != std::string::npos || lower.find("froid") != std::string::npos || lower.find("gel") != std::string::npos) {
        return "Grand froid";
    }

    if (isLikelyEnglishAlertText(lower)) {
        return "Alerte météo";
    }

    return event;
}

static std::string buildAlertDescriptionSummaryFr(const ForecastManager* forecast) {
    if (!forecast || !forecast->alert_active) {
        return "";
    }

    std::string level_label = getAlertLevelLabelFr(forecast->alert.severity);
    // CONVERSION EXPLICITE ICI : String (Arduino) -> std::string
    std::string event_cpp = std::string(forecast->alert.event.c_str());
    
    std::string event_fr = translateAlertToFrench(event_cpp);
    if (event_fr.empty()) {
        event_fr = "événement météo";
    }

    std::string summary = "Vigilance ";
    summary += level_label;
    summary += " : ";
    summary += event_fr;
    summary += ". Suivez les consignes officielles.";
    return summary;
}

static std::string getAlertDescriptionFr(const ForecastManager* forecast) {
    return buildAlertDescriptionSummaryFr(forecast);
}

// Dégage une tendance de fond en croisant la direction de la pression sur 1h, 12h, 24h
// (et 48h si disponible sur SD) : une direction cohérente sur toutes les fenêtres
// indique une vraie tendance durable, plutôt qu'une simple variation court terme.
static std::string computeGlobalTrendLabelFr(const MeteoTrend& trend) {
    const float pressure_1h = trend.pres.delta_1h;
    const float humidity_1h = trend.hum.delta_1h;
    const float temp_1h = trend.temp.delta_1h;

    // Signaux court terme explicites (variation rapide sur 1h)
    if (pressure_1h >= 1.0f && humidity_1h <= -2.0f) return "Vers beau temps";
    if (pressure_1h <= -1.0f && humidity_1h >= 2.0f) return "Vers pluie";
    if (pressure_1h <= -1.0f && temp_1h <= -0.3f) return "Vers refroidissement";
    if (pressure_1h >= 1.0f && temp_1h >= 0.3f) return "Vers amélioration";

    auto vote = [](const std::string& direction) {
        if (direction == "hausse") return 1;
        if (direction == "baisse") return -1;
        return 0;
    };

    int score = vote(trend.pres.direction_1h) + vote(trend.pres.direction_12h) + vote(trend.pres.direction_24h);
    int windows = 3;
    if (trend.available_48h) {
        score += vote(trend.pres.direction_48h);
        windows = 4;
    }

    if (score >= windows - 1) return "Amélioration durable";
    if (score <= -(windows - 1)) return "Dégradation durable";
    if (score == 0) return "Tendance stable";
    return "Tendance variable";
}

WebManager::WebManager() : _server(80) {
}

void WebManager::begin(HistoryManager& history, SdManager& sd, ForecastManager& forecast, SensorManager& sensors, AnalyticsBeacon& analytics) {
    LOG_INFO("Initialisation du WebManager...");

    _history = &history;
    _analytics = &analytics;
    _sd = &sd;
    _forecast = &forecast;
    _sensors = &sensors;
    _ota_upload_error = false;
    _ota_restart_at_ms = 0;

    if (MDNS.begin(WEB_MDNS_HOSTNAME)) {
        LOG_INFO("mDNS demarre : http://" WEB_MDNS_HOSTNAME ".local");
        MDNS.addService("http", "tcp", 80);
    } else {
        LOG_ERROR("Erreur demarrage mDNS");
    }

    _setupRoutes();
    _setupApi();

    _server.begin();
    LOG_INFO("Serveur Web demarre sur le port 80");
}

void WebManager::handle() {
    if (_ota_restart_at_ms != 0 && millis() >= _ota_restart_at_ms) {
        _ota_restart_at_ms = 0;
        LOG_WARNING("OTA reboot now");
        ESP.restart();
    }
}

void WebManager::_setupRoutes() {
    _server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", web_index_html);
    });
    _server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/css", web_style_css);
    });
    _server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "application/javascript", web_app_js);
    });
    _server.on("/stats.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", web_stats_html);
    });
    _server.on("/longterm.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", web_longterm_html);
    });
    _server.on("/files.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", web_files_html);
    });
    _server.on("/files.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "application/javascript", web_files_js);
    });
    _server.on("/footer.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "application/javascript", web_footer_js);
    });
    _server.on("/menu.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "application/javascript", web_menu_js);
    });
    _server.on("/logs.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", web_logs_html);
    });
    _server.on("/system.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", web_system_html);
    });
    // Redirection de l'ancienne URL OTA vers la nouvelle page Système.
    _server.on("/ota.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("/system.html");
    });
    
    _server.on("/files", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("/files.html");
    });
    _server.on("/sd", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("/files.html?fs=sd");
    });
    _server.on("/logs", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", web_logs_html);
    });
    
    _server.on("/api/logs", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("text/plain");
        int count = getLogCount();
        for (int i = 0; i < count; i++) {
            response->println(getLog(i).c_str());
        }
        request->send(response);
    });

    _server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Page non trouvee");
    });
}

void WebManager::_setupApi() {
    // Helper Lambda pour FS (utilise String pour l'API AsyncWebServer, c'est inévitable ici)
    auto getFs = [this](const String& fsName, fs::FS*& outFs) -> bool {
        if (fsName == "sd") {
            if (_sd && _sd->isAvailable()) {
                outFs = &SD;
                return true;
            }
            return false;
        }
        outFs = &LittleFS;
        return true;
    };

    // API Live
    _server.on("/api/live", HTTP_GET, [this](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        DynamicJsonDocument doc(768);

        SensorData live_data = {0.0f, 0.0f, 0.0f, false};
        if (_sensors) {
            live_data = _sensors->read();
        }

        doc["temp"] = live_data.temperature;
        doc["hum"] = live_data.humidity;
        doc["pres"] = live_data.pressure;
        doc["sensor_valid"] = live_data.valid;
        doc["wifi_rssi"] = WiFi.RSSI();
        doc["uptime"] = millis() / 1000;

        if (_forecast) {
            doc["alert_active"] = _forecast->alert_active;
            doc["alert_severity"] = _forecast->alert.severity;
            doc["alert_sender"] = _forecast->alert.sender.c_str();
            doc["alert_event"] = _forecast->alert.event.c_str();
            
            // CONVERSION EXPLICITE : String -> std::string pour le traitement
            std::string event_cpp(_forecast->alert.event.c_str());
            
            doc["alert_event_fr"] = translateAlertToFrench(event_cpp).c_str();
            doc["alert_description_fr"] = getAlertDescriptionFr(_forecast).c_str();
            doc["alert_level_label_fr"] = getAlertLevelLabelFr(_forecast->alert.severity);
            doc["alert_start_unix"] = _forecast->alert.start_unix;
            doc["alert_end_unix"] = _forecast->alert.end_unix;
        } else {
            doc["alert_active"] = false;
            doc["alert_severity"] = 0;
        }

        serializeJson(doc, *response);
        request->send(response);
    });

    // API Alert
    _server.on("/api/alert", HTTP_GET, [this](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        DynamicJsonDocument doc(1024);

        if (_forecast) {
            doc["active"] = _forecast->alert_active;
            doc["severity"] = _forecast->alert.severity;
            doc["sender"] = _forecast->alert.sender.c_str();
            doc["event"] = _forecast->alert.event.c_str();
            
            // CONVERSION EXPLICITE
            std::string event_cpp(_forecast->alert.event.c_str());
            doc["event_fr"] = translateAlertToFrench(event_cpp).c_str();
            doc["description_fr"] = getAlertDescriptionFr(_forecast).c_str();
            doc["alert_level_label_fr"] = getAlertLevelLabelFr(_forecast->alert.severity);
            doc["start_unix"] = _forecast->alert.start_unix;
            doc["end_unix"] = _forecast->alert.end_unix;
        } else {
            doc["active"] = false;
            doc["severity"] = 0;
        }

        serializeJson(doc, *response);
        request->send(response);
    });


    // API History Summary : synthèse pré-calculée d'une plage [from, to] à partir
    // des fichiers .stats journaliers (min/max/moyenne + variation), sans relire
    // les mesures. Sert à afficher la synthèse de la page Historique quasi
    // instantanément. Renvoie {"valid":false} si aucune donnée .stats disponible.
    _server.on("/api/history/summary", HTTP_GET, [this](AsyncWebServerRequest *request) {
        long from_s = request->hasParam("from") ? request->getParam("from")->value().toInt() : 0;
        long to_s = request->hasParam("to") ? request->getParam("to")->value().toInt() : 0;

        AsyncResponseStream *response = request->beginResponseStream("application/json");
        if (from_s <= 0 || to_s <= from_s) {
            response->print("{\"valid\":false}");
            request->send(response);
            return;
        }

        RangeSynthesis r = _history->querySynthesis(static_cast<time_t>(from_s), static_cast<time_t>(to_s));
        if (!r.valid) {
            response->print("{\"valid\":false}");
            request->send(response);
            return;
        }

        char buffer[512];
        snprintf(buffer, sizeof(buffer),
            "{\"valid\":true,\"count\":%u,"
            "\"temp\":{\"min\":%.1f,\"max\":%.1f,\"avg\":%.1f,\"first\":%.1f,\"last\":%.1f,\"delta\":%.1f},"
            "\"hum\":{\"min\":%.0f,\"max\":%.0f,\"avg\":%.0f,\"first\":%.0f,\"last\":%.0f,\"delta\":%.0f},"
            "\"pres\":{\"min\":%.1f,\"max\":%.1f,\"avg\":%.1f,\"first\":%.1f,\"last\":%.1f,\"delta\":%.1f}}",
            r.count,
            r.t_min, r.t_max, r.t_avg, r.t_first, r.t_last, r.t_last - r.t_first,
            r.h_min, r.h_max, r.h_avg, r.h_first, r.h_last, r.h_last - r.h_first,
            r.p_min, r.p_max, r.p_avg, r.p_first, r.p_last, r.p_last - r.p_first);
        response->print(buffer);
        request->send(response);
    });

    // API Stats
    _server.on("/api/stats", HTTP_GET, [this](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        DynamicJsonDocument doc(4096);

        Stats24h stats = _history->getRecentStats();
        if (stats.count > 0) {
            doc["temp"]["min"] = stats.temp.min;
            doc["temp"]["max"] = stats.temp.max;
            doc["temp"]["avg"] = stats.temp.avg();
            doc["hum"]["min"] = stats.hum.min;
            doc["hum"]["max"] = stats.hum.max;
            doc["hum"]["avg"] = stats.hum.avg();
            doc["pres"]["min"] = stats.pres.min;
            doc["pres"]["max"] = stats.pres.max;
            doc["pres"]["avg"] = stats.pres.avg();
        } else {
            doc["temp"]["min"] = 0; doc["temp"]["max"] = 0; doc["temp"]["avg"] = 0;
            doc["hum"]["min"] = 0; doc["hum"]["max"] = 0; doc["hum"]["avg"] = 0;
            doc["pres"]["min"] = 0; doc["pres"]["max"] = 0; doc["pres"]["avg"] = 0;
        }
        doc["count"] = stats.count;

        MeteoTrend trend = _history->getTrend();
        doc["trend"]["global_label_fr"] = computeGlobalTrendLabelFr(trend).c_str();
        doc["trend"]["available_48h"] = trend.available_48h;
        doc["trend"]["temp"]["delta_1h"] = trend.temp.delta_1h;
        doc["trend"]["temp"]["delta_12h"] = trend.temp.delta_12h;
        doc["trend"]["temp"]["delta_24h"] = trend.temp.delta_24h;
        doc["trend"]["temp"]["delta_48h"] = trend.temp.delta_48h;
        doc["trend"]["temp"]["direction_1h"] = trend.temp.direction_1h.c_str();
        doc["trend"]["temp"]["direction_12h"] = trend.temp.direction_12h.c_str();
        doc["trend"]["temp"]["direction_24h"] = trend.temp.direction_24h.c_str();
        doc["trend"]["temp"]["direction_48h"] = trend.temp.direction_48h.c_str();
        doc["trend"]["hum"]["delta_1h"] = trend.hum.delta_1h;
        doc["trend"]["hum"]["delta_12h"] = trend.hum.delta_12h;
        doc["trend"]["hum"]["delta_24h"] = trend.hum.delta_24h;
        doc["trend"]["hum"]["delta_48h"] = trend.hum.delta_48h;
        doc["trend"]["hum"]["direction_1h"] = trend.hum.direction_1h.c_str();
        doc["trend"]["hum"]["direction_12h"] = trend.hum.direction_12h.c_str();
        doc["trend"]["hum"]["direction_24h"] = trend.hum.direction_24h.c_str();
        doc["trend"]["hum"]["direction_48h"] = trend.hum.direction_48h.c_str();
        doc["trend"]["pres"]["delta_1h"] = trend.pres.delta_1h;
        doc["trend"]["pres"]["delta_12h"] = trend.pres.delta_12h;
        doc["trend"]["pres"]["delta_24h"] = trend.pres.delta_24h;
        doc["trend"]["pres"]["delta_48h"] = trend.pres.delta_48h;
        doc["trend"]["pres"]["direction_1h"] = trend.pres.direction_1h.c_str();
        doc["trend"]["pres"]["direction_12h"] = trend.pres.direction_12h.c_str();
        doc["trend"]["pres"]["direction_24h"] = trend.pres.direction_24h.c_str();
        doc["trend"]["pres"]["direction_48h"] = trend.pres.direction_48h.c_str();

        serializeJson(doc, *response);
        request->send(response);
    });

    // API System
    _server.on("/api/system", HTTP_GET, [this](AsyncWebServerRequest *request) {
        std::string sysInfo = getSystemInfoJson(_sd);
        request->send(200, "application/json", sysInfo.c_str());
    });

    // API Analytics : présence du service morfAnalytics (détection LAN optionnelle).
    // Toujours disponible ; renvoie simplement l'état constaté. MeteoHub n'en dépend pas.
    _server.on("/api/analytics", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!_analytics) {
            request->send(200, "application/json", "{\"available\":false}");
            return;
        }
        // `detected` decrit ce que la decouverte automatique a trouve ;
        // `effective_url` est l'adresse a ouvrir, adresse manuelle comprise.
        // Le nom annonce n'est qu'un LIBELLE : la reconnaissance se fait sur la
        // capacite, jamais sur ce nom (l'utilisateur peut renommer son service).
        char buf[512];
        snprintf(buf, sizeof(buf),
            "{\"available\":%s,\"mode\":\"%s\",\"effective_url\":\"%s\","
            "\"capability\":\"%s\",\"manual_url\":\"%s\","
            "\"detected\":{\"found\":%s,\"name\":\"%s\",\"version\":\"%s\","
            "\"host\":\"%s\",\"status_port\":%d,\"last_seen_s\":%ld}}",
            _analytics->isAvailable() ? "true" : "false",
            _analytics->isManual() ? "manual" : "auto",
            _analytics->effectiveUrl().c_str(),
            ANALYTICS_CAPABILITY,
            _analytics->manualUrl().c_str(),
            _analytics->isDetected() ? "true" : "false",
            _analytics->name().c_str(), _analytics->version().c_str(),
            _analytics->host().c_str(), _analytics->statusPort(),
            _analytics->lastSeenAgoSec());
        request->send(200, "application/json", buf);
    });

    // Enregistre (ou efface) l'adresse manuelle du service d'analyse. Persistee
    // en NVS : elle survit au redemarrage, contrairement a la decouverte.
    _server.on("/api/analytics/config", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (!_analytics) {
            request->send(503, "application/json", "{\"ok\":false}");
            return;
        }
        String url = request->hasParam("manual_url", true)
            ? request->getParam("manual_url", true)->value() : String("");
        url.trim();

        // Une adresse vide est valide : elle signifie « revenir en automatique ».
        if (url.length() > 0) {
            if (!url.startsWith("http://") && !url.startsWith("https://")) {
                request->send(400, "application/json",
                    "{\"ok\":false,\"error\":\"l'adresse doit commencer par http:// ou https://\"}");
                return;
            }
            if (url.length() > 120) {
                request->send(400, "application/json",
                    "{\"ok\":false,\"error\":\"adresse trop longue\"}");
                return;
            }
        }
        _analytics->setManualUrl(url.c_str());
        char buf[200];
        snprintf(buf, sizeof(buf), "{\"ok\":true,\"mode\":\"%s\",\"effective_url\":\"%s\"}",
                 _analytics->isManual() ? "manual" : "auto",
                 _analytics->effectiveUrl().c_str());
        request->send(200, "application/json", buf);
    });

    // API LED : lecture / réglage de la luminosité de la NeoLED (0-255, persistée).
    _server.on("/api/led", HTTP_GET, [](AsyncWebServerRequest *request) {
        char buf[48];
        snprintf(buf, sizeof(buf), "{\"brightness\":%u}", neoGetBrightness());
        request->send(200, "application/json", buf);
    });
    _server.on("/api/led", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("brightness", true) && !request->hasParam("brightness")) {
            request->send(400, "application/json", "{\"ok\":false,\"message\":\"parametre brightness manquant\"}");
            return;
        }
        long v = request->hasParam("brightness", true)
                     ? request->getParam("brightness", true)->value().toInt()
                     : request->getParam("brightness")->value().toInt();
        if (v < 0) v = 0;
        if (v > 255) v = 255;
        neoSetBrightness(static_cast<uint8_t>(v));
        char buf[48];
        snprintf(buf, sizeof(buf), "{\"ok\":true,\"brightness\":%u}", neoGetBrightness());
        request->send(200, "application/json", buf);
    });

    // API Config Export : configuration effective au format JSON (téléchargement).
    _server.on("/api/config/export", HTTP_GET, [](AsyncWebServerRequest *request) {
        char buf[768];
        snprintf(buf, sizeof(buf),
            "{\n"
            "  \"project\": {\"name\": \"%s\", \"version\": \"%s\", \"build_date\": \"%s\", \"build_time\": \"%s\", \"git_commit\": \"%s\"},\n"
            "  \"network\": {\"mdns_host\": \"%s\"},\n"
            "  \"graph\": {\"scale_mode\": %d, \"scale_margin_pct\": %d, \"temp_min\": %.1f, \"temp_max\": %.1f, \"hum_min\": %.1f, \"hum_max\": %.1f, \"pres_min\": %.1f, \"pres_max\": %.1f},\n"
            "  \"led\": {\"brightness\": %u},\n"
            "  \"sampling_interval_s\": 60\n"
            "}\n",
            PROJECT_NAME, PROJECT_VERSION, BUILD_DATE, BUILD_TIME, GIT_COMMIT,
            WEB_MDNS_HOSTNAME,
            GRAPH_SCALE_MODE, GRAPH_SCALE_MARGIN_PCT,
            (double)GRAPH_TEMP_MIN, (double)GRAPH_TEMP_MAX,
            (double)GRAPH_HUM_MIN, (double)GRAPH_HUM_MAX,
            (double)GRAPH_PRES_MIN, (double)GRAPH_PRES_MAX,
            neoGetBrightness());
        AsyncWebServerResponse *response = request->beginResponse(200, "application/json", buf);
        response->addHeader("Content-Disposition", "attachment; filename=\"meteohub-config.json\"");
        request->send(response);
    });

    // API History Export CSV : export brut des mesures de la plage au format CSV
    // (téléchargement, pour Excel/LibreOffice). ?from=&to= optionnels.
    _server.on("/api/history/export.csv", HTTP_GET, [this](AsyncWebServerRequest *request) {
        long to_s = request->hasParam("to") ? request->getParam("to")->value().toInt() : (long)time(NULL);
        long from_s = request->hasParam("from") ? request->getParam("from")->value().toInt() : 1577836800L; // 2020-01-01
        if (to_s <= 0) to_s = (long)time(NULL);
        if (from_s < 0) from_s = 0;

        AsyncResponseStream *response = request->beginResponseStream("text/csv");
        response->addHeader("Content-Disposition", "attachment; filename=\"meteohub-history.csv\"");
        _history->exportCsv(static_cast<time_t>(from_s), static_cast<time_t>(to_s),
            [response](const char* line) { response->print(line); });
        request->send(response);
    });

    // --- Collecte incrémentale (morfAnalytics) -------------------------------
    // Ces deux routes servent UNIQUEMENT à un collecteur externe qui recopie
    // l'historique. Elles exposent les mesures brutes indexées par leur position
    // dans le fichier du jour, et non par horodatage : les fichiers étant écrits
    // en ajout seul, (jour, index) ne recule jamais et ne se répète jamais, même
    // lors d'un changement d'heure ou d'un recalage NTP. Un collecteur mémorise
    // ce couple et reprend exactement où il s'était arrêté.
    // MeteoHub reste la source de vérité : ces routes sont en lecture seule.

    _server.on("/api/history/days", HTTP_GET, [this](AsyncWebServerRequest *request) {
        std::vector<DayIndexEntry> days = _history->listDays();

        AsyncResponseStream *response = request->beginResponseStream("application/json");
        response->print("{\"days\":[");
        bool first = true;
        for (const auto& d : days) {
            if (!first) response->print(",");
            first = false;
            char buf[128];
            snprintf(buf, sizeof(buf),
                "{\"day\":%lu,\"nrec\":%lu,\"first_ts\":%lu,\"last_ts\":%lu}",
                (unsigned long)d.day_key, (unsigned long)d.nrec,
                (unsigned long)d.first_ts, (unsigned long)d.last_ts);
            response->print(buf);
        }
        response->print("]}");
        request->send(response);
    });

    _server.on("/api/history/raw", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!request->hasParam("day")) {
            request->send(400, "application/json", "{\"error\":\"missing day\"}");
            return;
        }
        const uint32_t day_key = (uint32_t)request->getParam("day")->value().toInt();
        const uint32_t index = request->hasParam("index")
            ? (uint32_t)request->getParam("index")->value().toInt() : 0;
        uint32_t limit = request->hasParam("limit")
            ? (uint32_t)request->getParam("limit")->value().toInt() : 250;
        // Borne haute STRICTE. Mesuré sur l'appareil : au-delà d'environ 300
        // enregistrements (~9 Ko), le flux de réponse asynchrone abandonne et la
        // requête se termine sans rien renvoyer. Le collecteur pagine donc, ce
        // qui coûte quelques requêtes de plus mais ne dépend pas d'une taille de
        // réponse que l'appareil ne sait pas tenir.
        if (limit == 0 || limit > 250) limit = 250;

        AsyncResponseStream *response = request->beginResponseStream("application/json");
        // L'en-tête est émis avant la lecture : `total` est écrit après coup, donc
        // on ouvre le tableau d'abord et on referme l'objet avec les compteurs.
        response->print("{\"day\":");
        response->print(day_key);
        response->print(",\"index\":");
        response->print(index);
        response->print(",\"data\":[");

        uint32_t count = 0;
        bool first = true;
        // Format compact [ts,t,h,p] : ~30 octets par mesure au lieu de ~55 en
        // objet nommé, soit un import complet nettement plus léger pour l'ESP32.
        const uint32_t total = _history->exportRaw(day_key, index, limit,
            [&](const RawRecord& r) {
                if (!first) response->print(",");
                first = false;
                char buf[64];
                snprintf(buf, sizeof(buf), "[%lu,%.1f,%.0f,%.1f]",
                         (unsigned long)r.ts, r.t, r.h, r.p);
                response->print(buf);
                // Rend la main régulièrement, comme le fait /api/history : sans
                // cela, la boucle monopolise la tâche réseau et le flux de
                // réponse ne s'écoule jamais.
                COOPERATIVE_YIELD_EVERY(count, 32);
                count++;
            });

        char tail[64];
        snprintf(tail, sizeof(tail), "],\"count\":%lu,\"total\":%lu}",
                 (unsigned long)count, (unsigned long)total);
        response->print(tail);
        request->send(response);
    });

    // ATTENTION A L'ORDRE : ESPAsyncWebServer fait correspondre une route a
    // toute URL qui COMMENCE par elle. Enregistree avant ses sous-routes,
    // "/api/history" captait "/api/history/summary", "/api/history/export.csv",
    // "/api/history/days" et "/api/history/raw" : la synthese et l'export CSV
    // renvoyaient silencieusement le JSON de l'historique, et les deux routes
    // de collecte ne repondaient jamais. Elle est donc declaree EN DERNIER.
    // API History
    _server.on("/api/history", HTTP_GET, [this](AsyncWebServerRequest *request) {
        // Mode plage absolue : ?from=<unix>&to=<unix>[&interval=<s>]
        // Utilisé par la page Historique pour la sélection et la comparaison de
        // périodes (données lues sur la carte SD + historique RAM récent).
        if (request->hasParam("from") && request->hasParam("to")) {
            long from_s = request->getParam("from")->value().toInt();
            long to_s = request->getParam("to")->value().toInt();
            if (from_s > 0 && to_s > from_s) {
                long interval_s = request->hasParam("interval") ? request->getParam("interval")->value().toInt() : 0;
                if (interval_s <= 0) {
                    interval_s = (to_s - from_s) / 300; // ~300 points par défaut
                    if (interval_s < 60) interval_s = 60;
                }

                std::vector<HistoryPoint> points = _history->queryRange(
                    static_cast<time_t>(from_s), static_cast<time_t>(to_s), interval_s);

                AsyncResponseStream *response = request->beginResponseStream("application/json");
                response->print("{\"data\":[");
                bool first = true;
                size_t point_index = 0;
                for (const auto& pt : points) {
                    COOPERATIVE_YIELD_EVERY(point_index, 64);
                    point_index++;
                    if (!first) response->print(",");
                    first = false;

                    // Chaque grandeur est émise séparément : une valeur aberrante
                    // écartée pour une grandeur donne null, sans invalider les autres.
                    char buffer[160];
                    char tb[16], hb[16], pb[16];
                    if (pt.tvalid) snprintf(tb, sizeof(tb), "%.1f", pt.temp); else strcpy(tb, "null");
                    if (pt.hvalid) snprintf(hb, sizeof(hb), "%.0f", pt.hum);  else strcpy(hb, "null");
                    if (pt.pvalid) snprintf(pb, sizeof(pb), "%.1f", pt.pres); else strcpy(pb, "null");
                    snprintf(buffer, sizeof(buffer),
                        "{\"t\":%ld,\"temp\":%s,\"hum\":%s,\"pres\":%s}",
                        static_cast<long>(pt.t), tb, hb, pb);
                    response->print(buffer);
                }
                response->print("]}");
                request->send(response);
                return;
            }
        }

        // Mode fenêtre glissante : ?window=<s>&interval=<s>&points=<n> (rétrocompatible)
        const auto& full_history = _history->getRecentHistory();
        int window_s = request->hasParam("window") ? request->getParam("window")->value().toInt() : 0;
        int interval_s = request->hasParam("interval") ? request->getParam("interval")->value().toInt() : 0;
        int max_points = request->hasParam("points") ? request->getParam("points")->value().toInt() : 0;

        size_t start_index = 0;
        if (!full_history.empty() && window_s > 0) {
            const long latest_ts = static_cast<long>(full_history.back().timestamp);
            const long min_ts = latest_ts - static_cast<long>(window_s);
            size_t i = full_history.size();
            while (i > 0 && static_cast<long>(full_history[i - 1].timestamp) >= min_ts) {
                i--;
            }
            start_index = i;
        }

        AsyncResponseStream *response = request->beginResponseStream("application/json");
        response->print("{\"data\":[");

        bool first = true;
        const size_t available_points = full_history.size() - start_index;

        if (interval_s > 0 && available_points > 0) {
            const long latest_ts = static_cast<long>(full_history.back().timestamp);
            const long window_start_ts = (window_s > 0) ? (latest_ts - static_cast<long>(window_s)) : static_cast<long>(full_history[start_index].timestamp);

            size_t idx = start_index;
            for (long bucket_start = window_start_ts; bucket_start <= latest_ts; bucket_start += interval_s) {
                const long bucket_end = bucket_start + interval_s;
                double sum_t = 0.0, sum_h = 0.0, sum_p = 0.0;
                int count = 0;

                while (idx < full_history.size()) {
                    const long ts = static_cast<long>(full_history[idx].timestamp);
                    if (ts < bucket_start) { idx++; continue; }
                    if (ts >= bucket_end) break;

                    sum_t += full_history[idx].t;
                    sum_h += full_history[idx].h;
                    sum_p += full_history[idx].p;
                    count++;
                    idx++;
                    COOPERATIVE_YIELD_EVERY(idx, 64);
                }

                if (count == 0) continue;

                if (!first) response->print(",");
                first = false;

                char buffer[128];
                snprintf(buffer, sizeof(buffer), "{\"t\":%ld,\"temp\":%.1f,\"hum\":%.0f,\"pres\":%.1f}",
                    bucket_end, sum_t / count, sum_h / count, sum_p / count);
                response->print(buffer);
            }
        } else {
            size_t step = 1;
            if (max_points > 0 && available_points > static_cast<size_t>(max_points)) {
                step = (available_points + static_cast<size_t>(max_points) - 1) / static_cast<size_t>(max_points);
            }

            for (size_t i = start_index; i < full_history.size(); i += step) {
                COOPERATIVE_YIELD_EVERY(i - start_index, 64);
                if (!first) response->print(",");
                first = false;

                const auto& record = full_history[i];
                char buffer[128];
                snprintf(buffer, sizeof(buffer), "{\"t\":%ld,\"temp\":%.1f,\"hum\":%.0f,\"pres\":%.1f}",
                    static_cast<long>(record.timestamp), record.t, record.h, record.p);
                response->print(buffer);
            }
        }

        response->print("]}");
        request->send(response);
    });

    // API Files List
    _server.on("/api/files/list", HTTP_GET, [this, getFs](AsyncWebServerRequest *request) {
        String fsName = request->hasParam("fs") ? request->getParam("fs")->value() : "littlefs";
        String path = request->hasParam("path") ? request->getParam("path")->value() : "/";
        if (!path.startsWith("/")) path = "/" + path;

        fs::FS* pFs = nullptr;
        if (!getFs(fsName, pFs)) {
            request->send(503, "text/plain", "SD Card not available");
            return;
        }

        File root = pFs->open(path);
        if (!root || !root.isDirectory()) {
            root = pFs->open("/"); 
        }

        AsyncResponseStream *response = request->beginResponseStream("application/json");
        response->print("[");
        
        File file = root.openNextFile();
        bool first = true;
        size_t file_count = 0;
        
        while(file) {
            COOPERATIVE_YIELD_EVERY(file_count, 32);
            if(!first) response->print(",");
            response->print("{\"name\":\"");
            
            const char* fname = file.name();
            response->print(fname ? fname : "");
            
            response->print("\",\"size\":");
            response->print(file.size());
            response->print(",\"isDir\":");
            response->print(file.isDirectory() ? "true" : "false");
            response->print("}");
            
            first = false;
            file = root.openNextFile();
            file_count++;
        }
        response->print("]");
        request->send(response);
    });

    // API Download
    _server.on("/api/files/download", HTTP_GET, [this, getFs](AsyncWebServerRequest *request) {
        String fsName = request->hasParam("fs") ? request->getParam("fs")->value() : "littlefs";
        fs::FS* pFs = nullptr;
        if (!getFs(fsName, pFs)) {
            request->send(503, "text/plain", "SD Card not available");
            return;
        }

        if(request->hasParam("path")) {
            String path = request->getParam("path")->value();
            if(!path.startsWith("/")) path = "/" + path;
            
            if (path.indexOf("..") != -1) {
                request->send(403, "text/plain", "Invalid path");
                return;
            }

            if(pFs->exists(path)) {
                request->send(*pFs, path, "application/octet-stream", true);
            } else {
                request->send(404, "text/plain", "Fichier non trouve");
            }
        } else {
            request->send(400, "text/plain", "Parametre path manquant");
        }
    });

    // API Delete
    _server.on("/api/files/delete", HTTP_DELETE, [this, getFs](AsyncWebServerRequest *request) {
        String fsName = request->hasParam("fs") ? request->getParam("fs")->value() : "littlefs";
        fs::FS* pFs = nullptr;
        if (!getFs(fsName, pFs)) {
            request->send(503, "text/plain", "SD Card not available");
            return;
        }

        if(request->hasParam("path")) {
            String path = request->getParam("path")->value();
            if(!path.startsWith("/")) path = "/" + path;

            if (path == "/" || path.indexOf("..") != -1) {
                request->send(403, "text/plain", "Forbidden");
                return;
            }

            if(pFs->exists(path)) {
                if (pFs->remove(path)) {
                    request->send(200, "text/plain", "OK");
                } else {
                    request->send(500, "text/plain", "Echec suppression (dossier non vide ?)");
                }
            } else {
                request->send(404, "text/plain", "Fichier non trouve");
            }
        } else {
            request->send(400, "text/plain", "Parametre path manquant");
        }
    });

    // API OTA Update
    _server.on("/api/ota/update", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (_ota_upload_error || Update.hasError()) {
            _ota_upload_error = false;
            request->send(500, "application/json", "{\"ok\":false,\"message\":\"OTA update failed\"}");
            LOG_ERROR("OTA update failed");
            return;
        }

        request->send(200, "application/json", "{\"ok\":true,\"message\":\"OTA update uploaded. Rebooting...\"}");
        _ota_restart_at_ms = millis() + 2500;
        LOG_INFO("OTA update successful. Reboot scheduled.");
    }, [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
        (void)request;
        // On convertit filename (String) en std::string pour les logs si nécessaire, 
        // mais LOG_INFO gère souvent les deux. Pour être pur C++, on convertit.
        std::string fname_cpp(filename.c_str());

        if (index == 0) {
            _ota_upload_error = false;
            LOG_INFO("OTA upload start: " + fname_cpp);
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                _ota_upload_error = true;
                Update.printError(Serial);
                LOG_ERROR("OTA begin failed");
                return;
            }
        }

        if (!_ota_upload_error) {
            if (Update.write(data, len) != len) {
                _ota_upload_error = true;
                Update.printError(Serial);
                LOG_ERROR("OTA write failed");
                return;
            }
        }

        if (final) {
            if (!_ota_upload_error) {
                if (!Update.end(true)) {
                    _ota_upload_error = true;
                    Update.printError(Serial);
                    LOG_ERROR("OTA end failed");
                } else {
                    std::string logMsg = "OTA upload end (bytes=" + std::to_string(index + len) + ")";
                    LOG_INFO(logMsg);
                }
            }
        }
    });

    // API Files Upload
    _server.on("/api/files/upload", HTTP_POST, [](AsyncWebServerRequest *request){
        if (request->hasParam("error", true)) {
             request->send(500, "text/plain", "Upload failed");
        } else {
             request->send(200, "text/plain", "OK");
        }
    }, [this, getFs](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
        static File fsUploadFile;
        static fs::FS* pFsUpload = nullptr;

        if(!index){
            String fsName = request->hasParam("fs") ? request->getParam("fs")->value() : "littlefs";
            
            pFsUpload = nullptr;
            if (!getFs(fsName, pFsUpload)) {
                return; 
            }

            if(!filename.startsWith("/")) filename = "/" + filename;
            if (filename.indexOf("..") != -1) return;

            fsUploadFile = pFsUpload->open(filename, FILE_WRITE);
            if (!fsUploadFile) {
                std::string fname_cpp(filename.c_str());
                LOG_ERROR("Failed to open file for upload: " + fname_cpp);
            }
        }
        
        if(fsUploadFile && len > 0){
            size_t written = fsUploadFile.write(data, len);
            if (written != len) {
                LOG_ERROR("Disk full or write error");
                fsUploadFile.close();
                fsUploadFile = File();
            }
        }
        
        if(final){
            if(fsUploadFile) {
                fsUploadFile.close();
                fsUploadFile = File();
            }
        }
    });
}