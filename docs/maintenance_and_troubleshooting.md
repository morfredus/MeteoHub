# Maintenance et dépannage

**Débutant ?** Voir le [Guide Débutant](beginner/index.md)

Version minimale valide : 1.6.3

- Si le Wi-Fi ne se connecte pas, vérifier `include/secrets.h`.
- Si les prévisions sont vides, vérifier la clé API et l’accès réseau.
- Si l’historique est vide, vérifier le montage LittleFS et l’état SD optionnel.
- Utiliser la page **Système** pour les mises à jour firmware (OTA), le réglage de la luminosité et les exports.

## Capteurs I2C instables (valeurs à zéro, erreurs `i2cRead`)

Symptômes dans les logs : `i2cRead returned Error -1`, `i2cWriteReadNonStop returned Error -1`, ou des mesures ponctuelles à 0.

Côté logiciel, MeteoHub encaisse déjà ces incidents : chaque lecture est validée, réessayée jusqu'à 3 fois, le bus I2C est réinitialisé automatiquement après plusieurs échecs, une lecture ratée n'est pas enregistrée, et les valeurs aberrantes résiduelles sont écartées des graphes et statistiques. Si les erreurs restent **fréquentes**, la cause est généralement matérielle :

1. **Résistances de pull-up SDA/SCL** : un seul jeu de pull-ups (≈ 4,7 kΩ vers 3,3 V) doit être présent sur le bus. Si plusieurs modules (AHT/BMP, OLED) intègrent chacun leurs pull-ups, ils se retrouvent en parallèle et surchargent le bus : retirer les pull-ups redondants.
2. **Câblage** : fils I2C les plus courts possible, masse commune franche, connexions fiables (SDA = IO8, SCL = IO9).
3. **Alimentation 3,3 V stable** : les pics de consommation Wi-Fi peuvent faire chuter la tension et perturber les capteurs.
4. **Vitesse du bus** : si le problème persiste, baisser la fréquence I2C (`I2C_FREQ_HZ` dans `src/modules/sensors.cpp`, ex. 50000 Hz) apporte de la marge.

## Historique et stockage

- L'historique interne est stocké au **format binaire** (`/history/AAAA/MM/AAAA-MM-JJ.bin` + fichiers `.stats`). Le CSV n'est utilisé que pour l'export (page Système).
- Les anciens fichiers `.csv` sont **convertis automatiquement** au binaire au premier démarrage après mise à jour (puis renommés `.csv.bak`) ; surveiller les lignes `History migration…` dans le moniteur série.
- Sauvegarder le dossier `/history` de la carte SD avant une mise à jour majeure du firmware, par prudence.

## Dépannage carte SD

### La carte SD n'est pas détectée ou échoue au montage

Si les logs indiquent `SD Mount FAILED` ou `cardType is CARD_NONE` :

1. **Reformater la carte en FAT32**
   - C'est la cause la plus fréquente. Les opérations d'écriture interrompues peuvent corrompre le système de fichiers.
   - Utilisez un outil comme **SD Memory Card Formatter** ou le gestionnaire de disque de votre OS.
   - Choisissez impérativement le système de fichiers **FAT32** (exFAT et NTFS ne sont pas supportés).
   - Taille d'allocation : 32 ko (recommandé).

2. **Vérifier l'alimentation**
   - La carte SD doit être alimentée en **3.3V**.
   - Un module avec régulateur 3.3V intégré est recommandé.

3. **Vérifier le câblage**
   - Les fils doivent être les plus courts possible (< 10 cm).
   - Vérifiez les connexions : CLK (GPIO 21), MISO (GPIO 47), MOSI (GPIO 38), CS (GPIO 39).

4. **Tester une autre carte**
   - Certaines cartes anciennes ou de très grande capacité (> 64 Go) peuvent être incompatibles.
   - Privilégiez les cartes de 4 Go à 32 Go pour une compatibilité maximale.
