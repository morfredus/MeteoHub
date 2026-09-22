#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "meteo_packet.h"
#include "sync/sync_tracker.h"
#include "sync/time_anchor.h"

// ============================================================================
// MeteoSyncService (hub) - suivi de reception, dedup, horodatage, voie inverse
// ============================================================================
// Reunit la logique de synchronisation cote hub, par sonde (node_id) :
//   - SyncTracker : accuse cumulatif + detection de trous + dedup + anti-blocage ;
//   - TimeAnchor  : reconstruction de l'heure de MESURE (vs heure d'arrivee) ;
//   - persistance NVS : l'etat survit a un reboot du hub (reprise sans
//     intervention, sans re-archiver ce qui l'etait deja).
//
// Le service DECIDE (archiver ou non, en live ou en historique, a quelle heure)
// et FABRIQUE la reponse SyncControl ; l'appelant (main) applique la decision a
// l'historique et delegue l'envoi de la reponse au recepteur ESP-NOW.
namespace mhsync {

// Seuil de plausibilite d'une heure Unix reelle (~2020-09) : en dessous, une
// horloge n'est pas synchronisee. Sert a distinguer un sensor_ts absolu (sonde a
// l'heure) d'une horloge relative, et a ne transmettre l'epoch que si le hub est
// reellement synchronise NTP.
constexpr uint32_t kEpochPlausible = 1600000000u;

// Nombre de mesures que le hub invite la sonde a renvoyer par cycle (indice ;
// la sonde applique en plus son propre plafond d'autonomie).
constexpr uint16_t HUB_SYNC_WANT_COUNT = 10;
// Nombre max de sondes suivies simultanement (le parc n'en a qu'une, marge).
constexpr uint8_t HUB_MAX_NODES = 4;
// Bornes de persistance de l'ensemble « en avance » (au-dela, on accepte un
// eventuel re-archivage rare apres reboot plutot qu'un blob NVS demesure).
constexpr uint16_t HUB_PERSIST_AHEAD_MAX = 256;

class MeteoSyncService {
public:
    struct Decision {
        bool archive = false;     // true = mesure nouvelle a archiver (sinon doublon)
        bool isLive = false;      // true = trame LIVE (met a jour l'affichage live)
        int64_t measurementTs = 0;// heure de MESURE reconstruite (epoch s)
        SyncControl reply{};      // accuse cumulatif + trou a combler (a renvoyer)
        bool haveReply = false;   // false si l'etat ne permet pas encore de repondre
    };

    void begin() {
        _prefs.begin("mhsync", /*readOnly=*/false);
        for (uint8_t i = 0; i < HUB_MAX_NODES; i++) loadNode(i);
    }

    // Traite une trame recue. `nowReal` = heure reelle du hub (time(NULL)).
    Decision onPacket(uint8_t nodeId, uint32_t seq, uint32_t sensorTs,
                      uint8_t frameType, uint32_t oldestSeq, int64_t nowReal) {
        Node& n = node(nodeId);
        Decision d;
        d.isLive = (frameType == FRAME_LIVE);

        // 1) Une trame LIVE ancre l'horloge (elle vaut « maintenant »). Une
        //    retransmission NE touche PAS l'ancre (elle porte un vieux sensor_ts).
        if (d.isLive) n.anchor.updateFromLive(sensorTs, nowReal);

        // 2) Ce que la sonde peut ENCORE fournir : avance l'accuse au-dela d'un
        //    trou definitivement perdu (mesure sortie du buffer de 30 j).
        n.tracker.noteSensorOldest(oldestSeq);

        // 3) Heure de MESURE. Si la sonde est a l'heure (recalee sur l'epoch du
        //    hub), sensor_ts EST deja une heure Unix absolue : on la prend telle
        //    quelle (exacte meme apres une coupure d'alimentation). Sinon (sonde
        //    pas encore recalee), sensor_ts est une horloge relative : on
        //    reconstruit via l'ancre. Une valeur reconstruite aberrante sera
        //    ecartee plus loin par le garde de plausibilite de l'archivage.
        if (sensorTs > kEpochPlausible) {
            d.measurementTs = (int64_t)sensorTs;
        } else {
            d.measurementTs = n.anchor.has() ? n.anchor.reconstruct(sensorTs) : nowReal;
        }

        // 4) Dedup : nouveau seq -> a archiver ; deja connu -> doublon (ignore).
        d.archive = n.tracker.markReceived(seq);

        // 5) Reponse : accuse cumulatif + plus bas trou a combler.
        d.reply.node_id = nodeId;
        d.reply.ack_seq = n.tracker.ackContiguous();
        d.reply.want_from_seq = n.tracker.firstMissing();
        d.reply.want_count = n.tracker.hasGap() ? HUB_SYNC_WANT_COUNT : 0;
        // Heure reelle du hub (NTP) transportee vers la sonde pour recaler son
        // horloge. 0 si le hub n'est pas encore synchronise (heure implausible).
        d.reply.hub_epoch = (nowReal > (int64_t)kEpochPlausible) ? (uint32_t)nowReal : 0;
        d.haveReply = true;

        n.dirty = true;
        return d;
    }

