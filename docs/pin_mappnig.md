# PIN MAPPING — ESP32‑S3 DevKitC‑1 N16R8  
Version basée sur le câblage réel (board_config.h)

Ce document décrit le mapping des broches réellement utilisé sur la carte MeteoHub‑S3.  
Il reflète le câblage **physique**, **testé**, **fiable au boot**, et optimisé pour éviter :

- boot loops  
- conflits de strapping  
- interférences SPI/I²C  
- erreurs de température dues à la chaleur des modules  

---

## 🟩 I²C — Capteurs AHT20 + BMP280 + OLED SH1106

| Fonction | GPIO | Notes |
|---------|------|-------|
| SDA     | **15** | Pin neutre, aucun rôle boot |
| SCL     | **16** | Stable, pas de strapping |

Bus I²C partagé entre :  
- AHT20  
- BMP280  
- OLED SH1106  

---

## 🟦 Boutons

| Fonction | GPIO | Notes |
|---------|------|-------|
| Boot (natif) | **0** | INPUT_PULLUP |
| Back         | **7** | INPUT_PULLUP |
| Confirm      | **8** | INPUT_PULLUP |

Aucun bouton sur GPIO sensibles → boot 100% fiable.

---

## 🟧 Encodeur (EC11 / HW‑040)

| Fonction | GPIO | Notes |
|---------|------|-------|
| A       | **4** | Pin neutre |
| B       | **5** | Pin neutre |
| SW      | **6** | Pin neutre |

Aucun conflit avec les broches de strapping.

---

## 🟩 NeoPixel

| Fonction | GPIO | Notes |
|---------|------|-------|
| NeoPixel | **48** | GPIO imposé par l’ESP32‑S3 |

---

## 🟦 Module SD — SPI secondaire SAFE

| Fonction | GPIO | Rôle SD | Notes |
|---------|------|----------|-------|
| CLK     | **9**  | SCK  | Safe |
| MISO    | **10** | DAT0 | Safe |
| MOSI    | **11** | CMD  | Safe |
| CS      | **12** | DAT3 | Aucun rôle boot sur S3 |
| DAT2    | **13** | DAT2 | Optionnel |
| DET     | **14** | Détection carte | LOW = présente |

SPI totalement isolé des broches sensibles → aucun blocage au boot.

---

## 🟩 Résumé global

| Sous‑système | GPIO utilisés | Boot Safe |
|--------------|---------------|-----------|
| I²C          | 15, 16        | ✔️ |
| Boutons      | 0, 7, 8       | ✔️ |
| Encodeur     | 4, 5, 6       | ✔️ |
| NeoPixel     | 48            | ✔️ |
| SD SPI       | 9, 10, 11, 12, 13, 14 | ✔️ |

---

## 🟦 Notes importantes

- Aucune broche de strapping n’est utilisée en sortie.  
- CS SD sur GPIO12 est **safe sur ESP32‑S3** (contrairement à l’ESP32 classique).  
- Le capteur est éloigné thermiquement pour éviter les erreurs de mesure.  
- Le mapping reflète la carte **réelle**, déjà soudée.  

