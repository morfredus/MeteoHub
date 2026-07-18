# [1.6.3] – 2026-07-18
### Fixed
- **Page Statistiques : minima toujours à 0 (séries de valeurs aberrantes).** Le filtre temporel de `getRecentStats()` ne repérait qu'un pic **d'un seul point** ; or les échecs I2C répétés (avant le correctif 1.6.2) ont pu enregistrer **plusieurs mesures à 0 d'affilée**, que ce filtre laissait passer (le voisin étant aussi à 0). `getRecentStats()` utilise désormais un filtre **robuste médiane/MAD** par grandeur (`robustMetric`, `src/managers/history_manager.cpp`) : le seuil de rejet est piloté par la dispersion réelle des données (médiane ± 5·1,4826·MAD, avec un plancher), ce qui écarte les valeurs aberrantes **même en série** sans toucher aux variations normales. Les mesures enregistrées depuis la 1.6.2 étant déjà propres, les anciens zéros restants disparaissent aussi des statistiques au fil de leur sortie de la fenêtre 24 h.

# [1.6.2] – 2026-07-18
### Fixed
- **Stabilité de l'acquisition capteur / valeurs à zéro à la source.** La lecture (`src/modules/sensors.cpp`) ignorait le booléen de succès de `aht.getEvent()` et forçait `valid=true` : une erreur I2C (`i2cRead returned Error -1`) faisait enregistrer une mesure à `0`. Désormais :
  - le succès **et** la plausibilité de chaque lecture sont vérifiés (rejet des `NaN`, du `0 %` d'humidité, des valeurs hors plage) ; en cas d'échec, `valid=false` et l'historique n'enregistre rien (la minute est simplement sautée) ;
  - **jusqu'à 3 tentatives** par lecture (les erreurs I2C sont le plus souvent transitoires) ;
  - **récupération automatique du bus I2C** après 5 échecs consécutifs (`Wire.end()` + réinitialisation du bus et des capteurs) ;
  - la **dernière valeur valide** est renvoyée pour l'affichage temps réel quand une lecture échoue (au lieu d'afficher 0), avec `valid=false` ;
  - `Wire.setClock(100 kHz)`, `Wire.setTimeOut(50 ms)` (évite un blocage long si le bus se coince) et suréchantillonnage + filtre IIR du BMP280 (`setSampling`) pour des lectures plus stables.

# [1.6.1] – 2026-07-18
### Fixed
- **Valeurs aberrantes toujours visibles (graphe Historique) et statistiques à 0 incohérentes.** En 1.6.0, le filtrage n'agissait que côté client sur les points **déjà agrégés** (donc dilués, et sans traiter les bords), et la page **Statistiques** (`/api/stats` → `getRecentStats`) calculait min/max/moyenne sur les mesures **brutes** — d'où des minima à `0` / `-0.0` lorsqu'un capteur renvoie ponctuellement une valeur nulle. Le filtrage est désormais fait **au niveau des mesures brutes, côté serveur**, avant toute agrégation :
  - `queryRange()` applique un filtre anti-aberrations **en flux** (fenêtre glissante de 3 mesures) **par grandeur** : une mesure incohérente avec ses deux voisines est écartée de l'agrégation de la grandeur concernée (les autres restent valides). Les tranches sont désormais renvoyées avec une validité **par grandeur** (`temp`/`hum`/`pres` à `null` indépendamment) et l'API `/api/history` sérialise ces `null` séparément.
  - `getRecentStats()` (page Statistiques) écarte de la même façon les valeurs aberrantes par grandeur avant de calculer min/max/moyenne.
  - Le filtrage sur données brutes (échantillonnage 1 min) est bien plus efficace qu'au niveau des points agrégés : une valeur nulle isolée entre deux mesures normales est un pic évident. Le filtre client (`data/app.js`) est conservé comme second rempart. Les données brutes restent inchangées dans les fichiers.

