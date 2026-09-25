#if defined(ESP32_S3_OLED)
#include "display_interface.h"
#include "oled_display.h"
#include <algorithm>
#include <cctype>
#include <float.h>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <time.h>
#include "pages_oled.h"
#include "config.h"
#include "battery_alert.h"
#include "espnow_receiver.h"
#include "../utils/logs.h"
#include "../utils/system.h"
#include <Arduino.h>
#include <WiFi.h>

namespace {
constexpr int OLED_WIDTH = 128;
#if OLED_CONTROLLER == OLED_CTRL_SSD1306
constexpr int OLED_SAFE_TOP_Y = 16;
#else
constexpr int OLED_SAFE_TOP_Y = 13;
#endif
constexpr int OLED_HEADER_Y = 0;
constexpr int OLED_CONTENT_TOP_Y = OLED_SAFE_TOP_Y;
constexpr int OLED_LINE_1_Y = OLED_CONTENT_TOP_Y;
constexpr int OLED_LINE_2_Y = OLED_CONTENT_TOP_Y + 12;
constexpr int OLED_LINE_3_Y = OLED_CONTENT_TOP_Y + 24;
constexpr int OLED_LINE_4_Y = OLED_CONTENT_TOP_Y + 36;
constexpr int OLED_LINE_5_Y = OLED_CONTENT_TOP_Y + 46;
constexpr int OLED_GRAPH_X = 0;
constexpr int OLED_GRAPH_Y = OLED_SAFE_TOP_Y;
constexpr int OLED_GRAPH_W = 90;
constexpr int OLED_GRAPH_H = 28;
constexpr int OLED_VALUE_COL_X = 92;
constexpr int OLED_TIME_BOTTOM_Y = OLED_SAFE_TOP_Y + 38;
constexpr int OLED_BOOT_BAR_W = 100;
constexpr int OLED_BOOT_BAR_H = 10;

std::string shortenWeatherDescriptionForOled(const std::string& description) {
	if (description.empty()) {
		return "N/A";
	}

	std::string shortened = description;

	auto replaceAll = [&shortened](const std::string& from, const std::string& to) {
		size_t position = 0;
		while ((position = shortened.find(from, position)) != std::string::npos) {
			shortened.replace(position, from.size(), to);
			position += to.size();
		}
	};

	replaceAll("Partiellement", "Part.");
	replaceAll("partiellement", "part.");
	replaceAll("Nuageux", "Nuag.");
	replaceAll("nuageux", "nuag.");
	replaceAll("Couvert", "Couv.");
	replaceAll("couvert", "couv.");
	replaceAll("Légère", "Lég.");
	replaceAll("légère", "lég.");

	return shortened;
}

int safeTopForDisplay() {
    return OLED_SAFE_TOP_Y;
}
}

// 1. Formate un float avec précision donnée
std::string formatFloat(float value, int precision) {
	std::ostringstream stream;
	stream << std::fixed << std::setprecision(precision) << value;
	return stream.str();
}
 
// 2. Génère l'entête de page avec titre, heure et pagination
std::string getHeader(const std::string& title, int pageIndex, int pageCount) {
	struct tm timeinfo;
	char timeStr[6];
	if (getLocalTime(&timeinfo)) {
		strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
	} else {
		strcpy(timeStr, "--:--");
	}
	return title + " " + std::string(timeStr) + " (" + std::to_string(pageIndex) + "/" + std::to_string(pageCount) + ")";
}

