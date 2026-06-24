# Guide utilisateur

**Débutant ?** Voir le [Guide Débutant](beginner/index.md)

Version minimale valide : 1.2.0

- Tourner l’encodeur pour naviguer entre les pages.
- Le clic encodeur ouvre le menu.
- Back sort du menu/des confirmations.
- Confirm valide les actions contextuelles.


Pages principales :
- Météo
- Prévisions
- Graphes (température, humidité, pression)
- Réseau
- Système
- Logs

---

## Fonctionnalités avancées et API web

### Interface web
- Accès à l’interface web via l’adresse http://<nom>.local (mDNS) ou IP locale.
- Visualisation des données en temps réel, historiques, graphiques, logs, fichiers, et OTA.
- Données temps réel (page d'accueil, rafraîchies périodiquement) : API `/api/live` (température, humidité, pression, validité capteur, RSSI WiFi, uptime, état d'alerte).
- Page Historique 24h (`/longterm.html`, menu « Historique 24h ») : graphiques détaillés sur les dernières 24h, basés sur `/api/history`.

### Gestion des fichiers (LittleFS/SD)
- Navigation, téléchargement, suppression, upload de fichiers via l’interface web (onglet Fichiers), y compris dans les sous-dossiers.
- Accès aux fichiers SD si une carte est insérée et reconnue.
- API : `/api/files/list`, `/api/files/download`, `/api/files/delete`, `/api/files/upload` (paramètre `fs=sd` ou `fs=littlefs`).

### Historique et statistiques
- Visualisation graphique de l’historique (température, humidité, pression).
- API : `/api/history` (paramètres `window`, `interval`, `points` pour l’agrégation).
- Statistiques 24h via `/api/stats` (min, max, moyenne).
- Page Statistiques (`/stats.html`) : tendances détaillées sur 1h, 12h, 24h et 48h (température, humidité, pression), ainsi qu’une tendance générale qui croise la direction de la pression sur ces fenêtres pour dégager une véritable évolution (amélioration/dégradation durable, stable, ou variable). La fenêtre 48h nécessite une carte SD avec l’historique journalier (fichier CSV de J-2) ; elle s’affiche « N/D » si indisponible.

### Logs et diagnostic
- Accès aux logs système via l’interface web (onglet Logs) et API `/api/logs`.
- Diagnostic système via `/api/system` (informations matérielles, firmware, SD, uptime, etc.).

### OTA (mise à jour firmware)
- Mise à jour du firmware via l’interface web (onglet OTA).
- Upload du fichier binaire, reboot automatique après succès.
- API : `/api/ota/update`.

### Alertes météo et tendances
- Affichage des alertes météo (niveau, type, description, couleur) sur l’interface et l’OLED.
- Traduction automatique des alertes (anglais → français).
- Synthèse de tendance météo (amélioration, pluie, perturbation, etc.).
- Fenêtre de détail d'alerte (bouton « Détails ») : affiche le résumé en français ainsi que le bulletin source d'OpenWeatherMap (langue d'origine, souvent l'anglais) en complément.
- API : `/api/alert` (détail complet : sévérité, émetteur, événement traduit, résumé français, bulletin source brut, horodatages de validité).

### Maintenance avancée
- Appui long sur le bouton BOOT au démarrage : formatage du système de fichiers interne (LittleFS) et redémarrage.

Pour plus de détails sur chaque API, voir la documentation technique ou le code source (web_manager.cpp).
