# Analyse avancée — guide du débutant

Retour à l'[index de la documentation](index.md).

---

## En deux mots

MeteoHub mesure, enregistre et affiche. Il fait cela très bien, mais un ESP32
reste un petit ordinateur : calculer une moyenne sur trois ans de mesures ou
comparer la température du jour à celle des années précédentes dépasse ce qu'il
peut faire.

C'est le rôle d'un **service d'analyse** installé à côté, sur un ordinateur qui
reste allumé — typiquement un Raspberry Pi. Le service recopie l'historique de
MeteoHub et effectue les calculs lourds à sa place.

Trois points à retenir :

- **C'est totalement optionnel.** Sans service d'analyse, MeteoHub fonctionne
  exactement comme avant. Aucune fonction ne disparaît.
- **MeteoHub garde ses données.** Le service n'en fait qu'une copie et ne peut
  rien modifier sur la station : il ne sait que lire.
- **Il n'y a normalement rien à configurer.** MeteoHub trouve le service tout
  seul.

---

## Comment MeteoHub trouve le service

Le service d'analyse annonce sa présence sur le réseau local, un peu comme une
imprimante Wi-Fi se signale aux ordinateurs de la maison. MeteoHub écoute ces
annonces et, dès qu'il en reçoit une, propose l'accès aux analyses.

Aucune adresse à saisir, aucun réglage : brancher le service sur le même réseau
suffit.

### Ce que MeteoHub reconnaît exactement

MeteoHub ne cherche **pas un nom d'application**, mais une **capacité** — c'est
à dire ce que le service sait faire, ici « analyses avancées ».

La différence a l'air théorique, elle est très concrète. MeteoHub est un logiciel
libre : chacun peut renommer les programmes qu'il installe. Si vous rebaptisez
votre service « Ma Météo à Moi », MeteoHub continuera de le trouver et affichera
ce nouveau nom dans le menu. Si la reconnaissance s'était appuyée sur le nom,
tout aurait cessé de fonctionner au premier renommage.

> **À retenir** : le nom est un libellé, il s'affiche. La capacité est
> l'identifiant technique, elle sert à reconnaître le service.

---

## Naviguer entre MeteoHub et le service d'analyse

Quand un service est trouvé, une entrée supplémentaire apparaît dans le menu de
MeteoHub, juste avant « Système ». Elle porte le nom que le service annonce.

<!-- CAPTURE : menu-analyse.png -->
![Le menu de MeteoHub avec l'entrée d'analyse avancée](images/menu-analyse.png)

Cette entrée ouvre le service **dans le même onglet**, comme un lien normal. On
passe d'une application à l'autre sans accumuler les onglets. Pour revenir, le
service affiche en haut de sa page un lien **« ← Retour à MeteoHub »**.

<!-- CAPTURE : retour-meteohub.png -->
![Le lien de retour vers MeteoHub](images/retour-meteohub.png)

Les deux programmes restent malgré tout indépendants : chacun a sa propre
adresse, ses propres mises à jour, et l'un peut être arrêté sans gêner l'autre.

---

## Vérifier l'état, dans la page Système

La page **Système** indique en permanence si un service est trouvé.

<!-- CAPTURE : systeme-detecte.png -->
![La section Analyse avancée de la page Système](images/systeme-detecte.png)

Trois messages possibles :

| Message | Signification |
|---|---|
| ✅ Analyse avancée disponible … détecté automatiquement | Tout va bien, rien à faire |
| ✅ Analyse avancée disponible — adresse saisie manuellement | Une adresse a été renseignée à la main (voir plus bas) |
| ⚠️ Analyse avancée indisponible | Aucun service trouvé ; MeteoHub fonctionne normalement |

Le message d'indisponibilité n'est **pas une erreur**. Il apparaît simplement
quand aucun service n'est installé, ou quand celui-ci est éteint.

---

## Si la détection automatique ne marche pas

Dans certaines installations, les annonces réseau n'atteignent pas MeteoHub. Les
causes les plus courantes :

- le service et la station ne sont pas sur le même réseau (deux box, ou un réseau
  « invité » séparé) ;
- une liaison VPN sépare les deux ;
- le point d'accès Wi-Fi isole les appareils les uns des autres (option souvent
  appelée « isolation des clients ») ;
- un pare-feu bloque les annonces sur le port 45454.

Dans ce cas, l'adresse peut être saisie à la main. Ouvrir la page **Système**,
section **Analyse avancée**, puis déplier **« Adresse manuelle (optionnel) »**.

<!-- CAPTURE : systeme-adresse-manuelle.png -->
![Le champ d'adresse manuelle déplié](images/systeme-adresse-manuelle.png)

Saisir l'adresse complète du service, **en commençant par `http://`** :

```
http://raspberrypi.local:8799
```

ou, si le nom ne fonctionne pas chez vous, l'adresse numérique :

```
http://192.168.1.55:8799
```

Puis cliquer sur **Enregistrer**.

Quelques précisions utiles :

- L'adresse est **conservée après extinction** de la station.
- Une adresse manuelle **prend le pas** sur la détection automatique, même si un
  service est trouvé par ailleurs.
- Pour revenir au fonctionnement automatique, **vider le champ** et enregistrer.
- Une adresse qui ne commence pas par `http://` ou `https://` est refusée, avec
  un message explicite : c'est l'oubli le plus fréquent.

---

## Questions fréquentes

**Faut-il un service d'analyse pour utiliser MeteoHub ?**
Non. C'est un complément. Sans lui, mesures, historique, graphiques et exports
fonctionnent tous.

**Le service peut-il abîmer mes données ?**
Non. Il ne fait que lire. Techniquement, il n'émet que des requêtes de lecture ;
il ne dispose d'aucun moyen d'écrire sur la station.

**Que se passe-t-il si j'éteins le service ?**
L'entrée disparaît du menu au bout d'une minute environ, et la page Système
repasse en « indisponible ». Rien d'autre ne change.

**Puis-je renommer le service ?**
Oui, c'est prévu. MeteoHub le reconnaîtra toujours et affichera le nouveau nom.

**J'ai plusieurs services d'analyse sur mon réseau.**
MeteoHub retient le dernier qui s'est annoncé. Pour en imposer un précisément,
saisir son adresse à la main. Le choix explicite entre plusieurs services
viendra dans une version ultérieure.

---

## Pour aller plus loin

- [Guide utilisateur](user_guide.md) — l'ensemble des fonctions de MeteoHub.
- [Décisions : intégration d'un service d'analyse](decisions/analytics-integration.md)
  — les choix arrêtés et, surtout, **pourquoi** : c'est là qu'on trouve le
  raisonnement derrière la détection par capacité, la navigation dans le même
  onglet ou l'adresse manuelle.
- [Architecture du projet](project_architecture.md) — les API et l'organisation
  interne.
