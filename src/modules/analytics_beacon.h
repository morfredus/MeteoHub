#pragma once
#include <Arduino.h>
#include <string>

// -----------------------------------------------------------------------------
// AnalyticsBeacon : détection PASSIVE et OPTIONNELLE du service morfAnalytics.
//
// morfAnalytics annonce sa présence sur le LAN via un heartbeat morfBeacon (JSON
// compact en broadcast UDP, protocole "morfbeacon/1"). MeteoHub se contente de
// l'écouter : aucune requête, aucune dépendance. Si le service est présent,
// isAvailable() devient vrai et l'interface peut proposer les analyses avancées ;
// sinon, MeteoHub fonctionne exactement comme s'il n'existait pas.
//
// MeteoHub reste la source de vérité : ce module ne fait que CONSTATER une présence.
// -----------------------------------------------------------------------------
class AnalyticsBeacon {
public:
    void begin();   // ouvre l'écoute UDP (après connexion WiFi)
    void update();  // à appeler dans loop() : traite les heartbeats reçus

    bool isAvailable() const; // true si un heartbeat récent a été reçu
    const std::string& host() const { return _host; }
    const std::string& version() const { return _version; }
    int statusPort() const { return _statusPort; }
    long lastSeenAgoSec() const; // secondes depuis la dernière détection, -1 si jamais

private:
    bool _started = false;
    bool _everSeen = false;
    unsigned long _lastSeenMs = 0;
    std::string _host;
    std::string _version;
    int _statusPort = 0;
};
