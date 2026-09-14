#pragma once
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
};
