#pragma once

// =====================================================
// Sélection de la carte (define posé par l'env PlatformIO)
//   - BOARD_S3_SUPERMINI : env esp32-s3-supermini
//   - (défaut)           : env esp32-s3-oled, DevKitC-1 N16R8
// Le reste du firmware ne connaît que les alias *_PIN et les
// capacités (UI_SINGLE_BUTTON…) : ajouter une carte se fait ici.
// =====================================================

#if defined(BOARD_S3_SUPERMINI)

// =====================================================
// Carte : ESP32-S3 Super Mini (v0.2)
// 4 Mo flash / 2 Mo PSRAM quad — même carte que la sonde
// MeteoHubSensor (env esp32-s3-supermini).
//
// Montage simplifié (4 modules seulement) :
// - Capteur AHT20 + BMP280 (I2C)
// - Écran OLED 0,96" JMD0.96D-1 (SSD1306 128x64, I2C, même bus)
// - Module carte micro-SD (SPI)
// - Un bouton poussoir (défilement des pages / menu)
//
// Tout le I2C et le SPI SD sont sur les GPIO 8 à 13 : un seul
// côté de la carte, fils courts. GPIO 19/20 = USB natif, ne
// pas les utiliser. GPIO 3, 45, 46 = straps : laissés libres.
// =====================================================


// =====================================================
// I2C — bus partagé AHT20 (0x38) + BMP280 (0x76/0x77)
// + OLED SSD1306 (0x3C). Mêmes GPIO que la sonde.
// =====================================================
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9


// =====================================================
// Boutons
// =====================================================
// BOOT : bouton soudé sur la Super Mini (GPIO 0, pin de
// strapping). Entrée uniquement ; sert à la maintenance
// (maintenu au démarrage -> formatage LittleFS).
#define BUTTON_BOOT_PIN     0

// Bouton de navigation unique (poussoir entre GPIO 5 et GND,
// pull-up interne). Appui court = page suivante, appui long
// = menu / validation (voir managers/ui_manager.cpp).
#define BUTTON_NAV_PIN      5

// Capacité d'entrée : un seul bouton, pas d'encodeur ni de
// boutons Back / Confirm. Pilote l'UI et exclut encoder.cpp.
#define UI_SINGLE_BUTTON


// =====================================================
// LED RGB WS2812 soudée sur la Super Mini
// (GPIO 48 ; le pinout annote parfois GP46, qui est une
// entrée seule sur S3 et ne peut pas la piloter).
// =====================================================
#define NEOPIXEL_PIN 48


// =====================================================
// Module SD — SPI (FSPI), brochage identique à l'ancienne
// carte : le module se recâble tel quel.
// =====================================================
#define SD_CLK_PIN   13   // SCK  -> broche "CLK/SCK" du module
#define SD_MISO_PIN  12   // DO   -> broche "MISO/DO" (DAT0)
#define SD_MOSI_PIN  11   // CMD  -> broche "MOSI/DI" (CMD)
#define SD_CS_PIN    10   // CS   -> broche "CS" (DAT3)

// D1 et DAT2 (inutilisées en SPI) : laisser non connectées.
// L'Adafruit 4682 porte déjà ses pull-up sur toutes les lignes
// SDIO ; testé le 24/09/2026 : la SD fonctionne avec ou sans
// résistance sur D1 / DAT2.

// Détection de présence de carte : désactivée par défaut
// (la plupart des modules SD n'exposent pas la broche CD).
// Si le module en a une : câbler CD sur GPIO 6 et définir
// SD_DET_PIN à 6 (build_flags ou ici).
#ifndef SD_DET_PIN
#define SD_DET_PIN   -1
#endif

#ifndef SD_DET_ACTIVE_LEVEL
#define SD_DET_ACTIVE_LEVEL LOW
#endif

#else // --- ESP32-S3 DevKitC-1 N16R8 (montage historique) ---

