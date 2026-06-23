# PIN MAPPING — ESP32‑S3 DevKitC‑1 N16R8
Version basée sur le câblage réel (board_config.h) — v1.2.0

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

## 🟩 I²C — Capteurs AHT20 + BMP280 + OLED SH1106
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
| Boot (natif) | **0**  | INPUT_PULLUP — strapping respecté |
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
| NeoPixel | **48** | GPIO imposé par l'ESP32‑S3 (DevKitC-1) — source de chaleur, à tenir hors de la zone froide du capteur |

---

## 🟦 Module SD — SPI secondaire SAFE
*(Routage bas → éloigné de l'alimentation)*

| Fonction | GPIO | Rôle SD | Notes |
|---------|------|----------|-------|
| CLK     | **21** | SCK  | Aucun conflit PSRAM / USB / strapping |
| MISO    | **47** | DAT0 | Safe |
| MOSI    | **38** | CMD  | Safe |
| CS      | **39** | DAT3 | Aucun rôle boot sur S3 |
| DET     | **40** | Détection carte | LOW = présente (ligne lente, routage long OK) |

> D1 et DAT2 : **ne pas câbler** (pull-up 10kΩ → 3V3).

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
| SD SPI (éloigné alim.) | 21, 47, 38, 39, 40 | ✔️ |

---

## 🟦 Notes importantes

- Aucune broche de strapping n'est utilisée en sortie.
- CS SD sur GPIO39 est **safe sur ESP32‑S3** (contrairement à l'ESP32 classique).
- Le capteur est éloigné thermiquement (zone froide) pour éviter les erreurs de mesure dues à la chaleur résiduelle des modules voisins.
- Le lecteur SD est éloigné physiquement de l'alimentation pour limiter le bruit électrique sur le bus SPI.
- Le mapping reflète la carte **réelle**, déjà soudée (`include/board_config.h`).
