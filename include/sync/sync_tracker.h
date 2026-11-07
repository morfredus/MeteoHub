#pragma once
#include <cstdint>
#include <vector>
#include <algorithm>

// ============================================================================
// SyncTracker - suivi cote hub des mesures recues par sonde
// ============================================================================
// Modele « TCP-like » : le hub tient, par sonde, le plus grand seq tel qu'il
// possede TOUT jusqu'a lui (ackContiguous). Les seq recus AU-DESSUS d'un trou
// sont memorises a part (ensemble « en avance ») jusqu'a ce que le trou se
// comble et fasse avancer l'accuse cumulatif.
//
// Roles :
//   - detecter les trous (le premier manquant est toujours ackContiguous+1) ;
//   - dedupliquer : markReceived renvoie false si le seq etait deja connu, ce qui
//     rend l'archivage idempotent (une retransmission ne cree pas de doublon) ;
//   - fournir l'accuse cumulatif renvoye a la sonde (SyncControl.ack_seq) ;
//   - anti-blocage : si la sonde signale que sa plus vieille mesure disponible
//     (oldest_seq) depasse le trou attendu, ce trou est PERDU (sorti du buffer de
//     30 j) : on avance l'accuse au-dela plutot que de le reclamer sans fin.
//
// Logique pure (aucune dependance Arduino) -> testable en natif. La persistance
// (survie au reboot du hub) est geree par l'appelant via exportState/importState.
namespace mhsync {

// Borne dure de l'ensemble « en avance ». Un trou durable ne doit pas faire
// enfler la memoire sans fin : au-dela, on considere le plus vieux trou perdu et
// on avance l'accuse (le buffer sonde de 30 j est de toute facon la vraie borne).
constexpr size_t MAX_AHEAD = 4096;

class SyncTracker {
public:
    // Enregistre la reception du seq. Renvoie true si c'est du NOUVEAU (a
    // archiver), false si deja connu (doublon -> ne pas ré-archiver).
    bool markReceived(uint32_t seq) {
        if (seq == 0) return false;                 // 0 = « pas de seq », ignore
        if (seq <= _ackContiguous) return false;    // deja couvert par l'accuse
        // Deja dans l'ensemble en avance ?
        auto it = std::lower_bound(_ahead.begin(), _ahead.end(), seq);
        if (it != _ahead.end() && *it == seq) return false; // doublon

        _lastMarked = seq;
        _lastPrevAck = _ackContiguous;
        _ahead.insert(it, seq);
        coalesce();
        enforceAheadBound();
        return true;
    }

    // Annule le DERNIER markReceived(seq) qui a renvoye true, quand la mesure n'a
    // finalement pas pu etre ecrite (carte SD en echec) : sans cela le seq resterait
    // « recu », accuse a la sonde, et la mesure serait perdue pour de bon. Le seq
    // redevient un trou ; la sonde le renverra. Renvoie false si seq n'est pas le
    // dernier marque (rien n'est alors modifie).
    bool unmarkReceived(uint32_t seq) {
        if (seq == 0 || seq != _lastMarked) return false;
        auto it = std::lower_bound(_ahead.begin(), _ahead.end(), seq);
        if (it != _ahead.end() && *it == seq) {
            _ahead.erase(it);               // seq etait reste au-dessus d'un trou
        } else if (seq > _lastPrevAck && seq <= _ackContiguous) {
            // seq a comble un trou et l'accuse a avance : restituer les seq qui
            // l'avaient suivi (ils etaient en avance), et reculer l'accuse.
            std::vector<uint32_t> restored;
            for (uint32_t s = seq + 1; s <= _ackContiguous; s++) restored.push_back(s);
            _ahead.insert(_ahead.begin(), restored.begin(), restored.end());
            _ackContiguous = _lastPrevAck;
        } else {
            return false;
        }
        _lastMarked = 0;
        return true;
    }

