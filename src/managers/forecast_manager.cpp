#include <cctype>
#include <string>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "forecast_manager.h"
#include "secrets.h"
#include "../utils/logs.h"

// Mettre à jour toutes les 30 minutes
const unsigned long UPDATE_INTERVAL = 30 * 60 * 1000;

static std::string toLowerCopy(const std::string& text) {
    std::string lower = text;
    for (auto& character : lower) {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }
    return lower;
}

static int detectSeverityFromText(const std::string& text) {
    std::string lower = toLowerCopy(text);

    if (lower.find("rouge") != std::string::npos ||
        lower.find("red") != std::string::npos ||
        lower.find("extreme") != std::string::npos) {
        return 3;
    }

    if (lower.find("orange") != std::string::npos ||
        lower.find("severe") != std::string::npos ||
        lower.find("high") != std::string::npos) {
        return 2;
    }

    if (lower.find("jaune") != std::string::npos ||
        lower.find("yellow") != std::string::npos ||
        lower.find("moderate") != std::string::npos ||
        lower.find("minor") != std::string::npos) {
        return 1;
    }

    return 1;
}

static int detectHazardPriority(const std::string& text) {
    std::string lower = toLowerCopy(text);

    if (lower.find("crue") != std::string::npos ||
        lower.find("flood") != std::string::npos ||
        lower.find("inond") != std::string::npos ||
        lower.find("river") != std::string::npos) {
        return 3;
    }

    if (lower.find("rain") != std::string::npos ||
        lower.find("pluie") != std::string::npos) {
        return 2;
    }

    return 1;
}

void ForecastManager::begin() {
    lastUpdate = -UPDATE_INTERVAL; // Force an update on first run
}

void ForecastManager::update() {
    unsigned long now = millis();
    if (now - lastUpdate < UPDATE_INTERVAL) {
        return;
    }
    lastUpdate = now;

    LOG_DEBUG("Fetching forecast...");

    HTTPClient http;
    std::string url = "https://api.openweathermap.org/data/3.0/onecall?lat=";
    url += OWM_LAT;
    url += "&lon=";
    url += OWM_LON;
    url += "&appid=";
    url += OWM_API_KEY;
    url += "&units=";
    url += OWM_UNITS;
    url += "&lang=";
    url += OWM_LANG;
    url += "&exclude=minutely,hourly,current";

    http.begin(url.c_str());
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        std::string payload = http.getString().c_str();
        parseResponse(payload);
        LOG_INFO("Forecast OK");
    } else {
        LOG_WARNING(std::string("Forecast fail: ") + std::to_string(httpCode));
    }

    http.end();
}

void ForecastManager::parseResponse(const std::string& payload) {
    DynamicJsonDocument doc(8192); // Allocate enough memory for the response
    DeserializationError error = deserializeJson(doc, payload.c_str());

    if (error) {
        LOG_ERROR("JSON parse err");
        return;
    }

    // Today's Forecast (index 0 of daily array)
    JsonObject daily_0 = doc["daily"][0];
    today.temp_min = daily_0["temp"]["min"];
    today.temp_max = daily_0["temp"]["max"];
    today.description = daily_0["weather"][0]["description"].as<const char*>();
    if (!today.description.empty()) {
        today.description[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(today.description[0])));
    }

    // Tomorrow's Forecast (index 1 of daily array)
    JsonObject daily_1 = doc["daily"][1];
    tomorrow.temp_min = daily_1["temp"]["min"];
    tomorrow.temp_max = daily_1["temp"]["max"];
    tomorrow.description = daily_1["weather"][0]["description"].as<const char*>();
    if (!tomorrow.description.empty()) {
        tomorrow.description[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(tomorrow.description[0])));
    }

    // Alerts
    JsonArray alerts = doc["alerts"].as<JsonArray>();
    if (!alerts.isNull() && alerts.size() > 0) {
        alert_active = true;

        int bestScore = -1;
        std::string bestSender;
        std::string bestEvent;
        std::string bestDescription;
        int bestSeverity = 1;
        long bestStartUnix = 0;
        long bestEndUnix = 0;

        int index = 0;
        for (JsonObject item : alerts) {
            std::string sender = item["sender_name"] | "";
            std::string event = item["event"] | "";
            std::string description = item["description"] | "";
            long start_unix = item["start"] | 0;
            long end_unix = item["end"] | 0;

            std::string combined = event + " " + description;
            int severity = detectSeverityFromText(combined);
            int hazardPriority = detectHazardPriority(combined);

            int score = (severity * 100) + (hazardPriority * 10) - index;
            if (score > bestScore) {
                bestScore = score;
                bestSender = sender;
                bestEvent = event;
                bestDescription = description;
                bestSeverity = severity;
                bestStartUnix = start_unix;
                bestEndUnix = end_unix;
            }
            index++;
        }

        alert.sender = bestSender;
        alert.event = bestEvent;
        alert.description = bestDescription;
        alert.severity = bestSeverity;
        alert.start_unix = bestStartUnix;
        alert.end_unix = bestEndUnix;
    } else {
        alert_active = false;
        alert.severity = 0;
        alert.start_unix = 0;
        alert.end_unix = 0;
    }
}