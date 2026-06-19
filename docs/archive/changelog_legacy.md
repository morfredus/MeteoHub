
# Journal des modifications du projet
 
Version minimale valide : 1.0.165

## Version 1.0.165
- **Refactorisation (Affichage OLED)** : Renommage de tous les fichiers et références `sh1106_display.*` en `oled_display.*` pour clarifier et permettre la gestion multi-contrôleur. Toute la documentation et le code utilisent désormais la dénomination OLED neutre.
- **Fonctionnalité (Support UTF-8 OLED)** : Amélioration de l'affichage des caractères accentués/UTF-8 sur OLED en remplaçant tous les appels à `drawStr`/`getStrWidth` par `drawUTF8`/`getUTF8Width` dans la classe d'affichage OLED. Cela garantit l'affichage correct des caractères français et accentués sur tous les OLED supportés.
- **Documentation** : Guides utilisateur (EN/FR) mis à jour pour expliquer le nouveau nommage OLED et le support d'affichage UTF-8. Version minimale valide portée à 1.0.165.

## Version 1.0.154
- **Documentation (Rafraîchissement complet)** : Mise à jour de l’ensemble de la documentation utilisateur EN/FR vers la baseline OTA/OLED/SD actuelle et synchronisation des versions minimales valides.
- **Documentation (UX OTA)** : Ajout du parcours OTA web explicite (`/ota.html`, barre de progression, zone inline de validation, retour automatique au tableau de bord).
- **Notes de release** : Clarification sur l’OTA web basé sur les capacités natives ESP32 `Update`/ArduinoOTA, sans nouvelle dépendance externe.

## Version 1.0.153
- **Correctif (Compilation OTA Web)** : Déclaration des membres d’état OTA manquants (`_ota_upload_error`, `_ota_restart_at_ms`) dans `WebManager` pour corriger les erreurs de build des handlers OTA.
- **Stabilité (OTA Runtime)** : Réinitialisation des variables d’état OTA au démarrage du WebManager pour garantir un comportement déterministe des uploads OTA.

## Version 1.0.152
- **Fonctionnalité (Page Web OTA)** : Ajout d’une page OTA dédiée avec envoi firmware, barre de progression en direct et zone de statut inline à hauteur fixe.
- **Fonctionnalité (API OTA)** : Ajout de l’endpoint d’upload `/api/ota/update` avec retour de validation firmware et redémarrage automatique planifié en cas de succès.
- **UX (Flux OTA)** : Après une mise à jour OTA réussie, l’interface redirige automatiquement vers le tableau de bord principal.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).

## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (avec alias de compatibilité) pour refléter le backend U8g2 compatible SH1106 et SSD1306.
- **Nettoyage (Layout OLED)** : Centralisation des principales constantes de géométrie OLED dans le renderer de pages pour réduire les valeurs magiques et faciliter la maintenance.

## Version 1.0.148
- **Correctif (Récupération écriture SD)** : Durcissement du chemin d’écriture de l’historique SD avec diagnostics explicites (`mkdir` en échec, `open` en échec) et détection stricte des écritures partielles.
- **Résilience (SD Runtime)** : En cas d’échec d’ouverture en `FILE_APPEND`, le firmware tente désormais un remount SD immédiat puis une unique tentative de réécriture.

## Version 1.0.147
- **Nettoyage (Migration OLED)** : Archivage des recommandations obsolètes sur l’ancienne bibliothèque OLED et alignement de la documentation sur la baseline actuelle U8g2.
- **Documentation (Feuille de route)** : Ajout de fichiers TODO bilingues dédiés aux améliorations code et expérience utilisateur (`docs/todo.md`, `docs/todo.md`).


## Version 1.0.146
- **Correctif (Mapping SPI SD)** : `SdManager` utilise désormais les macros SD dédiées (`SD_SCK_PIN`, `SD_MISO_PIN`, `SD_MOSI_PIN`) de `board_config.h` pour le montage et l'initialisation bas niveau du formatage, afin d'éviter les décalages silencieux de pins SPI.
- **Diagnostic (Runtime SD)** : Ajout d'un log explicite du mapping des pins SD (CS/SCK/MISO/MOSI) avant les tentatives de montage, pour accélérer le dépannage câblage/runtime.

## Version 1.0.151
- **Fonctionnalité (OTA)** : Ajout de la prise en charge ArduinoOTA avec liaison du hostname (`WEB_MDNS_HOSTNAME`) et traitement runtime dans la boucle principale.
- **Diagnostics (OTA)** : Ajout des logs du cycle OTA (start/end/progress/error) et d’un démarrage tardif si le WiFi devient disponible après le boot.

## Version 1.0.150
- **Fonctionnalité (Page météo OLED)** : Ajout du texte météo actuel (issu d’OpenWeatherMap, localisé selon `lang`) sur la première page OLED, affiché sous la pression atmosphérique.
- **UI (Lisibilité OLED)** : Ajout d’un fallback et d’une troncature sûre pour la ligne météo actuelle afin de conserver une mise en page stable sur écran 128x64.

## Version 1.0.149
- **Refactorisation (Nommage OLED)** : Renommage du module de pages OLED en `pages_oled.*` et mise à jour des fonctions de pages/splash OLED vers des suffixes neutres `_oled`.
- **Refactorisation (Classe Affichage)** : Renommage de la classe d’affichage en `OledDisplay` (