// 3. Traduit un événement météo en français lisible
std::string translateAlert(const std::string& event) {
	std::string lowerEvent = event;
	std::transform(lowerEvent.begin(), lowerEvent.end(), lowerEvent.begin(), [](unsigned char character) {
		return static_cast<char>(std::tolower(character));
	});

	if (lowerEvent.find("crue") != std::string::npos) return "Vigilance Crues";
	if (lowerEvent.find("flood") != std::string::npos || lowerEvent.find("inond") != std::string::npos) return "Vigilance Inondation";
	if (lowerEvent.find("rain") != std::string::npos || lowerEvent.find("pluie") != std::string::npos) return "Vigilance Pluie";
	if (lowerEvent.find("wind") != std::string::npos || lowerEvent.find("vent") != std::string::npos) return "Vigilance Vent";
	if (lowerEvent.find("thunderstorm") != std::string::npos) return "Vigilance Orages";
	if (lowerEvent.find("snow") != std::string::npos) return "Vigilance Neige";
	if (lowerEvent.find("heat") != std::string::npos) return "Vigilance Canicule";
	if (lowerEvent.find("cold") != std::string::npos) return "Vigilance Grand Froid";
	if (lowerEvent.find("ice") != std::string::npos) return "Vigilance Verglas";
	return event; // Fallback si aucune traduction
}

// 4. Page réseau : affiche SSID, IP, RSSI
void pageNetwork_oled(DisplayInterface& d, WifiManager& wifi, int pageIndex, int pageCount) {
	d.clear();
	d.text(0, OLED_HEADER_Y, getHeader("Net.", pageIndex, pageCount));

	d.text(0, OLED_LINE_1_Y, std::string("SSID: ") + wifi.ssid());
	d.text(0, OLED_LINE_2_Y, std::string("IP:   ") + wifi.ip());
	{
		const int ch = wifi.channel();
		// Canal > 13 = 5 GHz : ESP-NOW de la sonde C3 n'arrivera jamais.
		if (ch > 13) {
			d.text(0, OLED_LINE_3_Y, std::string("CH:") + std::to_string(ch) + " 5GHz!");
		} else {
			d.text(0, OLED_LINE_3_Y, std::string("CH:") + std::to_string(ch)
				+ "  " + std::to_string(wifi.rssi()) + "dBm");
		}
	}
	d.text(0, OLED_LINE_4_Y, wifi.mac());

	d.show();
}

// 5. Page système : affiche infos mémoire et version
void pageSystem_oled(DisplayInterface& d, int pageIndex, int pageCount) {
	SystemInfo s = getSystemInfo();

	d.clear();
	d.text(0, OLED_HEADER_Y, getHeader("Sys.", pageIndex, pageCount));

	d.text(0, OLED_LINE_1_Y, std::string("Heap:  ") + std::to_string(s.heapFree / 1024) + " KB");
	d.text(0, OLED_LINE_2_Y, std::string("PSRAM: ") + std::to_string(s.psramFree / 1024) + " KB");
	d.text(0, OLED_LINE_3_Y, std::string("Flash: ") + std::to_string(s.flashSize / 1024 / 1024) + " MB");
	d.text(0, OLED_LINE_4_Y, std::string("Ver:   ") + std::string(PROJECT_VERSION));

	d.show();
}

// 7b. Page Capteur : etat de la sonde exterieure (batterie, liaison ESP-NOW,
// fraicheur). Les metriques "materielles" du capteur ont leur page dediee ici,
// plutot que de deborder la page Systeme : l'OLED n'a la place que de 4 lignes.
void pageSensor_oled(DisplayInterface& d, HistoryManager& history, int pageIndex, int pageCount) {
	d.clear();
	d.text(0, OLED_HEADER_Y, getHeader("Capteur", pageIndex, pageCount));

	const unsigned long ageMs = history.outdoorAgeMs();
	const bool hasLive = history.hasLiveOutdoor() && ageMs != 0xFFFFFFFFUL;

	// Ligne 1 : batterie (tension + %) si la sonde la rapporte, sinon "absente".
	if (hasLive && history.lastOutdoorLive().has_battery) {
		const OutdoorData& live = history.lastOutdoorLive();
		char l[22];
		snprintf(l, sizeof(l), "Bat %.2fV %u%%", live.battery_voltage,
		         (unsigned)live.battery_percent);
		d.text(0, OLED_LINE_1_Y, l);
	} else {
		d.text(0, OLED_LINE_1_Y, "Bat: absente");
	}

	// Ligne 2 : canal ESP-NOW courant.
	uint8_t ch = 0;
	uint32_t rx = 0, ok = 0;
	if (EspNowReceiver::instance()) {
		ch = EspNowReceiver::instance()->getWifiChannel();
		rx = EspNowReceiver::instance()->getPacketsReceived();
		ok = EspNowReceiver::instance()->getPacketsValid();
	}
	d.text(0, OLED_LINE_2_Y, std::string("Canal: ") + std::to_string(ch));

	// Ligne 3 : sante de la liaison (trames recues / valides depuis le boot).
	char tr[22];
	snprintf(tr, sizeof(tr), "Trames rx%lu ok%lu",
	         (unsigned long)rx, (unsigned long)ok);
	d.text(0, OLED_LINE_3_Y, tr);

	// Ligne 4 : fraicheur de la derniere trame recue.
	if (hasLive) {
		const unsigned long mins = ageMs / 60000UL;
		if (mins == 0) d.text(0, OLED_LINE_4_Y, "Recu: <1min");
		else d.text(0, OLED_LINE_4_Y, std::string("Recu: ") + std::to_string(mins) + "min");
	} else {
		d.text(0, OLED_LINE_4_Y, "OUT absent");
	}

	d.show();
}

