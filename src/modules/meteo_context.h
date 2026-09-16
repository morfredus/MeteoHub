#pragma once
#include <Arduino.h>

// ============================================================================
// Contextes de mesure MeteoHub
// Séparation explicite entre mesures intérieures (IN) et extérieures (OUT)
// ============================================================================

// Contexte de mesure : IN = intérieur, OUT = extérieur
enum class MeteoContext : uint8_t {
    IN  = 0,  // Mesures capteurs locaux (AHT20/BMP280) - intérieur
    OUT = 1   // Mesures reçues par ESP-NOW depuis MeteoHubSensor - extérieur
};

// Données météo intérieures (capteurs locaux AHT20/BMP280)
struct IndoorData {
    float temperature;
    float humidity;
    float pressure;
    bool valid;
    
    IndoorData() : temperature(0), humidity(0), pressure(0), valid(false) {}
};

// Données météo extérieures (reçues par ESP-NOW depuis MeteoHubSensor)
struct OutdoorData {
    float temperature;
    float humidity;
    float pressure;
    bool valid;
    
    // Extensions futures (vent, pluie, UV) - préparées pour MeteoFieldFlags
    float wind_speed;
    float wind_gust;
    uint16_t wind_direction_deg;
    float rain_rate;
    float rain_accumulated;
    float solar_lux;
    float uv_index;

    // État d'alimentation de la sonde déportée (sur pile/accu). has_battery=false
    // tant qu'aucune trame ne porte le champ batterie (FIELD_BATTERY).
    float battery_voltage;
    uint8_t battery_percent;
    bool has_battery;

    // Diagnostic de la sonde (paquet v2) : raison du dernier reset + compteur de
    // reveils + numero de trame. Journalises a chaque reception pour comprendre,
    // au retour d'une trame apres un trou, pourquoi la sonde a decroche.
    uint8_t reset_reason;
    uint16_t wake_count;
    uint32_t sequence;

    OutdoorData() : temperature(0), humidity(0), pressure(0), valid(false),
                    wind_speed(0), wind_gust(0), wind_direction_deg(0),
                    rain_rate(0), rain_accumulated(0), solar_lux(0), uv_index(0),
                    battery_voltage(0), battery_percent(0), has_battery(false),
                    reset_reason(0), wake_count(0), sequence(0) {}
};

// Structure générique de mesure avec contexte explicite
struct MeteoData {
    MeteoContext context;
    float temperature;
    float humidity;
    float pressure;
    bool valid;
    
    // Extensions futures (vent, pluie, UV)
    float wind_speed;
    float wind_gust;
    uint16_t wind_direction_deg;
    float rain_rate;
    float rain_accumulated;
    float solar_lux;
    float uv_index;
    
    MeteoData() : context(MeteoContext::IN), temperature(0), humidity(0), pressure(0), valid(false),
                  wind_speed(0), wind_gust(0), wind_direction_deg(0),
                  rain_rate(0), rain_accumulated(0), solar_lux(0), uv_index(0) {}
};

// Convertit IndoorData en MeteoData (contexte IN)
inline MeteoData indoorToMeteo(const IndoorData& in) {
    MeteoData m;
    m.context = MeteoContext::IN;
    m.temperature = in.temperature;
    m.humidity = in.humidity;
    m.pressure = in.pressure;
    m.valid = in.valid;
    return m;
}

// Convertit OutdoorData en MeteoData (contexte OUT)
inline MeteoData outdoorToMeteo(const OutdoorData& out) {
    MeteoData m;
    m.context = MeteoContext::OUT;
    m.temperature = out.temperature;
    m.humidity = out.humidity;
    m.pressure = out.pressure;
    m.valid = out.valid;
    m.wind_speed = out.wind_speed;
    m.wind_gust = out.wind_gust;
    m.wind_direction_deg = out.wind_direction_deg;
    m.rain_rate = out.rain_rate;
    m.rain_accumulated = out.rain_accumulated;
    m.solar_lux = out.solar_lux;
    m.uv_index = out.uv_index;
    return m;
}

// ============================================================================
// Lecture effective : quelle valeur afficher/utiliser, SANS perdre sa provenance
// ============================================================================
// OUT est la référence extérieure. Quand OUT est momentanément indisponible, on
// peut se replier sur IN pour ne pas laisser un champ vide — mais la valeur
// reste marquée comme provenant de IN (fallback), jamais maquillée en mesure
// extérieure. Ce résolveur est le point unique de cette décision, partagé par
// l'OLED et l'interface web (étapes 5 et 6).

enum class MeteoSourceState : uint8_t {
    UNAVAILABLE = 0, // aucune valeur exploitable (ni OUT ni IN)
    FRESH,           // OUT récent
    STALE,           // OUT présent mais périmé (dernière valeur connue)
    FALLBACK         // valeur de secours issue de IN (OUT indisponible)
};

struct EffectiveReading {
    float value = 0.0f;
    MeteoContext source = MeteoContext::OUT;
    MeteoSourceState state = MeteoSourceState::UNAVAILABLE;
    bool valid = false; // false = aucune valeur (ni OUT ni IN)
};

// Résout une grandeur (température, humidité...) en préservant sa provenance.
// `outAgeMs` = âge de la dernière valeur OUT. Les seuils viennent de config.h
// (OUTDOOR_FRESH_MAX_MS, OUTDOOR_UNAVAILABLE_MS).
inline EffectiveReading resolveEffective(
    bool outValid, float outValue, unsigned long outAgeMs,
    bool inValid, float inValue,
    unsigned long freshMaxMs, unsigned long unavailableMs) {
    EffectiveReading r;
    // OUT exploitable tant qu'il n'est pas déclaré indisponible.
    if (outValid && outAgeMs <= unavailableMs) {
        r.value = outValue;
        r.source = MeteoContext::OUT;
        r.state = (outAgeMs <= freshMaxMs) ? MeteoSourceState::FRESH
                                           : MeteoSourceState::STALE;
        r.valid = true;
        return r;
    }
    // OUT indisponible : repli explicite sur IN (provenance conservée).
    if (inValid) {
        r.value = inValue;
        r.source = MeteoContext::IN;
        r.state = MeteoSourceState::FALLBACK;
        r.valid = true;
        return r;
    }
    // Ni OUT ni IN : rien à afficher.
    return r;
}

// Libellés courts pour l'affichage et le JSON.
inline const char* meteoSourceLabel(MeteoContext c) {
    return (c == MeteoContext::OUT) ? "OUT" : "IN";
}
inline const char* meteoStateLabel(MeteoSourceState s) {
    switch (s) {
        case MeteoSourceState::FRESH:    return "fresh";
        case MeteoSourceState::STALE:    return "stale";
        case MeteoSourceState::FALLBACK: return "fallback";
        default:                         return "unavailable";
    }
}
