#pragma once
#include "sensor_calibration.h"

// -----------------------------------------------------------------------------
// Persistance des calibrations en NVS : capteur intérieur (AHT20 du hub) et
// sonde extérieure. Un seul propriétaire pour l'espace NVS, deux consommateurs
// (SensorManager pour l'intérieur, EspNowReceiver pour l'extérieur).
//
// La sonde extérieure n'est PAS calibrée chez elle : elle transmet des mesures
// brutes et n'a aucune interface de réglage. Corriger à la réception, sur le
// hub, évite un reflashage et garde les deux réglages au même endroit.
// -----------------------------------------------------------------------------

struct CalibrationSet {
    mhcal::Offsets indoor;
    mhcal::Offsets outdoor;
};

// Lit les décalages (0 / 0 pour un hub jamais calibré), bornés.
CalibrationSet loadCalibration();

// Borne puis enregistre les décalages. Renvoie les valeurs réellement retenues.
CalibrationSet saveCalibration(CalibrationSet set);
