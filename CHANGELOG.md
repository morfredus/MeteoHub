# [Non publié]

# [1.13.0] – 2026-07-21

### Ajouté

- **Annonce de présence sur le LAN (protocole `morfbeacon/1`).** MeteoHub
  *écoutait* déjà ce protocole pour repérer un service d'analyse ; il l'**émet**
  désormais, et devient donc découvrable par le même mécanisme que les services
  Linux et Windows du parc.

  Auparavant il n'était trouvé que par sonde TCP, ce qui suppose de connaître son
  nom mDNS à l'avance — l'inverse d'une découverte, et la raison pour laquelle
  des listes statiques restaient nécessaires dans `morfsystem.json`.

- **Route `GET /status`** au format `morfbeacon/1`, et déclaration de la capacité
  `web_ui` : un observateur du parc peut proposer un lien vers l'interface de
  MeteoHub **sans rien connaître de MeteoHub**. La capacité annonce, `/status`
  détaille — déclarer l'une sans servir l'autre désignerait une interface que
  personne ne saurait ouvrir.

  Émetteur vendoré dans `third_party/morf/beacon-arduino/`, copie de
  `arduino/morfbeacon_emitter.h` du dépôt morfBeacon. Le partage porte sur le
  **protocole**, pas sur le code : la bibliothèque Qt ne tourne pas sur un ESP32.

- **Fichier `VERSION` à la racine, désormais autoritaire.** La version était
  écrite en dur dans `platformio.ini` (`-D PROJECT_VERSION='"1.12.0"'`), donc à
  un endroit différent des onze autres projets de l'écosystème, où un fichier
  `VERSION` fait autorité. Aucun outil ne pouvait établir l'inventaire des
  versions du parc, puisque deux projets sur quatorze publiaient la leur
  autrement.

  `scripts/version.py` lit ce fichier et injecte `PROJECT_VERSION` à la
  compilation. Ajouter le fichier **sans** ce script aurait créé deux sources de
  vérité pour une même valeur — le défaut que le script existe précisément pour
  éviter. `platformio.ini` ne porte plus la version, il la lit.

- **`LICENSE`** (GPL-3.0-only), **`CONTRIBUTING.md`** et **`ROADMAP.md`**, qui
  manquaient alors que le reste de l'écosystème les fournit. `CONTRIBUTING.md`
  consigne les contraintes propres à la cible ESP32-S3 — mémoire comptée,
  corruption de carte SD, dérive d'horloge, chutes du bus I2C — qui expliquent
  des choix du code que rien ne justifiait par écrit.

### Corrigé

- **Le brochage SPI de la carte SD documenté ne correspondait pas au firmware.**
  `hardware_wiring.md`, `pin_mapping.md` et `maintenance_and_troubleshooting.md`
  annonçaient CLK 21 / MISO 47 / MOSI 38 / CS 39, alors que `board_config.h`
  utilise **13 / 12 / 11 / 10** depuis sa révision du 02/07/2026. Un lecteur qui
  câblait d'après la documentation obtenait une carte SD non détectée, et le
  guide de dépannage lui faisait vérifier les mêmes broches erronées. Les trois
  documents sont alignés sur `board_config.h`, seule source de vérité.
- `docs/pin_mappnig.md` renommé en `docs/pin_mapping.md` (coquille dans le nom).
  Ce document de 119 lignes n'était référencé nulle part et absent de
  `docs/index.md` : le brochage réel de la carte était invisible pour le lecteur.
  Il est maintenant listé dans l'index.
- `docs/beginner/faq.md` : chemin des sources corrigé (`src/modules/sensors.*`).
- `docs/beginner/readme.md`, page d'accueil du dossier sur GitHub, dupliquait
  `index.md` sans aucun lien : ses entrées sont désormais cliquables.

