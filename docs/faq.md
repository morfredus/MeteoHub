# FAQ

**Débutant ?** Voir le [Guide Débutant](beginner/index.md)

Version minimale valide : 1.6.3

## Quel afficheur est supporté ?
Uniquement OLED (SH1106/SSD1306).

## Quel environnement PlatformIO utiliser ?
`esp32-s3-oled`.

## La carte SD est-elle obligatoire ?
Non, elle est optionnelle pour l’archivage long terme.

## Mon SSD1306 a une bande jaune en haut. L’interface peut-elle chevaucher cette zone ?
Le titre de page reste sur la ligne haute (zone jaune), tandis que la zone de contenu est maintenue sous la zone haute réservée SSD1306 pour éviter le chevauchement.

## Où sont passés les onglets « Fichiers », « Logs » et « Mise à jour OTA » du menu web ?
Le menu web a été simplifié à quatre entrées : **Tableau de bord**, **Statistiques**, **Historique**, **Système**. La mise à jour OTA, la luminosité de la LED, les exports (CSV / configuration) et les accès aux Fichiers et Logs sont regroupés sur la page **Système**. L'ancienne URL `/ota.html` redirige vers `/system.html`.

## Sous quel format l'historique est-il stocké ?
Au format **binaire compact**, découpé par jour (`/history/AAAA/MM/AAAA-MM-JJ.bin`), avec un fichier de statistiques `.stats` par jour. Le **CSV** ne sert plus qu'à l'export (page Système). Les anciens fichiers CSV sont convertis automatiquement au premier démarrage après mise à jour.

## Je vois des erreurs `i2cRead` / des valeurs à zéro. Est-ce grave ?
MeteoHub valide chaque lecture, réessaie, réinitialise le bus si besoin, et n'enregistre pas les lectures ratées ; les valeurs aberrantes résiduelles sont écartées des graphes et statistiques. Si les erreurs sont fréquentes, la cause est souvent matérielle (pull-ups I2C redondants, câblage, alimentation) — voir [Maintenance et dépannage](maintenance_and_troubleshooting.md).

## Puis-je exporter mes données ?
Oui : depuis la page **Système**, export de l'historique en **CSV** (24 h / 7 j / 30 j / tout) et export de la **configuration** en JSON.
