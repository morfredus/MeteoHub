#pragma once
#include <Arduino.h>
#include <time.h>
#include <vector>
#include "battery_series.h"

class SdManager;

// -----------------------------------------------------------------------------
// BatteryLog : archive de l'etat de la batterie de la sonde, pour en suivre
// l'evolution (decharge, effet du froid, date de remplacement).
//
// Stockage : un CSV par mois sur la SD, /history/battery/AAAA-MM.csv, une ligne
// « ts,volts,pct » par mesure NOUVELLE de la sonde associee (les retransmissions y
// entrent a l'heure de leur mesure d'origine ; les doublons jamais). Volontairement
// SEPARE de l'historique meteo : son format binaire est lu par morfAnalytics, et la
// batterie n'est pas une mesure meteo. Sans SD, rien n'est archive (un avertissement,
// une seule fois) : la petite flash du hub n'est pas faite pour une serie qui grandit.
// -----------------------------------------------------------------------------
class BatteryLog {
public:
    void begin(SdManager* sd) { _sd = sd; }

    // Ajoute une mesure a l'heure de MESURE `ts`. false si non archivee.
    bool add(time_t ts, float volts, uint8_t pct);

    // Points agreges de [from, to], au plus ~400. `bucketSeconds` = duree d'un seau.
    std::vector<mhbat::Point> query(time_t from, time_t to, uint32_t& bucketSeconds);

private:
    SdManager* _sd = nullptr;
    bool _warnedNoSd = false;
};

extern BatteryLog batteryLog;
