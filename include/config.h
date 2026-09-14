#pragma once


// =====================
// Paramètres graphiques
// =====================
#define GRAPH_SCALE_FIXED        0   // Utiliser min/max fixes
#define GRAPH_SCALE_DYNAMIC      1   // Utiliser min/max dynamiques
#define GRAPH_SCALE_MIXED        2   // Mixte : dynamique élargi
#define GRAPH_SCALE_MODE         GRAPH_SCALE_MIXED // Mode d'échelle pour les graphes
#define GRAPH_SCALE_MARGIN_PCT   50   // Zoom du mode mixte : 0 = échelle complète (min/max fixes), 100 = amplitude exacte des données
#define GRAPH_TEMP_MIN           -10.0f   // Température minimale affichée (°C)
#define GRAPH_TEMP_MAX            40.0f   // Température maximale affichée (°C)
#define GRAPH_HUM_MIN             20.0f   // Humidité minimale affichée (%)
#define GRAPH_HUM_MAX             90.0f   // Humidité maximale affichée (%)
#define GRAPH_PRES_MIN           970.0f   // Pression minimale affichée (hPa)
#define GRAPH_PRES_MAX          1040.0f   // Pression maximale affichée (hPa)

// ===============
// Paramètres OLED
// ===============
#define OLED_CONTRAST            180
#define OLED_CTRL_SH1106         1
#define OLED_CTRL_SSD1306        2
#define OLED_CONTROLLER          OLED_CTRL_SH1106
#define OLED_I2C_ADDRESS         0x3C

// ===============
// Paramètres réseau
// ===============
#define WEB_MDNS_HOSTNAME        "meteohub" // Accessible via http://meteohub.local
#define WIFI_RETRY_DELAY_MS      5000
#define ENABLE_PING_TEST         1

// SoftAP local : balise de canal pour la sonde (scan SSID, pas d'association).
// WPA2 : ce n'est pas un LAN, juste pour ne pas laisser un AP ouvert.
#define ESPNOW_SOFTAP_SSID       "MH-NOW"
#define ESPNOW_SOFTAP_PASS       "mhnowesp"
#define ESPNOW_WIFI_CHANNEL      6  // Repli tant que le STA Livebox n'a pas le canal

// ===============
// Paramètres système
// ===============
#define DASHBOARD_REFRESH_MS     1000
#define BUTTON_GUARD_MS          200

// ===============
// Cadence de MESURE intérieure (IN) -> historique
// ===============
// Cadence à laquelle MeteoHub ACQUIERT une mesure intérieure et l'enregistre à
// l'historique. À aligner sur la cadence de la sonde extérieure
// (SENSOR_MEASUREMENT_INTERVAL_SECONDS côté MeteoHubSensor) pour que les séries
// IN et OUT soient homogènes. La météo évolue lentement : 5 min par défaut.
// À NE PAS confondre avec le rafraîchissement de l'UI (DASHBOARD_REFRESH_MS) :
// l'interface lit la dernière mesure connue sans provoquer d'acquisition.
// Tests : 30 / 120 / 300 / 600 (30 s / 2 / 5 / 10 min).
#define INDOOR_MEASUREMENT_INTERVAL_SECONDS 300

// ===============
// Fraîcheur des données extérieures (OUT / ESP-NOW)
// ===============
// La sonde MeteoHubSensor émet une trame toutes les ~30 s. On qualifie donc la
// dernière valeur OUT reçue :
//   - FRAÎCHE   tant que son âge <= OUTDOOR_FRESH_MAX_MS ;
//   - PÉRIMÉE   au-delà (on affiche encore la dernière connue, signalée périmée) ;
//   - INDISPO.  au-delà de OUTDOOR_UNAVAILABLE_MS -> repli possible sur IN.
// Ces seuils servent au résolveur de lecture effective (meteo_context.h), pas à
// l'archivage : une valeur périmée reste une vraie mesure extérieure historisée.
// Seuils calés sur la cadence de la sonde (5 min par défaut, cf.
// SENSOR_MEASUREMENT_INTERVAL_SECONDS côté MeteoHubSensor). Frais = au plus une
// trame de retard + gigue ; indisponible = plusieurs trames manquées.
#define OUTDOOR_FRESH_MAX_MS       420000UL   // 7 min : une trame (5 min) + marge
#define OUTDOOR_UNAVAILABLE_MS     1200000UL  // 20 min : ~4 trames manquées -> absente

// Seuil d'alerte batterie de la sonde extérieure (déportée, sur pile/accu).
// En dessous, l'OLED et l'interface web signalent une pile faible.
#define OUTDOOR_BATTERY_LOW_PCT    20

// ===============
// Monitoring réseau (logs par UDP)
// ===============
// Diffuse les logs (applicatifs + cœur ESP : WiFi, I2C, watchdog…) par UDP, pour
// les suivre à distance sans câble série (ex. dans Tabby). Voir docs/user_guide.md.
#define UDP_LOG_ENABLED          1                     // 1 = activer, 0 = désactiver
#define UDP_LOG_PORT             5005                  // port UDP d'écoute côté PC
#define UDP_LOG_HOST             "255.255.255.255"     // IP du PC récepteur, ou 255.255.255.255 pour un broadcast sur le réseau local

// ===============
// Détection morfAnalytics (écosystème morfSystem, OPTIONNEL)
// ===============
// MeteoHub reste 100 % autonome. Il écoute passivement le heartbeat morfBeacon du
// service morfAnalytics (analyses avancées) sur le LAN : si présent, l'interface le
// signale ; s'il est absent, le comportement nominal ne change en RIEN.
#define ANALYTICS_BEACON_ENABLED 1
#define ANALYTICS_BEACON_PORT    45454          // port morfBeacon (protocole morfbeacon/1)
#define ANALYTICS_APP_NAME       "morfAnalytics" // nom d'app annoncé à détecter
#define ANALYTICS_TIMEOUT_MS     60000          // au-delà (sans heartbeat), considéré hors ligne
