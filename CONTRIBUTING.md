# Contribuer à MeteoHub S3

MeteoHub S3 est une station météo sur ESP32-S3 : acquisition, historisation
locale, affichage OLED et interface Web. Les contraintes qui suivent découlent
de la cible matérielle, pas de préférences de style.

## 1. Philosophie

**L'équipement est propriétaire de ses données.** MeteoHub écrit son historique,
les autres services le lisent. morfAnalytics en recopie une part dans un cache
de travail pour y mener des analyses lourdes, sans jamais écrire en retour.
Toute évolution qui ferait dépendre MeteoHub d'un service extérieur pour
conserver ou relire ses mesures va à l'encontre de ce principe.

**Une mesure ratée ne s'enregistre pas.** Les lectures I2C sont validées et
réessayées ; un échec ne produit pas une valeur par défaut. Une case vide se
relit comme une absence de mesure, un zéro se relit comme une mesure — la
distinction compte au moment d'exploiter l'historique.

**Le format binaire porte un en-tête.** L'historique est stocké en fichiers
binaires journaliers avec en-tête de fichier, précisément pour rester relisible
après une évolution du format. Modifier la structure sans faire évoluer
l'en-tête casserait silencieusement les données existantes et le collecteur de
morfAnalytics.

**Pas de cloud dans le chemin critique.** Les prévisions récupérées depuis
Internet sont un confort. L'appareil doit rester pleinement fonctionnel sur un
réseau local isolé.

## 2. Contraintes de la cible

L'ESP32-S3 n'est pas une machine de bureau, et plusieurs choix du projet n'ont
de sens qu'au regard de cela :

- **La mémoire est comptée.** Préférer les tampons de taille fixe, éviter les
  allocations répétées dans les boucles d'affichage et d'acquisition.
- **La carte SD se corrompt.** Les écritures sont protégées par mutex et suivies
  d'un `flush()` explicite. Ne pas contourner ce chemin.
- **L'horloge dérive.** Ne jamais fonder un ordre ou une déduplication sur
  l'horodatage seul : c'est la raison pour laquelle la collecte incrémentale
  utilise un curseur (jour, index).
- **Le bus I2C tombe.** La récupération automatique après échecs fait partie du
  fonctionnement normal, pas d'un cas d'erreur exceptionnel.

## 3. Version

La version fait autorité dans le fichier **`VERSION`** à la racine. Elle est
injectée à la compilation par `scripts/version.py`, qui définit
`PROJECT_VERSION`.

**Ne pas réécrire la version dans `platformio.ini`.** Elle y était autrefois en
dur, ce qui la plaçait hors de la convention des autres projets de l'écosystème
et rendait l'inventaire du parc impossible. Deux endroits pour une même valeur
finissent toujours par diverger.

Le fichier `VERSION` et l'en-tête du `CHANGELOG.md` doivent rester alignés.

## 4. Développement

```bash
platformio run                      # compiler
platformio run --target upload      # téléverser
platformio device monitor           # console série
```

Les assets Web sont générés dans `include/web_pages.h` par
`scripts/embed_web_files.py`, exécuté automatiquement avant chaque compilation.
Ne pas modifier le fichier généré : éditer les sources et recompiler.

Les logs applicatifs et ceux du cœur ESP peuvent être diffusés en UDP pour un
suivi sans fil (configuration dans `config.h`), ce qui évite de rester attaché
au câble série pendant les essais.

## 5. Style

- Un commentaire explique **pourquoi**, pas quoi.
- Un correctif décrit le symptôme qu'il supprime : c'est ce qui permet plus tard
  de savoir si la contrainte tient toujours.
- Les messages destinés à l'utilisateur sont en français, comme l'interface.

## 6. Avant de proposer une modification

- La compilation passe pour `esp32-s3-oled`.
- La version affichée par le firmware correspond au fichier `VERSION`.
- L'historique écrit par la version modifiée reste relisible par la précédente,
  ou l'en-tête de format a évolué en conséquence.
- Le `CHANGELOG.md` décrit le changement du point de vue de l'utilisateur.

## 7. Licence

En contribuant, vous acceptez que votre contribution soit distribuée sous
**GPL-3.0-only**, comme le reste de l'écosystème.