# [1.12.0] – 2026-07-19
### Changed
- **Le service d'analyse est reconnu à sa CAPACITÉ, plus à son nom.** MeteoHub cherchait un heartbeat dont le champ `app` valait exactement `morfAnalytics`. Or le projet est sous licence GPL : chacun peut renommer son service, et la détection cessait alors de fonctionner. MeteoHub cherche désormais un service annonçant la capacité **`advanced_analysis`** (nouveau champ `capabilities` du protocole morfBeacon, voir morfBeacon 0.2.0) et n'utilise le nom annoncé que comme **libellé** affiché dans le menu et la page Système. Renommer son service n'interrompt plus l'intégration.

  **Conséquence de mise à jour** : un service d'analyse antérieur à morfAnalytics 0.4.0 n'annonce pas de capacité et n'est donc plus détecté. Le mettre à jour, ou renseigner son adresse manuellement (ci-dessous).
- **L'entrée de menu s'ouvre dans le même onglet** (`data/menu.js`), au lieu d'un nouvel onglet. L'utilisateur passe d'un service à l'autre comme entre deux pages, sans accumuler d'onglets ; le service d'analyse propose en retour un lien « Retour à MeteoHub ». Chaque application conserve par ailleurs son indépendance technique et son propre cycle de vie.
- **L'entrée de menu porte le nom annoncé par le service**, et non un libellé figé.

### Added
- **Adresse manuelle du service d'analyse** (page **Système** → Analyse avancée → « Adresse manuelle »). La découverte automatique suppose que les annonces UDP atteignent la station, ce qui n'est pas le cas sur un réseau segmenté, à travers un VPN, ou avec un point d'accès isolant les clients. Une adresse peut alors être saisie ; elle est **persistée en NVS**, survit au redémarrage et **prend le pas** sur la découverte. Vider le champ rétablit le mode automatique. Une adresse ne commençant pas par `http://` ou `https://` est refusée avec un message explicite.
- **API enrichie** : `GET /api/analytics` renvoie désormais `{available, mode, effective_url, capability, manual_url, detected:{found,name,version,host,status_port,last_seen_s}}` ; `POST /api/analytics/config` (paramètre `manual_url`) enregistre ou efface l'adresse manuelle.
- **Guide débutant [docs/analyse_avancee.md](docs/analyse_avancee.md)** : à quoi sert un service d'analyse, comment la détection fonctionne, comment naviguer entre les deux applications et que faire si la découverte automatique ne passe pas.

### Notes
- La sélection explicite entre plusieurs services détectés et la personnalisation du nom affiché sont volontairement reportées : elles répondent à des besoins plus spécifiques et n'apportent rien au fonctionnement courant. En présence de plusieurs services, MeteoHub retient le dernier annoncé ; une adresse manuelle permet d'en imposer un.
- Les emplacements de captures d'écran de `docs/analyse_avancee.md` sont **en attente des images** (voir `docs/images/README.md`, qui décrit précisément ce qu'il faut cadrer).

# [1.11.2] – 2026-07-19
### Fixed
- **`/api/history/summary` et `/api/history/export.csv` renvoyaient le JSON de l'historique** (bug préexistant, antérieur à la 1.11.0). ESPAsyncWebServer fait correspondre une route à toute URL qui **commence** par elle : `/api/history`, enregistrée avant ses sous-routes, les captait toutes. La synthèse renvoyait donc la liste des mesures, et le bouton **Export CSV** téléchargeait un fichier JSON portant l'extension `.csv`. La route générique est désormais déclarée **en dernier**, après toutes ses sous-routes (`src/managers/web_manager.cpp`).
- **`/api/history/days` et `/api/history/raw` ne répondaient jamais**, pour la même raison : captées par `/api/history`, elles tombaient dans une branche qui n'émet aucune réponse pour leurs paramètres.
- **Journées fantômes dans `/api/history/days`.** Les fichiers `.stats` étaient pris pour des fichiers de mesures : `sscanf` renvoie le nombre de conversions **affectées**, si bien que `"%4u-%2u-%2u.bin"` acceptait `AAAA-MM-JJ.stats`, l'échec du littéral `.bin` final n'étant pas compté. Chaque journée apparaissait deux fois, la seconde avec des horodatages aberrants (2005-2014). L'extension est désormais vérifiée séparément de la date.
- **`/api/history/raw` échouait au-delà d'environ 300 enregistrements** (réponse abandonnée, aucun octet renvoyé). Deux causes : la boucle d'émission ne rendait jamais la main à la tâche réseau (ajout de `COOPERATIVE_YIELD_EVERY`, comme le fait déjà `/api/history`), et la borne haute était fixée à 2880, très au-delà de ce que le flux de réponse asynchrone tient réellement. Elle est ramenée à **250**, mesurée sûre sur l'appareil ; le collecteur pagine.

  Ces quatre défauts ne sont observables que sur le matériel : ils compilent sans le moindre avertissement.

