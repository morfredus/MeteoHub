#pragma once
// Facade de l'historique des mesures. Elle ne garde que l'etat « live » OUT, les points
// d'entree publics (addIndoor / addOutdoor*), le verrou des tampons RAM et la delegation ; le
// travail est fait par les unites portables de src/history/ (testees sur l'hote) :
//   day_file_format  format des .bin journaliers (en-tete, enregistrements v1/v2, ajout aligne)
//   day_store        magasin SD d'un flux (instancie deux fois : IN et OUT)
//   recent_ring      tampon RAM + fichier LittleFS des derniers releves
//   history_query    plage agregee, stats 24 h, tendances, synthese
//   history_export   export CSV reprenable
//   history_jobs     effacement par lots, migration des anciens CSV
//   forecast_archive previsions archivees
#include <vector>
#include <string>
#include <functional>
#include <mutex>
#include <Arduino.h>
#include "sd_manager.h"
#include "arduino_fs.h"
#include "../modules/meteo_context.h"
#include "../history/history_types.h"
#include "../history/day_file_format.h"
#include "../history/day_store.h"
#include "../history/recent_ring.h"
#include "../history/history_query.h"
#include "../history/history_export.h"
#include "../history/history_jobs.h"

// Enregistrement de 16 octets d'un .bin journalier (ts, t, h, p), voir day_file_format.h.
using BinRecordPublic = mhhist::BinRecord;

class HistoryManager {
public:
    HistoryManager();
    void begin(SdManager* sd = nullptr);
    // Travaux de fond, appeles depuis loop() : effacement par lots (requestClear).
    void service();

    // Méthodes existantes (maintenues pour compatibilité, traitées comme IN)
    void add(float t, float h, float p);
    const std::vector<HistoryRecord>& getRecentHistory() const;
    Stats24h getRecentStats() const;

    // Nouvelles méthodes explicites IN/OUT
    void addIndoor(const IndoorData& data);

    // --- Synchronisation fiable (v3) : archivage horodate a l'heure de MESURE --
    // addOutdoorLive : trame courante reçue en direct. Met à jour la valeur
    //   « live » (affichage, fraîcheur) ET archive, à l'horodatage `measurementTs`
    //   (reconstruit par le hub, ≈ maintenant pour une trame live).
    // addOutdoorHistorical : mesure RETRANSMISE (ancienne). Archive UNIQUEMENT,
    //   à son heure de mesure d'origine, SANS toucher la valeur « live » (une
    //   vieille mesure ne doit pas se faire passer pour la mesure courante).
    // Les deux sont IDEMPOTENTES côté appelant (dedup par seq via MeteoSyncService).
    // Les deux renvoient true quand la mesure est ARCHIVEE (ou qu'il n'y a rien a
    // archiver) et false si l'archivage a echoue : l'appelant ne doit alors PAS
    // accuser la mesure a la sonde (elle la renverra).
    bool addOutdoorLive(const OutdoorData& data, time_t measurementTs);
    bool addOutdoorHistorical(const OutdoorData& data, time_t measurementTs);
    // Mesure suspecte archivée AVEC sa marque (cold_boot_rule.h), hors live et
    // hors stats. false si elle n'a pas pu l'être (l'appelant la met en quarantaine).
    bool addOutdoorFlagged(const OutdoorData& data, time_t measurementTs, uint32_t flags);
    // Dernière mesure extérieure archivée en direct (false si aucune).
    bool lastOutdoorRecord(OutdoorHistoryRecord& out) const;
    // Rafraîchit la valeur « live » (affichage + fraîcheur) SANS archiver : pour
    // une trame live déjà connue (doublon), rare mais possible.
    void refreshOutdoorLive(const OutdoorData& data);

    // Dernier relevé OUT vu (RAM), même si l'archivage a été sauté (NTP absent).
    bool hasLiveOutdoor() const { return _hasLiveOutdoor; }
    // Copie (verrou) : ecrit par loop(), lu aussi par les pages web.
    OutdoorData lastOutdoorLive() const;
    // Verrou des tampons RAM : a prendre pour iterer getRecentHistory() hors de loop().
    std::mutex& ramMutex() const { return _ramMutex; }

