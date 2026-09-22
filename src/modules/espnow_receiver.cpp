#include "espnow_receiver.h"
#include "config.h"
#include "../utils/logs.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <cstring>
#include <string>
#include <algorithm>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

EspNowReceiver* EspNowReceiver::_self = nullptr;

// File courte : une sonde émet au plus toutes les 30 s, 4 trames suffisent
// si loop() est occupé un instant (OTA, lecture SD).
static QueueHandle_t s_packetQueue = nullptr;

// Callback C : la pile ESP-NOW attend une convention C, pas une méthode C++.
void meteoEspNowRecv(const uint8_t* mac, const uint8_t* data, int len) {
    if (EspNowReceiver::_self != nullptr && mac != nullptr) {
        memcpy(EspNowReceiver::_self->_lastSrcMac, mac, 6);
        EspNowReceiver::_self->_haveSrcMac = true;
    }
    EspNowReceiver::enqueueRaw(data, len);
}

EspNowReceiver::EspNowReceiver() {
    _self = this;
}

void EspNowReceiver::disableWifiSleep() {
    // Pourquoi : en STA, le modem s'endort entre deux DTIM. ESP-NOW (surtout
    // en broadcast, sans ACK du S3) arrive pendant ce sommeil et est perdu.
    // Le capteur affiche alors NOW: OK alors que le hub n'a rien vu.
    WiFi.setSleep(false);
    esp_wifi_set_ps(WIFI_PS_NONE);
}

void EspNowReceiver::refreshChannel() {
    uint8_t primary = 0;
    wifi_second_chan_t second = WIFI_SECOND_CHAN_NONE;
    if (esp_wifi_get_channel(&primary, &second) == ESP_OK) {
        _wifiChannel = primary;
    } else if (WiFi.status() == WL_CONNECTED) {
        _wifiChannel = static_cast<uint8_t>(WiFi.channel());
    }
}

bool EspNowReceiver::addBroadcastPeer() {
    // La sonde reste STA non associee : les trames broadcast arrivent sur
    // l'interface STA du hub, pas sur le SoftAP.
    const uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_now_del_peer(broadcastMac);

    esp_now_peer_info_t peer{};
    memcpy(peer.peer_addr, broadcastMac, 6);
    peer.channel = 0;
    peer.encrypt = false;
    peer.ifidx = WIFI_IF_STA;

    if (esp_now_add_peer(&peer) != ESP_OK) {
        LOG_WARNING("ESP-NOW: failed to add broadcast peer on STA");
        return false;
    }

    LOG_INFO("ESP-NOW: broadcast peer on STA");
    return true;
}

bool EspNowReceiver::begin() {
    _lastBeginAttemptMs = millis();

    if (WiFi.getMode() == WIFI_MODE_NULL) {
        LOG_WARNING("ESP-NOW: WiFi mode is null, delaying init");
        return false;
    }

    disableWifiSleep();
    refreshChannel();
    if (_wifiChannel == 0) {
        _wifiChannel = ESPNOW_WIFI_CHANNEL;
        esp_wifi_set_channel(_wifiChannel, WIFI_SECOND_CHAN_NONE);
    }

    if (s_packetQueue == nullptr) {
        s_packetQueue = xQueueCreate(4, sizeof(MeteoPacket));
        if (s_packetQueue == nullptr) {
            LOG_ERROR("ESP-NOW: Packet queue allocation failed");
            return false;
        }
    }

    // Ré-init propre après reconnexion Wi-Fi
    if (_initialized) {
        esp_now_deinit();
        _initialized = false;
    }

    if (esp_now_init() != ESP_OK) {
        LOG_ERROR("ESP-NOW: Initialization failed");
        return false;
    }

    esp_now_register_recv_cb(meteoEspNowRecv);
    addBroadcastPeer();

    _initialized = true;
    LOG_INFO("ESP-NOW: receiver ready ch=" + std::to_string(_wifiChannel)
             + " pkt=" + std::to_string(sizeof(MeteoPacket)) + "B"
             + " sta=" + std::string(WiFi.macAddress().c_str()));
    return true;
}

void EspNowReceiver::setOutdoorDataCallback(OutdoorDataCallback callback) {
    _outdoorCallback = callback;
}