# [1.11.1] – 2026-07-19
### Fixed
- **`GET /api/history/days` ne répondait jamais.** La route restait bloquée indéfiniment (aucune réponse, l'appareil restant par ailleurs parfaitement fonctionnel) : `HistoryManager::listDays()` **imbriquait** les parcours de répertoires, gardant ouverts en même temps les itérateurs de `/history`, `/history/AAAA` et `/history/AAAA/MM`. Maintenir plusieurs itérations de répertoires simultanées bloque la lecture de la carte SD sur ESP32. Chaque niveau est désormais **entièrement lu et refermé avant de descendre** au suivant (`listEntries()`), avec un garde-fou sur le nombre d'entrées. Les horodatages extrêmes de chaque journée sont en outre lus dans l'en-tête du fichier plutôt que par deux positionnements supplémentaires, chaque accès SD étant coûteux.

  Le défaut n'était pas détectable à la compilation : il ne se manifeste que sur une vraie carte SD.

# [1.11.0] – 2026-07-19
### Added
- **API de recopie de l'historique pour un serveur d'analyse (lecture seule).** Deux nouvelles routes permettent à **morfAnalytics** de constituer sa copie de travail sans jamais rien modifier sur MeteoHub, qui demeure la **source de vérité** :
  - `GET /api/history/days` — journées présentes sur la carte SD, avec pour chacune le nombre de mesures enregistrées (`{day, nrec, first_ts, last_ts}`) ;
  - `GET /api/history/raw?day=AAAAMMJJ&index=N&limit=M` — mesures brutes de la journée à partir de la position `N`, au format compact `[horodatage, température, humidité, pression]` (~30 octets par mesure au lieu de ~55 en objet nommé).

  Une mesure est repérée par sa **position dans le fichier du jour**, et non par son horodatage : les fichiers `.bin` étant écrits en **ajout seul**, cette position ne change jamais, alors qu'un horodatage peut **reculer** lors d'un recalage NTP ou **se répéter** lors du passage à l'heure d'hiver — un repère temporel ferait donc sauter ou dupliquer des mesures. Le serveur d'analyse retient le couple (jour, position) et ne redemande que ce qui lui manque. Implémentation `HistoryManager::listDays()` / `exportRaw()` (`src/managers/history_manager.*`), réutilisant le parcours séquentiel par blocs introduit en 1.8.0.
- **Entrée de menu « Analyse avancée ».** Lorsque le service morfAnalytics est détecté sur le réseau local (`GET /api/analytics`), un lien vers sa page d'analyse est inséré au menu **juste avant « Système »**, qui reste la dernière entrée (`data/menu.js`). En l'absence de service détecté, le menu est strictement inchangé : **aucune dépendance** n'est introduite.

### Notes
- Ces deux routes ne servent qu'à un serveur d'analyse. Pour un export manuel, le **CSV** (`GET /api/history/export.csv`) reste la voie recommandée, directement exploitable dans un tableur. (Cet export était en réalité cassé jusqu'à la 1.11.2 — voir cette version.)

# [1.10.0] – 2026-07-19
### Added
- **Détection optionnelle du service morfAnalytics (écosystème morfSystem).** MeteoHub écoute passivement le heartbeat morfBeacon (`morfbeacon/1`, broadcast UDP sur le port `45454`) et signale la présence du moteur d'analyse `morfAnalytics` (`src/modules/analytics_beacon.*`, API `GET /api/analytics`). La page **Système** affiche « Analyse avancée disponible / indisponible ». **Aucune dépendance** : si aucun serveur n'est détecté, le comportement nominal de MeteoHub (mesures, historique, graphiques, exports) est strictement inchangé. MeteoHub reste la **source de vérité** ; ce module ne fait que constater une présence. Configurable dans `include/config.h` (`ANALYTICS_BEACON_ENABLED`, `ANALYTICS_BEACON_PORT`, `ANALYTICS_APP_NAME`, `ANALYTICS_TIMEOUT_MS`). Voir la vision d'ensemble dans `MORFSYSTEM_ARCHITECTURE.md` (au niveau de l'écosystème).

# [1.9.1] – 2026-07-19
### Fixed
- **Monitoring UDP : logs de démarrage manquants (dont la vitesse SPI de la carte SD).** Le module UDP était initialisé tard (après le WiFi) et la tâche d'émission **jetait** les logs tant que le réseau n'était pas connecté ; tous les messages de boot (montage SD et fréquence retenue, init capteurs, etc.) étaient donc perdus pour le moniteur UDP. Désormais : la capture est installée **dès le début du `setup()`** (au plus tôt), les logs de démarrage sont **conservés dans une file tampon** (agrandie) puis **rejoués dans l'ordre dès la connexion WiFi**, sans être jetés (`src/utils/udp_logger.cpp`, `src/main.cpp`). Le moniteur UDP reçoit ainsi la totalité des logs applicatifs et cœur ESP à partir du `setup()` (seules les quelques lignes du bootloader ROM et de l'init du cœur Arduino, émises avant `setup()`, restent propres au port série physique).

