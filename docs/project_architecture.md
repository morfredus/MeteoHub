# Architecture du projet

**Débutant ?** Voir le [Guide Débutant](beginner/index.md)

Version minimale valide : 1.9.0

- `src/modules/` : modules matériels et affichage OLED (`sensors`, `oled_display`, `neopixel_status`, `encoder` (DevKitC), `button` (Super Mini), `pages_oled`, `espnow_receiver`…)
- `src/managers/` : gestionnaires fonctionnels et orchestration (`web_manager`, `history_manager`, `sd_manager`, `wifi_manager`, `forecast_manager`, `ui_manager`)
- `src/utils/` : utilitaires réutilisables (logs, infos système, yield coopératif)
- `include/` : en-têtes de configuration protégés
- `data/` : ressources de l'interface web (HTML/CSS/JS), embarquées dans le firmware au build (`scripts/embed_web_files.py` → `include/web_pages.h`)

## Stockage de l'historique

L'arborescence est **symétrique entre l'intérieur (IN) et l'extérieur (OUT)** :

- Sur la carte SD, un dossier par contexte, puis par année/mois :
  `/history/indoor/AAAA/MM/AAAA-MM-JJ.bin` et `/history/outdoor/AAAA/MM/AAAA-MM-JJ.bin`.
- Chaque `.bin` commence par un en-tête (magic, version, tailles, capteurs présents, nombre de mesures, premiers/derniers horodatages) suivi d'enregistrements de taille fixe (horodatage + T/H/P). L'en-tête assure la **compatibilité ascendante** (de futurs capteurs peuvent agrandir l'enregistrement sans migration).
- Un fichier `.stats` par jour conserve les statistiques déjà calculées (min/max/moyenne, comptes, premier/dernier relevé), mis à jour au fil des acquisitions.
- L'accès est direct (recherche dichotomique sur les enregistrements de taille fixe) : une consultation ne lit que les fichiers des jours concernés.
- Un historique récent est aussi conservé en RAM et sur LittleFS, un fichier par contexte : `/history/indoor_recent.dat` et `/history/outdoor_recent.dat` (redémarrage rapide).
- Les **prévisions** « du lendemain » sont archivées à part, un fichier par jour cible : `/history/forecast/AAAA-MM-JJ.json` (sert à l'analyse « prévu vs observé » de morfAnalytics).
- Le **CSV** n'est qu'un format d'export.

> L'effacement complet (RAM + fichiers récents + arborescence SD, IN et OUT) se fait depuis le menu de l'appareil **ou** depuis la page Système de l'interface web (`POST /api/history/clear`).

## Acquisition et qualité des données

- `SensorManager` (AHT20 + BMP280 sur I2C) : lecture validée (succès I2C + plausibilité), réessais, récupération du bus après échecs, conservation de la dernière valeur valide. Ces mesures sont le contexte **IN** (intérieur).
- `EspNowReceiver` : réception des trames `MeteoPacket` de la sonde extérieure (ESP-NOW). Décodage et archivage dans `loop()`, pas dans le callback radio. Ces mesures sont le contexte **OUT**. La pression affichée sur l'OLED est toujours OUT.
- Filtrage des valeurs aberrantes à l'exploitation, par grandeur : cohérence temporelle pour les graphes (`queryRange`), filtre robuste médiane/MAD pour les statistiques (`getRecentStats`). Les données brutes ne sont pas modifiées.

## Interface web

- Pages : Tableau de bord (`/`), Statistiques (`/stats.html`), Historique (`/longterm.html`), Système (`/system.html`), plus les outils Fichiers (`/files.html`) et Logs (`/logs`) accessibles depuis Système.
- API principales : `/api/live` (blocs `in`/`out`/`effective` avec provenance et fraîcheur), `/api/history` (accepte `ctx=in|out`), `/api/history/summary`, `/api/history/export.csv`, `/api/history/days`, `/api/history/raw`, `/api/history/clear` (POST, efface tout), `/api/forecast/history` (prévisions archivées), `/api/stats`, `/api/alert`, `/api/system`, `/api/led`, `/api/config/export`, `/api/files/*`, `/api/ota/update`, `/api/logs`, `/api/analytics`, `/api/analytics/config`.
