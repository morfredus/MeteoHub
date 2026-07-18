#include <Wire.h>
#include <math.h>
#include <string>
#include "board_config.h"
#include "sensors.h"
#include "../utils/logs.h"

#ifndef I2C_FREQ_HZ
#define I2C_FREQ_HZ 100000   // 100 kHz : conservateur, plus tolérant au câblage
#endif
#define SENSOR_READ_RETRIES   3   // tentatives par lecture avant de renoncer
#define SENSOR_RECOVER_AFTER  5   // échecs consécutifs avant réinitialisation du bus
#define I2C_TIMEOUT_MS        50  // évite un blocage long si le bus est coincé

void SensorManager::applyBmpSampling() {
    // Suréchantillonnage + filtre IIR : lectures de pression plus stables.
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                    Adafruit_BMP280::SAMPLING_X2,   // température
                    Adafruit_BMP280::SAMPLING_X16,  // pression
                    Adafruit_BMP280::FILTER_X16,
                    Adafruit_BMP280::STANDBY_MS_500);
}

bool SensorManager::begin() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(I2C_FREQ_HZ);
    Wire.setTimeOut(I2C_TIMEOUT_MS);

    // Initialisation AHT20 (adresse 0x38 par défaut)
    ahtFound = aht.begin();

    // Initialisation BMP280 (adresse 0x77 ou 0x76)
    if (bmp.begin(0x77)) { bmpFound = true; bmpAddr = 0x77; }
    else if (bmp.begin(0x76)) { bmpFound = true; bmpAddr = 0x76; }
    else { bmpFound = false; }

    if (bmpFound) applyBmpSampling();

    if (ahtFound || bmpFound) {
        LOG_INFO("Sensors OK");
    } else {
        LOG_WARNING("Sensors Missing");
    }

    return ahtFound || bmpFound;
}

// Lecture AHT20 (température + humidité) avec contrôle de succès et de plausibilité.
bool SensorManager::readAht(float& t, float& h) {
    sensors_event_t hum, temp;
    if (!aht.getEvent(&hum, &temp)) return false; // échec I2C signalé par la lib
    t = temp.temperature;
    h = hum.relative_humidity;
    if (isnan(t) || isnan(h)) return false;
    if (t <= -40.0f || t >= 85.0f) return false;
    if (h < 1.0f || h > 100.0f) return false; // 0 % => lecture douteuse (glitch I2C)
    return true;
}

// Lecture BMP280 (pression, en hPa) avec contrôle de plausibilité.
bool SensorManager::readBmp(float& p) {
    const float pa = bmp.readPressure(); // Pa
    if (isnan(pa) || pa <= 0.0f) return false;
    p = pa / 100.0f;
    if (p < 800.0f || p > 1200.0f) return false;
    return true;
}

// Réinitialise le bus I2C et les capteurs après une série d'échecs (bus coincé).
void SensorManager::recoverBus() {
    LOG_WARNING("Sensor: I2C bus recovery");
    Wire.end();
    delay(10);
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(I2C_FREQ_HZ);
    Wire.setTimeOut(I2C_TIMEOUT_MS);
    if (ahtFound) aht.begin();
    if (bmpFound && bmp.begin(bmpAddr)) applyBmpSampling();
    _consecutiveFailures = 0;
}

SensorData SensorManager::read() {
    float t = NAN, h = NAN, p = NAN;
    bool ahtOk = false, bmpOk = false;

    // Plusieurs tentatives : la plupart des erreurs I2C sont transitoires.
    for (int attempt = 0; attempt < SENSOR_READ_RETRIES; ++attempt) {
        if (ahtFound && !ahtOk) ahtOk = readAht(t, h);
        if (bmpFound && !bmpOk) bmpOk = readBmp(p);
        if ((!ahtFound || ahtOk) && (!bmpFound || bmpOk)) break;
        delay(20);
    }

    // Repli température sur le BMP280 si l'AHT20 est absent.
    if (!ahtFound && bmpOk) {
        const float bt = bmp.readTemperature();
        if (!isnan(bt) && bt > -40.0f && bt < 85.0f) { t = bt; ahtOk = true; }
    }

    // La mesure n'est valide que si tous les capteurs présents ont répondu.
    const bool ok = (ahtFound ? ahtOk : true) && (bmpFound ? bmpOk : true) && (ahtFound || bmpFound);

    SensorData data;
    if (ok) {
        data.temperature = t;
        data.humidity = ahtFound ? h : 0.0f; // pas d'humidité sans AHT20
        data.pressure = bmpFound ? p : 0.0f;
        data.valid = true;
        _lastTemp = data.temperature;
        _lastHum = data.humidity;
        _lastPres = data.pressure;
        _hasLast = true;
        _consecutiveFailures = 0;
    } else {
        // Lecture échouée : on renvoie la dernière valeur connue (pour l'affichage
        // temps réel) mais avec valid=false, afin que l'historique n'enregistre rien.
        data.temperature = _hasLast ? _lastTemp : 0.0f;
        data.humidity = _hasLast ? _lastHum : 0.0f;
        data.pressure = _hasLast ? _lastPres : 0.0f;
        data.valid = false;
        _consecutiveFailures++;
        LOG_WARNING("Sensor read failed (" + std::to_string(_consecutiveFailures) + ")");
        if (_consecutiveFailures >= SENSOR_RECOVER_AFTER) recoverBus();
    }

    return data;
}

//