# [1.6.0] – 2026-07-18
### Added
- **En-tête de fichier pour le format binaire (pérennité / compatibilité ascendante).** Chaque fichier `.bin` commence désormais par un en-tête `FileHeader` (`src/managers/history_manager.cpp`) : magic `"MTHB"`, version de format, taille d'en-tête, taille d'enregistrement, drapeaux de capteurs présents, nombre d'enregistrements et horodatages du premier/dernier relevé. MeteoHub identifie ainsi le format avant de lire et se déplace selon `recordSize` : de futurs capteurs (qualité de l'air, vent, UV…) pourront agrandir l'enregistrement **sans imposer de migrer** les anciens fichiers. La lecture (`probeBin`/`readBinRecordAt`) gère de façon transparente les fichiers avec en-tête **et** les fichiers sans en-tête de la v1.4.0 ; ces derniers sont convertis une fois lors du prochain enregistrement du jour (`upgradeLegacyBin`). La migration CSV et l'export s'appuient sur le même en-tête.
- **Détection des valeurs aberrantes à l'exploitation.** Un pic ou un creux d'un seul point (valeur incohérente au regard des relevés qui l'entourent, suivie d'un retour immédiat à la normale) est désormais **écarté du tracé des graphiques et des statistiques**, les points valides étant reliés directement (`filterOutliers`, `spanGaps`, `data/app.js`). La détection repose sur la **cohérence temporelle** (écart fort avec les deux voisins alors que ceux-ci restent cohérents entre eux), et non sur un seuil fixe ; un plancher de bruit par grandeur évite de nettoyer les micro-variations. Les **données brutes restent conservées** dans les fichiers : seule leur exploitation est adaptée. La synthèse de la page Historique est calculée sur ces mêmes séries filtrées (statistiques plus représentatives). Appliqué aussi au tableau de bord.

# [1.5.0] – 2026-07-18
### Added
- **Page « Système » (ex-« Mise à jour OTA »), hub de gestion.** La page (`data/system.html`) regroupe désormais, en plus de la mise à jour OTA :
  - **Luminosité de la NeoLED** : curseur 0-255 appliqué en direct et **persisté en NVS** (survit au redémarrage) — module `neopixel_status` (`neoSetBrightness`/`neoGetBrightness`, `Preferences`), API `GET`/`POST /api/led`.
  - **Export CSV de l'historique** : dernières 24 h / 7 j / 30 j / tout, en flux (`HistoryManager::exportCsv()`, lecture des `.bin`), API `GET /api/history/export.csv?from=&to=` avec en-tête de téléchargement. Le CSV redevient ainsi purement un format d'export.
  - **Export de la configuration** effective au format JSON — API `GET /api/config/export`.
  - **Accès aux outils** : liens vers le gestionnaire de fichiers et les logs.

### Changed
- **Menu principal réduit à 4 entrées : Tableau de bord, Statistiques, Historique, Système** (`data/menu.js`). Les pages **Fichiers** et **Logs** ne sont plus dans le menu : elles restent accessibles depuis la page Système.
- **Pied de page épuré** : suppression des icônes 💾 (Fichiers) et 📜 (Logs) — l'accès passe par la page Système (`data/footer.js`).
- L'ancienne URL `/ota.html` **redirige** vers `/system.html` (`src/managers/web_manager.cpp`) ; `data/ota.html` est supprimé.