    // Vrai dès qu'au moins UNE vraie trame OUT a été reçue par radio depuis le
    // boot (jamais vrai pour une valeur seedée depuis le disque). Tant qu'il est
    // faux, l'OUT est « en attente de première réception » : rien n'est archivé
    // côté OUT, et l'affichage bascule sur l'absence (fallback IN de présentation).
    bool hasReceivedOutdoorSinceBoot() const { return _hasOutdoorRadioMs; }

    // Âge (ms) de la dernière trame OUT REÇUE par radio. Sert au résolveur de
    // lecture effective (fraîcheur/fallback). Renvoie une valeur volontairement
    // énorme si aucune trame n'a encore été reçue depuis le boot (une valeur
    // seedée depuis l'historique disque n'est donc jamais considérée fraîche).
    unsigned long outdoorAgeMs() const {
        if (!_hasOutdoorRadioMs) return 0xFFFFFFFFUL;
        return millis() - _lastOutdoorRadioMs;
    }

    Stats24h getIndoorStats() const;
    Stats24h getOutdoorStats() const;

    // Agrège les mesures sur une plage temporelle absolue [from, to] (secondes Unix),
    // en tranches de interval_s secondes. Les données sont lues en priorité depuis les
    // fichiers binaires journaliers de la carte SD (couvrant potentiellement plusieurs jours),
    // complétées par l'historique RAM pour la portion la plus récente non encore écrite
    // sur SD (ou l'intégralité si aucune carte SD n'est disponible).
    std::vector<HistoryPoint> queryRange(time_t from, time_t to, long interval_s) const;

    // Version OUT de queryRange (même cœur, racine /history/outdoor).
    std::vector<OutdoorHistoryPoint> queryOutdoorRange(time_t from, time_t to, long interval_s) const;

    // Synthèse pré-calculée d'une plage [from, to], agrégée depuis les fichiers
    // .stats journaliers (aucune relecture des mesures). Renvoie valid=false si
    // aucun .stats n'est disponible pour la plage.
    RangeSynthesis querySynthesis(time_t from, time_t to) const;

    // Export CSV REPRENABLE de la plage [from, to] (en-tete inclus), lu depuis les
    // fichiers binaires journaliers. L'appelant (reponse web chunked) garde un curseur
    // et rappelle exportCsvChunk() : rien n'est jamais accumule en RAM au-dela d'un jour.
    // Le CSV reste le format d'export, adapte a Excel/LibreOffice.
    using CsvCursor = mhhist::CsvCursor;
    // Ecrit au plus maxLen octets dans buf et renvoie le nombre d'octets (0 = fin).
    size_t exportCsvChunk(CsvCursor& cursor, char* buf, size_t maxLen) const;

    // --- Collecte incrémentale externe (morfAnalytics) -----------------------
    // Les fichiers journaliers sont écrits en AJOUT SEUL : l'index d'un
    // enregistrement dans son fichier ne change jamais. Le couple
    // (jour, index) forme donc un curseur strictement monotone, contrairement à
    // un horodatage, qui peut reculer ou se répéter lors d'un changement d'heure
    // ou d'un recalage NTP. Un collecteur mémorise (jour, index) et ne demande
    // que les enregistrements suivants - jamais de doublon, jamais de trou.

    // Journées présentes sur la carte SD, triées par date croissante.
    // Version par défaut = flux IN (legacy) ; version OUT = flux extérieur.
    std::vector<DayIndexEntry> listDays() const;
    std::vector<DayIndexEntry> listDaysOutdoor() const;

