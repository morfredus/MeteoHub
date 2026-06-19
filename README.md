# MeteoHub S3

> **Version minimale valide : 1.1.3**

## Documentation complète
- [Index de la documentation](docs/index.md)

## Documentation Débutant
- [Guide Débutant](docs/beginner/index.md)

## Présentation
MeteoHub S3 est un projet PlatformIO pour ESP32-S3 centré sur un tableau de bord OLED (SH1106/SSD1306 via U8g2). Il affiche les données capteurs locales, les prévisions météo, les logs et l'état système.

## Matériel nécessaire
- Écran OLED (SH1106 ou SSD1306, I2C)
- Module encodeur HW-040 (encodeur + bouton central)
- Boutons Back et Confirm
- Capteurs AHT20 et BMP280
- Carte SD optionnelle pour l'archivage long terme (Recommandé: Format FAT32, 4-32 Go)

## Compilation
- Installer PlatformIO dans VS Code
- Sélectionner l'environnement : `esp32-s3-oled`
- Build : `platformio run`
- Upload : `platformio run --target upload`

## Fonctionnalités principales (Nouveautés v1.1.x)
- **Stabilité SD renforcée (v1.1.2)** : Écritures sécurisées avec `flush()` explicite et protection par Mutex contre la corruption de fichiers.
- **Moteur C++ Standard (v1.1.0)** : Refonte du cœur web pour utiliser `std::string`, garantissant une meilleure stabilité mémoire.
- **Gestion de fichiers avancée** : Upload, téléchargement et suppression de fichiers via l'interface web.
- **Mises à jour OTA robustes** : Système de mise à jour sans fil sécurisé.
- **Graphiques avancés** : Trois modes d'échelle (Fixe, Dynamique, Mixte) pour température, humidité et pression.

## Utilisation
Voir [Guide utilisateur](docs/user_guide.md) pour le détail des modes d'échelle et la gestion des fichiers.

### Note sur la carte SD
Pour éviter toute corruption, assurez-vous que :
1. La carte est formatée en **FAT32** (taille d'allocation 32 ko).
2. Le système est correctement éteint avant de retirer la carte (bien que le `flush()` protège les données à chaque écriture).

---

Pour toute modification de config.h, les valeurs sont injectées dans l'UI web lors du build.