# [1.4.0] – 2026-07-18
### Changed
- **Refonte du stockage de l'historique : format binaire journalier au lieu du CSV.** Le fonctionnement interne de MeteoHub ne repose plus sur des fichiers CSV mais sur un format binaire compact, mieux adapté à l'ESP32 et à un historique appelé à grandir pendant des années. Le CSV est conservé uniquement comme format d'export (lisible dans Excel/LibreOffice).
  - **Enregistrements de taille fixe (16 octets)** : `timestamp` (uint32) + température/humidité/pression (float), soit ~2× plus compact que le CSV, et surtout un **accès direct à une mesure par sa position** (recherche dichotomique) sans relire ce qui précède (`struct BinRecord`, `src/managers/history_manager.cpp`).
  - **Découpage par jour** : `/history/AAAA/MM/AAAA-MM-JJ.bin`. Une consultation 24 h ne lit qu'un fichier, une semaine sept. `queryRange()` saute directement (dichotomie) au premier enregistrement de la plage puis lit séquentiellement jusqu'à la borne — le coût ne dépend plus de la taille totale de l'historique.
  - **Fichiers de statistiques journalières `.bin` → `.stats`** : à côté de chaque `.bin`, un fichier `.stats` contient les valeurs déjà calculées au fil des acquisitions (min/max/moyenne de T°/Hu/Pression, nombre de mesures, première/dernière mesure et leurs horodatages). Mis à jour de façon incrémentale à chaque mesure (`DayStats`, `updateDayStats()`), ils permettent d'afficher la **synthèse quasi instantanément** sans relire les milliers de points du graphe.
  - **Nouvel endpoint `/api/history/summary?from=&to=`** : agrège les `.stats` de la plage et renvoie min/max/moyenne + variation par grandeur (`HistoryManager::querySynthesis()`). La synthèse de la page Historique l'utilise en priorité (extrêmes réels, non lissés par l'agrégation du graphe), avec repli sur un calcul côté client si aucun `.stats` n'est disponible.
  - **Migration automatique au démarrage** : les anciens fichiers `/history/AAAA-MM-JJ.csv` sont convertis une fois en `.bin` + `.stats`, puis renommés en `.csv.bak` pour ne pas être retraités. La lecture conserve un repli sur ces CSV tant qu'un `.bin` équivalent n'existe pas.
  - `readSdSampleNear()` (tendance 48 h de la page Statistiques) et `clearHistory()` (suppression récursive de l'arborescence) sont adaptés au nouveau format.

# [1.3.2] – 2026-07-18
### Added
- **Synthèse Historique : écart A ↔ B en comparaison.** Lorsqu'une période de comparaison est active, chaque carte de synthèse affiche en plus l'écart des moyennes `A − B` (avec flèche/couleur), pour chiffrer d'un coup d'œil de combien la période principale est plus chaude/humide/haute en pression que la période comparée (`data/app.js`, `data/style.css` : `.synth-compare`).

# [1.3.1] – 2026-07-18
### Fixed
- **Page Historique : icône du calendrier des champs de dates peu visible.** L'icône native du sélecteur `datetime-local` restait sombre sur le fond foncé. Ajout de `color-scheme: dark` (et d'un filtre `invert` de repli sur `::-webkit-calendar-picker-indicator`) pour la rendre claire et lisible (`data/style.css`).

# [1.3.0] – 2026-07-18
### Added
- **Page Historique : ligne de synthèse optionnelle au-dessus du graphe.** Une bascule « Synthèse » (masquée par défaut) affiche, pour la période sélectionnée, un résumé calculé automatiquement par grandeur (température, humidité, pression) : variation sur la période (dernier − premier point, avec flèche ▲/▼/= et couleur), minimum, maximum et moyenne. Objectif : savoir d'un coup d'œil si la période a été stable, si un front est passé, si l'humidité s'est effondrée, sans analyser les courbes. Le calcul est effectué côté client (`data/app.js`, `computeSynthesis()` / `renderSynthesis()`) à partir des points déjà renvoyés par `/api/history` — aucun nouvel endpoint. La synthèse porte sur la période principale (A) ; activer/désactiver la bascule redessine sans relancer de requête (`data/longterm.html`, `data/style.css` : `.synth-panel`).

# [1.2.2] – 2026-07-18
### Changed
- **Page Historique : couleurs des courbes de comparaison.** Les courbes de la période comparée (B) reprennent désormais les mêmes couleurs que les courbes principales (température `#00a8ff`, humidité `#00ff88`, pression `#ff00ff`) et ne se distinguent plus que par leur tracé en pointillés (`data/app.js`, `COMPARE_COLORS`).