// 6. Page logs : affiche les derniers logs
void pageLogs_oled(DisplayInterface& d, int pageIndex, int pageCount, int scrollOffset) {
	d.clear();
	d.text(0, OLED_HEADER_Y, getHeader("Logs", pageIndex, pageCount));

    int totalLogs = getLogCount();
    // Sécurité si le buffer a changé entre temps
    if (scrollOffset >= totalLogs) scrollOffset = 0;

	int y = OLED_LINE_1_Y;
	for (int i = scrollOffset; i < totalLogs; i++) {
		d.text(0, y, getLog(i));
		y += 10;
		if (y > OLED_LINE_5_Y) break;
	}

	d.show();
}

// 7. Page météo : IN (capteurs locaux) + OUT (ESP-NOW). Pression = OUT uniquement.
void pageWeather_oled(DisplayInterface& d, SensorManager& sensors, ForecastManager& forecast, HistoryManager& history, int pageIndex, int pageCount) {
	d.clear();
	d.text(0, OLED_HEADER_Y, getHeader("Meteo", pageIndex, pageCount));

	SensorData indoorData = sensors.read(); // IN en direct (capteur local), sans archivage

	// Colonnes fixes pour aligner T et H entre IN et OUT (police 6x10, 21 car.).
	constexpr int COL_LABEL_X = 0;
	constexpr int COL_TEMP_X = 28;
	constexpr int COL_HUM_X = 80;

	if (indoorData.valid) {
		d.text(COL_LABEL_X, OLED_LINE_1_Y, "IN");
		d.text(COL_TEMP_X, OLED_LINE_1_Y, formatFloat(indoorData.temperature, 1) + "C");
		d.text(COL_HUM_X, OLED_LINE_1_Y, formatFloat(indoorData.humidity, 0) + "%");
	} else {
		d.text(COL_LABEL_X, OLED_LINE_1_Y, "IN");
		d.text(COL_TEMP_X, OLED_LINE_1_Y, "--.-C");
		d.text(COL_HUM_X, OLED_LINE_1_Y, "--%");
	}

	// Fraîcheur de l'OUT : on ne montre une valeur extérieure que si une trame
	// radio récente existe. Une valeur "live" seedée depuis le disque a un âge
	// énorme (aucune trame radio) -> traitée comme indisponible, pour ne jamais
	// afficher du vieux comme du courant. Seuils : voir config.h.
	float outT = 0, outH = 0, outP = 0;
	MeteoSourceState outState = MeteoSourceState::UNAVAILABLE;
	bool outBatLow = false;      // pile de la sonde sous le seuil
	uint8_t outBatPct = 0;
	const unsigned long outAge = history.outdoorAgeMs();
	if (history.hasLiveOutdoor()) {
		const OutdoorData& live = history.lastOutdoorLive();
		if (live.valid && outAge <= OUTDOOR_UNAVAILABLE_MS) {
			outT = live.temperature;
			outH = live.humidity;
			outP = live.pressure;
			outState = (outAge <= OUTDOOR_FRESH_MAX_MS) ? MeteoSourceState::FRESH
			                                            : MeteoSourceState::STALE;
		}
		if (live.has_battery) {
			outBatPct = live.battery_percent;
			// Même règle que la page web : % sous le seuil OU palier d'alerte en
			// tension (celui qui déclenche la notification morfNotify).
			outBatLow = (live.battery_percent <= OUTDOOR_BATTERY_LOW_PCT)
			            || batteryAlert.level() >= mhbat::LEVEL_LOW;
		}
	}
	const bool hasOut = (outState != MeteoSourceState::UNAVAILABLE);
	const bool hasOutP = hasOut && outP > 300.0f && outP < 1100.0f;

	// Repli : OUT indisponible mais IN valide -> on montre la valeur intérieure
	// à la place, préfixée d'un "I" discret. Ainsi la ligne OUT reste utile sans
	// jamais faire passer une mesure intérieure pour une vraie mesure extérieure.
	const bool outFallback = !hasOut && indoorData.valid;

	// Libellé OUT : "OUT" frais, "OUT~" périmé (dernière valeur connue affichée).
	d.text(COL_LABEL_X, OLED_LINE_2_Y,
	       (outState == MeteoSourceState::STALE) ? "OUT~" : "OUT");
	if (hasOut) {
		d.text(COL_TEMP_X, OLED_LINE_2_Y, formatFloat(outT, 1) + "C");
		d.text(COL_HUM_X, OLED_LINE_2_Y, formatFloat(outH, 0) + "%");
	} else if (outFallback) {
		// "I" = donnée de secours issue de l'intérieur.
		d.text(COL_TEMP_X, OLED_LINE_2_Y, std::string("I") + formatFloat(indoorData.temperature, 1) + "C");
		d.text(COL_HUM_X, OLED_LINE_2_Y, std::string("I") + formatFloat(indoorData.humidity, 0) + "%");
	} else {
		d.text(COL_TEMP_X, OLED_LINE_2_Y, "--.-C");
		d.text(COL_HUM_X, OLED_LINE_2_Y, "--%");
	}

	// Pression atmosphérique = sonde extérieure uniquement. Sur l'OLED on
	// n'affiche pas de repli IN (peu utile visuellement) ; la couche données
	// (/api/live effective.pres) fournit malgré tout le secours IN si besoin.
	d.text(COL_LABEL_X, OLED_LINE_3_Y, "P");
	d.text(COL_TEMP_X, OLED_LINE_3_Y,
	       hasOutP ? (formatFloat(outP, 0) + " hPa") : std::string("---- hPa"));

	// Ligne 4 : selon l'état OUT. Frais -> description météo. Périmé -> âge.
	// Absent -> debug radio si aucune trame reçue depuis le boot (distingue
	// "muet" de "CRC rejeté"), sinon signale le repli sur IN (affiché ci-dessus).
	if (hasOut && outBatLow) {
		// Alerte pile faible de la sonde : prioritaire sur la description météo.
		d.text(0, OLED_LINE_4_Y, std::string("PILE SONDE ")
			+ std::to_string(outBatPct) + "% !");
	} else if (outState == MeteoSourceState::FRESH) {
		d.center(OLED_LINE_4_Y, shortenWeatherDescriptionForOled(forecast.today.description));
	} else if (outState == MeteoSourceState::STALE) {
		d.text(0, OLED_LINE_4_Y, std::string("OUT perime ~")
			+ std::to_string(outAge / 60000UL) + "min");
	} else if (outAge == 0xFFFFFFFFUL) {
		uint8_t ch = 0;
		uint32_t rx = 0, ok = 0;
		if (EspNowReceiver::instance()) {
			ch = EspNowReceiver::instance()->getWifiChannel();
			rx = EspNowReceiver::instance()->getPacketsReceived();
			ok = EspNowReceiver::instance()->getPacketsValid();
		} else if (WiFi.status() == WL_CONNECTED) {
			ch = static_cast<uint8_t>(WiFi.channel());
		}
		d.text(0, OLED_LINE_4_Y, std::string("NOW ch") + std::to_string(ch)
			+ " rx" + std::to_string(rx) + " ok" + std::to_string(ok));
	} else if (outFallback) {
		// Secours IN déjà signalé par le "I" sur les valeurs : on garde la ligne
		// pour la description météo (prévision externe, indépendante de la sonde).
		d.center(OLED_LINE_4_Y, shortenWeatherDescriptionForOled(forecast.today.description));
	} else {
		d.text(0, OLED_LINE_4_Y, "OUT absent");
	}

	d.show();
}

