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

// Trame de mesure + MAC de son emetteur : la MAC voyage AVEC la trame dans la
// file, pour que loop() sache sans ambiguite quelle sonde l'a envoyee.
struct RxPacket { uint8_t mac[6]; MeteoPacket packet; };

// Trames d'appairage (demande / confirmation d'une sonde), mises de cote par le
// callback radio et traitees dans loop() : on n'emet ni n'ajoute de peer depuis
// la tache Wi-Fi.
struct PairRx { uint8_t mac[6]; MeteoPairFrame frame; };
static QueueHandle_t s_pairQueue = nullptr;

// Callback C : la pile ESP-NOW attend une convention C, pas une méthode C++.
void meteoEspNowRecv(const uint8_t* mac, const uint8_t* data, int len) {
    // Trame d'appairage : file dediee, et surtout PAS de mise a jour de
    // _lastSrcMac (la voie inverse SyncControl vise la sonde qui MESURE).
    if (EspNowReceiver::enqueuePair(mac, data, len)) return;
    if (EspNowReceiver::_self != nullptr && mac != nullptr) {
        memcpy(EspNowReceiver::_self->_lastSrcMac, mac, 6);
        EspNowReceiver::_self->_haveSrcMac = true;
    }
    EspNowReceiver::enqueueRaw(mac, data, len);
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

    if (s_pairQueue == nullptr) {
        s_pairQueue = xQueueCreate(4, sizeof(PairRx));
    }
    if (s_packetQueue == nullptr) {
        s_packetQueue = xQueueCreate(4, sizeof(RxPacket));
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

bool EspNowReceiver::sendControl(SyncControl& ctrl, const uint8_t* mac) {
    // Voie inverse : accuse cumulatif + trou a combler, renvoye a la sonde qui
    // vient d'emettre. On finalise magic/version + CRC ici, on (re)declare la MAC
    // de la sonde comme peer unicast, puis on emet. La fenetre d'ecoute de la
    // sonde etant courte, cet envoi doit suivre de pres la reception de sa trame.
    if (!_initialized || mac == nullptr) return false;

    ctrl.magic[0] = METEO_CONTROL_MAGIC_0;
    ctrl.magic[1] = METEO_CONTROL_MAGIC_1;
    ctrl.protocol_version = METEO_PROTOCOL_VERSION;
    const size_t lenForCrc = sizeof(SyncControl) - sizeof(uint16_t);
    ctrl.crc16 = calculateCrc16(reinterpret_cast<const uint8_t*>(&ctrl), lenForCrc);

    // Peer unicast vers la sonde (channel 0 = suit le canal radio courant).
    if (!ensurePeer(mac)) {
        LOG_WARNING("ESP-NOW: add sensor peer failed (SyncControl)");
        return false;
    }

    const esp_err_t r = esp_now_send(mac, reinterpret_cast<const uint8_t*>(&ctrl),
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
    processPairFrames();

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

bool EspNowReceiver::ensurePeer(const uint8_t* mac) {
    if (esp_now_is_peer_exist(mac)) return true;
    esp_now_peer_info_t peer{};
    memcpy(peer.peer_addr, mac, 6);
    peer.channel = 0;          // suit le canal radio courant
    peer.ifidx = WIFI_IF_STA;
    peer.encrypt = false;
    return esp_now_add_peer(&peer) == ESP_OK;
}

bool EspNowReceiver::enqueuePair(const uint8_t* mac, const uint8_t* data, int len) {
    // Trame d'appairage = taille 44 o + magic 'M','P' + version + CRC valides
    // (isValidPairFrame, quelques µs). Une trame de mesure (63 o) ou de controle
    // (20 o) n'est jamais concernee et poursuit son chemin habituel.
    if (mac == nullptr || s_pairQueue == nullptr) return false;
    PairRx rx;
    if (!isValidPairFrame(data, len, &rx.frame)) return false;
    memcpy(rx.mac, mac, 6);
    xQueueSend(s_pairQueue, &rx, 0); // file pleine : la sonde redemande
    return true;
}

// Appairage cote hub : repondre a TOUTE demande valide (c'est l'utilisateur qui
// choisit le hub, en ne laissant allume que lui pendant la procedure), puis
// enregistrer la reprise quand la sonde confirme.
void EspNowReceiver::processPairFrames() {
    if (s_pairQueue == nullptr) return;
    PairRx rx;
    while (xQueueReceive(s_pairQueue, &rx, 0) == pdTRUE) {
        const MeteoPairFrame& f = rx.frame;
        char src[18];
        snprintf(src, sizeof(src), "%02X:%02X:%02X:%02X:%02X:%02X",
                 rx.mac[0], rx.mac[1], rx.mac[2], rx.mac[3], rx.mac[4], rx.mac[5]);
        // La MAC annoncee doit etre celle qui emet (trame coherente).
        if (memcmp(f.sta_mac, rx.mac, 6) != 0) continue;

        if (f.type == PAIR_REQUEST) {
            if (!_initialized || !ensurePeer(rx.mac)) {
                LOG_WARNING(std::string("ESP-NOW: appairage, peer sonde impossible ") + src);
                continue;
            }
            MeteoPairFrame resp;
            memset(&resp, 0, sizeof(resp));
            resp.type = PAIR_RESPONSE;
            resp.node_id = f.node_id;
            resp.nonce = f.nonce;                  // relie la reponse a CETTE demande
            WiFi.macAddress(resp.sta_mac);         // cible unicast des mesures
            WiFi.softAPmacAddress(resp.ap_mac);    // BSSID « MH-NOW » de CE hub
            refreshChannel();
            resp.channel = _wifiChannel;
            strncpy(resp.name, WEB_MDNS_HOSTNAME, sizeof(resp.name) - 1);
            sealPairFrame(resp);
            const esp_err_t r = esp_now_send(rx.mac, reinterpret_cast<const uint8_t*>(&resp),
                                             sizeof(resp));
            LOG_INFO(std::string("ESP-NOW: demande d'appairage de ") + src
                     + " node=" + std::to_string(f.node_id)
                     + (r == ESP_OK ? " -> reponse envoyee ch=" + std::to_string(_wifiChannel)
                                    : " -> envoi reponse KO " + std::to_string((int)r)));
        } else if (f.type == PAIR_CONFIRM) {
            LOG_INFO(std::string("ESP-NOW: sonde appairee ") + src
                     + " node=" + std::to_string(f.node_id)
                     + " reprise apres seq=" + std::to_string(f.base_seq));
            if (_pairedCallback) _pairedCallback(f.node_id, rx.mac, f.base_seq);
        }
    }
}

void EspNowReceiver::enqueueRaw(const uint8_t* mac, const uint8_t* data, int len) {
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
    RxPacket rx;
    memset(&rx, 0, sizeof(rx));
    if (mac) memcpy(rx.mac, mac, 6);
    const size_t copyLen = std::min(static_cast<size_t>(len), sizeof(MeteoPacket));
    memcpy(&rx.packet, data, copyLen);

    if (xQueueSend(s_packetQueue, &rx, 0) != pdTRUE) {
        _self->_packetsInvalid++;
    }
}

void EspNowReceiver::processQueuedPackets() {
    if (s_packetQueue == nullptr) {
        return;
    }

    RxPacket rx;
    while (xQueueReceive(s_packetQueue, &rx, 0) == pdTRUE) {
        const MeteoPacket& packet = rx.packet;
        if (!validatePacket(packet)) {
            _packetsInvalid++;
            continue;
        }

        _packetsValid++;
        OutdoorData outdoor = convertToOutdoorData(packet);
        memcpy(outdoor.src_mac, rx.mac, 6);
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
    outdoor.uptime_sec = packet.uptime_sec;

    const bool flagged = (packet.valid_fields
                          & (FIELD_TEMPERATURE | FIELD_HUMIDITY | FIELD_PRESSURE)) != 0;
    // Repli : une trame CRC-valide avec une T plausible reste affichable même
    // si le masque a été mal rempli côté sonde.
    const bool plausible = (packet.temperature > -40.0f && packet.temperature < 85.0f
                            && packet.humidity >= 0.0f && packet.humidity <= 100.0f);
    outdoor.valid = flagged || plausible;

    return outdoor;
}
