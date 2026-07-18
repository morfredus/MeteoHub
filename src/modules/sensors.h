#pragma once
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>

struct SensorData {
    float temperature;
    float humidity;
    float pressure;
    bool valid;
};

class SensorManager {
public:
    bool begin();
    SensorData read();

private:
    Adafruit_AHTX0 aht;
    Adafruit_BMP280 bmp;
    bool ahtFound = false;
    bool bmpFound = false;
    uint8_t bmpAddr = 0x77;

    // Dernière lecture valide, renvoyée (avec valid=false) quand une lecture échoue
    // afin de ne pas afficher 0 en temps réel.
    float _lastTemp = 0, _lastHum = 0, _lastPres = 0;
    bool _hasLast = false;
    int _consecutiveFailures = 0;

    bool readAht(float& t, float& h);
    bool readBmp(float& p);
    void applyBmpSampling();
    void recoverBus();
};

//
