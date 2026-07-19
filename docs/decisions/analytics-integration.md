# Intégration d'un service d'analyse externe

Version minimale valide : 1.12.0

## Statut

**Actée et en service.** Guide destiné aux utilisateurs :
[Analyse avancée](../analyse_avancee.md).

## Contexte

MeteoHub enregistre une mesure par minute sur carte SD, soit environ 525 000 par
an. Il sait les afficher et les exporter, mais un ESP32 ne peut pas calculer une
normale sur plusieurs années ni corréler des grandeurs sur des mois : ni la
mémoire ni le temps de calcul ne le permettent.

Un service séparé (**morfAnalytics**, typiquement sur un Raspberry Pi) prend ces
traitements en charge. Ce document consigne les choix d'intégration et,
surtout, **pourquoi** ils ont été retenus.

---

## 1. Le service recopie l'historique en HTTP, il n'est pas alimenté par morfSync

**Décision.** Le serveur d'analyse interroge les API de MeteoHub en lecture seule
et constitue sa propre copie. MeteoHub n'est pas client morfSync.

**Pourquoi.** L'enveloppe de synchronisation morfSync (UUID, révision, origine,
horodatages) pèse plus lourd que la mesure elle-même, qui tient en 16 octets et
est écrite chaque minute. Faire de l'ESP32 un client morfSync aurait demandé un
générateur d'UUID, une file d'attente hors ligne et un client HTTP de
synchronisation, pour transporter une charge utile écrasée par ses propres
métadonnées.

**Conséquence.** morfSync reste destiné à diffuser les **résultats d'analyse** à
l'écosystème, pas les mesures brutes.

---

## 2. Le sens de circulation est unique : MeteoHub écrit, le service lit

**Décision.** Les API de collecte sont en lecture seule. Le collecteur n'émet que
des requêtes `GET`.

**Pourquoi.** MeteoHub est la **source de vérité**. Le cache du serveur est une
copie jetable : on peut l'effacer, il se reconstruit intégralement. L'inverse est
faux. Cette asymétrie doit être structurelle et non affaire de discipline — d'où
l'absence de toute route d'écriture, qui rend l'erreur impossible plutôt
qu'improbable.

---

## 3. La reprise de collecte se repère par (jour, position), jamais par horodatage

**Décision.** `GET /api/history/raw?day=AAAAMMJJ&index=N` identifie une mesure par
sa position dans le fichier du jour.

**Pourquoi.** Un horodatage n'est pas un repère fiable. Il **recule** lors d'un
recalage NTP, et il **se répète** au passage à l'heure d'hiver, où une heure
entière est vécue deux fois. Un collecteur qui reprendrait « après le dernier
horodatage connu » sauterait des mesures dans un cas et en dupliquerait dans
l'autre. Les fichiers `.bin` étant écrits en **ajout seul**, la position d'une
mesure ne change jamais : elle constitue un repère strictement croissant.

**Bénéfice supplémentaire.** Ce couple sert de clé primaire côté serveur, ce qui
rend l'import **idempotent** : redemander une plage déjà connue ne crée aucun
doublon, et le repère peut être perdu sans risque.

---

## 4. La page d'analyse est servie par le serveur, pas par MeteoHub

**Décision.** MeteoHub n'héberge aucune page d'analyse ; il ajoute un lien vers
celle du serveur.

**Pourquoi.** Trois raisons. Le stockage embarqué est précieux et une page riche
le consommerait pour une fonction optionnelle. Le serveur peut faire évoluer son
interface sans reflasher la station. Enfin, cette page devient réutilisable par
d'autres projets de l'écosystème, ce qui n'aurait pas été le cas embarquée dans
un firmware de station météo.

---

## 5. La découverte se fait par capacité, jamais par nom

**Décision.** MeteoHub cherche un service annonçant la capacité
`advanced_analysis` (champ `capabilities` du protocole morfBeacon). Le nom
annoncé n'est utilisé que comme **libellé** affiché.

