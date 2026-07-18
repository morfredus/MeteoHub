# Architecture du projet

**Débutant ?** Voir le [Guide Débutant](beginner/index.md)

Version minimale valide : 1.6.3

- `src/modules/` : modules matériels et affichage OLED (`sensors`, `oled_display`, `neopixel_status`, `encoder`, `pages_oled`…)
- `src/managers/` : gestionnaires fonctionnels et orchestration (`web_manager`, `history_manager`, `sd_manager`, `wifi_manager`, `forecast_manager`, `ui_manager`)
- `src/utils/` : utilitaires réutilisables (logs, infos système, yield coopératif)
- `include/` : en-têtes de configuration protégés
- `data/` : ressources de l'interface web (HTML/CSS/JS), embarquées dans le firmware au build (`scripts/embed_web_files.py` → `include/web_pages.h`)

## Stockage de l'historique

- Format **binaire journalier** : `/history/AAAA/MM/AAAA-MM-JJ.bin`. Chaque fichier commence par un en-tête (`FileHeader` : magic `MTHB`, version, tailles, capteurs présents, nombre de mesures, premiers/derniers horodatages) suivi d'enregistrements de taille fixe (16 o : horodatage + T/H/P). L'en-tête assure la **compatibilité ascendante** (de futurs capteurs peuvent agrandir l'enregistrement sans migration).
- Un fichier `.stats` par jour conserve les statistiques déjà calculées (min/max/moyenne, comptes, premier/dernier relevé), mis à jour au fil des acquisitions.
- L'accès est direct (recherche dichotomique sur les enregistrements de taille fixe) : une consultation ne lit que les fichiers des jours concernés.
- Le **CSV** n'est plus qu'un format d'export ; les anciens CSV sont migrés au démarrage.
- Un historique récent (~24 h) est aussi conservé en RAM et sur LittleFS (`/history/recent.dat`) pour un redémarrage rapide.

## Acquisition et qualité des données

- `SensorManager` (AHT20 + BMP280 sur I2C) : lecture validée (succès I2C + plausibilité), réessais, récupération du bus après échecs, conservation de la dernière valeur valide.
- Filtrage des valeurs aberrantes à l'exploitation, par grandeur : cohérence temporelle pour les graphes (`queryRange`), filtre robuste médiane/MAD pour les statistiques (`getRecentStats`). Les données brutes ne sont pas modifiées.

## Interface web

- Pages : Tableau de bord (`/`), Statistiques (`/stats.html`), Historique (`/longterm.html`), Système (`/system.html`), plus les outils Fichiers (`/files.html`) et Logs (`/logs`) accessibles depuis Système.
- API principales : `/api/live`, `/api/history`, `/api/history/summary`, `/api/history/export.csv`, `/api/stats`, `/api/alert`, `/api/system`, `/api/led`, `/api/config/export`, `/api/files/*`, `/api/ota/update`, `/api/logs`.
