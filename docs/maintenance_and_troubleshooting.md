# Maintenance et dépannage

**Débutant ?** Voir le [Guide Débutant](beginner/index.md)

Version minimale valide : 1.2.0

- Si le Wi-Fi ne se connecte pas, vérifier `include/secrets.h`.
- Si les prévisions sont vides, vérifier la clé API et l’accès réseau.
- Si l’historique est vide, vérifier le montage LittleFS et l’état SD optionnel.
- Utiliser la page OTA pour les mises à jour firmware.

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
