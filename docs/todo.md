# TODO

**Débutant ?** Voir le [Guide Débutant](beginner/index.md)

Version minimale valide : 1.9.0

## TODO code
- [x] Renommer `Sh1106Display` vers un nom neutre (`OledDisplay`) pour refléter correctement le backend U8g2 actuel qui prend en charge SH1106 et SSD1306.
- [x] Centraliser les constantes de géométrie OLED (`largeur`, `hauteur`, marges) pour supprimer les valeurs magiques dans `pages_oled.cpp`.
- [ ] Ajouter une petite API de diagnostic d’affichage (FPS courant, durée du dernier rendu, compteur d’erreurs I2C) exposée dans `/api/system`.
- [ ] Ajouter un flag de build optionnel pour compiler uniquement un backend contrôleur OLED (`SH1106` ou `SSD1306`) afin de réduire la taille binaire.
- [ ] Ajouter des tests unitaires côté hôte pour les fonctions utilitaires pures de `pages_oled.cpp` (formatage, génération des titres, traduction des alertes).
- [ ] Sondes non associées : ne plus journaliser chaque trame reçue (`ESP-NOW: packet node=…`) d'une sonde ignorée, garder seulement le rappel `trame ignoree` toutes les 10 min. À faire hors période de test (les logs complets servent tant qu'on teste avec la sonde d'établi).
- [x] Table des causes de redémarrage de la sonde (`resetReasonName`) : ajouter la cause USB des ESP32-S3, affichée aujourd'hui « UNKNOWN » quand un moniteur série redémarre la sonde.
- [ ] « Mode appairage » côté hub : n'accepter une NOUVELLE sonde associée que si le hub a été mis en mode appairage (appui long ou page web, ~2 min), pour qu'un appairage de test ne déloge pas la sonde de production.

## TODO expérience utilisateur
- [ ] Ajouter une page de réglages OLED rapide (contraste + aperçu adresse I2C) directement sur l’appareil.
- [ ] Ajouter un assistant de premier démarrage OLED qui valide le câblage (SDA/SCL/adresse) et affiche des étapes claires si aucun écran n’est détecté.
- [ ] Ajouter un toast/ligne de statut non bloquant lors des tentatives de reconnexion SD pour expliciter les échecs temporaires de sauvegarde.
- [ ] Ajouter un réglage de vitesse de transition des pages et de densité du graphe pour améliorer la lisibilité selon les usages.
- [ ] Ajouter une aide contextuelle sur appui long (légende/icônes/indices selon l’écran).

## Rendu UTF-8 OLED

Depuis la version 1.0.155, le backend OLED (`OledDisplay`) prend en charge le rendu UTF-8 pour tous les textes, y compris les caractères accentués et symboles spéciaux. Le module d'affichage porte désormais le nom neutre `OledDisplay` pour refléter sa compatibilité avec SH1106 et SSD1306.
