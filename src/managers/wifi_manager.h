#pragma once
#include <cstdint>
#include <string>

class WifiManager {
public:
    void begin();
    void update();

    // A appeler une fois mDNS/web demarres : a partir de la, un retour du Wi-Fi relance
    // mDNS et NTP (s'ils sont partis sans reseau, ou apres une coupure).
    void servicesStarted() { markServicesStarted(); }

    std::string ssid() const { return currentSSID; }
    std::string ip() const;
    int rssi() const;
    int channel() const;
    std::string mac() const;

private:
    std::string currentSSID;
    unsigned long lastAttempt = 0;
    bool _sleepDisabled = false;
    bool _apStarted = false;
    bool _apFollowedSta = false;
    bool _servicesStarted = false;
    bool _wasDown = false;          // vrai des qu'une coupure a ete constatee
    uint8_t _failures = 0;          // echecs consecutifs : pilote le backoff
    unsigned long _retryDelayMs = 0;
    unsigned long _downSince = 0;   // millis() du debut de la coupure en cours (0 = connecte)

    void onReconnected();
    void applyTxPower();
    void markServicesStarted();
};
