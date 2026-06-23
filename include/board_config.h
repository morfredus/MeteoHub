#pragma once

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
// Aucun conflit avec PSRAM Octal / USB / pins de strapping.
#define SD_CLK_PIN   13   // SCK  -> broche "CLK" du connecteur micro-SD
#define SD_MISO_PIN  12   // DO   -> broche "DO/SO" (DAT0)
#define SD_MOSI_PIN  11   // CMD  -> broche "CMD/SI"
#define SD_CS_PIN    10   // CS   -> broche "DAT3/CS"

// D1 et DAT2 : non câblées (pins inutilisées en mode SPI 1 bit).
// Sur le schéma, R1 et R2 (10 kΩ) tirent ces lignes vers +3.3V
// pour éviter qu'elles flottent.

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