# Configuration

**Débutant ?** Voir le [Guide Débutant](beginner/index.md)

Version minimale valide : 1.9.0


## Fichiers modifiables
- `include/secrets.h` (identifiants, clé API, coordonnées)
- `include/config.h` (tous les paramètres modifiables par l'utilisateur, voir ci-dessous)

## Paramètres utilisateur dans config.h

### Paramètres graphiques
- `GRAPH_SCALE_MODE` : Mode d'échelle pour les graphes (0=fixe, 1=dynamique, 2=mixte)
- `GRAPH_SCALE_MARGIN_PCT` : valeur par défaut du **Zoom** du mode mixte (0 = échelle complète des min/max fixes, 100 = amplitude exacte des données). Réglable en direct dans l'interface web ; ce paramètre ne concerne que le graphe OLED. Le tableau de bord et la page Historique ont chacun leur défaut propre (90 % et 75 %).
- `GRAPH_TEMP_MIN` / `GRAPH_TEMP_MAX` : Température min/max pour les graphes
- `GRAPH_HUM_MIN` / `GRAPH_HUM_MAX` : Humidité min/max pour les graphes
- `GRAPH_PRES_MIN` / `GRAPH_PRES_MAX` : Pression min/max pour les graphes

### Paramètres OLED
- `OLED_CONTRAST` : Contraste de l'écran OLED
- `OLED_CONTROLLER` : Type de contrôleur OLED, choisi selon la carte (SH1106 sur DevKitC, SSD1306 sur Super Mini)
- `OLED_I2C_ADDRESS` : Adresse I2C de l'OLED

### Paramètres réseau
- `WEB_MDNS_HOSTNAME` : Nom mDNS pour accès local
- `WIFI_RETRY_DELAY_MS` : Délai entre tentatives WiFi (ms)
- `ENABLE_PING_TEST` : Activer le test de ping (1=activé)

### Carte SD
- `SD_CARD_ENABLED` : `1` par défaut ; `0` pour un montage sans lecteur SD (aucune tentative de montage, voir [Fonctionnement sans carte SD](maintenance_and_troubleshooting.md#fonctionnement-sans-carte-sd))

### Paramètres système
- `DASHBOARD_REFRESH_MS` : Fréquence de rafraîchissement du dashboard (ms)
- `BUTTON_GUARD_MS` : Anti-rebond pour l'encodeur et les boutons (DevKitC, ms)
- `BUTTON_DEBOUNCE_MS` / `BUTTON_LONG_PRESS_MS` : anti-rebond et seuil d'appui long du bouton unique (Super Mini, ms)
- `MORF_ECOSYSTEM_ENABLED` : `1` par défaut (heartbeat morfBeacon émis + écoute de morfAnalytics) ; `0` sur un banc de test, pour que morfAnalytics ne découvre pas ce hub et ne mélange pas ses mesures à celles de la vraie station

### Monitoring des logs par UDP
- `UDP_LOG_ENABLED` : diffuser les logs par UDP sur le réseau (1 = activé)
- `UDP_LOG_PORT` : port UDP d'écoute côté PC (défaut 5005)
- `UDP_LOG_HOST` : IP du PC récepteur, ou `255.255.255.255` pour un broadcast local
- Au démarrage, le hub journalise la raison de son dernier redémarrage (`Hub reset reason: …`) : `TASK_WDT`/`PANIC` = plantage, `USB` = moniteur série reconnecté, `POWERON` = alimentation, `SW` = redémarrage volontaire.
- Détails et réception (Tabby) : voir le [Guide utilisateur](user_guide.md#monitoring-des-logs-par-udp-sans-câble-série-ex-avec-tabby).

## Banc de test : env `esp32-s3-oled-test`
Le banc compile **le même code** que la production ; seule la configuration change, imposée par des `-D` dans `platformio.ini` (prioritaires sur les `#ifndef` de `config.h`) :
- `WEB_MDNS_HOSTNAME` = `meteohub-test` ;
- `UDP_LOG_PORT` = `5006` (morfMonitor écoute 5005) ;
- `MORF_ECOSYSTEM_ENABLED` = `0` (morfAnalytics ne découvre pas ce hub) ;
- `SD_CARD_ENABLED` = `0` (pas de SD sur ce montage).

Cible matérielle : DevKitC-1 N16R8 (même carte que `esp32-s3-oled`). Pas de cible de livraison : un banc se flashe à la main, `platformio run -e esp32-s3-oled-test -t upload`. Effacer la flash au premier passage d'une carte de la prod au banc (ou l'inverse) : NVS et LittleFS sont propres à chaque rôle.

## Réglages persistés à l'exécution (NVS)
Certains réglages se modifient directement depuis l'interface web (page **Système**) et sont conservés au redémarrage, sans recompilation :
- **Luminosité de la NeoLED** (0-255), stockée en NVS (espace `meteohub`).
- **Calibration des capteurs** : décalages de température (°C, borné à ±10) et
  d'humidité (points de %HR, borné à ±20) pour le capteur intérieur du hub et
  pour la sonde extérieure, stockés en NVS (espace `sensor_cal`). Voir
  [Calibrer les capteurs](user_guide.md#calibrer-les-capteurs).

## Export de la configuration
La configuration effective (projet, réseau, graphes, luminosité LED, calibration des capteurs, intervalle d'échantillonnage) peut être exportée au format JSON depuis la page **Système** (ou via `GET /api/config/export`).

---
Fichier réservé :
- `include/board_config.h` (mapping matériel par carte, sélectionné par le define `BOARD_S3_SUPERMINI`, dont les broches I2C `I2C_SDA_PIN`/`I2C_SCL_PIN` et la NeoLED)

Métadonnées :
Le nom/la version du projet sont injectés depuis `platformio.ini` via build flags.
