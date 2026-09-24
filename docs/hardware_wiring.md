# Câblage matériel

Version minimale valide : 1.9.0

Ce projet cible uniquement une configuration OLED, sur deux cartes au choix :

- **ESP32-S3 DevKitC-1 N16R8** (env `esp32-s3-oled`) : montage historique, décrit ci-dessous ;
- **ESP32-S3 Super Mini** (env `esp32-s3-supermini`) : montage simplifié à 4 modules, voir
  [la section dédiée](#esp32-s3-super-mini-env-esp32-s3-supermini).

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
| I2C SDA                 | 8    | OLED, AHT20, BMP280 - routage haut (zone froide) |
| I2C SCL                 | 9    | OLED, AHT20, BMP280 - routage haut (zone froide) |
| Bouton BOOT             | 0    | Strapping respecté                    |
| Bouton CONFIRM          | 15   | Routé en haut, vers l'écran            |
| Bouton BACK             | 1    | Routé en bas                          |
| Neopixel                | 48   | LED intégrée                           |
| Encodeur A (TRA)        | 42   | EC11/HW-040                            |
| Encodeur B (TRB)        | 2    | EC11/HW-040                            |
| Bouton Encodeur (PSH)   | 41   | EC11/HW-040                            |
| SD CLK (SCK)            | 13   | SPI SD - routage bas (éloigné alim.)   |
| SD MISO (DAT0/SO)       | 12   | SPI SD - routage bas (éloigné alim.)   |
| SD MOSI (CMD/SI)        | 11   | SPI SD - routage bas (éloigné alim.)   |
| SD CS (DAT3/CS)         | 10   | SPI SD - routage bas (éloigné alim.)   |
| SD DÉTECTION            | 40   | LOW=présente, HIGH=absente (polarité configurable) |

| SD DAT2 (via 10 kΩ)     | 3    | Strap, jamais piloté par le firmware   |
| SD D1 (via 10 kΩ)       | 46   | Strap, jamais piloté par le firmware   |

> D1 et DAT2 du module SD : reliés à GPIO 3 / GPIO 46 via 10 kΩ lors du diagnostic du
> 02/07/2026, en même temps que le changement de broches SPI (21/47/38/39 → 13/12/11/10).
> Très probablement superflu : le module (Adafruit 4682) porte déjà ses pull-up et la
> Super Mini fonctionne sans (testé le 24/09/2026). Pour trancher : retirer les deux
> résistances et vérifier le montage SD. Ne jamais réutiliser GPIO 3 / 46 (straps).
> Voir board_config.h pour le mapping de référence et SD_DET_ACTIVE_LEVEL pour la polarité.

---

## ESP32-S3 Super Mini (env esp32-s3-supermini)

Version minimale valide : 1.43.0

Carte : **ESP32-S3 Super Mini** (v0.2, 4 Mo flash / 2 Mo PSRAM quad) — la même que
la sonde MeteoHubSensor. Montage simplifié à 4 modules :

- OLED 0,96" **JMD0.96D-1** (SSD1306 128x64, I2C, adresse 0x3C)
- Capteur **AHT20 + BMP280** (I2C, 0x38 + 0x76/0x77)
- Module **carte micro-SD** (SPI)
- **Un bouton poussoir** de navigation
- LED RGB WS2812 et bouton BOOT : déjà soudés sur la Super Mini

### Tableau de correspondance des broches

Tout l'I2C et le SPI SD tiennent sur les GPIO 8 à 13, regroupés sur la même rangée
de la Super Mini (vérifier la sérigraphie de ta carte : l'ordre des broches varie
selon les fabricants).

| Fonction              | GPIO | Côté module                             |
|-----------------------|------|-----------------------------------------|
| I2C SDA               | 8    | SDA de l'OLED **et** du capteur         |
| I2C SCL               | 9    | SCL de l'OLED **et** du capteur         |
| SD CS                 | 10   | CS                                      |
| SD MOSI               | 11   | MOSI / DI                               |
| SD MISO               | 12   | MISO / DO                               |
| SD CLK                | 13   | SCK / CLK                               |
| Bouton navigation     | 5    | une patte du poussoir, l'autre à **GND** (pull-up interne) |
| Bouton BOOT           | 0    | soudé sur la carte (maintenance au boot) |
| LED RGB WS2812        | 48   | soudée sur la carte                     |
| SD détection (option) | 6    | seulement si le module expose CD : définir `SD_DET_PIN 6` |

Broches à laisser libres : **19/20** (USB natif), **3, 45, 46** (straps).

> **D1 et DAT2 du module SD : aucune résistance nécessaire (testé le 24/09/2026).**
> L'Adafruit 4682 porte déjà des pull-up sur toutes ses lignes SDIO : sur la Super Mini,
> la carte SD fonctionne avec ou sans liaison 10 kΩ sur D1 / DAT2. Laisser ces deux
> broches du module non connectées.

#### Alimentation des modules

- **OLED JMD0.96D-1** : ordre habituel **GND - VCC - SCL - SDA**, mais il varie d'un
  lot à l'autre : suis la sérigraphie du module, pas sa position. VCC sur **3V3**.
- **AHT20 + BMP280** : VCC sur **3V3**. Le module porte déjà ses pull-up I2C ; l'OLED
  aussi : c'est suffisant, n'en ajoute pas.
- **Module SD** : l'Adafruit 4682 (celui de la DevKitC) s'alimente en **3V3** (broche
  « 3V ») ; il expose aussi DET, utilisable sur GPIO 6. Un module avec régulateur
  (AMS1117) et buffer de niveaux (module bleu « Micro SD Card Adapter ») s'alimente,
  lui, en **5V**. Les signaux restent en 3,3 V dans tous les cas.

#### Placement

La Super Mini chauffe (WiFi permanent + LED RGB) : éloigne le capteur AHT20/BMP280 de
la carte au bout de ses fils (quelques cm suffisent, idéalement hors du boîtier ou
derrière une cloison ventilée), sinon la température intérieure mesurée sera
surévaluée.