**Pourquoi.** Le projet est distribué sous licence GPL : chacun peut modifier,
renommer et adapter les services. Un utilisateur qui rebaptise le sien « Ma Météo
à Moi » ou « Weather Lab » est dans son droit le plus strict. Une détection
fondée sur le nom (`app == "morfAnalytics"`) aurait cessé de fonctionner à cet
instant précis, sans message d'erreur compréhensible.

Deux notions distinctes se dégagent :

| Notion | Champ | Nature | Exemples |
|---|---|---|---|
| **Identité** | `app` | modifiable, affichée | `Mon Analyse Météo`, `Weather Lab` |
| **Capacité** | `capabilities` | stable, technique | `advanced_analysis` |

**Vérifié** : un service renommé « Mon Analyse Meteo » est détecté et affiché
sous ce nom ; un service nommé exactement « morfAnalytics » mais n'annonçant pas
la capacité n'est **pas** détecté. La reconnaissance ne retombe jamais sur le nom.

**Conséquence de mise à jour.** Un service antérieur à morfAnalytics 0.4.0
n'annonce pas de capacité et n'est plus découvert automatiquement. C'est le prix
assumé de l'abandon du nom comme critère ; l'adresse manuelle (§ 7) sert de
recours.

---

## 6. La navigation se fait dans le même onglet, avec un lien de retour

**Décision.** L'entrée de menu ouvre le service dans l'onglet courant. Le service
affiche en retour un lien « ← Retour à MeteoHub ».

**Pourquoi.** Ouvrir un nouvel onglet à chaque consultation en accumule
rapidement, sans bénéfice : l'utilisateur perçoit les deux interfaces comme un
même ensemble et navigue de l'une à l'autre comme entre deux pages. Le lien de
retour est **indissociable** de ce choix : sans lui, seul le bouton « précédent »
du navigateur permettrait de revenir, ce qui est une impasse d'ergonomie.

**Ce que ce choix ne remet pas en cause.** Les deux applications restent
techniquement indépendantes : adresses distinctes, cycles de version distincts,
et l'arrêt de l'une n'empêche pas l'autre de fonctionner. L'intégration ne tient
qu'à la découverte et à la navigation, jamais à un couplage de code.

---

## 7. Une adresse manuelle sert de recours, persistée en NVS

**Décision.** La page Système permet de saisir une adresse, conservée en NVS et
**prioritaire** sur la découverte automatique.

**Pourquoi.** La découverte suppose que les annonces UDP diffusées atteignent la
station. Ce n'est pas le cas partout : réseau découpé en sous-réseaux, liaison
VPN, point d'accès Wi-Fi isolant les clients, pare-feu filtrant le port 45454.
Sans recours, l'intégration serait inutilisable dans ces installations, sans
qu'aucun réglage ne permette d'y remédier.

La priorité donnée au manuel découle du même raisonnement : une adresse n'est
saisie que par quelqu'un ayant constaté que l'automatique ne fonctionnait pas.
Laisser la découverte l'emporter reviendrait à ignorer une décision explicite.

---

## 8. Reports assumés

Sont **volontairement** absents de cette version :

- **la sélection explicite entre plusieurs services détectés** — en présence de
  plusieurs, MeteoHub retient le dernier annoncé ; une adresse manuelle permet
  d'en imposer un ;
- **la personnalisation du nom affiché** — le nom annoncé par le service suffit,
  et il est déjà modifiable côté service.

**Pourquoi.** Ces deux fonctions répondent à des besoins spécifiques et
n'apportent rien au fonctionnement courant, alors qu'elles coûteraient de la
mémoire embarquée et de la complexité d'interface. Elles seront ajoutées si
l'usage les réclame, ce qui reste à démontrer.

---

## Suivi

- [Analyse avancée](../analyse_avancee.md) — guide utilisateur.
- [Guide utilisateur](../user_guide.md) — section « Analyse avancée ».
- [Architecture du projet](../project_architecture.md) — liste des API.