// 8. Page graphique : historique IN (T/H) ou OUT (pression atmosphérique)
void pageGraph_oled(DisplayInterface& d, HistoryManager& history, int type, int pageIndex, int pageCount, bool outdoor) {
	   d.clear();
    
    std::string title;
    if (type == 0) title = outdoor ? "OUT Temp" : "IN Temp";
    else if (type == 1) title = outdoor ? "OUT Hum" : "IN Hum";
    else title = outdoor ? "OUT Pres" : "IN Pres";
    
    d.text(0, OLED_HEADER_Y, getHeader(title, pageIndex, pageCount));

    // Données : lues sur SD + RAM (comme la page web), et NON la RAM seule. Les
    // anciens buffers `getRecentHistory()` / `getOutdoorHistory()` ne contenaient
    // que le récent (quasi vide après un effacement ou un redémarrage) : l'OLED
    // paraissait vide alors que la carte SD a tout l'historique. On agrège ici la
    // même source que la page web sur une fenêtre de 24 h.
    //
    // Une seule requête sert les 3 grandeurs (T/H/P) ; on la met en cache car
    // l'OLED se rafraîchit à 1 Hz et relire la carte à chaque tick la martèlerait.
    struct GPoint { long ts; float t, h, p; bool tv, hv, pv; };
    static std::vector<GPoint> s_gCacheIn, s_gCacheOut;
    static uint32_t s_gCacheInMs = 0, s_gCacheOutMs = 0;
    constexpr uint32_t kGraphCacheTtlMs = 30000;      // 30 s
    constexpr long OLED_GRAPH_WINDOW_S = 24L * 3600L; // 24 h affichées
    const long gInterval = OLED_GRAPH_WINDOW_S / OLED_GRAPH_W; // ~960 s / tranche

    std::vector<GPoint>& cache = outdoor ? s_gCacheOut : s_gCacheIn;
    uint32_t& cacheMs = outdoor ? s_gCacheOutMs : s_gCacheInMs;
    const uint32_t nowMs = millis();
    if (cache.empty() || (nowMs - cacheMs) > kGraphCacheTtlMs) {
        cache.clear();
        const time_t now = time(NULL);
        const time_t from = now - OLED_GRAPH_WINDOW_S;
        if (outdoor) {
            auto pts = history.queryOutdoorRange(from, now, gInterval);
            for (const auto& p : pts)
                cache.push_back({(long)p.t, p.temp, p.hum, p.pres, p.tvalid, p.hvalid, p.pvalid});
        } else {
            auto pts = history.queryRange(from, now, gInterval);
            for (const auto& p : pts)
                cache.push_back({(long)p.t, p.temp, p.hum, p.pres, p.tvalid, p.hvalid, p.pvalid});
        }
        cacheMs = nowMs;
    }

    // Ne garde que les tranches valides pour la grandeur affichée : les tranches
    // vides sont absentes du tracé (pas de trou), on relie les points restants et
    // on ne coupe le trait que sur un vrai silence capteur (seuil plus bas).
    std::vector<HistoryRecord> records;
    records.reserve(cache.size());
    for (const auto& g : cache) {
        const bool ok = (type == 0) ? g.tv : (type == 1) ? g.hv : g.pv;
        if (!ok) continue;
        HistoryRecord h;
        h.timestamp = g.ts;
        h.t = g.t; h.h = g.h; h.p = g.p;
        records.push_back(h);
    }
    if (records.empty()) {
        d.center(OLED_LINE_3_Y, outdoor ? "OUT: pas de donnees" : "IN: pas de donnees");
        d.show();
        return;
    }

    int count = static_cast<int>(records.size());

    // Layout : Graphique a gauche, Valeurs a droite, Temps en bas
    int graphX = OLED_GRAPH_X;
    int graphY = OLED_GRAPH_Y;
    int graphW = OLED_GRAPH_W; // Largeur reduite pour laisser place au texte a droite
    int graphH = OLED_GRAPH_H; // Hauteur reduite pour laisser place au temps en bas
    int bottomY = graphY + graphH;

    // N'utiliser que la fenetre affichee (points les plus recents)
    int visibleCount = count;
    if (visibleCount > graphW + 1) {
        visibleCount = graphW + 1;
    }
    int startIndex = count - visibleCount;

    float minVal, maxVal;
    float userMin, userMax;
    if (type == 0) {
        userMin = GRAPH_TEMP_MIN;
        userMax = GRAPH_TEMP_MAX;
    } else if (type == 1) {
        userMin = GRAPH_HUM_MIN;
        userMax = GRAPH_HUM_MAX;
    } else {
        userMin = GRAPH_PRES_MIN;
        userMax = GRAPH_PRES_MAX;
    }

    if (GRAPH_SCALE_MODE == GRAPH_SCALE_FIXED) {
        minVal = userMin;
        maxVal = userMax;
    } else {
        // Calcul dynamique
        float dynMin = FLT_MAX;
        float dynMax = -FLT_MAX;
        for (int i = startIndex; i < count; i++) {
            float val;
            if (type == 0) val = records[i].t;
            else if (type == 1) val = records[i].h;
            else val = records[i].p;
            if (val < dynMin) dynMin = val;
            if (val > dynMax) dynMax = val;
        }
        if (dynMin == dynMax) {
            float margin = (type == 2) ? 0.5f : 0.3f;
            dynMin -= margin;
            dynMax += margin;
        }
        if (GRAPH_SCALE_MODE == GRAPH_SCALE_DYNAMIC) {
            minVal = dynMin;
            maxVal = dynMax;
        } else {
            // Mode mixte : élargissement
            float range = dynMax - dynMin;
            float margin = range * (GRAPH_SCALE_MARGIN_PCT / 100.0f);
            minVal = dynMin - margin;
            maxVal = dynMax + margin;
            // Limiter aux bornes utilisateur
            if (minVal < userMin) minVal = userMin;
            if (maxVal > userMax) maxVal = userMax;
        }
    }

    int labelPrecision = 0;
    if (type == 0) {
        labelPrecision = 1;
    } else if (type == 2) {
        labelPrecision = 1;
    }

    std::string maxLabel = formatFloat(maxVal, labelPrecision);
    std::string minLabel = formatFloat(minVal, labelPrecision);
    if (maxLabel == minLabel && labelPrecision < 2) {
        labelPrecision++;
        maxLabel = formatFloat(maxVal, labelPrecision);
        minLabel = formatFloat(minVal, labelPrecision);
    }

    // Affichage Echelle Valeurs (Droite)
    d.text(OLED_VALUE_COL_X, graphY, maxLabel);
    d.text(OLED_VALUE_COL_X, bottomY - 8, minLabel);

    // Affichage Echelle Temps (Bas) : la fenetre est desormais 24 h.
    d.text(OLED_GRAPH_X, OLED_TIME_BOTTOM_Y, "-24h");
    d.text(70, OLED_TIME_BOTTOM_Y, "now");

    // Seuil de coupure du trait : on relie les points par-dessus les tranches
    // vides et on ne coupe QUE sur un vrai silence capteur. max(2,5 tranches,
    // 20 min), meme principe que la page web et morfAnalytics. L'ancien seuil fixe
    // de 90 s etait un relicat de la cadence 1/min : a 5 min, il coupait TOUT.
    const long gapThresh = std::max((long)(2.5 * gInterval), 20L * 60L);

    // Drawing loop with gap detection
    for (int i = startIndex + 1; i < count; i++) {
        if (records[i].timestamp - records[i-1].timestamp > gapThresh) {
            continue;
        }

        float val1, val2;
        if (type == 0) { val1 = records[i-1].t; val2 = records[i].t; }
        else if (type == 1) { val1 = records[i-1].h; val2 = records[i].h; }
        else { val1 = records[i-1].p; val2 = records[i].p; }

        int localIndex1 = (i - 1) - startIndex;
        int localIndex2 = i - startIndex;
        int x1 = map(localIndex1, 0, visibleCount - 1, graphX, graphX + graphW);
        int x2 = map(localIndex2, 0, visibleCount - 1, graphX, graphX + graphW);
        int y1 = bottomY - (int)(((val1 - minVal) / (maxVal - minVal)) * graphH);
        int y2 = bottomY - (int)(((val2 - minVal) / (maxVal - minVal)) * graphH);
        d.drawLine(x1, y1, x2, y2);
    }
    d.show();
}

