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
- `OLED_CONTROLLER` : Type de contrôleur OLED (SH1106/SSD1306)
- `OLED_I2C_ADDRESS` : Adresse I2C de l'OLED

### Paramètres réseau
- `WEB_MDNS_HOSTNAME` : Nom mDNS pour accès local
- `WIFI_RETRY_DELAY_MS` : Délai entre tentatives WiFi (ms)
- `ENABLE_PING_TEST` : Activer le test de ping (1=activé)

### Paramètres système
- `DASHBOARD_REFRESH_MS` : Fréquence de rafraîchissement du dashboard (ms)
- `BUTTON_GUARD_MS` : Anti-rebond pour les boutons (ms)

### Monitoring des logs par UDP
- `UDP_LOG_ENABLED` : diffuser les logs par UDP sur le réseau (1 = activé)
- `UDP_LOG_PORT` : port UDP d'écoute côté PC (défaut 5005)
- `UDP_LOG_HOST` : IP du PC récepteur, ou `255.255.255.255` pour un broadcast local
- Détails et réception (Tabby) : voir le [Guide utilisateur](user_guide.md#monitoring-des-logs-par-udp-sans-câble-série-ex-avec-tabby).

## Réglages persistés à l'exécution (NVS)
Certains réglages se modifient directement depuis l'interface web (page **Système**) et sont conservés au redémarrage, sans recompilation :
- **Luminosité de la NeoLED** (0-255), stockée en NVS (espace `meteohub`).

## Export de la configuration
La configuration effective (projet, réseau, graphes, luminosité LED, intervalle d'échantillonnage) peut être exportée au format JSON depuis la page **Système** (ou via `GET /api/config/export`).

---
Fichier réservé :
- `include/board_config.h` (mapping matériel, dont les broches I2C `I2C_SDA_PIN`/`I2C_SCL_PIN` et la NeoLED)

Métadonnées :
Le nom/la version du projet sont injectés depuis `platformio.ini` via build flags.
