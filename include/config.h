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
#if defined(BOARD_S3_SUPERMINI)
#define OLED_CONTROLLER          OLED_CTRL_SSD1306 // JMD0.96D-1 : 0,96" SSD1306 128x64
#else
#define OLED_CONTROLLER          OLED_CTRL_SH1106  // 1,3" SH1106 128x64
#endif
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
#define BUTTON_GUARD_MS          200   // encodeur + boutons (DevKitC)

// Bouton de navigation unique (Super Mini) : au-delà de BUTTON_LONG_PRESS_MS
// maintenu, l'appui est « long » (menu / validation) ; en dessous, « court »
// (page / élément suivant).
#define BUTTON_DEBOUNCE_MS       30
#define BUTTON_LONG_PRESS_MS     800

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

// Notification « accu à changer » (via morfNotify), en TENSION : le % de la sonde
// est linéaire 2,6-4,2 V, alors qu'un Li-ion s'effondre sous ~3,4 V (20 % linéaire
// = 2,9 V, déjà presque vide ; coupure de protection à 2,4 V). Voir
// battery_alert_logic.h pour l'anti-rebond et l'hystérésis.
#define BATTERY_ALERT_WARN_V       3.40f  // « à remplacer dans les prochains jours »
#define BATTERY_ALERT_CRIT_V       3.20f  // « à remplacer maintenant »
#define BATTERY_ALERT_REARM_V      3.80f  // accu changé : surveillance réarmée
#define BATTERY_ALERT_CONFIRM      3      // trames live consécutives (~15 min à 5 min)

// ===============
// Monitoring réseau (logs par UDP)
// ===============
// Diffuse les logs (applicatifs + cœur ESP : WiFi, I2C, watchdog…) par UDP, pour
// les suivre à distance sans câble série (ex. dans Tabby). Voir docs/user_guide.md.
#define UDP_LOG_ENABLED          1                     // 1 = activer, 0 = désactiver
#define UDP_LOG_PORT             5005                  // port UDP d'écoute côté PC
#define UDP_LOG_HOST             "255.255.255.255"     // IP du PC récepteur, ou 255.255.255.255 pour un broadcast sur le réseau local

// ===============
// Auto-récupération mémoire (garde-fou contre le figeage web)
// ===============
// Symptôme observé : après plusieurs heures, l'UI web (ESPAsyncWebServer/AsyncTCP)
// cesse de répondre sans que l'appareil redémarre (les longues lectures SD
// réarment le watchdog, donc il ne se déclenche jamais). AsyncTCP a besoin de
// blocs CONTIGUS : une heap fragmentée le tue même quand le total libre semble
// suffisant. On surveille donc le total libre ET le plus gros bloc allouable ;
// si l'un reste critique assez longtemps, on redémarre proprement plutôt que de
// rester figé. Valeurs prudentes, ajustables selon la heap observée dans /status.
#define HEAP_GUARD_ENABLED       1
#define HEAP_MIN_FREE_BYTES      22000  // seuil de heap totale libre (octets)
#define HEAP_MIN_BLOCK_BYTES     10000  // seuil du plus gros bloc allouable (AsyncTCP)
#define HEAP_LOW_GRACE_MS        15000  // durée sous seuil avant redémarrage (ms)
#define HEAP_CHECK_PERIOD_MS     5000   // période de vérification (ms)

// ===============
// Écosystème morfSystem (morfAnalytics)
// ===============
// MORF_ECOSYSTEM_ENABLED coupe d'un coup les deux sens de l'écosystème :
//   - l'ÉMISSION du heartbeat morfBeacon (découverte de MeteoHub sur le LAN) ;
//   - l'ÉCOUTE du beacon morfAnalytics (lien « analyses avancées »).
// À passer à 0 sur un banc de test : sinon morfAnalytics découvrirait ce hub et
// mélangerait des mesures de test à l'historique de la vraie station.
// MeteoHub reste 100 % autonome dans les deux cas.
#ifndef MORF_ECOSYSTEM_ENABLED
#define MORF_ECOSYSTEM_ENABLED   1
#endif

// Détection passive de morfAnalytics : il est reconnu à sa capacité annoncée ;
// s'il est absent, le comportement nominal ne change en RIEN.
#define ANALYTICS_BEACON_ENABLED MORF_ECOSYSTEM_ENABLED
#define ANALYTICS_BEACON_PORT    45454          // port morfBeacon (protocole morfbeacon/1)
#define ANALYTICS_APP_NAME       "morfAnalytics" // nom d'app annoncé à détecter
#define ANALYTICS_TIMEOUT_MS     60000          // au-delà (sans heartbeat), considéré hors ligne