// 9. Page prévisions : affiche prévisions aujourd'hui, demain, alertes
void pageForecast_oled(DisplayInterface& d, ForecastManager& forecast, int view, int pageIndex, int pageCount) {
    d.clear();
    d.text(0, OLED_HEADER_Y, getHeader("Prev.", pageIndex, pageCount));

    if (view == 0) { // Vue "Aujourd'hui"
        d.center(OLED_LINE_1_Y, "Aujourd'hui");
        d.text(0, OLED_LINE_2_Y, std::string("Min: ") + formatFloat(forecast.today.temp_min, 0) + "C / Max: " + formatFloat(forecast.today.temp_max, 0) + "C");
        d.center(OLED_LINE_3_Y, forecast.today.description);

    } else if (view == 1) { // Vue "Demain"
        d.center(OLED_LINE_1_Y, "Demain");
        d.text(0, OLED_LINE_2_Y, std::string("Min: ") + formatFloat(forecast.tomorrow.temp_min, 0) + "C / Max: " + formatFloat(forecast.tomorrow.temp_max, 0) + "C");
        d.center(OLED_LINE_3_Y, forecast.tomorrow.description);

    } else { // Vue "Alertes"
        d.center(OLED_LINE_1_Y, "Alertes Météo");
        if (forecast.alert_active) {
            d.center(OLED_LINE_2_Y, forecast.alert.sender);
            
            // Traduction et affichage de l'alerte
            std::string event = translateAlert(forecast.alert.event);
            if (event.size() > 20) {
                size_t split = event.rfind(' ', 20);
                if (split == std::string::npos) split = 20;
                d.center(OLED_LINE_3_Y, event.substr(0, split));
                if (split + 1 < event.size()) {
                    d.center(OLED_LINE_4_Y, event.substr(split + 1));
                }
            } else {
                d.center(OLED_LINE_4_Y, event);
            }
        } else {
            d.center(OLED_LINE_4_Y, "Aucune alerte");
        }
    }
    d.show();
}

