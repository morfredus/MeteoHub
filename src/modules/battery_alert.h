#pragma once
#include <Arduino.h>
#include <string>
#include "battery_alert_logic.h"

// -----------------------------------------------------------------------------
// BatteryAlert : prévient (via morfNotify) que l'accu de la sonde extérieure doit
// être changé, AVANT la coupure.
//
// La décision (paliers en tension, anti-rebond, hystérésis) est dans
// battery_alert_logic.h, testée sur l'hôte. Ce module ajoute ce qui touche au
// matériel :
//   - la persistance NVS du palier déjà notifié (un redémarrage du hub ne renvoie
//     pas l'alerte) ;
//   - l'envoi HTTP à morfNotify, trouvé par sa capacité `notification` (morfBeacon).
//
// Quand envoyer : juste APRÈS une trame live. La sonde vient alors de repartir
// en sommeil pour ~5 min ; un POST qui bloque la boucle quelques secondes ne
// peut pas lui faire manquer la fenêtre de réponse ESP-NOW (~300 ms). Un envoi
// raté est retenté à la trame suivante : l'alerte est retardée, jamais perdue.
//
// Sans morfNotify sur le réseau (ou MORF_ECOSYSTEM_ENABLED = 0), rien ne part :
// l'OLED et la page web signalent toujours l'accu faible, MeteoHub reste autonome.
// -----------------------------------------------------------------------------
class BatteryAlert {
public:
    BatteryAlert();
    void begin();                        // relit le palier notifié en NVS

    // Tension d'une trame LIVE de la sonde (jamais une retransmission).
    void onLiveReading(float volts);

    // À appeler dans loop() : tente l'envoi dû, si morfNotify est connu.
    void update(const std::string& notifyUrl);

    mhbat::Level level() const { return _logic.observed(); }
    bool isPending() const { return _logic.hasPending(); }
    bool hasReading() const { return _logic.hasReading(); }

private:
    bool send(const std::string& url);
    void persistNotified();

    mhbat::BatteryAlertLogic _logic;
    unsigned long _lastLiveMs = 0;       // 0 = aucune trame live depuis le boot
    bool _attemptedSinceLive = false;    // une tentative par trame live, pas plus
};

extern BatteryAlert batteryAlert;
