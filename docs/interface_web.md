# Interface web

MeteoHub sert une interface web directement depuis l'ESP32-S3. Il suffit d'ouvrir
son adresse dans un navigateur (par exemple `http://meteohub.local`) sur le même
réseau. Aucune application à installer.

> Les captures ci-dessous utilisent des **données d'exemple anonymisées** : les
> valeurs, l'historique et les adresses réseau sont fictifs et servent uniquement
> à illustrer l'interface.

## Tableau de bord

Vue d'accueil, centrée sur la météo extérieure : une valeur « effective » en grand
(température/humidité extérieures, avec repli intérieur signalé si la sonde est
absente), puis deux cartouches distincts **Extérieur (OUT)** et **Intérieur
(confort)**, l'état de la pile de la sonde, et le cartouche d'alerte météo. Au
redémarrage, tant qu'aucune trame de la sonde n'est arrivée, la carte extérieure
indique « en attente du premier relevé » plutôt qu'une fausse valeur. (Le graphe
d'historique n'est plus sur l'accueil : il vit sur la page Historique.)

![Tableau de bord MeteoHub (données d'exemple)](pictures/interface-tableau-de-bord.png)

## Statistiques

Résumé min / moyenne / max sur les dernières 24 h, pour l'extérieur ET l'intérieur,
et tendance météo (extérieur) à 1 h, 12 h, 24 h et 48 h.

![Page Statistiques (données d'exemple)](pictures/interface-statistiques.png)

## Historique

Le graphe d'historique, avec deux réglages seulement : la **Source** (Extérieur,
Intérieur, ou **Intérieur + Extérieur** tracés ensemble pour comparer) et la
**Période** (24 h, 48 h, 7 j, 30 j, aujourd'hui, ou une plage personnalisée).
L'échelle de temps s'adapte à la période choisie. L'analyse approfondie, elle,
est le rôle de morfAnalytics.

## Système

Réglages et maintenance : détection du service d'analyse avancée, luminosité de
la LED, export CSV / configuration, mise à jour du firmware (OTA) et accès aux
outils.

![Page Système (données d'exemple)](pictures/interface-systeme.png)
