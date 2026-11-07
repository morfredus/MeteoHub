#pragma once
// Travaux de fond de l'historique : ils sont longs (suppression recursive de la carte, migration
// d'anciens fichiers) et ne doivent JAMAIS tourner d'un bloc dans la tache async_tcp (le
// watchdog de tache declenchait un reboot). Chacun avance par petits lots depuis loop().
#include <atomic>
#include <functional>

#include "day_store.h"
#include "fs_port.h"

namespace mhhist {

// Supprime au plus `budget` fichiers sous `path` (puis les dossiers devenus vides). Renvoie true
// quand `path` n'existe plus. Chaque niveau est lu PUIS referme avant de descendre : jamais deux
// iterateurs imbriques (voir Fs::listDir). Le budget borne les TENTATIVES ; `progress` (optionnel)
// compte les suppressions reellement reussies, pour detecter une carte qui refuse tout.
bool removeDirStep(Fs& fs, const char* path, int& budget, int* progress = nullptr);

// Suppression complete et synchrone (menu OLED, tache loop). Pour les requetes web, utiliser
// ClearJob : cette boucle est trop longue pour async_tcp.
void removeDirRecursive(Fs& fs, const char* path);

// Cree l'arborescence symetrique vide : <baseDir>, <inRoot> et <outRoot>.
void createHistoryTree(Fs& fs, const char* baseDir, const char* inRoot, const char* outRoot);

// Effacement de tout l'historique demande depuis le web. La route repond tout de suite (202) ;
// service(), appele depuis loop(), supprime la carte par lots de 20 fichiers par passage.
class ClearJob {
public:
    void request() { _pending = true; }
    bool pending() const { return _pending; }

    // Un passage de loop(). clearRam : vide les tampons RAM et les fichiers LittleFS (au premier
    // passage). sdAvailable : carte utilisable. onTreeRemoved : recree l'arborescence et vide les
    // caches une fois /history supprime.
    void service(Fs& fs, const char* historyDir, bool sdAvailable,
                 const std::function<void()>& clearRam,
                 const std::function<void()>& onTreeRemoved);

private:
    std::atomic<bool> _pending{false};
    bool _active = false;
    int _stuckPasses = 0;
};

// Migration unique des anciens CSV plats (<legacyCsvDir>/AAAA-MM-JJ.csv) vers le format binaire
// journalier de `inStore` (+ .stats). Chaque CSV migre est renomme en .bak ; un CSV dont la
// journee a deja un .bin, ou dont la conversion echoue, reste en place (jamais de perte).
// L'appelant s'assure que la carte est disponible.
void migrateCsvToBinary(DayStore& inStore);

} // namespace mhhist