# [1.2.1] – 2026-07-18
### Fixed
- **Reboot en boucle à la consultation de l'historique (task watchdog `async_tcp`).** La lecture des fichiers CSV sur la carte SD par `HistoryManager::queryRange()` s'exécute dans la tâche `async_tcp` (handler ESPAsyncWebServer), laquelle est surveillée par le task watchdog (`CONFIG_ASYNC_TCP_USE_WDT`). L'ancien yield coopératif (`delay(0)`) ne réarmait pas ce watchdog : une lecture un peu longue déclenchait un abort puis un redémarrage en boucle. `cooperativeYieldEvery()` (`src/utils/cooperative_yield.h`) appelle désormais `esp_task_wdt_reset()` avant de céder la main (protège aussi `readSdSampleNear()` via `/api/stats`), et la boucle de lecture SD de `queryRange()` cède la main plus souvent (toutes les 32 lignes). Côté web (`data/app.js`), le rafraîchissement automatique de la page Historique est en outre limité aux périodes ≤ 48 h pour éviter de relancer en continu un scan de plusieurs fichiers CSV.

# [1.2.0] – 2026-07-18
### Added
- **Page « Historique » (ex-« Historique 24h ») : sélection et comparaison de périodes.** La page (`data/longterm.html`, `data/app.js`) permet désormais de choisir la fenêtre affichée — dernières 24 h / 48 h / 7 jours / 30 jours, « aujourd'hui », ou une plage personnalisée (`du`/`au` via des champs date-heure) — et de comparer deux périodes de même durée (aucune, « période précédente », ou « autre période… »). La période B est superposée en traits pointillés et alignée par index sur la période A ; une ligne d'information rappelle les plages exactes affichées. Les sélecteurs de période et de comparaison sont regroupés dans une barre en haut de page ; toute modification est appliquée automatiquement (pas de bouton « Afficher »), les champs de dates personnalisés ne s'affichant que lorsque l'option correspondante est choisie.
- **Cartouche « Chargement en cours… »** centré sur le graphe pendant la récupération des données de la page Historique (`#chartLoading`, `data/style.css`).
- **API `/api/history` : mode plage absolue `?from=<unix>&to=<unix>[&interval=<s>]`.** Nouvelle méthode `HistoryManager::queryRange()` (`src/managers/history_manager.cpp`) qui agrège les mesures sur une plage temporelle arbitraire, en lisant en priorité les fichiers CSV journaliers de la carte SD (`/history/AAAA-MM-JJ.csv`, potentiellement sur plusieurs jours) puis en complétant par l'historique RAM le plus récent non encore écrit sur SD (ou l'intégralité en l'absence de carte SD). Le nombre de tranches est borné (800 max) pour maîtriser la mémoire, l'intervalle étant élargi automatiquement si besoin. Les tranches sans donnée sont émises avec des valeurs `null` afin de conserver l'alignement des index entre deux périodes comparées. Le mode fenêtre glissante historique (`window`/`interval`/`points`) reste inchangé et rétrocompatible (utilisé par le tableau de bord).

### Changed
- **Graphe d'historique : légendes raccourcies.** Les libellés des trois séries deviennent `°C`, `Hu%` et `hPa` (au lieu de « Température (°C) », « Humidité (%) », « Pression (hPa) »), plus lisibles sur mobile. En comparaison, les séries de la période B portent le suffixe `(B)`.
- **Contrôle d'échelle « Élargissement » renommé « Zoom » et sémantique inversée.** Le curseur va désormais de 0 à 100 % : à `0 %` l'échelle correspond aux min/max fixes configurés (la courbe apparaît quasiment plate/unique), à `100 %` l'échelle épouse exactement l'amplitude des données (la courbe occupe toute la hauteur). En interne (`getDynamicMinMax`, mode « Mixte »), l'ancienne marge additive est remplacée par une interpolation linéaire entre l'échelle complète et l'amplitude dynamique. Valeur par défaut spécifique à chaque page, portée par l'attribut `value` du curseur : `90 %` sur le tableau de bord (`data/index.html`), `75 %` sur la page Historique (`data/longterm.html`). Le calcul d'échelle (`updateChartScale`) prend en compte l'ensemble des séries affichées, y compris la période comparée.
- **Entrée de menu « Historique 24h » renommée « Historique »** (`data/menu.js`, titre et en-tête de `data/longterm.html`).

