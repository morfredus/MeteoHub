#pragma once
#include "board_config.h"
// Encodeur rotatif : DevKitC seulement. Sur carte à bouton unique
// (UI_SINGLE_BUTTON), ni ce module ni la lib ESP32Encoder ne sont compilés.
#if !defined(UI_SINGLE_BUTTON)
#include <Arduino.h>
#include <ESP32Encoder.h>
 
class Encoder {

public:
    void begin();
    void update();

    bool rotatedCW();
    bool rotatedCCW();
    bool clicked();
    void clearQueue();
    int getStepCount() const;

private:
    static constexpr int8_t COUNTS_PER_DETENT = 4;
    static constexpr uint16_t CLICK_DEBOUNCE_MS = 120;
    static constexpr int16_t STEP_QUEUE_LIMIT = 20;

    ESP32Encoder rotary;
    int64_t lastCount = 0;
    int32_t pulseAccumulator = 0;
    int16_t stepQueue = 0;

    bool lastBtn = false;
    bool clickFlag = false;
    unsigned long lastClickMs = 0;
};
#endif