# [1.9.0] – 2026-07-19
### Added
- **Monitoring des logs par UDP (sans câble série).** Nouveau module `src/utils/udp_logger.*` : les logs applicatifs (`addLog`) **et** ceux du cœur ESP-IDF/Arduino (WiFi, I2C, watchdog…) sont diffusés en UDP sur le réseau local, pour être suivis à distance (par ex. dans **Tabby**). L'émission passe par une **file FreeRTOS + une tâche dédiée** : le contexte d'origine d'un log (y compris la pile réseau) n'émet jamais lui-même de paquet, ce qui évite tout risque de réentrance/blocage. Les logs sont aussi désormais mis en miroir sur le port série (`Serial`). Configurable dans `include/config.h` : `UDP_LOG_ENABLED`, `UDP_LOG_PORT` (5005 par défaut), `UDP_LOG_HOST` (IP du PC récepteur ou `255.255.255.255` pour un broadcast). La réception côté PC et la configuration de Tabby sont décrites dans le guide utilisateur.

# [1.8.0] – 2026-07-19
### Changed
- **Affichage de l'historique nettement plus rapide (lecture SD optimisée).** Deux optimisations, sans perte de précision :
  - **Lecture séquentielle par blocs** : `queryRange()` et l'export CSV lisaient chaque mesure une par une (un `seek` + un `read` de 16 octets par enregistrement, soit ~1440 accès pour 24 h). Un nouveau parcours `forEachBinRecordFrom()` lit désormais les enregistrements contigus par blocs de 1 Ko (un seul `seek` initial, aucun `seek` par mesure), réduisant massivement le nombre d'accès SD.
  - **Cascade de fréquence SPI de la carte SD** : la carte était systématiquement montée à **1 MHz**. `SdManager` tente maintenant, en cascade décroissante, **20 → 10 → 4 → 1 MHz** et retient la plus haute fréquence qui fonctionne réellement (montage + création de dossier + test **écriture/relecture**). Une fréquence trop élevée qui « monte » mais lit mal est rejetée et l'on redescend d'un cran ; le formatage n'est jamais déclenché par un simple échec de vitesse. En cas de matériel marginal, le repli à 1 MHz garantit l'absence de régression.

# [1.7.1] – 2026-07-19
### Added
- **Page Historique : bascule « Temps réel ».** Une case à cocher (activée par défaut), placée à côté de « Synthèse », permet d'activer/désactiver le rafraîchissement automatique du graphe (`data/longterm.html`, `data/app.js`). Le rafraîchissement automatique reste par ailleurs limité aux périodes relatives ≤ 48 h sans comparaison.

# [1.7.0] – 2026-07-19
### Added
- **Page Statistiques : option de mise à jour en temps réel.** Une bascule « Mise à jour en temps réel » (activée par défaut) permet d'activer/désactiver le rafraîchissement automatique des tableaux (`data/stats.html`, `data/app.js`). Un bouton « Actualiser » permet un rafraîchissement manuel quand l'automatique est désactivé, et l'heure de dernière mise à jour est affichée.

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