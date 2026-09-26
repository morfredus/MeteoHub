#pragma once
#include <Arduino.h>
#include <time.h>
#include "meteo_context.h"

class SdManager;

// -----------------------------------------------------------------------------
// OutdoorQuarantine : mesures extérieures REÇUES et ACCUSÉES, mais tenues hors de
// l'historique météo parce que leur contexte les rend suspectes.
//
// Principe (spécification de Fred, 2026-09-26) : la sonde mesure ce qu'elle
// mesure, sans juger ; la qualification se fait plus haut. Le hub ne DÉTRUIT
// rien : une mesure écartée est accusée (la sonde peut la retirer de son buffer
// sans perte, la chaîne d'accusé reste intacte) ET conservée ici, brute, avec
// la raison de l'écart. morfAnalytics pourra l'examiner plus tard.
//
// Premier motif : « cold_boot », la 1re mesure LIVE après un démarrage à froid
// de la sonde (reset autre que la sortie de veille, wake <= 1) : après un flash
// ou un passage sur l'USB, la carte restée éveillée a chauffé ses capteurs.
//
// Stockage : CSV en ajout seul sur la SD (/history/outdoor_quarantine.csv). Sans
// SD, repli borné sur LittleFS (quelques dizaines de Ko) : ces mesures sont
// rares, et un repli illimité mettrait en danger la petite flash de la Super Mini.
// -----------------------------------------------------------------------------
class OutdoorQuarantine {
public:
    void begin(SdManager* sd) { _sd = sd; }

    // Conserve la mesure. `mac` = MAC de la sonde, `reason` = motif court.
    bool add(const OutdoorData& o, time_t measurementTs, const char* reason);

    // Chemin du fichier à servir (SD si présent, sinon repli LittleFS), "" si aucun.
    // `onSd` indique le système de fichiers.
    const char* currentPath(bool& onSd);

    uint32_t countSinceBoot() const { return _count; }

private:
    SdManager* _sd = nullptr;
    uint32_t _count = 0;
};

extern OutdoorQuarantine outdoorQuarantine;
