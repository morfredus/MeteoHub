#pragma once
#include <Arduino.h>
#include <esp_now.h>
#include <functional>
#include "meteo_context.h"
#include "meteo_packet.h"

// ============================================================================
// Récepteur ESP-NOW pour MeteoHub
// Reçoit les MeteoPacket depuis MeteoHubSensor (ESP32-C3) et les convertit
// en OutdoorData. Le callback radio ne fait que copier la trame : le décodage
// et l'historique restent dans loop() pour ne pas toucher LittleFS/SD depuis
// la tâche Wi-Fi.
// ============================================================================

class EspNowReceiver {
public:
    EspNowReceiver();

    // Initialise ESP-NOW (Wi-Fi déjà en STA). Réessayable si le premier appel
    // a lieu avant que la radio soit prête.
    bool begin();

    // Callback appelé depuis loop() quand un paquet OUT est validé.
    using OutdoorDataCallback = std::function<void(const OutdoorData&)>;
    void setOutdoorDataCallback(OutdoorDataCallback callback);

    // Vide la file de réception et relance begin() si besoin.
    void update();

    bool isReady() const { return _initialized; }

    // Voie inverse (v3) : renvoie un SyncControl a la DERNIERE sonde vue (unicast
    // vers _lastSrcMac). Renvoie false si aucune sonde connue ou envoi impossible.
    // Le CRC est calcule ici ; l'appelant remplit ack_seq/want_*.
    bool sendControl(SyncControl& ctrl);

    uint32_t getPacketsReceived() const { return _packetsReceived; }
    uint32_t getPacketsValid() const { return _packetsValid; }
    uint32_t getPacketsInvalid() const { return _packetsInvalid; }
    uint8_t getWifiChannel() const { return _wifiChannel; }
    int getLastRxLen() const { return _lastRxLen; }

    static EspNowReceiver* instance() { return _self; }

private:
    static EspNowReceiver* _self;

    OutdoorDataCallback _outdoorCallback;
    bool _initialized = false;
    bool _wifiWasConnected = false;
    unsigned long _lastBeginAttemptMs = 0;
    unsigned long _lastStatusLogMs = 0;       // cadence du refresh canal (15 s)
    unsigned long _lastStatusHeartbeatMs = 0; // dernier log "battement de coeur"
    // Compteurs au dernier LOG : on ne re-logue que s'ils ont change (nouvelle
    // trame), pour ne pas repeter 20 lignes identiques entre deux envois (5 min).
    uint32_t _loggedReceived = 0;
    uint32_t _loggedValid = 0;
    uint32_t _loggedInvalid = 0;

    uint32_t _packetsReceived = 0;
    uint32_t _packetsValid = 0;
    uint32_t _packetsInvalid = 0;
    uint8_t _wifiChannel = 0;
    int _lastRxLen = 0;
    uint8_t _lastSrcMac[6]{};
    bool _haveSrcMac = false;

    static void enqueueRaw(const uint8_t* data, int len);
    void processQueuedPackets();
    bool addBroadcastPeer();
    void disableWifiSleep();
    void refreshChannel();

    bool validatePacket(const MeteoPacket& packet) const;
    OutdoorData convertToOutdoorData(const MeteoPacket& packet) const;

    friend void meteoEspNowRecv(const uint8_t* mac, const uint8_t* data, int len);
};