// --- Ecrans de démarrage (Splash & Boot) ---

void drawSplashScreen_oled(DisplayInterface& d) {
    OledDisplay& disp = static_cast<OledDisplay&>(d);
    disp.clear();
    
    // Animation simple pour OLED
    const int safe_top = safeTopForDisplay();
    disp.center(safe_top + 10, "morfredus");
    disp.drawLine(10, safe_top + 24, 118, safe_top + 24);
    
    disp.show();
    delay(2000);

    disp.clear();
    disp.center(safe_top + 8, PROJECT_NAME);
    disp.center(safe_top + 20, std::string("v") + PROJECT_VERSION);
    disp.center(safe_top + 36, "Initialisation...");
    disp.show();
    delay(1500);
}

void drawBootProgress_oled(DisplayInterface& d, int step, int total, const std::string& msg) {
    OledDisplay& disp = static_cast<OledDisplay&>(d);
    disp.clear();
    
    const int safe_top = safeTopForDisplay();
    disp.center(safe_top + 2, PROJECT_NAME);
    
    // Barre de progression
    int barW = OLED_BOOT_BAR_W;
    int barH = OLED_BOOT_BAR_H;
    int barX = (OLED_WIDTH - barW) / 2;
    int barY = safe_top + 20;
    
    disp.bar(barX, barY, barW, barH, step, total);
    
    disp.center(safe_top + 38, msg);
    disp.show();
}
#endif