bool EspNowReceiver::sendControl(SyncControl& ctrl) {
    // Voie inverse : accuse cumulatif + trou a combler, renvoye a la sonde qui
    // vient d'emettre. On finalise magic/version + CRC ici, on (re)declare la MAC
    // de la sonde comme peer unicast, puis on emet. La fenetre d'ecoute de la
    // sonde etant courte, cet envoi doit suivre de pres la reception de sa trame.
    if (!_initialized || !_haveSrcMac) return false;

    ctrl.magic[0] = METEO_CONTROL_MAGIC_0;
    ctrl.magic[1] = METEO_CONTROL_MAGIC_1;
    ctrl.protocol_version = METEO_PROTOCOL_VERSION;
    const size_t lenForCrc = sizeof(SyncControl) - sizeof(uint16_t);
    ctrl.crc16 = calculateCrc16(reinterpret_cast<const uint8_t*>(&ctrl), lenForCrc);

    // Peer unicast vers la sonde (channel 0 = suit le canal radio courant).
    if (!esp_now_is_peer_exist(_lastSrcMac)) {
        esp_now_peer_info_t peer{};
        memcpy(peer.peer_addr, _lastSrcMac, 6);
        peer.channel = 0;
        peer.ifidx = WIFI_IF_STA;
        peer.encrypt = false;
        if (esp_now_add_peer(&peer) != ESP_OK) {
            LOG_WARNING("ESP-NOW: add sensor peer failed (SyncControl)");
            return false;
        }
    }

    const esp_err_t r = esp_now_send(_lastSrcMac, reinterpret_cast<const uint8_t*>(&ctrl),
                                     sizeof(SyncControl));
    if (r != ESP_OK) {
        LOG_WARNING("ESP-NOW: SyncControl send error " + std::to_string((int)r));
        return false;
    }
    return true;
}

void EspNowReceiver::update() {
    const bool wifiUp = (WiFi.status() == WL_CONNECTED);

    if (wifiUp && !_wifiWasConnected) {
        // Front montant : le canal STA est désormais celui de l'AP.
        begin();
    } else if (!_initialized) {
        if (millis() - _lastBeginAttemptMs >= 5000) {
            begin();
        }
    }

    if (wifiUp) {
        // Le firmware STA peut réactiver le sleep modem : on le recoupe.
        disableWifiSleep();
    }
    _wifiWasConnected = wifiUp;

    processQueuedPackets();

    // Le refresh de canal reste frequent (15 s) pour suivre une migration du hub ;
    // le LOG, lui, n'est emis que s'il APPORTE une info : compteurs changes (une
    // trame vient d'arriver ou d'etre rejetee) ou battement de coeur espace (preuve
    // de vie + canal courant). Sans ce filtre, 20 lignes identiques tombaient entre
    // deux envois (la sonde n'emet que toutes les 5 min).
    static constexpr unsigned long STATUS_HEARTBEAT_MS = 300000UL; // 5 min
    if (millis() - _lastStatusLogMs >= 15000) {
        _lastStatusLogMs = millis();
        refreshChannel();

        const bool changed = (_packetsReceived != _loggedReceived)
                          || (_packetsValid != _loggedValid)
                          || (_packetsInvalid != _loggedInvalid);
        const bool heartbeat = (millis() - _lastStatusHeartbeatMs >= STATUS_HEARTBEAT_MS);
        if (changed || heartbeat) {
            _loggedReceived = _packetsReceived;
            _loggedValid = _packetsValid;
            _loggedInvalid = _packetsInvalid;
            _lastStatusHeartbeatMs = millis();

            char src[18] = "--:--:--:--:--:--";
            if (_haveSrcMac) {
                snprintf(src, sizeof(src), "%02X:%02X:%02X:%02X:%02X:%02X",
                         _lastSrcMac[0], _lastSrcMac[1], _lastSrcMac[2],
                         _lastSrcMac[3], _lastSrcMac[4], _lastSrcMac[5]);
            }
            LOG_INFO("ESP-NOW: ch=" + std::to_string(_wifiChannel)
                     + " rx=" + std::to_string(_packetsReceived)
                     + " ok=" + std::to_string(_packetsValid)
                     + " bad=" + std::to_string(_packetsInvalid)
                     + " last_len=" + std::to_string(_lastRxLen)
                     + " src=" + std::string(src));
        }
    }
}

void EspNowReceiver::enqueueRaw(const uint8_t* data, int len) {
    if (_self == nullptr || s_packetQueue == nullptr) {
        return;
    }

    _self->_packetsReceived++;
    _self->_lastRxLen = len;

    if (data == nullptr || len < 12) {
        _self->_packetsInvalid++;
        return;
    }

    // On copie ce qui rentre, même si la taille diverge : le CRC dira si la
    // trame est la nôtre. Un rejet strict sur sizeof() rendait l'échec silencieux.
    MeteoPacket packet;
    memset(&packet, 0, sizeof(packet));
    const size_t copyLen = std::min(static_cast<size_t>(len), sizeof(MeteoPacket));
    memcpy(&packet, data, copyLen);

    if (xQueueSend(s_packetQueue, &packet, 0) != pdTRUE) {
        _self->_packetsInvalid++;
    }
}

