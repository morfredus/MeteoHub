# Démarrage rapide

**Débutant ?** Voir le [Guide Débutant](beginner/index.md)

Version minimale valide : 1.9.0

1. Cloner le projet.
2. Créer `include/secrets.h` à partir de `include/secrets_example.h`.
3. Sélectionner l’environnement PlatformIO selon la carte :
   - `esp32-s3-oled` : ESP32-S3 DevKitC-1 N16R8 (encodeur + boutons, OLED SH1106 1,3") ;
   - `esp32-s3-supermini` : ESP32-S3 Super Mini (un bouton, OLED SSD1306 0,96"). Premier flash
     en USB obligatoire (table de partitions 4 Mo).
4. Compiler avec `platformio run`.
5. Flasher avec `platformio run --target upload`.
6. Ouvrir le moniteur série (`platformio device monitor`) pour valider le démarrage.
