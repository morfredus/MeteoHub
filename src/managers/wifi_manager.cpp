#include <string>
#include <WiFi.h>
#include <esp_wifi.h>

#include "wifi_manager.h"
#include "config.h"
#include "secrets.h"
#include "../utils/logs.h"

namespace {
constexpr uint8_t kSta24Ghz =
    WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N;

void lockSta24GHz() {
    // Pourquoi : la Livebox annonce le meme SSID en 5 GHz. ESP-NOW n'existe
    // qu'en 2.4 GHz ; un STA en 5 GHz rend la sonde inaudible.
    esp_wifi_set_protocol(WIFI_IF_STA, kSta24Ghz);
    esp_wifi_set_protocol(WIFI_IF_AP, kSta24Ghz);
    esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT20);
    esp_wifi_set_bandwidth(WIFI_IF_AP, WIFI_BW_HT20);
}

bool startEspNowAp(int ch) {
    return WiFi.softAP(ESPNOW_SOFTAP_SSID, ESPNOW_SOFTAP_PASS, ch, 0, 4);
}

bool staOn5GHz() {
    const int ch = WiFi.channel();
    return ch > 13;
}
}

void WifiManager::begin() {
    // AP d'abord, canal de repli : la sonde peut scanner MH-NOW sans attendre
    // la Livebox.
    WiFi.mode(WIFI_AP_STA);
    WiFi.disconnect(false, false);
    delay(50);
    lockSta24GHz();
    const bool ok = startEspNowAp(ESPNOW_WIFI_CHANNEL);
    _apStarted = ok;
    LOG_INFO(std::string("WiFi: AP ESP-NOW ")
             + (ok ? "ok" : "fail")
             + " ch=" + std::to_string(ESPNOW_WIFI_CHANNEL)
             + " apmac=" + std::string(WiFi.softAPmacAddress().c_str()));
    lastAttempt = millis() - WIFI_RETRY_DELAY_MS;
}

void WifiManager::update() {
    if (WiFi.status() == WL_CONNECTED) {
        if (staOn5GHz()) {
            LOG_WARNING("WiFi: 5 GHz ch=" + std::to_string(WiFi.channel())
                        + " refuse pour ESP-NOW, reconnect 2.4");
            WiFi.disconnect(true, false);
            _sleepDisabled = false;
            delay(50);
            lockSta24GHz();
            return;
        }
        // Indispensable pour qu'ESP-NOW soit recu pendant que le STA reste associe.
        if (!_sleepDisabled) {
            WiFi.setSleep(false);
            _sleepDisabled = true;
            LOG_INFO("WiFi: modem sleep disabled (ESP-NOW RX)");
        }
        // Recaler le SoftAP une fois sur le canal Livebox (handshake STA).
        if (!_apFollowedSta) {
            const int ch = WiFi.channel();
            if (ch >= 1 && ch <= 13) {
                const bool ok = startEspNowAp(ch);
                _apStarted = ok;
                _apFollowedSta = ok;
                LOG_INFO(std::string("WiFi: AP ESP-NOW ")
                         + (ok ? "ok" : "fail")
                         + " ch=" + std::to_string(ch)
                         + " apmac=" + std::string(WiFi.softAPmacAddress().c_str()));
            }
        }
        return;
    }
    _sleepDisabled = false;
    _apFollowedSta = false;

    unsigned long now = millis();
    if (now - lastAttempt < WIFI_RETRY_DELAY_MS) return;
    lastAttempt = now;

    for (size_t i = 0; i < WIFI_CREDENTIALS_COUNT; i++) {
        lockSta24GHz();
        WiFi.begin(WIFI_CREDENTIALS[i].ssid, WIFI_CREDENTIALS[i].password);
        unsigned long t0 = millis();
        while (millis() - t0 < 3000) {
            if (WiFi.status() == WL_CONNECTED) {
                lockSta24GHz();
                WiFi.setSleep(false);
                _sleepDisabled = true;
                if (staOn5GHz()) {
                    LOG_WARNING("WiFi: 5 GHz ch=" + std::to_string(WiFi.channel())
                                + " refuse pour ESP-NOW, reconnect 2.4");
                    WiFi.disconnect(true, false);
                    delay(200);
                    lockSta24GHz();
                    return;
                }
                currentSSID = WIFI_CREDENTIALS[i].ssid;
                LOG_INFO(std::string("WiFi: ") + currentSSID
                         + " ch=" + std::to_string(WiFi.channel()));
                return;
            }
            delay(100);
        }
    }
}

std::string WifiManager::ip() const {
    if (WiFi.status() != WL_CONNECTED) return "0.0.0.0";
    return WiFi.localIP().toString().c_str();
}

int WifiManager::rssi() const {
    if (WiFi.status() != WL_CONNECTED) return 0;
    return WiFi.RSSI();
}

int WifiManager::channel() const {
    if (WiFi.status() != WL_CONNECTED) return 0;
    return WiFi.channel();
}

std::string WifiManager::mac() const {
    return WiFi.macAddress().c_str();
}