void EspNowReceiver::processQueuedPackets() {
    if (s_packetQueue == nullptr) {
        return;
    }

    MeteoPacket packet;
    while (xQueueReceive(s_packetQueue, &packet, 0) == pdTRUE) {
        if (!validatePacket(packet)) {
            _packetsInvalid++;
            continue;
        }

        _packetsValid++;
        OutdoorData outdoor = convertToOutdoorData(packet);
        if (_outdoorCallback) {
            _outdoorCallback(outdoor);
        }

        LOG_INFO("ESP-NOW: packet node=" + std::to_string(packet.node_id)
                 + " seq=" + std::to_string(packet.sequence)
                 + " T=" + std::to_string(outdoor.temperature)
                 + " H=" + std::to_string(outdoor.humidity)
                 + " P=" + std::to_string(outdoor.pressure));
    }
}

bool EspNowReceiver::validatePacket(const MeteoPacket& packet) const {
    if (packet.magic[0] != METEO_PACKET_MAGIC_0 || packet.magic[1] != METEO_PACKET_MAGIC_1) {
        LOG_WARNING("ESP-NOW: Invalid magic (len=" + std::to_string(_lastRxLen) +
                    " expect=" + std::to_string(sizeof(MeteoPacket)) + ")");
        return false;
    }


    if (packet.protocol_version != METEO_PROTOCOL_VERSION) {
        LOG_WARNING("ESP-NOW: Unsupported protocol version "
                    + std::to_string(packet.protocol_version));
        return false;
    }

    const size_t dataLenForCrc = sizeof(MeteoPacket) - sizeof(uint16_t);
    uint16_t calculatedCrc = calculateCrc16(
        reinterpret_cast<const uint8_t*>(&packet), dataLenForCrc);
    if (calculatedCrc != packet.crc16) {
        LOG_WARNING("ESP-NOW: CRC mismatch (len=" + std::to_string(_lastRxLen) + ")");
        LOG_DEBUG("CRC expected=" + std::to_string(packet.crc16) + " calculated=" + std::to_string(calculatedCrc));
        return false;
    }


    return true;
}

OutdoorData EspNowReceiver::convertToOutdoorData(const MeteoPacket& packet) const {
    OutdoorData outdoor;

    outdoor.temperature = packet.temperature;
    outdoor.humidity = packet.humidity;
    outdoor.pressure = packet.pressure;
    outdoor.wind_speed = packet.wind_speed;
    outdoor.wind_gust = packet.wind_gust;
    outdoor.wind_direction_deg = packet.wind_direction_deg;
    outdoor.rain_rate = packet.rain_rate;
    outdoor.rain_accumulated = packet.rain_accumulated;
    outdoor.solar_lux = 0;
    outdoor.uv_index = 0;

    // État batterie de la sonde : conservé pour l'alerte pile faible (OLED + web).
    if (packet.valid_fields & FIELD_BATTERY) {
        outdoor.battery_voltage = packet.battery_voltage;
        outdoor.battery_percent = packet.battery_percent;
        outdoor.has_battery = true;
    }

    // Diagnostic v2 : reporté tel quel pour journalisation (voir main.cpp).
    outdoor.reset_reason = packet.reset_reason;
    outdoor.wake_count = packet.wake_count;
    outdoor.sequence = packet.sequence;

    // Synchronisation fiable (v3) : reportés pour la logique de sync/dedup/anchor.
    outdoor.sensor_ts = packet.sensor_ts;
    outdoor.frame_type = packet.frame_type;
    outdoor.oldest_seq = packet.oldest_seq;
    outdoor.node_id = packet.node_id;

    const bool flagged = (packet.valid_fields
                          & (FIELD_TEMPERATURE | FIELD_HUMIDITY | FIELD_PRESSURE)) != 0;
    // Repli : une trame CRC-valide avec une T plausible reste affichable même
    // si le masque a été mal rempli côté sonde.
    const bool plausible = (packet.temperature > -40.0f && packet.temperature < 85.0f
                            && packet.humidity >= 0.0f && packet.humidity <= 100.0f);
    outdoor.valid = flagged || plausible;

    return outdoor;
}
