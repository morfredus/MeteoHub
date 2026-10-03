#pragma once
#include <cstdint>
#include <string>

class WifiManager {
public:
    void begin();
    void update();

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
    bool _wasDown = false;          // vrai des qu'une coupure a ete constatee
    uint8_t _failures = 0;          // echecs consecutifs : pilote le backoff
    unsigned long _retryDelayMs = 0;

    void onReconnected();
};
