# PIN MAPPING

Deux cartes, sélectionnées dans `include/board_config.h` par le define de l'env PlatformIO :

- [ESP32-S3 DevKitC-1 N16R8](#esp32-s3-devkitc-1-n16r8-env-esp32-s3-oled) (`esp32-s3-oled`)
- [ESP32-S3 Super Mini](#esp32-s3-super-mini-env-esp32-s3-supermini) (`esp32-s3-supermini`)

# ESP32-S3 DevKitC-1 N16R8 (env esp32-s3-oled)
Version basée sur le câblage réel (`include/board_config.h`), révisé après le
diagnostic SD du 02/07/2026.

Ce document décrit le mapping des broches réellement utilisé sur la carte MeteoHub‑S3.
Il reflète le câblage **physique**, **testé**, **fiable au boot**, et optimisé pour éviter :

- boot loops
- conflits de strapping
- interférences SPI/I²C
- erreurs de température dues à la chaleur des modules voisins

---

## 🟥 Contraintes de montage physique (v1.2.0)

Le mapping et le placement des modules respectent désormais deux contraintes physiques :

1. **Éloignement lecteur SD / alimentation**
   Le lecteur SD (CLK/MISO/MOSI/CS, routage « bas ») doit être monté **physiquement éloigné** du
   régulateur d'alimentation et de tout convertisseur (5V→3V3, USB-PD, etc.). Les variations de
   courant de l'alimentation génèrent du bruit électrique qui peut perturber le bus SPI du
   lecteur SD (corruption d'écriture, échecs de montage intermittents).

2. **Zone froide autour du capteur (AHT20 / BMP280)**
   Le capteur I²C (routage « haut ») doit disposer d'une **zone froide** dégagée autour de lui :
   aucun composant générant de la chaleur (régulateur, NeoPixel, microcontrôleur lui-même) ne
   doit être placé à proximité immédiate. Cette zone évite que le capteur ne mesure la chaleur
   résiduelle dissipée par ses voisins au lieu de la température ambiante réelle, ce qui fausse
   les mesures de température (et donc d'humidité compensée).

> En pratique : capteur + OLED côté **haut** du boîtier (zone froide, loin de l'alimentation),
> lecteur SD côté **bas** du boîtier (loin de l'alimentation, isolé des broches sensibles).

---

## 🟩 I²C - Capteurs AHT20 + BMP280 + OLED SH1106
*(Routage haut → zone froide, capteur isolé thermiquement)*

| Fonction | GPIO | Notes |
|---------|------|-------|
| SDA     | **8** | Pin neutre, aucun rôle boot |
| SCL     | **9** | Stable, pas de strapping |

Bus I²C partagé entre :
- AHT20
- BMP280
- OLED SH1106

---

## 🟦 Boutons

| Fonction | GPIO | Notes |
|---------|------|-------|
| Boot (natif) | **0**  | INPUT_PULLUP - strapping respecté |
| Confirm      | **15** | Routé en haut, vers l'écran |
| Back         | **1**  | Routé en bas |

Aucun bouton sur GPIO sensibles → boot 100% fiable.

---

## 🟧 Encodeur (EC11 / HW‑040)

| Fonction | GPIO | Notes |
|---------|------|-------|
| A (TRA) | **42** | Pin neutre |
| B (TRB) | **2**  | Pin neutre |
| SW (PSH)| **41** | Pin neutre |

Aucun conflit avec les broches de strapping.

---

## 🟩 NeoPixel

| Fonction | GPIO | Notes |
|---------|------|-------|
| NeoPixel | **48** | GPIO imposé par l'ESP32‑S3 (DevKitC-1) - source de chaleur, à tenir hors de la zone froide du capteur |

---

## 🟦 Module SD - SPI secondaire SAFE
*(Routage bas → éloigné de l'alimentation)*

| Fonction | GPIO | Rôle SD | Notes |
|---------|------|----------|-------|
| CLK     | **13** | SCK  | Aucun conflit PSRAM Octal / USB / strapping |
| MISO    | **12** | DAT0 | Safe |
| MOSI    | **11** | CMD  | Safe |
| CS      | **10** | DAT3 | Aucun rôle boot sur S3 |
| DET     | **40** | Détection carte | LOW = présente (ligne lente, routage long OK) |

| DAT2    | **3**  | via R 10 kΩ | Strap JTAG, jamais piloté par le firmware |
| D1      | **46** | via R 10 kΩ | Strap, jamais piloté par le firmware |

> D1 et DAT2 du module SD : reliés à GPIO 3 / GPIO 46 via 10 kΩ lors du diagnostic du
> 02/07/2026, en même temps que le changement de broches SPI (21/47/38/39 → 13/12/11/10).
> Très probablement superflu : le module (Adafruit 4682) porte déjà ses pull-up et la
> Super Mini fonctionne sans (testé le 24/09/2026). Pour trancher : retirer les deux
> résistances et vérifier le montage SD. Ne jamais réutiliser GPIO 3 / 46 (straps).

SPI totalement isolé des broches sensibles → aucun blocage au boot. Le lecteur doit être
monté à distance du régulateur d'alimentation (voir contraintes de montage ci-dessus).

---

## 🟩 Résumé global

| Sous‑système | GPIO utilisés | Boot Safe |
|--------------|---------------|-----------|
| I²C (zone froide)  | 8, 9          | ✔️ |
| Boutons      | 0, 15, 1      | ✔️ |
| Encodeur     | 42, 2, 41     | ✔️ |
| NeoPixel     | 48            | ✔️ |
| SD SPI (éloigné alim.) | 13, 12, 11, 10, 40 ; 3, 46 via 10 kΩ (DAT2, D1) | ✔️ |

---

## 🟦 Notes importantes

- Aucune broche de strapping n'est utilisée en sortie. GPIO 3 et 46 (straps) sont reliés à DAT2 / D1 du module SD via 10 kΩ, mais restent en haute impédance : le firmware ne les configure jamais.
- CS SD sur GPIO39 est **safe sur ESP32‑S3** (contrairement à l'ESP32 classique).
- Le capteur est éloigné thermiquement (zone froide) pour éviter les erreurs de mesure dues à la chaleur résiduelle des modules voisins.
- Le lecteur SD est éloigné physiquement de l'alimentation pour limiter le bruit électrique sur le bus SPI.
- Le mapping reflète la carte **réelle**, déjà soudée (`include/board_config.h`).

---

# ESP32-S3 Super Mini (env esp32-s3-supermini)

Depuis la v1.43.0. Carte identique à la sonde MeteoHubSensor : 4 Mo flash, 2 Mo PSRAM
quad, USB natif, LED RGB WS2812 et bouton BOOT soudés. OLED 0,96" JMD0.96D-1 (SSD1306),
un seul bouton de navigation. Câblage et alimentation des modules :
[hardware_wiring.md](hardware_wiring.md#esp32-s3-super-mini-env-esp32-s3-supermini).

## I²C - AHT20 + BMP280 + OLED SSD1306 (JMD0.96D-1)

| Fonction | GPIO | Notes |
|---------|------|-------|
| SDA     | **8** | Mêmes GPIO que la sonde |
| SCL     | **9** | Pas de strapping |

Adresses : AHT20 = 0x38, BMP280 = 0x76/0x77, OLED = 0x3C.

---

## Module SD - SPI (FSPI)

| Fonction | GPIO | Rôle SD | Notes |
|---------|------|----------|-------|
| CS      | **10** | DAT3 | |
| MOSI    | **11** | CMD  | |
| MISO    | **12** | DAT0 | |
| CLK     | **13** | SCK  | |
| DET     | **6** (option) | Détection carte | Désactivée par défaut (`SD_DET_PIN = -1`) |

Brochage SPI identique à l'ancienne carte DevKitC : le module se recâble tel quel.

---

## Bouton et signalisation

| Fonction | GPIO | Notes |
|---------|------|-------|
| Bouton navigation | **5**  | Poussoir vers GND, pull-up interne. Court = suivant, long = menu / valider |
| BOOT (soudé)      | **0**  | Strapping. Maintenu au boot = formatage LittleFS |
| LED RGB WS2812    | **48** | Soudée sur la carte |

---

## Résumé

| Sous-système | GPIO utilisés | Boot safe |
|--------------|---------------|-----------|
| I²C          | 8, 9          | ✔️ |
| SD SPI       | 10, 11, 12, 13 (+6 option) | ✔️ |
| Bouton       | 5 (+0 BOOT)   | ✔️ |
| LED RGB      | 48            | ✔️ |

Réservées / à laisser libres : **19, 20** (USB), **3, 45, 46** (straps).
D1 et DAT2 du module SD : non connectées (pull-up déjà sur le module, testé).
Libres pour évolutions : 1, 2, 4, 7, 43 (TX), 44 (RX).
