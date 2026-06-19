# Câblage matériel

Version minimale valide : 1.1.3

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

## Tableau de correspondance des broches (depuis board_config.h)

| Fonction                | GPIO | Remarques                              |
|-------------------------|------|----------------------------------------|
| I2C SDA                 | 15   | OLED, AHT20, BMP280                    |
| I2C SCL                 | 16   | OLED, AHT20, BMP280                    |
| Bouton BOOT             | 0    |                                        |
| Bouton BACK             | 7    |                                        |
| Bouton CONFIRM          | 8    |                                        |
| Neopixel                | 48   | LED intégrée                           |
| Encodeur A              | 4    | EC11/HW-040                            |
| Encodeur B              | 5    | EC11/HW-040                            |
| Bouton Encodeur         | 6    | EC11/HW-040                            |
| SD CLK (SCK)            | 9    | SPI SD                                 |
| SD MISO (DAT0/SO)       | 10   | SPI SD                                 |
| SD MOSI (CMD/SI)        | 11   | SPI SD                                 |
| SD CS (DAT3/CS)         | 12   | SPI SD                                 |
| SD DAT2                 | 13   | SPI SD (pas toujours utilisé)          |
| SD DÉTECTION            | 14   | LOW=présente, HIGH=absente (polarité configurable) |

> Voir board_config.h pour le mapping de référence et SD_DET_ACTIVE_LEVEL pour la polarité.