    // Prend en compte ce que la sonde peut ENCORE fournir. Si le prochain trou
    // (ackContiguous+1) est plus ancien que oldest_seq, il est irrecuperable :
    // on avance l'accuse jusqu'a oldest_seq-1. oldest_seq==0 (buffer vide) n'a
    // aucun effet.
    void noteSensorOldest(uint32_t oldestSeq) {
        if (oldestSeq == 0) return;
        if (oldestSeq > _ackContiguous + 1) {
            _ackContiguous = oldestSeq - 1;
            // Purge les seq en avance desormais couverts, puis re-coalesce.
            _ahead.erase(std::remove_if(_ahead.begin(), _ahead.end(),
                         [this](uint32_t s) { return s <= _ackContiguous; }),
                         _ahead.end());
            coalesce();
        }
    }

    uint32_t ackContiguous() const { return _ackContiguous; }

    // Plus grand seq jamais vu (accuse ou en avance). 0 = aucun historique.
    uint32_t maxSeen() const { return _ahead.empty() ? _ackContiguous : _ahead.back(); }

    // Une trame LIVE porte toujours la mesure la plus recente de la sonde : son
    // seq est >= a tout ce qu'elle a deja emis (une re-emission du meme live
    // repete le MEME seq, jamais un plus petit). Un live PLUS PETIT que le
    // maximum deja vu ne s'explique donc que par une sonde qui a REPRIS sa
    // numerotation a 1 (NVS effacee, carte remplacee). Sans cette detection, le
    // hub prendrait toutes ses nouvelles mesures pour des doublons (non archivees,
    // mais accusees) jusqu'a ce que la sonde depasse l'ancien maximum.
    bool isCounterRestart(uint32_t liveSeq) const {
        return liveSeq != 0 && liveSeq < maxSeen();
    }
    bool hasGap() const { return !_ahead.empty(); }
    // Premier seq manquant (celui a reclamer en priorite). 0 si aucun trou et
    // rien au-dela.
    uint32_t firstMissing() const {
        return _ahead.empty() ? 0 : (_ackContiguous + 1);
    }
    size_t aheadCount() const { return _ahead.size(); }

    // --- Persistance (survie au reboot hub) --------------------------------
    // Etat serialisable minimal : l'accuse cumulatif + les seq en avance. On peut
    // se contenter de l'accuse seul (les trous se re-decouvrent), mais garder
    // l'ensemble evite de re-reclamer ce qu'on avait deja au-dela d'un trou.
    struct State {
        uint32_t ackContiguous = 0;
        std::vector<uint32_t> ahead;
    };
    State exportState() const { return State{_ackContiguous, _ahead}; }
    void importState(const State& s) {
        _ackContiguous = s.ackContiguous;
        _ahead = s.ahead;
        std::sort(_ahead.begin(), _ahead.end());
        _ahead.erase(std::unique(_ahead.begin(), _ahead.end()), _ahead.end());
        _ahead.erase(std::remove_if(_ahead.begin(), _ahead.end(),
                     [this](uint32_t v) { return v <= _ackContiguous; }), _ahead.end());
        coalesce();
    }

private:
    // Fait avancer l'accuse tant que le seq juste au-dessus est present en avance.
    void coalesce() {
        while (!_ahead.empty() && _ahead.front() == _ackContiguous + 1) {
            _ackContiguous = _ahead.front();
            _ahead.erase(_ahead.begin());
        }
    }
    // Garde-fou memoire : si l'ensemble en avance explose (trou tres durable),
    // on abandonne le plus vieux trou et on avance l'accuse jusqu'au plus petit
    // seq en avance, puis on coalesce.
    void enforceAheadBound() {
        if (_ahead.size() <= MAX_AHEAD) return;
        // Le plus petit seq en avance devient le nouveau contigu-1.
        _ackContiguous = _ahead.front() - 1;
        coalesce();
    }

    uint32_t _ackContiguous = 0;
    uint32_t _lastMarked = 0;   // dernier seq accepte par markReceived (annulable)
    uint32_t _lastPrevAck = 0;  // accuse cumulatif juste avant ce markReceived
    std::vector<uint32_t> _ahead; // seq recus au-dessus du trou, tries croissants
};

} // namespace mhsync
