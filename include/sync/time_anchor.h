#pragma once
#include <cstdint>

// ============================================================================
// TimeAnchor - reconstruction de l'HEURE DE MESURE (et non d'arrivee)
// ============================================================================
// La sonde n'a ni RTC ni NTP : elle ne connait pas l'heure murale. Elle horodate
// chaque mesure avec sensor_ts, une horloge RELATIVE monotone (secondes cumulees,
// persistee, survivant deep sleep et coupure). Le hub, lui, connait l'heure
// reelle. A chaque trame LIVE (fraiche), il ancre : « sensor_ts = A correspond a
// l'heure reelle R ». Il reconstruit alors l'heure de mesure de N'IMPORTE quelle
// trame (y compris une retransmission vieille de plusieurs heures) :
//
//     mesure_reelle = R + (record_sensor_ts - A)
//
// Ainsi une mesure prise a 17h20 mais retransmise a 19h00 est archivee a 17h20.
// L'ancre est rafraichie a chaque trame LIVE : la reconstruction d'une trame
// recente reste juste a un intervalle pres, malgre la derive de l'horloge sonde.
//
// Logique pure -> testable en natif.
namespace mhsync {

class TimeAnchor {
public:
    // Met a jour l'ancre depuis une trame LIVE (sensor_ts <-> heure reelle).
    // A n'appeler QUE sur une trame live : une retransmission porte un vieux
    // sensor_ts et fausserait l'ancre.
    void updateFromLive(uint32_t sensorTs, int64_t realTs) {
        _anchorSensorTs = sensorTs;
        _anchorRealTs = realTs;
        _has = true;
    }

    bool has() const { return _has; }

    // Reconstruit l'heure de mesure reelle d'un enregistrement a partir de son
    // sensor_ts. Arithmetique SIGNEE : un record plus ancien que l'ancre donne un
    // delta negatif (heure anterieure), ce qui est correct.
    int64_t reconstruct(uint32_t recordSensorTs) const {
        const int64_t delta = static_cast<int64_t>(recordSensorTs)
                            - static_cast<int64_t>(_anchorSensorTs);
        return _anchorRealTs + delta;
    }

private:
    uint32_t _anchorSensorTs = 0;
    int64_t _anchorRealTs = 0;
    bool _has = false;
};

} // namespace mhsync