# [1.1.5] – 2026-06-23
### Added
- **Tendance météo sur 1h/12h/24h/48h** (page Statistiques) : `HistoryManager::getTrend()` calcule désormais le delta et la direction (hausse/baisse/stable) de la température, l'humidité et la pression sur quatre fenêtres temporelles au lieu de deux (1h et 24h auparavant). La fenêtre 12h est dérivée de l'historique RAM (24h disponibles à ~1 point/min) ; la fenêtre 48h est récupérée en lisant le fichier CSV journalier de J-2 sur la carte SD (`/history/AAAA-MM-JJ.csv`, nouvelle méthode `HistoryManager::readSdSampleNear()`) et n'est disponible que si une carte SD avec historique est présente (flag `available_48h`, affiché « N/D » sinon).
- **Tendance générale plus fiable** : `computeGlobalTrendLabelFr()` (`src/managers/web_manager.cpp`) croise désormais la direction de la pression sur les fenêtres 1h/12h/24h(/48h) pour distinguer une vraie tendance de fond (« Amélioration durable », « Dégradation durable ») d'une simple fluctuation court terme, en plus des signaux rapides déjà existants (chute brutale de pression + humidité en hausse, etc.).
- Page Statistiques (`data/stats.html`, `data/app.js`) : le tableau « Tendance météo » affiche désormais 4 colonnes (1h/12h/24h/48h) avec flèches de direction, et la tendance générale est affichée séparément sous le tableau.

# [1.1.4] – 2026-06-23
### Fixed
1. **Page Statistiques : tendances toujours à zéro/"stable"**
   - **Problème** : Le endpoint `/api/stats` (`src/managers/web_manager.cpp`) calculait correctement les tendances (`HistoryManager::getTrend()`) mais ne sérialisait dans la réponse JSON que `trend.global_label_fr`. Les sous-objets `trend.temp`, `trend.hum` et `trend.pres` attendus par le frontend (`data/app.js`, fonction `fetchStats`) n'existaient jamais dans la réponse, donc les deltas 1h/24h affichaient toujours `0.0` et les directions toujours `stable`, empêchant de dégager une tendance.
   - **Solution** : Ajout de la sérialisation complète de `trend.temp`, `trend.hum` et `trend.pres` (`delta_1h`, `delta_24h`, `direction_1h`, `direction_24h`) dans la réponse de `/api/stats`. Augmentation de la taille du buffer JSON (`DynamicJsonDocument`) de 2048 à 3072 octets pour accueillir les champs supplémentaires.
   - **Fichier** : `src/managers/web_manager.cpp`.

2. **Page Fichiers : téléchargement impossible dans les sous-dossiers**
   - **Problème** : Le endpoint `/api/files/list` renvoyait le chemin complet du fichier (`file.path()`, ex. `/sous-dossier/fichier.txt`) dans le champ JSON `"name"`, alors que le frontend (`data/files.js`) traite ce champ comme un simple nom de fichier et reconstruit le chemin complet en le concaténant avec le dossier courant. Résultat : un chemin dupliqué (ex. `/sous-dossier/sous-dossier/fichier.txt`) introuvable côté serveur, d'où l'échec systématique du téléchargement (et de la suppression) pour tout fichier non situé à la racine.
   - **Solution** : Utilisation de `file.name()` (nom de base uniquement) au lieu de `file.path()` pour le champ `"name"` de la liste de fichiers.
   - **Fichier** : `src/managers/web_manager.cpp`.

# [1.1.3] – 2026-03-15
### Fixed
- **Affichage corrompu du menu lors de la sélection des items** : Ajout d'un `d->clear()` au début du bloc de rendu du menu dans `UiManager::drawPage()`. Lors de la navigation dans le menu, l'écran n'était pas effacé avant le redessin car `screen_context_changed` était `false` (le mode menu n'avait pas changé). Les anciens items se superposaient aux nouveaux, provoquant un affichage corrompu.

# [1.1.2] – 2026-03-08
### Fixed
Correction critique de la corruption du système de fichiers et erreurs de compilation associées.