// =====================================================
// Carte : ESP32-S3-DevKitC-1 (module ESP32-S3-N16R8)
// 16 Mo flash / 8 Mo PSRAM Octal
//
// Ce fichier reflète le schéma de câblage dessiné à la main :
// - SD card (J1) en bas à gauche
// - Capteur AHT20 + BMP280 en haut
// - Écran 1,3" (contrôleur SH1106) en bas à droite,
//   alimenté par le même bus I2C que le capteur
// - Encodeur rotatif EC11 / HW-040
// =====================================================


// =====================================================
// I2C — bus partagé AHT20 + BMP280 + écran OLED SH1106
// (routage en haut de la carte, vers les capteurs,
// puis redescend vers le connecteur de l'écran : pins
// "SDA" et "SCL" visibles sur le connecteur 9 broches
// de l'écran sur le schéma)
// =====================================================
#define I2C_SDA_PIN 8    // confirmé sur le schéma : SDA = IO8
#define I2C_SCL_PIN 9    // confirmé sur le schéma : SCL = IO9


// =====================================================
// Boutons
// =====================================================
// BOOT : entrée uniquement, c'est une pin de strapping au boot.
// On la respecte (pas de sortie dessus, pas de pull externe qui
// changerait l'état au démarrage).
#define BUTTON_BOOT_PIN     0

// CONFIRM : correspond à la pin "PSH" du connecteur de l'écran
// sur le schéma (routé en haut, vers le bloc écran).
#define BUTTON_CONFIRM_PIN  15

// BACK : bouton indépendant, routé en bas de la carte.
#define BUTTON_BACK_PIN     1


// =====================================================
// Neopixel embarqué (DevKitC-1)
// =====================================================
#define NEOPIXEL_PIN 48


// =====================================================
// Encodeur rotatif (EC11 / HW-040)
// Sur le schéma : correspond aux pins "TRA", "TRB", "BAK"
// du connecteur de l'écran (le module encodeur est câblé
// sur le même connecteur que l'écran).
// =====================================================
#define ENCODER_A_PIN      42   // TRA
#define ENCODER_B_PIN      2    // TRB
#define ENCODER_BTN_PIN    41   // PSH (bouton poussoir de l'encodeur)


// =====================================================
// Module SD (J1) — SPI secondaire "safe"
// (routage en bas à gauche sur le schéma)
// =====================================================
// Modules SD à 6 broches (sans DAT1/DAT2 routées) : à éviter,
// génèrent des échecs SD.begin() aléatoires selon les cartes SD.
// Cf. diagnostic complet du 02/07/2026.
// Aucun conflit avec PSRAM Octal / USB / pins de strapping.
#define SD_CLK_PIN   13   // SCK  -> broche "CLK" du connecteur micro-SD
#define SD_MISO_PIN  12   // DO   -> broche "DO/SO" (DAT0)
#define SD_MOSI_PIN  11   // CMD  -> broche "CMD/SI"
#define SD_CS_PIN    10   // CS   -> broche "DAT3/CS"

// D1 et DAT2 (inutilisées en mode SPI 1 bit) — câblage réel :
//   DAT2 -> R 10 kΩ -> GPIO 3 ;  D1 -> R 10 kΩ -> GPIO 46
// Ajouté lors du diagnostic SD du 02/07/2026, EN MÊME TEMPS que
// le passage des broches SPI de 21/47/38/39 à 13/12/11/10. Très
// probablement superflu : l'Adafruit 4682 porte déjà ses pull-up
// SDIO, GPIO 3 est flottant et GPIO 46 n'a qu'une faible
// pull-down interne ; la Super Mini fonctionne sans (24/09/2026).
// Le firmware ne pilote JAMAIS GPIO 3 / 46 (straps).

// Détection de présence de carte (ligne lente, peu sensible au
// routage, donc tolère un chemin plus long sur le PCB).
#ifndef SD_DET_PIN
#define SD_DET_PIN   40
#endif

#ifndef SD_DET_ACTIVE_LEVEL
#define SD_DET_ACTIVE_LEVEL LOW
#endif


// =====================================================
// Configuration conditionnelle PlatformIO
// =====================================================
#if defined(ESP32_S3_OLED)
    #define ENCODER_MODEL_EC11
#endif

#endif
