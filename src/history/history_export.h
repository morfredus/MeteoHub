#pragma once
// Export CSV REPRENABLE de l'historique (/api/history/export.csv). La reponse web est
// "chunked" : elle garde un curseur et rappelle exportCsvChunk() a chaque morceau demande par
// la socket. Rien n'est jamais accumule en RAM au-dela d'un jour, et la tache async_tcp ne
// parcourt jamais toute la plage d'un seul tenant. Le CSV reste le format d'export, adapte a
// Excel/LibreOffice.
//
// (L'export BRUT indexe par (jour, index) pour morfAnalytics est dans DayStore::exportRaw.)
#include <cstddef>
#include <ctime>
#include <string>
#include <vector>

#include "day_file_format.h"
#include "day_store.h"

namespace mhhist {

struct CsvCursor {
    bool outdoor = false;           // flux exporte (choisit le magasin cote gestionnaire)
    time_t from = 0, to = 0;
    time_t day = 0;                 // initialiser a `from`
    bool headerDone = false, done = false, dayLoaded = false;
    size_t index = 0;               // prochain enregistrement du jour charge
    std::string prevBin;            // evite un double parcours (bord d'heure d'ete)
    std::vector<BinRecord> dayRecs;
};

// Ecrit au plus maxLen octets dans buf et renvoie le nombre d'octets (0 = fin). Une ligne qui
// ne tient pas dans ce qui reste de buf ouvre l'appel suivant : jamais de ligne coupee.
size_t exportCsvChunk(const DayStore& store, CsvCursor& cursor, char* buf, size_t maxLen);

} // namespace mhhist
