# Captures d'écran de la documentation

Ce dossier contient les images référencées par la documentation. Les fichiers
listés ci-dessous sont **attendus mais absents** : les liens correspondants
apparaissent cassés tant qu'ils ne sont pas déposés ici.

## Conseils communs

- Format **PNG**, largeur **1200 à 1400 px** (lisible sans être énorme).
- Navigateur en **thème clair**, fenêtre assez large pour que le menu tienne sur
  une seule ligne.
- Masquer ou recadrer ce qui ne concerne pas la capture (favoris, autres onglets).
- Une adresse locale (`meteohub.local`, `192.168.x.x`) peut rester visible : elle
  n'a pas de valeur en dehors du réseau. En revanche, éviter de laisser paraître
  un nom de réseau Wi-Fi ou un identifiant personnel.

## Images attendues

### `menu-analyse.png`
**Où** : n'importe quelle page de MeteoHub, une fois un service d'analyse détecté.
**Cadrage** : l'en-tête et la barre de menu uniquement, avec les cinq entrées
visibles — Tableau de bord, Statistiques, Historique, **l'entrée d'analyse**,
Système. L'important est de montrer que l'entrée s'insère **avant** « Système ».

### `retour-meteohub.png`
**Où** : la page du service d'analyse (`http://<serveur>:8799/`).
**Cadrage** : le haut de la page, montrant le lien **« ← Retour à MeteoHub »**
au-dessus du titre. Inutile de descendre plus bas dans la page.

### `systeme-detecte.png`
**Où** : page **Système** de MeteoHub, service d'analyse allumé et détecté.
**Cadrage** : la carte « 🧠 Analyse avancée » entière, avec le message vert
indiquant le nom, la version et la machine du service détecté. Le bloc
« Adresse manuelle » reste replié.

### `systeme-adresse-manuelle.png`
**Où** : page **Système**, bloc « Adresse manuelle (optionnel) » **déplié**.
**Cadrage** : la carte entière, montrant le texte explicatif, le champ de saisie
et le bouton Enregistrer. Idéalement avec une adresse d'exemple saisie dans le
champ, pour que le lecteur voie la forme attendue.

## Après avoir déposé les images

Vérifier que les liens fonctionnent en ouvrant
[docs/analyse_avancee.md](../analyse_avancee.md) dans un afficheur Markdown
(l'aperçu de GitHub convient).
