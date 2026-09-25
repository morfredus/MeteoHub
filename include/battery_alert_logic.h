#pragma once
#include <stdint.h>

// -----------------------------------------------------------------------------
// BatteryAlertLogic : décide QUAND prévenir que l'accu de la sonde doit être
// changé. Logique pure (ni Arduino, ni réseau, ni NVS) : testée sur l'hôte
// (pio test -e native), l'envoi réel vit dans modules/battery_alert.
//
// Pourquoi raisonner en TENSION et non en pourcentage : la sonde convertit sa
// tension en % de façon LINÉAIRE entre 2,6 V et 4,2 V. Or la courbe d'un Li-ion
// est plate puis s'effondre : 20 % « linéaire » = 2,9 V, l'accu est déjà quasi
// vide et la protection coupe à 2,4 V. Une alerte sur le % arriverait trop tard.
//
// Trois paliers :
//   OK       : rien à signaler ;
//   LOW      : tension <= warnV -> « à remplacer prochainement » ;
//   CRITICAL : tension <= critV -> « à remplacer maintenant ».
//
// Anti-rebond : un palier n'est franchi qu'après `confirm` lectures CONSÉCUTIVES
// sous le seuil (un appel de courant pendant l'émission peut creuser une lecture
// isolée). Hystérésis : on ne revient à OK (accu changé ou rechargé) qu'après
// `confirm` lectures au-dessus de rearmV, nettement plus haut que warnV : une
// tension qui oscille autour d'un seuil ne déclenche pas une rafale d'alertes.
//
// « Observé » vs « notifié » : le palier observé est l'état de l'accu ; le palier
// notifié est ce qui a déjà été ENVOYÉ (persisté par l'appelant). Une notification
// est due tant que les deux diffèrent, et n'est marquée faite qu'après un envoi
// réussi : un morfNotify absent retarde l'alerte, il ne la perd pas.
// -----------------------------------------------------------------------------

namespace mhbat {

enum Level : uint8_t {
    LEVEL_OK       = 0,
    LEVEL_LOW      = 1,
    LEVEL_CRITICAL = 2,
};

struct Thresholds {
    float   warnV;    // palier LOW
    float   critV;    // palier CRITICAL (< warnV)
    float   rearmV;   // retour à OK (> warnV)
    uint8_t confirm;  // lectures consécutives exigées pour changer de palier
};

class BatteryAlertLogic {
public:
    explicit BatteryAlertLogic(const Thresholds& t) : _t(t) {}

    // Au démarrage : reprend le palier déjà notifié (persisté). Le palier observé
    // part de la même valeur, sinon un simple redémarrage du hub renverrait la
    // même alerte.
    void restore(uint8_t notifiedLevel) {
        if (notifiedLevel > LEVEL_CRITICAL) notifiedLevel = LEVEL_OK;
        _notified = static_cast<Level>(notifiedLevel);
        _observed = _notified;
    }

    // Une lecture de tension (trame LIVE seulement : une retransmission rejoue une
    // tension passée, elle ne dit rien de l'accu aujourd'hui).
    void onReading(float volts) {
        _lastVoltage = volts;
        _hasReading  = true;

        _lowStreak   = (volts <= _t.warnV)  ? inc(_lowStreak)   : 0;
        _critStreak  = (volts <= _t.critV)  ? inc(_critStreak)  : 0;
        _rearmStreak = (volts >= _t.rearmV) ? inc(_rearmStreak) : 0;

        if (_critStreak >= _t.confirm) {
            _observed = LEVEL_CRITICAL;
        } else if (_lowStreak >= _t.confirm && _observed < LEVEL_LOW) {
            _observed = LEVEL_LOW;
        } else if (_rearmStreak >= _t.confirm) {
            _observed = LEVEL_OK;   // accu remplacé ou rechargé
        }
        // Entre warnV et rearmV : bande d'hystérésis, le palier ne bouge pas.
    }

    Level observed() const { return _observed; }
    Level notified() const { return _notified; }
    bool  hasReading() const { return _hasReading; }
    float lastVoltage() const { return _lastVoltage; }

    // Une notification est-elle due ? Aggravation (LOW puis CRITICAL) ou retour
    // à OK après une alerte (confirmation que l'accu a bien été changé).
    bool hasPending() const { return _observed != _notified; }

    // À appeler une fois la notification ACCEPTÉE par morfNotify.
    void markNotified() { _notified = _observed; }

private:
    static uint8_t inc(uint8_t v) { return v < 255 ? static_cast<uint8_t>(v + 1) : v; }

    Thresholds _t;
    Level   _observed = LEVEL_OK;
    Level   _notified = LEVEL_OK;
    uint8_t _lowStreak = 0, _critStreak = 0, _rearmStreak = 0;
    float   _lastVoltage = 0.0f;
    bool    _hasReading = false;
};

} // namespace mhbat
