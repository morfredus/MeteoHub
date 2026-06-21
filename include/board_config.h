#pragma once

// =====================================================
// I2C — AHT20 + BMP280 + OLED SH1106
// (Routage haut → capteur)
// =====================================================
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9


// =====================================================
// Boutons
// =====================================================
// BOOT : entrée uniquement (strapping respecté)
#define BUTTON_BOOT_PIN     0

// CONFIRM : routé en haut vers l'écran
#define BUTTON_CONFIRM_PIN  15

// BACK : routé en bas
#define BUTTON_BACK_PIN     1


// =====================================================
// Neopixel embarqué (DevKitC-1)
// =====================================================
#define NEOPIXEL_PIN 48


// =====================================================
// Encodeur rotatif (EC11 / HW-040)
// =====================================================
#define ENCODER_A_PIN      42   // TRA
#define ENCODER_B_PIN      2    // TRB
#define ENCODER_BTN_PIN    41   // PSH


// =====================================================
// Module SD — SPI secondaire SAFE (routage bas)
// =====================================================
// SPI : aucun conflit PSRAM / USB / strapping
#define SD_CLK_PIN   21   // SCK
#define SD_MISO_PIN  47   // DAT0 / SO
#define SD_MOSI_PIN  38   // CMD / SI
#define SD_CS_PIN    39   // DAT3 / CS

// D1 et DAT2 : NE PAS câbler (pull-up 10kΩ → 3V3)

// Détection de carte (ligne lente, routage long OK)
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