    // Émet les enregistrements [from_index, from_index + limit) du jour donné
    // (AAAAMMJJ). Renvoie le nombre total d'enregistrements du jour, ce qui
    // permet à l'appelant de savoir s'il reste des données à lire.
    // Version par défaut = IN (legacy) ; version OUT = flux extérieur.
    uint32_t exportRaw(uint32_t day_key, uint32_t from_index, uint32_t limit,
                       const std::function<void(const RawRecord&)>& emit) const;
    uint32_t exportRawOutdoor(uint32_t day_key, uint32_t from_index, uint32_t limit,
                              const std::function<void(const RawRecord&)>& emit) const;

    // --- Prévisions archivées (étape 9 : prévu vs observé) -------------------
    // Fige la prévision « day-ahead » sous son jour cible (fichier plat JSON).
    // Écrasé au fil de J-1 : le fichier du jour J finit avec la dernière prévision
    // émise en J-1 pour J, la comparaison de référence.
    void addForecast(const ForecastRecord& rec);
    // Émet le JSON brut de chaque prévision dont le jour cible tombe dans
    // [from, to] (streaming HTTP, sans tout charger en mémoire). Trié par jour.
    void forecastHistoryRaw(time_t from, time_t to,
                            const std::function<void(const char*)>& emit) const;

    // Gestion LittleFS
    // Effacement synchrone (menu OLED, tache loop).
    void clearHistory();
    // Effacement demande depuis le web : pose un drapeau, loop() fait le travail par
    // lots via service() (une suppression recursive dans async_tcp declenchait le watchdog).
    void requestClear();
    bool isClearing() const { return _clear.pending(); }
    // Tendance IN (confort) et OUT (météo). La tendance météo doit s'appuyer sur
    // l'extérieur : getTrendOutdoor() en est la source dédiée.
    MeteoTrend getTrend() const;
    MeteoTrend getTrendOutdoor() const;

private:
    SdManager* _sd = nullptr;

    // Dernier relevé OUT « live » + fraîcheur : écrits par loop(), lus par async_tcp.
    OutdoorData _lastOutdoorLive;
    bool _hasLiveOutdoor = false;
    unsigned long _lastOutdoorRadioMs = 0; // millis() de la dernière trame OUT reçue
    bool _hasOutdoorRadioMs = false;       // true dès la 1re trame radio (pas le seed disque)

    // Protege les deux tampons récents et _lastOutdoorLive : ecrits par loop(), lus par
    // async_tcp (pages web). Non recursif : ne jamais le prendre deux fois.
    mutable std::mutex _ramMutex;

    // L'ordre de declaration compte : chaque membre reference les precedents.
    // Acces fichiers (interface portable, voir src/history/fs_port.h). _sdFs est mutable :
    // simple passerelle sans etat, utilisee par des lectures const.
    mutable SdCardFs _sdFs;
    ArduinoFs _lfs;
    // Magasins SD des journees : meme classe, deux politiques (IN / OUT).
    mhhist::DayStore _in;
    mhhist::DayStore _out;
    // Derniers releves en RAM + fichier LittleFS associe. IN = flux historique (legacy).
    mhhist::InRecentRing _inRing;
    mhhist::OutRecentRing _outRing;
    // Vues lecture seule des tampons (sous verrou) pour les requetes.
    mhhist::RingSeries<mhhist::InRecentRing> _inSeries;
    mhhist::RingSeries<mhhist::OutRecentRing> _outSeries;
    // Effacement par lots demande par le web (voir requestClear / service).
    mhhist::ClearJob _clear;

    void loadRecent();
    void setOutdoorLive(const OutdoorData& data);
    bool saveToSdBinary(const HistoryRecord& record);
    bool saveOutdoorToSdBinary(const OutdoorHistoryRecord& record, uint32_t flags = 0);
    // Cœurs communs IN / OUT.
    MeteoTrend getTrendImpl(bool outdoor) const;
    std::vector<HistoryPoint> queryRangeImpl(time_t from, time_t to, long interval_s,
                                             bool outdoor) const;
    // Migration unique des anciens CSV plats (/history/AAAA-MM-JJ.csv) au démarrage.
    void migrateCsvToBinary();
    void createSdStructure();
    void clearRam();
    void invalidateDaysCache();
};
