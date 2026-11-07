#pragma once
// Stockage SD des mesures par journee : /history/<flux>/AAAA/MM/AAAA-MM-JJ.bin (+ .stats pour
// l'IN). Une meme classe sert les deux flux, IN (interieur) et OUT (exterieur) : elle
// remplace les anciennes paires jumelles de HistoryManager (chemins, creation des dossiers,
// sauvegarde, liste des jours, export brut, lecture proche d'un instant).
//
// Les ecarts REELS de comportement entre IN et OUT ne sont pas gommes : ils sont portes par
// DayStorePolicy, un champ commente par ecart. Les deux instances ecrivent exactement les
// memes octets que le code d'origine (voir test_equivalence.cpp).
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "day_file_format.h"
#include "fs_port.h"
#include "history_types.h"

namespace mhhist {

using mhfs::Fs;
using mhfs::FilePtr;

struct DayStorePolicy {
    const char* baseDir;   // parent commun des deux flux ("/history")
    const char* root;      // racine du flux ("/history/indoor" ou "/history/outdoor")
    const char* tag;       // prefixe des messages de log ("Bin Save" / "Bin Save OUT")

    // Format d'un fichier NEUF. IN : v1 (16 octets). OUT : v2 (20 octets, avec marques) depuis
    // la 1.54.0 ; un fichier existant garde son format jusqu'au bout.
    bool freshWithFlags;
    // Fichier existant sans en-tete (v1.4.0). IN : converti une fois (reecriture complete).
    // OUT : jamais converti, les journees anterieures a la 1.54.0 restent en v1 brut.
    bool upgradeLegacy;
    // En-tete du fichier. IN : recordCount / firstTimestamp / lastTimestamp tenus a jour apres
    // chaque ajout (ecriture "r+", best effort). OUT : laisses a 0, l'index des journees relit
    // les enregistrements.
    bool maintainHeader;
    // Statistiques du jour (.stats). IN seulement : l'OUT n'en a jamais ecrit ni lu.
    bool maintainStats;
    // Echec d'ouverture en ajout d'un fichier neuf : IN remonte la carte et retente une fois.
    bool retryOpenOnFresh;
    // Journalisation. OUT : un avertissement par dossier impossible a creer et un log INFO par
    // ecriture reussie ; IN : silencieux. Les echecs d'ecriture sont des ERREURS pour l'IN.
    bool logDirErrors;
    const char* okMessage;         // nullptr = rien
    const char* okFlaggedMessage;  // idem, pour une mesure marquee
    bool failuresAreErrors;
    // Ordre des enregistrements. IN : chronologique (recherche par dichotomie). OUT : une
    // mesure retransmise est ajoutee en fin de fichier avec son heure d'origine, donc lecture
    // complete puis tri.
    bool chronological;
    // Repli de lecture sur les anciens CSV plats <dir>/AAAA-MM-JJ.csv. IN seulement
    // ("/history") ; nullptr pour l'OUT, qui n'a jamais eu ce format.
    const char* legacyCsvDir;
};

// Politiques des deux flux (definies une seule fois, voir day_store.cpp).
extern const DayStorePolicy kIndoorPolicy;
extern const DayStorePolicy kOutdoorPolicy;

class DayStore {
public:
    DayStore(Fs& fs, const DayStorePolicy& policy) : _fs(fs), _p(policy) {}

    const DayStorePolicy& policy() const { return _p; }
    Fs& fs() const { return _fs; }

    // AAAAMMJJ d'une date locale.
    static uint32_t dayKey(const struct tm& tinfo);
    // Chemins du .bin et du .stats du jour de `tinfo`.
    void dayPaths(const struct tm& tinfo, char* binPath, char* statsPath, size_t sz) const;
    // Cree <baseDir>, <root>, <root>/AAAA et <root>/AAAA/MM s'ils manquent.
    bool ensureDirs(const struct tm& tinfo) const;

    // Ajoute une mesure au fichier du jour de `ts` (heure locale), en ajout seul. `flags` :
    // marques (OUT, format v2 uniquement ; false si le fichier du jour est en v1). Met a jour
    // en-tete, .stats et liste des jours selon la politique. false = non archivee.
    bool append(time_t ts, const BinRecord& rec, uint32_t flags);

    // .stats du jour de `day_ts` (relit le temporaire complet si la coupure est tombee entre
    // remove et rename). false si absent, illisible ou de mauvais magic.
    bool readDayStats(time_t day_ts, DayStats& out) const;

    // Mesure la plus proche de target_ts (tolerance 30 min) : .bin du jour puis, pour l'IN, CSV
    // plat legacy. Monte la carte au besoin.
    bool sampleNear(time_t target_ts, float& t_out, float& h_out, float& p_out) const;

    // Journees presentes, triees. Liste construite UNE fois (un open par fichier : ~150 jours =
    // plusieurs secondes dans async_tcp), tenue a jour a chaque ecriture, invalidee par un
    // effacement ou un fichier realigne.
    std::vector<DayIndexEntry> listDays() const;

    // Emet les enregistrements [from_index, from_index + limit) du jour (AAAAMMJJ) et renvoie le
    // nombre total d'enregistrements du jour. Export BRUT : marques comprises, positions
    // fidelles au fichier (la reprise de morfAnalytics en depend).
    uint32_t exportRaw(uint32_t day_key, uint32_t from_index, uint32_t limit,
                       const std::function<void(const RawRecord&)>& emit) const;

    void invalidateDaysCache();
    // Oublie le cache du jour courant : le prochain ajout relit le .stats.
    void resetStatsCache();

private:
    Fs& _fs;
    DayStorePolicy _p;

    // Cache du jour courant (evite de relire le .stats a chaque mesure). 0 = non initialise.
    DayStats _currentDayStats;
    uint32_t _currentDayKey = 0;
    void updateDayStats(time_t ts, const BinRecord& rec, const struct tm& tinfo);

    bool binSampleNear(time_t target_ts, float& t_out, float& h_out, float& p_out) const;
    bool csvSampleNear(time_t target_ts, float& t_out, float& h_out, float& p_out) const;
    std::vector<DayIndexEntry> scanDays() const;

    mutable std::mutex _daysMutex;
    struct DaysCache { bool valid = false; std::vector<DayIndexEntry> list; };
    mutable DaysCache _days;
    mutable uint32_t _daysGen = 0;  // change a chaque ecriture : evite de memoriser un scan perime
    void noteDayWritten(const struct tm& tinfo, uint32_t ts, bool invalidate);
};

// Ecrit `len` octets dans un temporaire puis le substitue a `path`. Une coupure en cours
// d'ecriture laisse l'ancien contenu intact (le .tmp incomplet est ignore). FAT : rename ne
// remplace pas une cible existante, d'ou le remove avant le rename ; si la coupure tombe entre
// les deux, le .tmp COMPLET reste et readDayStats() le relit.
bool writeFileReplacing(Fs& fs, const char* path, const void* data, size_t len);

// Convertit un ancien .bin sans en-tete (v1.4.0) en fichier avec en-tete, une seule fois
// (reecriture complete via un temporaire). false si la conversion a echoue ; l'original n'est
// alors jamais perdu.
bool upgradeLegacyBin(Fs& fs, const char* binPath, FileHeader& outHdr);

} // namespace mhhist
