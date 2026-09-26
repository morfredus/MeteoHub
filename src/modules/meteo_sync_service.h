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
        bool plausibleTs = false; // true = heure de mesure exploitable (archivable)
        int64_t measurementTs = 0;// heure de MESURE reconstruite (epoch s)
        SyncControl reply{};      // accuse cumulatif + trou a combler (a renvoyer)
        bool haveReply = false;   // false si l'etat ne permet pas encore de repondre
        bool counterRestart = false; // la sonde a repris sa numerotation (etat remis a zero)
        uint32_t previousMax = 0;    // plus grand seq connu avant la remise a zero
    };

    void begin() {
        _prefs.begin("mhsync", /*readOnly=*/false);
        for (uint8_t i = 0; i < HUB_MAX_NODES; i++) loadNode(i);
        _haveAssoc = _prefs.getBytesLength("assoc") == 6
                  && _prefs.getBytes("assoc", _assoc, 6) == 6;
    }

    // --- Sonde associee ------------------------------------------------------
    // La sonde qui a CONFIRME un appairage avec ce hub. Une fois connue, le hub
    // n'archive plus qu'elle : une sonde d'etabli (tests, flash) a portee ne
    // peut plus melanger ses mesures a celles de dehors. Tant qu'aucune sonde
    // n'a confirme d'appairage (hub neuf, sonde ancienne), toutes sont acceptees.
    bool hasAssociated() const { return _haveAssoc; }
    const uint8_t* associatedMac() const { return _assoc; }
    bool isAccepted(const uint8_t* mac) const {
        return !_haveAssoc || memcmp(mac, _assoc, 6) == 0;
    }
    void setAssociated(const uint8_t* mac) {
        if (_haveAssoc && memcmp(mac, _assoc, 6) == 0) return;
        memcpy(_assoc, mac, 6);
        _haveAssoc = true;
        _prefs.putBytes("assoc", _assoc, 6);
    }

    // Traite une trame recue de la sonde `mac`. `nowReal` = heure reelle du hub.
    // Le suivi est tenu PAR MAC : deux sondes partageant un node_id ne se
    // melangent plus. `nodeId` n'est repris que dans la reponse.
    Decision onPacket(const uint8_t* mac, uint8_t nodeId, uint32_t seq, uint32_t sensorTs,
                      uint8_t frameType, uint32_t oldestSeq, int64_t nowReal) {
        Node& n = node(mac);
        Decision d;
        d.isLive = (frameType == FRAME_LIVE);

        // 0) Sonde repartie de seq=1 (NVS effacee, carte remplacee) : l'ancien
        //    suivi ne veut plus rien dire. On repart d'un etat vierge pour ce
        //    node, sinon ses nouvelles mesures seraient toutes vues comme des
        //    doublons (perdues ET accusees) pendant des jours.
        if (d.isLive && n.tracker.isCounterRestart(seq)) {
            d.counterRestart = true;
            d.previousMax = n.tracker.maxSeen();
            n.tracker = SyncTracker();
            n.anchor = TimeAnchor();
        }

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

        // 4) Dedup ET couplage a l'ARCHIVABILITE. On ne compte une trame comme
        //    « recue » (avancement de l'accuse cumulatif) QUE si son heure de mesure
        //    est exploitable. Sinon (heure non reconstructible), on la LAISSE en trou :
        //    l'accuse n'avance pas, la sonde continue de la retransmettre plus tard
        //    (quand elle aura une heure fiable), plutot que de la marquer synced alors
        //    qu'on n'a pas pu l'archiver -> plus jamais de faux ACK avec perte.
        d.plausibleTs = d.measurementTs > (int64_t)kEpochPlausible;
        d.archive = d.plausibleTs && n.tracker.markReceived(seq);

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

    uint32_t ackContiguousOf(const uint8_t* mac) { return node(mac).tracker.ackContiguous(); }

    // Appairage : la sonde vient de choisir CE hub. `baseSeq` = dernier seq accuse
    // par son ancien hub. Tout ce qui est <= baseSeq a ete livre ailleurs : on ne
    // le reclame pas (sinon on attendrait 30 jours d'historique que la sonde ne
    // renverra jamais, puisqu'elle les sait deja livres). On ne reclame donc que
    // ce qui est encore en attente cote sonde. N'a d'effet que vers l'avant :
    // re-appairer le MEME hub ne lui fait rien oublier.
    void adoptBaseline(const uint8_t* mac, uint32_t baseSeq) {
        Node& n = node(mac);
        n.tracker.noteSensorOldest(baseSeq + 1);
        n.dirty = true;
    }

private:
    struct Node {
        bool used = false;
        bool dirty = false;
        uint8_t mac[6] = {0, 0, 0, 0, 0, 0};
        SyncTracker tracker;
        TimeAnchor anchor;
    };

    Node& node(const uint8_t* mac) {
        // Cherche un slot existant pour cette sonde.
        for (uint8_t i = 0; i < HUB_MAX_NODES; i++)
            if (_nodes[i].used && memcmp(_nodes[i].mac, mac, 6) == 0) return _nodes[i];
        // Sinon, prend un slot libre.
        for (uint8_t i = 0; i < HUB_MAX_NODES; i++) {
            if (!_nodes[i].used) {
                _nodes[i] = Node();
                _nodes[i].used = true;
                memcpy(_nodes[i].mac, mac, 6);
                return _nodes[i];
            }
        }
        // Parc plein (jamais en pratique) : reutilise le slot 0, remis a neuf.
        _nodes[0] = Node();
        _nodes[0].used = true;
        memcpy(_nodes[0].mac, mac, 6);
        return _nodes[0];
    }

    // --- Persistance NVS : cle "n<slot>" -> blob {magic, mac, ack, ahead[]} ------
    // Format 2 (1.47.0) : la sonde est identifiee par sa MAC. Un blob de l'ancien
    // format (identifie par node_id, sans magic) est ignore : il melangeait
    // potentiellement plusieurs sondes, le suivi repart proprement.
    static constexpr uint32_t kNodeMagic = 0x3243414Du; // 'MAC2'
    static constexpr uint32_t kHeadWords = 4;           // magic, mac(2 mots), ack

    void keyFor(uint8_t slot, char* out, size_t sz) { snprintf(out, sz, "n%u", (unsigned)slot); }

    void loadNode(uint8_t slot) {
        char key[8]; keyFor(slot, key, sizeof(key));
        const size_t len = _prefs.getBytesLength(key);
        if (len < sizeof(uint32_t) * kHeadWords) return; // rien de valide persiste
        static uint32_t w[kHeadWords + HUB_PERSIST_AHEAD_MAX];
        const size_t got = _prefs.getBytes(key, w, sizeof(w));
        if (got < sizeof(uint32_t) * kHeadWords || w[0] != kNodeMagic) return;
        uint8_t mac[6];
        memcpy(mac, &w[1], 6);
        SyncTracker::State st;
        st.ackContiguous = w[3];
        const uint32_t count = (uint32_t)(got / sizeof(uint32_t)) - kHeadWords;
        for (uint32_t i = 0; i < count; i++) st.ahead.push_back(w[kHeadWords + i]);
        _nodes[slot].used = true;
        memcpy(_nodes[slot].mac, mac, 6);
        _nodes[slot].tracker.importState(st);
    }

    void saveNode(uint8_t slot) {
        char key[8]; keyFor(slot, key, sizeof(key));
        const SyncTracker::State st = _nodes[slot].tracker.exportState();
        static uint32_t w[kHeadWords + HUB_PERSIST_AHEAD_MAX];
        w[0] = kNodeMagic;
        w[1] = 0; w[2] = 0;
        memcpy(&w[1], _nodes[slot].mac, 6);
        w[3] = st.ackContiguous;
        uint32_t count = st.ahead.size();
        if (count > HUB_PERSIST_AHEAD_MAX) count = HUB_PERSIST_AHEAD_MAX;
        for (uint32_t i = 0; i < count; i++) w[kHeadWords + i] = st.ahead[i];
        _prefs.putBytes(key, w, (kHeadWords + count) * sizeof(uint32_t));
    }

    Preferences _prefs;
    Node _nodes[HUB_MAX_NODES];
    uint8_t _assoc[6] = {0, 0, 0, 0, 0, 0};
    bool _haveAssoc = false;
};

} // namespace mhsync
