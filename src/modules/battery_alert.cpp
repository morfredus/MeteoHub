#include "battery_alert.h"
#include "config.h"
#include "../utils/logs.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// Clé NVS du palier déjà notifié (namespace "meteohub", partagé avec les autres
// réglages persistés ; 15 caractères max).
#define BATTERY_ALERT_NVS_KEY "bat_notified"

// Fenêtre après une trame live pendant laquelle l'envoi est permis : la sonde
// dort, la boucle peut se permettre un POST bloquant de quelques secondes.
static const unsigned long kSendWindowMs = 60000UL;

BatteryAlert batteryAlert;

BatteryAlert::BatteryAlert()
    : _logic({BATTERY_ALERT_WARN_V, BATTERY_ALERT_CRIT_V,
              BATTERY_ALERT_REARM_V, BATTERY_ALERT_CONFIRM}) {}

void BatteryAlert::begin() {
    Preferences prefs;
    uint8_t notified = 0;
    if (prefs.begin("meteohub", true)) { // lecture seule
        notified = prefs.getUChar(BATTERY_ALERT_NVS_KEY, 0);
        prefs.end();
    }
    _logic.restore(notified);
    if (notified)
        LOG_INFO("BatteryAlert: palier deja notifie = " + std::to_string(notified));
}

void BatteryAlert::persistNotified() {
    Preferences prefs;
    if (prefs.begin("meteohub", false)) {
        prefs.putUChar(BATTERY_ALERT_NVS_KEY, static_cast<uint8_t>(_logic.notified()));
        prefs.end();
    }
}

void BatteryAlert::onLiveReading(float volts) {
    const mhbat::Level before = _logic.observed();
    _logic.onReading(volts);
    if (_logic.observed() != before)
        LOG_WARNING("BatteryAlert: palier " + std::to_string(before) + " -> "
                    + std::to_string(_logic.observed()) + " (" + std::to_string(volts) + " V)");
    _lastLiveMs = millis();
    if (_lastLiveMs == 0) _lastLiveMs = 1; // 0 est réservé à « jamais »
    _attemptedSinceLive = false;
}

void BatteryAlert::update(const std::string& notifyUrl) {
    if (!_logic.hasPending() || _attemptedSinceLive || _lastLiveMs == 0) return;
    if (millis() - _lastLiveMs > kSendWindowMs) return; // attendre la trame suivante
    if (notifyUrl.empty() || WiFi.status() != WL_CONNECTED) return;

    _attemptedSinceLive = true;  // succès ou échec : prochain essai à la trame suivante
    if (send(notifyUrl)) {
        _logic.markNotified();
        persistNotified();
    }
}

// Tension au format français (3,38) pour le message lu sur le téléphone.
static std::string frVolts(float v) {
    char b[12];
    snprintf(b, sizeof(b), "%.2f", v);
    for (char* p = b; *p; ++p) if (*p == '.') *p = ',';
    return b;
}

bool BatteryAlert::send(const std::string& url) {
    const std::string v = frVolts(_logic.lastVoltage());
    const char* level;
    std::string message;
    switch (_logic.observed()) {
        case mhbat::LEVEL_CRITICAL:
            level = "error";
            message = "Accu de la sonde extérieure presque vide (" + v + " V). "
                      "À remplacer maintenant : la sonde va bientôt s'arrêter.";
            break;
        case mhbat::LEVEL_LOW:
            level = "warning";
            message = "Accu de la sonde extérieure faible (" + v + " V). "
                      "À remplacer dans les prochains jours, avant la coupure.";
            break;
        default:
            level = "success";
            message = "Accu de la sonde extérieure remplacé (" + v + " V). "
                      "Surveillance de la batterie réarmée.";
            break;
    }

    // Pas de `targets` : morfNotify applique ses destinations par défaut (un
    // ESP32 n'a pas accès à la liste du parc, /etc/morfsystem/alert-targets).
    StaticJsonDocument<384> doc;
    doc["title"]   = PROJECT_NAME;
    doc["message"] = message;
    doc["level"]   = level;
    std::string body;
    serializeJson(doc, body);

    HTTPClient http;
    http.setConnectTimeout(2000);
    http.setTimeout(3000);
    if (!http.begin(url.c_str())) {
        LOG_WARNING("BatteryAlert: URL morfNotify invalide " + url);
        return false;
    }
    http.addHeader("Content-Type", "application/json");
    const int code = http.POST(String(body.c_str()));
    http.end();

    if (code == 202 || code == 200) {
        LOG_INFO(std::string("BatteryAlert: notification envoyee (") + level + ", " + v + " V)");
        return true;
    }
    LOG_WARNING("BatteryAlert: envoi morfNotify echoue (HTTP " + std::to_string(code)
                + "), nouvel essai a la prochaine trame");
    return false;
}
