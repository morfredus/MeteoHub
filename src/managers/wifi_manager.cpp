#include <string>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <esp_wifi.h>
#include <time.h>

#include "wifi_manager.h"
#include "config.h"
#include "secrets.h"
#include "../utils/logs.h"

namespace {
constexpr unsigned long WIFI_MAX_DOWN_MS = 10UL * 60UL * 1000UL; // 10 min
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
    // Trace des evenements Wi-Fi : sans elle, un hub 'connecte mais injoignable'
    // reste inexplicable (raison de deconnexion, moment de l'obtention d'IP...).
    WiFi.onEvent([](arduino_event_id_t event, arduino_event_info_t info) {
        switch (event) {
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            LOG_INFO("WiFi evt: associe ch=" + std::to_string(info.wifi_sta_connected.channel));
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            LOG_INFO("WiFi evt: IP obtenue");
            break;
        case ARDUINO_EVENT_WIFI_STA_LOST_IP:
            LOG_WARNING("WiFi evt: IP perdue");
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            LOG_WARNING("WiFi evt: deconnecte, raison "
                        + std::to_string(info.wifi_sta_disconnected.reason));
            break;
        default:
            break;
        }
    });
    // AP d'abord, canal de repli : la sonde peut scanner MH-NOW sans attendre
    // la Livebox.
    WiFi.mode(WIFI_AP_STA);
    WiFi.disconnect(false, false);
    delay(50);
    lockSta24GHz();
    applyTxPower();
    const bool ok = startEspNowAp(ESPNOW_WIFI_CHANNEL);
    _apStarted = ok;
    LOG_INFO(std::string("WiFi: AP ESP-NOW ")
             + (ok ? "ok" : "fail")
             + " ch=" + std::to_string(ESPNOW_WIFI_CHANNEL)
             + " apmac=" + std::string(WiFi.softAPmacAddress().c_str()));
    lastAttempt = millis() - WIFI_RETRY_DELAY_MS;
}

void WifiManager::markServicesStarted() {
    // Une connexion deja obtenue avant ce point n'est pas un "retour" : les services
    // demarrent en ligne. Sinon (hors ligne) le prochain retour du Wi-Fi les relancera.
    _servicesStarted = true;
    _wasDown = (WiFi.status() != WL_CONNECTED);
}

void WifiManager::applyTxPower() {
    // A reappliquer apres chaque (re)connexion : le pilote peut remettre sa valeur par defaut.
    WiFi.setTxPower(HUB_TX_POWER_LEVEL);
    int8_t q = 0;  // unite du pilote : 0,25 dBm
    esp_wifi_get_max_tx_power(&q);
    LOG_INFO("WiFi: puissance TX max " + std::to_string(q / 4) + " dBm");
}

void WifiManager::onReconnected() {
    // Pourquoi : mDNS et SNTP sont lances UNE fois au boot. Si le hub demarre avant
    // la box (coupure de courant), ils partent sans reseau et ne se rattrapent pas
    // seuls : meteohub.local reste muet et l'heure jamais synchronisee jusqu'a un
    // redemarrage manuel. On les relance donc a chaque retour du Wi-Fi.
    MDNS.end();
    if (MDNS.begin(WEB_MDNS_HOSTNAME)) {
        MDNS.addService("http", "tcp", 80);
        LOG_INFO("WiFi: mDNS relance apres reconnexion");
    } else {
        LOG_ERROR("WiFi: echec relance mDNS apres reconnexion");
    }
    configTime(3600, 3600, "pool.ntp.org");
}

void WifiManager::update() {
    if (WiFi.status() == WL_CONNECTED) {
        _failures = 0;
        _retryDelayMs = WIFI_RETRY_DELAY_MS;
        _downSince = 0;
        if (_wasDown) {
            _wasDown = false;
            // Avant servicesStarted(), mDNS/NTP n'existent pas encore : ils demarreront
            // en ligne, inutile (et nuisible : la 1ere requete HTTP suivante echouait)
            // de les relancer pour la simple connexion du boot.
            if (_servicesStarted) onReconnected();
        }
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
                applyTxPower();  // WiFi.softAP() peut remettre la puissance par defaut
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
    _wasDown = true;

    // Filet de securite : un Wi-Fi mort que les relances ne reveillent pas (pile radio
    // bloquee, etat incoherent apres un flash...) ne se corrige qu'a la main. Au-dela de
    // WIFI_MAX_DOWN_MS sans connexion, on redemarre : le hub repart d'un etat propre.
    // Le seuil est long pour ne pas boucler pendant une vraie coupure de la box.
    if (_downSince == 0) _downSince = millis() ? millis() : 1;
    if (millis() - _downSince > WIFI_MAX_DOWN_MS) {
        LOG_ERROR("WiFi: coupure prolongee, redemarrage de securite");
        delay(200);
        ESP.restart();
    }

    // Backoff : 5 s, 10 s, 20 s ... plafonne a 30 s. Une box qui redemarre met du
    // temps ; marteler WiFi.begin() toutes les 5 s coupe chaque association en cours.
    if (_retryDelayMs == 0) _retryDelayMs = WIFI_RETRY_DELAY_MS;
    unsigned long now = millis();
    if (now - lastAttempt < _retryDelayMs) return;
    lastAttempt = now;
    if (_failures < 8) _failures++;
    _retryDelayMs = WIFI_RETRY_DELAY_MS << (_failures > 3 ? 3 : _failures - 1);
    if (_retryDelayMs > 30000) _retryDelayMs = 30000;

    for (size_t i = 0; i < WIFI_CREDENTIALS_COUNT; i++) {
        // Repart d'un etat propre (garde l'AP ESP-NOW, n'efface rien en NVS).
        WiFi.disconnect(false, false);
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
                applyTxPower();
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
