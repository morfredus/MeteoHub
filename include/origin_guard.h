#pragma once
#include <string>

// -----------------------------------------------------------------------------
// Garde anti-CSRF des ecritures HTTP (logique pure, testee sur l'hote).
//
// Une page web visitee depuis le LAN ne doit pas pouvoir declencher une ecriture sur
// le hub (effacement de l'historique, mise a jour OTA...). Un navigateur envoie TOUJOURS
// l'en-tete `Origin` sur une ecriture cross-origin ; les clients non navigateur
// (curl, scripts, autres services du parc) n'en envoient pas et passent.
//
// Regle : pour POST / PUT / PATCH / DELETE, si `Origin` est present et que son
// hote:port differe de l'en-tete `Host`, la requete est refusee (403).
// -----------------------------------------------------------------------------
namespace mhorigin {

// Extrait « hote:port » d'une valeur Origin (« http://meteohub.local:80 ») ; le port par
// defaut (80 / 443) est rendu explicite pour comparer « meteohub.local » a « meteohub.local:80 ».
inline std::string originHostPort(const std::string& origin) {
    std::string o = origin;
    int defPort = 80;
    const size_t sch = o.find("://");
    if (sch != std::string::npos) {
        if (o.compare(0, sch, "https") == 0) defPort = 443;
        o = o.substr(sch + 3);
    }
    const size_t slash = o.find('/');
    if (slash != std::string::npos) o = o.substr(0, slash);
    if (o.find(':') == std::string::npos) o += ":" + std::to_string(defPort);
    for (char& c : o) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return o;
}

inline std::string hostHeaderHostPort(const std::string& host) {
    std::string h = host;
    if (h.find(':') == std::string::npos) h += ":80";
    for (char& c : h) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return h;
}

// true = la requete d'ecriture est autorisee.
inline bool writeAllowed(const std::string& origin, const std::string& host) {
    if (origin.empty()) return true;            // client non navigateur
    if (origin == "null") return false;         // contexte opaque (iframe sandbox, file://)
    return originHostPort(origin) == hostHeaderHostPort(host);
}

}  // namespace mhorigin
