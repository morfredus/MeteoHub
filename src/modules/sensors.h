#pragma once
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include "meteo_context.h"

// SensorData devient explicitement IndoorData (mesures capteurs locaux IN)
// Maintenu pour compatibilité mais équivalent à IndoorData
struct SensorData {
    float temperature;
    float humidity;
    float pressure;
    bool valid;
    
    // Conversion implicite vers IndoorData
    operator IndoorData() const {
        IndoorData in;
        in.temperature = temperature;
        in.humidity = humidity;
        in.pressure = pressure;
        in.valid = valid;
        return in;
    }
};

class SensorManager {
public:
    bool begin();

    // Acquiert une mesure (lecture I2C) et met à jour le cache interne (dernière
    // valeur valide, réutilisée en repli si une lecture échoue). Appelé aussi bien
    // par le cycle d'archivage (cadence 5 min) que par l'affichage IN en direct :
    // une lecture ne crée jamais d'entrée d'historique (l'archivage est ailleurs).
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
