# FAQ

**Débutant ?** Voir le [Guide Débutant](beginner/index.md)

Version minimale valide : 1.2.0

## Quel afficheur est supporté ?
Uniquement OLED (SH1106/SSD1306).

## Quel environnement PlatformIO utiliser ?
`esp32-s3-oled`.

## La carte SD est-elle obligatoire ?
Non, elle est optionnelle pour l’archivage long terme.

## Mon SSD1306 a une bande jaune en haut. L’interface peut-elle chevaucher cette zone ?
Le titre de page reste sur la ligne haute (zone jaune), tandis que la zone de contenu est maintenue sous la zone haute réservée SSD1306 pour éviter le chevauchement.