1. **Ajout explicite de `flush()` avant fermeture de fichier**
   - **Problème** : Les données restaient dans le cache RAM et n'étaient pas écrites physiquement sur la carte SD/LittleFS avant la fermeture, causant une corruption de la table FAT en cas d'écriture rapide ou de coupure.
   - **Solution** : Ajout systématique de `file.flush()` avant chaque `file.close()` dans `history_manager.cpp` (fonctions `saveRecent`, `saveToSd`) et `sd_manager.cpp` (fonction `verifyWriteAccess`).
   - **Impact** : Garantit l'intégrité des fichiers CSV et binaires après chaque écriture.

2. **Correction du type de retour de `flush()`**
   - **Problème** : Erreur de compilation "l'expression doit avoir le type booléen" car `file.flush()` retourne `void` sur certaines versions du core ESP32, mais le code tentait de l'évaluer dans un `if`.
   - **Solution** : Suppression des tests conditionnels `if (!file.flush())`. La fonction est maintenant appelée de manière impérative.
   - **Fichiers** : `src/managers/sd_manager.cpp`, `src/managers/history_manager.cpp`.

3. **Ajout de l'inclusion manquante `cooperative_yield.h`**
   - **Problème** : Erreur de compilation "identificateur non défini" pour la macro `COOPERATIVE_YIELD_EVERY` dans `history_manager.cpp`.
   - **Solution** : Ajout de `#include "../utils/cooperative_yield.h"` en tête de fichier.

4. **Implémentation de Mutex pour la protection des écritures**
   - **Problème** : Risque de corruption si deux tâches (ex: sauvegarde historique et test web) écrivent simultanément sur la SD.
   - **Solution** : Ajout d'un `std::mutex` dans `SdManager` et utilisation de `std::lock_guard` dans les méthodes critiques (`verifyWriteAccess`, `ensureHistoryDirectory`, `openFileSafe`).

5. **Simplification de la logique de montage SD**
   - **Problème** : Échecs de montage à 10MHz et 4MHz sur cartes sensibles.
   - **Solution** : Fréquence unique fixée à 1 MHz (1000000 Hz) pour une stabilité maximale, supprimant les tentatives multi-fréquences inutiles.

# [1.1.1] – 2026-03-08
- Correction erreur de compilation dans `SdManager::resetSpiBus()` : la méthode `SPIClass::begin()` retournant `void` sur ESP32, la capture du retour booléen a été supprimée.
- Simplification de la logique de montage SD : tentative unique à 1 MHz pour maximiser la stabilité.
- Augmentation des délais d'initialisation SPI pour garantir la stabilité électrique lors du montage.

# [1.1.0] – 2026-03-08
- Refonte complète du `WebManager` pour utiliser exclusivement `std::string` (C++ Standard).
- Conversion explicite aux frontières entre les types Arduino (`String`) et C++ (`std::string`).
- Support complet de la gestion de fichiers (upload, download, suppression) et OTA.

# [1.0.181] – 2026-03-07
- Durcissement anti read-only: `SdManager::begin()` et `ensureMounted()` n'annoncent plus la SD disponible si création `/history` ou test d'écriture échoue.
- `ensureHistoryDirectory()` retourne désormais un booléen et tente un fallback `/sd/history` pour les cas de mountpoint atypiques.
- En cas d'échec d'écriture après format/remount, la SD est démontée et marquée indisponible pour éviter les erreurs répétées côté sauvegarde historique.

# [1.0.180] – 2026-03-07
- Ajout d'un fallback local `SD_DET_ACTIVE_LEVEL` dans `sd_manager.cpp` pour éviter toute erreur de symbole non défini selon l'ordre d'includes/toolchain.
- Maintien de `isCardDetected()` déclaré+défini dans `SdManager` avec check non bloquant au boot.

# [1.0.179] – 2026-03-07
- Réintroduction explicite de `isCardDetected()` dans `SdManager` pour lever l'erreur "identificateur non défini" signalée à la compilation/IDE.
- Vérification DET conservée non bloquante au démarrage SD (log diagnostic sans empêcher les tentatives de montage).

