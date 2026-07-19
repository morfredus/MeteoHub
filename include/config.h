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

// ===============
// Paramètres système
// ===============
#define DASHBOARD_REFRESH_MS     1000
#define BUTTON_GUARD_MS          200

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
