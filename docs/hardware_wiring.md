# Câblage matériel

Version minimale valide : 1.2.0

Ce projet cible uniquement une configuration OLED.

Références principales :
- `include/board_config.h` pour le mapping des broches
- `include/config.h` pour les réglages runtime d’affichage


Périphériques connectés :
- OLED (I2C)
- Encodeur rotatif + boutons
- AHT20 + BMP280 (I2C)
- NeoPixel
- Carte SD optionnelle (SPI)

## Contraintes de montage physique

Depuis la v1.2.0, le placement physique des modules sur le boîtier doit respecter deux règles :

- **Lecteur SD éloigné de l'alimentation** : le module SD (routage « bas ») doit être monté à
  distance du régulateur/convertisseur d'alimentation, pour éviter que le bruit électrique de
  ce dernier ne perturbe le bus SPI (corruption d'écriture, échecs de montage).
- **Zone froide autour du capteur** : le capteur AHT20/BMP280 (routage « haut ») doit disposer
  d'un dégagement (« zone froide ») sans composant chauffant à proximité (régulateur, NeoPixel,
  ESP32-S3), afin de ne pas mesurer la chaleur résiduelle de ses voisins à la place de la
  température ambiante réelle.

## Tableau de correspondance des broches (depuis board_config.h)

| Fonction                | GPIO | Remarques                              |
|-------------------------|------|----------------------------------------|
| I2C SDA                 | 8    | OLED, AHT20, BMP280 — routage haut (zone froide) |
| I2C SCL                 | 9    | OLED, AHT20, BMP280 — routage haut (zone froide) |
| Bouton BOOT             | 0    | Strapping respecté                    |
| Bouton CONFIRM          | 15   | Routé en haut, vers l'écran            |
| Bouton BACK             | 1    | Routé en bas                          |
| Neopixel                | 48   | LED intégrée                           |
| Encodeur A (TRA)        | 42   | EC11/HW-040                            |
| Encodeur B (TRB)        | 2    | EC11/HW-040                            |
| Bouton Encodeur (PSH)   | 41   | EC11/HW-040                            |
| SD CLK (SCK)            | 21   | SPI SD — routage bas (éloigné alim.)   |
| SD MISO (DAT0/SO)       | 47   | SPI SD — routage bas (éloigné alim.)   |
| SD MOSI (CMD/SI)        | 38   | SPI SD — routage bas (éloigné alim.)   |
| SD CS (DAT3/CS)         | 39   | SPI SD — routage bas (éloigné alim.)   |
| SD DÉTECTION            | 40   | LOW=présente, HIGH=absente (polarité configurable) |

> D1 et DAT2 du module SD : ne pas câbler (pull-up 10kΩ → 3V3).
> Voir board_config.h pour le mapping de référence et SD_DET_ACTIVE_LEVEL pour la polarité.
