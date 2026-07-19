#include "analytics_beacon.h"
#include "config.h"
#include "../utils/logs.h"

#if defined(ANALYTICS_BEACON_ENABLED) && ANALYTICS_BEACON_ENABLED

#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <cstring>

#ifndef ANALYTICS_BEACON_PORT
#define ANALYTICS_BEACON_PORT 45454
#endif
#ifndef ANALYTICS_TIMEOUT_MS
#define ANALYTICS_TIMEOUT_MS 60000
#endif

// Clé NVS de l'adresse manuelle (namespace "meteohub", partagé avec les autres
// réglages persistés). Les clés NVS sont limitées à 15 caractères.
#define ANALYTICS_NVS_KEY "an_url"

static WiFiUDP s_udp;

void AnalyticsBeacon::loadManualUrl() {
    Preferences prefs;
    if (prefs.begin("meteohub", true)) { // lecture seule
        String stored = prefs.getString(ANALYTICS_NVS_KEY, "");
        _manualUrl = stored.c_str();
        prefs.end();
    }
    if (!_manualUrl.empty())
        LOG_INFO("AnalyticsBeacon: adresse manuelle " + _manualUrl);
}

void AnalyticsBeacon::setManualUrl(const std::string& url) {
    _manualUrl = url;
    Preferences prefs;
    if (prefs.begin("meteohub", false)) { // lecture/écriture
        if (_manualUrl.empty())
            prefs.remove(ANALYTICS_NVS_KEY); // retour au mode automatique
        else
            prefs.putString(ANALYTICS_NVS_KEY, _manualUrl.c_str());
        prefs.end();
    }
}

void AnalyticsBeacon::begin() {
    if (_started) return;
    loadManualUrl(); // indépendant du WiFi : utilisable même sans découverte
    if (WiFi.status() != WL_CONNECTED) return; // (re)tenté depuis update() sinon
    s_udp.begin(ANALYTICS_BEACON_PORT);
    _started = true;
    LOG_INFO("AnalyticsBeacon: ecoute UDP " + std::to_string(ANALYTICS_BEACON_PORT)
             + ", capacite '" + ANALYTICS_CAPABILITY + "'");
}

void AnalyticsBeacon::update() {
    if (!_started) { begin(); return; }

    // Traite tous les paquets en attente (non bloquant).
    int sz = s_udp.parsePacket();
    while (sz > 0) {
        char buf[512];
        int n = s_udp.read(buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            StaticJsonDocument<512> doc;
            if (deserializeJson(doc, buf) == DeserializationError::Ok) {
                const char* proto = doc["proto"] | "";
                if (strcmp(proto, "morfbeacon/1") == 0) {
                    // On cherche la CAPACITÉ, pas le nom : le service peut
                    // s'appeler comme son propriétaire le souhaite.
                    bool hasCapability = false;
                    JsonArrayConst caps = doc["capabilities"].as<JsonArrayConst>();
                    for (JsonVariantConst c : caps) {
                        if (strcmp(c.as<const char*>() ? c.as<const char*>() : "",
                                   ANALYTICS_CAPABILITY) == 0) {
                            hasCapability = true;
                            break;
                        }
                    }
                    if (hasCapability) {
                        _name       = doc["app"] | "";   // libellé affiché
                        _version    = doc["version"] | "";
                        _host       = doc["host"] | "";
                        _statusPort = doc["status_port"] | 0;
                        _lastSeenMs = millis();
                        _everSeen   = true;
                    }
                }
            }
        }
        sz = s_udp.parsePacket();
    }
}

bool AnalyticsBeacon::isDetected() const {
    if (!_everSeen) return false;
    return (millis() - _lastSeenMs) < static_cast<unsigned long>(ANALYTICS_TIMEOUT_MS);
}

long AnalyticsBeacon::lastSeenAgoSec() const {
    if (!_everSeen) return -1;
    return static_cast<long>((millis() - _lastSeenMs) / 1000UL);
}

bool AnalyticsBeacon::isAvailable() const {
    return !_manualUrl.empty() || isDetected();
}

std::string AnalyticsBeacon::effectiveUrl() const {
    // L'adresse manuelle prime : elle n'est renseignée que si l'utilisateur a
    // constaté que la découverte automatique ne fonctionnait pas chez lui.
    if (!_manualUrl.empty()) return _manualUrl;
    if (!isDetected() || _host.empty() || _statusPort <= 0) return "";
    return "http://" + _host + ":" + std::to_string(_statusPort) + "/";
}

#else // ANALYTICS_BEACON_ENABLED == 0

void AnalyticsBeacon::begin() {}
void AnalyticsBeacon::update() {}
void AnalyticsBeacon::loadManualUrl() {}
void AnalyticsBeacon::setManualUrl(const std::string&) {}
bool AnalyticsBeacon::isDetected() const { return false; }
bool AnalyticsBeacon::isAvailable() const { return false; }
std::string AnalyticsBeacon::effectiveUrl() const { return ""; }
long AnalyticsBeacon::lastSeenAgoSec() const { return -1; }

#endif
