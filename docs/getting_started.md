# Démarrage rapide

**Débutant ?** Voir le [Guide Débutant](beginner/index.md)

Version minimale valide : 1.2.0

1. Cloner le projet.
2. Créer `include/secrets.h` à partir de `include/secrets_example.h`.
3. Sélectionner l’environnement PlatformIO `esp32-s3-oled`.
4. Compiler avec `platformio run`.
5. Flasher avec `platformio run --target upload`.
6. Ouvrir le moniteur série (`platformio device monitor`) pour valider le démarrage.
