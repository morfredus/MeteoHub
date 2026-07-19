# MeteoHub S3

> **Version minimale valide : 1.9.0**

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

## Fonctionnalités principales (Nouveautés v1.9.x)
- **Stockage binaire compact de l'historique (v1.4.0+)** : fichiers binaires par jour (`/history/AAAA/MM/AAAA-MM-JJ.bin`) avec en-tête de fichier (compatibilité ascendante) et fichiers `.stats` journaliers. Accès direct aux mesures, bien plus compact que le CSV. Le CSV ne sert plus qu'à l'export ; les anciens CSV sont migrés automatiquement au premier démarrage.
- **Affichage de l'historique accéléré (v1.8.0)** : lecture séquentielle par blocs (sans seek par mesure) et cascade de fréquence SPI de la carte SD (20/10/4/1 MHz, validée par un test écriture + relecture) accélèrent nettement le chargement, avec repli fiable.
- **Page Historique : sélection et comparaison de périodes** : fenêtre 24 h / 48 h / 7 j / 30 j / aujourd'hui / plage personnalisée, comparaison de deux périodes, ligne de synthèse optionnelle par grandeur, curseur d'échelle « Zoom » inversé et bascule « Temps réel ».
- **Page Système (hub)** : mise à jour OTA, luminosité de la NeoLED (persistée en NVS), exports CSV/configuration, accès au gestionnaire de fichiers et aux logs. Menu principal réduit à quatre entrées : Tableau de bord, Statistiques, Historique, Système.
- **Qualité des données et robustesse capteur (v1.6.x)** : lectures I2C validées, réessayées, avec récupération automatique du bus après échecs ; les lectures ratées ne sont pas enregistrées. Les valeurs aberrantes sont écartées des graphes (cohérence temporelle) et des statistiques (médiane/MAD robuste), les données brutes restant conservées.
- **Monitoring des logs par UDP (v1.9.0)** : les logs applicatifs et ceux du cœur ESP (WiFi, I2C, watchdog…) sont diffusés en UDP pour un suivi à distance sans fil (ex. dans Tabby), sans câble série. Configurable dans `config.h`.
- **Écritures SD fiables** : `flush()` explicite et protection par Mutex contre la corruption de fichiers.

## Utilisation
Voir [Guide utilisateur](docs/user_guide.md) pour le détail de la page Historique, de la page Système, des échelles et des exports.

### Note sur la carte SD
Pour éviter toute corruption, assurez-vous que :
1. La carte est formatée en **FAT32** (taille d'allocation 32 ko).
2. Le système est correctement éteint avant de retirer la carte (bien que le `flush()` protège les données à chaque écriture).
3. Sauvegardez le dossier `/history` avant une mise à jour majeure du firmware (le format binaire est migré automatiquement, mais une sauvegarde reste prudente).

---

`config.h` est compilé dans le firmware ; les ressources web de `data/` sont embarquées au build (`scripts/embed_web_files.py`). La configuration effective peut être exportée depuis la page **Système** ou via `GET /api/config/export`. Les réglages d'exécution (ex. luminosité de la NeoLED) sont conservés en NVS.