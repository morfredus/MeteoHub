#include "analytics_beacon.h"
#include "config.h"
#include "../utils/logs.h"

#if defined(ANALYTICS_BEACON_ENABLED) && ANALYTICS_BEACON_ENABLED

#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <cstring>

#ifndef ANALYTICS_BEACON_PORT
#define ANALYTICS_BEACON_PORT 45454
#endif
#ifndef ANALYTICS_APP_NAME
#define ANALYTICS_APP_NAME "morfAnalytics"
#endif
#ifndef ANALYTICS_TIMEOUT_MS
#define ANALYTICS_TIMEOUT_MS 60000
#endif

static WiFiUDP s_udp;

void AnalyticsBeacon::begin() {
    if (_started) return;
    if (WiFi.status() != WL_CONNECTED) return; // (re)tenté depuis update() sinon
    s_udp.begin(ANALYTICS_BEACON_PORT);
    _started = true;
    LOG_INFO("AnalyticsBeacon: listening on UDP " + std::to_string(ANALYTICS_BEACON_PORT));
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
                const char* app   = doc["app"] | "";
                if (strcmp(proto, "morfbeacon/1") == 0 && strcmp(app, ANALYTICS_APP_NAME) == 0) {
                    _host       = doc["host"] | "";
                    _version    = doc["version"] | "";
                    _statusPort = doc["status_port"] | 0;
                    _lastSeenMs = millis();
                    _everSeen   = true;
                }
            }
        }
        sz = s_udp.parsePacket();
    }
}

bool AnalyticsBeacon::isAvailable() const {
    if (!_everSeen) return false;
    return (millis() - _lastSeenMs) < static_cast<unsigned long>(ANALYTICS_TIMEOUT_MS);
}

long AnalyticsBeacon::lastSeenAgoSec() const {
    if (!_everSeen) return -1;
    return static_cast<long>((millis() - _lastSeenMs) / 1000UL);
}

#else // ANALYTICS_BEACON_ENABLED == 0

void AnalyticsBeacon::begin() {}
void AnalyticsBeacon::update() {}
bool AnalyticsBeacon::isAvailable() const { return false; }
long AnalyticsBeacon::lastSeenAgoSec() const { return -1; }

#endif