    // Persiste l'etat des sondes modifiees (a appeler apres avoir draine la file
    // de reception : au plus une salve d'ecritures NVS par cycle de traitement).
    void persistDirty() {
        for (uint8_t i = 0; i < HUB_MAX_NODES; i++) {
            if (_nodes[i].used && _nodes[i].dirty) {
                saveNode(i);
                _nodes[i].dirty = false;
            }
        }
    }

    uint32_t ackContiguousOf(uint8_t nodeId) { return node(nodeId).tracker.ackContiguous(); }

private:
    struct Node {
        bool used = false;
        bool dirty = false;
        uint8_t id = 0;
        SyncTracker tracker;
        TimeAnchor anchor;
    };

    Node& node(uint8_t nodeId) {
        // Cherche un slot existant pour ce node_id.
        for (uint8_t i = 0; i < HUB_MAX_NODES; i++)
            if (_nodes[i].used && _nodes[i].id == nodeId) return _nodes[i];
        // Sinon, prend un slot libre.
        for (uint8_t i = 0; i < HUB_MAX_NODES; i++) {
            if (!_nodes[i].used) { _nodes[i].used = true; _nodes[i].id = nodeId; return _nodes[i]; }
        }
        // Parc plein (jamais en pratique) : reutilise le slot 0.
        return _nodes[0];
    }

    // --- Persistance NVS : cle "n<slot>" -> blob {ack, count, seqs[]} --------
    void keyFor(uint8_t slot, char* out, size_t sz) { snprintf(out, sz, "n%u", (unsigned)slot); }

    void loadNode(uint8_t slot) {
        char key[8]; keyFor(slot, key, sizeof(key));
        const size_t len = _prefs.getBytesLength(key);
        if (len < sizeof(uint32_t) * 2) return; // rien de valide persiste
        static uint8_t buf[sizeof(uint32_t) * (2 + HUB_PERSIST_AHEAD_MAX)];
        const size_t got = _prefs.getBytes(key, buf, sizeof(buf));
        if (got < sizeof(uint32_t) * 2) return;
        const uint32_t* w = reinterpret_cast<const uint32_t*>(buf);
        const uint32_t nodeId = w[0];
        SyncTracker::State st;
        st.ackContiguous = w[1];
        const uint32_t count = (got / sizeof(uint32_t)) >= 2 ? (got / sizeof(uint32_t)) - 2 : 0;
        for (uint32_t i = 0; i < count; i++) st.ahead.push_back(w[2 + i]);
        _nodes[slot].used = true;
        _nodes[slot].id = (uint8_t)nodeId;
        _nodes[slot].tracker.importState(st);
    }

    void saveNode(uint8_t slot) {
        char key[8]; keyFor(slot, key, sizeof(key));
        const SyncTracker::State st = _nodes[slot].tracker.exportState();
        static uint32_t w[2 + HUB_PERSIST_AHEAD_MAX];
        w[0] = _nodes[slot].id;
        w[1] = st.ackContiguous;
        uint32_t count = st.ahead.size();
        if (count > HUB_PERSIST_AHEAD_MAX) count = HUB_PERSIST_AHEAD_MAX;
        for (uint32_t i = 0; i < count; i++) w[2 + i] = st.ahead[i];
        _prefs.putBytes(key, w, (2 + count) * sizeof(uint32_t));
    }

    Preferences _prefs;
    Node _nodes[HUB_MAX_NODES];
};

} // namespace mhsync
