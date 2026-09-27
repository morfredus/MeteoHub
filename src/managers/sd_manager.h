#pragma once

#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <memory>
#include <mutex> // Nécessaire pour le verrouillage
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class SdManager {
public:
    bool begin();
    bool format();
    bool isAvailable();
    bool ensureMounted();

private:
    bool _available = false;
    unsigned long _last_reconnect_attempt_ms = 0;
    unsigned long _reconnect_cooldown_ms = 15000;
    int _consecutive_reconnect_failures = 0;
    int _mount_frequency_hz = 0; // dernière fréquence SPI SD ayant fonctionné
    std::unique_ptr<SPIClass> _sd_spi;
    std::mutex _sd_mutex; // Mutex interne à la classe
    // Tâche autorisée à (re)monter la carte : celle qui a appelé begin(), soit
    // loopTask. Voir ensureMounted().
    TaskHandle_t _mount_task = nullptr;

    bool isCardDetected() const;
    void resetSpiBus();
    bool mountAtFrequency(int frequency_hz, bool format_if_fail);
    bool mountBestFrequency(bool format_if_fail);
    bool verifyWriteAccess();
    bool ensureHistoryDirectory();
    void logPinMapping() const;
};