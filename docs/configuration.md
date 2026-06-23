# Configuration

**Débutant ?** Voir le [Guide Débutant](beginner/index.md)

Version minimale valide : 1.2.0


## Fichiers modifiables
- `include/secrets.h` (identifiants, clé API, coordonnées)
- `include/config.h` (tous les paramètres modifiables par l'utilisateur, voir ci-dessous)

## Paramètres utilisateur dans config.h

### Paramètres graphiques
- `GRAPH_SCALE_MODE` : Mode d'échelle pour les graphes (0=fixe, 1=dynamic, 2=mixte)
- `GRAPH_SCALE_MARGIN_PCT` : Pourcentage d'élargissement pour le mode mixte
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

---
Fichier réservé :
- `include/board_config.h` (mapping matériel)

Métadonnées :
Le nom/la version du projet sont injectés depuis `platformio.ini` via build flags.
