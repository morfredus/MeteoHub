#pragma once
#include <Arduino.h>
#include <string>

// -----------------------------------------------------------------------------
// AnalyticsBeacon : détection PASSIVE et OPTIONNELLE d'un service d'analyse.
//
// Un service d'analyse annonce sa présence sur le LAN via un heartbeat morfBeacon
// (JSON compact en broadcast UDP, protocole "morfbeacon/1"). MeteoHub se contente
// de l'écouter : aucune requête, aucune dépendance. Si un tel service est présent,
// isAvailable() devient vrai et l'interface propose les analyses avancées ; sinon,
// MeteoHub fonctionne exactement comme s'il n'existait pas.
//
// --- Détection par CAPACITÉ, jamais par nom ---------------------------------
// Le service est reconnu à la capacité qu'il annonce (`advanced_analysis`), et
// NON à son nom. morfSystem étant sous licence GPL, chacun peut renommer son
// service (« Mon Analyse Météo », « Weather Lab »…) ; une détection fondée sur le
// nom cesserait de fonctionner au premier renommage. Le nom annoncé n'est donc
// utilisé que comme LIBELLÉ affiché dans l'interface.
//
// --- Adresse manuelle de secours --------------------------------------------
// La découverte automatique suppose que les broadcasts UDP atteignent MeteoHub,
// ce qui n'est pas le cas sur un réseau segmenté, à travers un VPN ou un point
// d'accès isolant les clients. Une adresse peut alors être saisie à la main
// (page Système) ; elle est persistée en NVS et prend le pas sur la découverte.
//
// MeteoHub reste la source de vérité : ce module ne fait que CONSTATER une
// présence, il n'interroge jamais le service.
// -----------------------------------------------------------------------------

// Capacité recherchée. Identifiant stable du protocole morfBeacon, à ne pas
// confondre avec le nom d'une application.
#ifndef ANALYTICS_CAPABILITY
#define ANALYTICS_CAPABILITY "advanced_analysis"
#endif

class AnalyticsBeacon {
public:
    void begin();   // ouvre l'écoute UDP + charge l'adresse manuelle (après WiFi)
    void update();  // à appeler dans loop() : traite les heartbeats reçus

    // --- Service découvert automatiquement -----------------------------------
    bool isDetected() const;  // true si un heartbeat récent porte la capacité
    const std::string& name() const { return _name; }       // nom ANNONCÉ (libellé)
    const std::string& version() const { return _version; }
    const std::string& host() const { return _host; }
    int statusPort() const { return _statusPort; }
    long lastSeenAgoSec() const; // secondes depuis la dernière détection, -1 si jamais

    // --- Adresse manuelle ----------------------------------------------------
    const std::string& manualUrl() const { return _manualUrl; }
    void setManualUrl(const std::string& url); // "" pour revenir en automatique

    // --- Résultat effectif ---------------------------------------------------
    // L'adresse manuelle prime : elle n'est renseignée que si l'utilisateur a
    // constaté que la découverte ne fonctionnait pas chez lui.
    bool isAvailable() const;      // une adresse utilisable existe-t-elle ?
    std::string effectiveUrl() const;  // adresse à ouvrir, "" si aucune
    bool isManual() const { return !_manualUrl.empty(); }

private:
    bool _started = false;
    bool _everSeen = false;
    unsigned long _lastSeenMs = 0;
    std::string _name;
    std::string _version;
    std::string _host;
    int _statusPort = 0;
    std::string _manualUrl;

    void loadManualUrl();
};