# [1.0.178] – 2026-03-07
- Correction supplémentaire de `SdManager::verifyWriteAccess()` pour supprimer définitivement les erreurs de parsing C++ (bloc unique, retours explicites, suppression fichier test centralisée).
- Réordonnancement des includes dans `sd_manager.cpp` (`Arduino.h` avant les logs) pour éviter les effets de bord de macro selon toolchain.

# [1.0.177] – 2026-03-07
- Correction de compilation dans `SdManager::verifyWriteAccess()` avec réécriture plus explicite de la séquence d'ouverture/écriture/fermeture du fichier test SD.
- Ajout de `#include <Arduino.h>` dans `sd_manager.cpp` pour fiabiliser la compilation croisée des types Arduino (`size_t`, API runtime) selon toolchain.

# [1.0.176] – 2026-03-07
- Réécriture complète du `SdManager` sur la méthode validée "mode stable 10MHz" : instance `FSPI` dédiée recréée avant chaque montage et `SD.begin(..., format_if_fail=...)`.
- Suppression de la dépendance bloquante à la broche DET dans la logique de montage pour éviter les faux négatifs de détection.
- Formatage robuste aligné sur le code de référence: remount à 10MHz avec `format_if_fail=true` puis test d'écriture critique.
- Conservation des fonctionnalités projet liées à la SD (historique `/history`, sauvegarde, lecture, upload/suppression via APIs existantes).

# [1.0.175] – 2026-03-07
- Renforcement du montage SD sur ESP32-S3: ajout d'essais à 1MHz et 400kHz en plus des fréquences rapides.
- Préparation explicite du bus SPI avant `SD.begin` (CS HIGH, MISO pull-up, clocks d'amorçage) pour améliorer la compatibilité des cartes/modules sensibles.
- Ajustement `max_files` lors du montage SD à 10 pour limiter les échecs liés aux ouvertures de fichiers simultanées.

# [1.0.174] – 2026-03-07
- Correction SD_DET: la détection de carte n'est plus bloquante pour le montage (certains modules ont une polarité inversée ou un signal bruité).
- Ajout d'un échantillonnage multi-lectures de la broche DET avec logs détaillés (LOW/HIGH) pour diagnostiquer le câblage réel.
- Ajout du paramètre `SD_DET_ACTIVE_LEVEL` (LOW/HIGH) dans `board_config.h` pour s'adapter aux lecteurs à polarité inversée.
- Si la carte est déjà montée, un état DET incohérent n'entraîne plus de démontage forcé; seule la vérification `SD.cardType()` décide de la disponibilité.

# [1.0.173] – 2026-03-07
- Refonte du gestionnaire SD pour s'aligner sur la méthode validée (SPI FSPI dédié + `SD.begin(..., format_if_fail=true)`).
- Respect strict du mapping défini dans `board_config.h` (CLK=9, D0/MISO=10, CMD/MOSI=11, D3/CS=12, DET=14).
- Ajout d'une détection de présence carte via `SD_DET_PIN` (LOW=présente) avant montage/réessais.
- Conservation des fonctionnalités SD existantes: lecture/écriture, incrémentation quotidienne des fichiers CSV d'historique, suppression/upload web et formatage.

# [1.0.172] – 2026-02-25
- Ajout et liens croisés de la documentation débutant (EN/FR) dans tous les documents utilisateur.
- Tous les guides, FAQ, configuration et index référencent désormais l'onboarding débutant.
- Version minimale valide : 1.0.172

# [1.0.171] – 2026-02-25
- Ajout de la gestion avancée des échelles pour les graphiques température, humidité et pression.
- Trois modes disponibles : fixe, dynamique, mixte (avec élargissement configurable).
- Contrôles interactifs sur l'UI web pour choisir le mode et le pourcentage.
- Aide contextuelle sous le graphique.
- Synchronisation automatique entre config.h et l'UI web.

# [1.0.170] - 2026-02-24
### Corrigé
- Application du même schéma de zone sûre que les graphes aux autres pages OLED.
- Conservation des titres dans la bande haute et déplacement du début de contenu prévisions/logs sous la zone haute réservée SSD1306.
- Ajustement de l'espacement des lignes de logs pour éviter le chevauchement haut sur les SSD1306 à bande jaune.