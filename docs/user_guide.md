# Guide utilisateur

**Débutant ?** Voir le [Guide Débutant](beginner/index.md)

Version minimale valide : 1.9.0

**DevKitC (encodeur + boutons)** :

- Tourner l’encodeur pour naviguer entre les pages.
- Le clic encodeur ouvre le menu.
- Back sort du menu/des confirmations.
- Confirm valide les actions contextuelles.

**Super Mini (un seul bouton)** :

| Contexte | Appui court (< 0,8 s) | Appui long (≥ 0,8 s) |
|---|---|---|
| Page normale | page suivante | ouvre le menu |
| Menu | élément suivant | exécute l'élément (« Retour » ferme le menu) |
| Confirmation (Format SD, effacements) | annule | confirme |

L'appui long réagit dès que le seuil est atteint, sans attendre le relâchement ; les
actions destructives exigent toujours un appui long volontaire. Sans molette, la page
**Logs** défile seule (toutes les 3 s) et la page **Prévisions** alterne ses trois vues
(toutes les 5 s).


Pages principales (dans l'ordre de défilement) :
- **Météo** : température et humidité **intérieures** (IN, capteurs du boîtier) et **extérieures** (OUT, sonde radio), plus la **pression atmosphérique de la sonde OUT** (jamais celle du BMP280 intérieur). Tant qu'aucune trame OUT n'est reçue, la 4e ligne affiche `NOW chX rxY okZ` (canal Wi-Fi, trames vues, trames validées).
- **Prévisions** : min/max et description du jour et du lendemain, et l'éventuelle alerte météo.
- **Graphes** : courbes intérieur et extérieur (température, humidité, pression), tracées sur les dernières **24 h** lues sur la carte SD (et non la seule mémoire vive), reliées en continu et coupées uniquement en cas de vrai silence capteur.
- **Réseau** : SSID, IP, **canal Wi-Fi**, RSSI et **MAC STA du hub**. La sonde scanne `MH-NOW` pour trouver le canal, puis émet en **unicast** vers cette MAC (accusé de réception matériel).
- **Appairage de la sonde** (MeteoHubSensor ≥ 0.23.0) : la sonde n'a plus la MAC du hub en dur. Pour la rattacher à ce hub, ne laisser allumé que lui, puis maintenir **BOOT ~3 s** sur la sonde (LED bleue fixe). MeteoHub répond à la demande avec son identité, sa MAC et son canal ; la sonde confirme, et le hub reprend après le dernier numéro de mesure accusé par l'ancien hub (il ne réclame que les mesures encore en attente). Les logs affichent `demande d'appairage de …` puis `sonde appairee … reprise apres seq=…`. MeteoHub répond à toute demande valide : c'est le fait de ne laisser allumé que le hub voulu qui désigne le récepteur. Si deux hubs répondent, la sonde refuse et garde son association.
- **Système** : mémoire libre (**Heap**, **PSRAM**), taille de la **flash** et **version** du firmware.
- **Capteur** : état de la sonde extérieure déportée, sur une page dédiée (l'OLED n'a la place que de quatre lignes) :
  - **Batterie** : tension (V) et niveau (%) de l'accu de la sonde, ou « Bat: absente » tant qu'aucune trame ne porte la mesure (sonde sur secteur, ou lecture batterie non câblée/non activée côté sonde).
  - **Canal** : canal Wi-Fi/ESP-NOW sur lequel la sonde et le hub dialoguent.
  - **Trames** : nombre de trames **reçues** (`rx`) et **validées** CRC (`ok`) depuis le démarrage ; un `ok` nettement inférieur à `rx` trahit une liaison bruitée.
  - **Fraîcheur** : temps écoulé depuis la dernière trame reçue (« Recu: N min »), ou « OUT absent » si la sonde est muette.
- **Logs** : dernières lignes de journal (défilables à l'encodeur ; défilement automatique sur Super Mini).

### Aperçu des pages OLED

Représentation schématique de l'écran (128x64). Le bandeau du haut montre le titre
et le numéro de page (`n/total`). Les valeurs sont des exemples.

```text
  Meteo        1/12          Prev.        2/12          OUT Temp     8/12
 IN   26.5C  58%            Aujourd'hui                     _/\_    35.0
 OUT  21.5C  68%            Min 14C / Max 24C            _/     \__
 P    1019 hPa              Ensoleille                  /          \ 28.0
 Ensoleille                                             -24h        now

  Reseau       9/12          Sys.        10/12          Capteur     11/12
 SSID: MonWiFi              Heap:  180 KB              Bat 3.98V 72%
 IP:   192.168.1.42         PSRAM: 8192 KB             Canal: 12
 CH:12  RSSI -58            Flash: 16 MB               Trames rx145 ok143
 AA:BB:CC:DD:EE:FF          Ver:   1.35.0              Recu: 2min
```

La page **Capteur** (11/12), juste après **Système**, regroupe l'état de la sonde
extérieure : batterie (V + %, ou « Bat: absente »), canal ESP-NOW, compteurs de
trames reçues/validées (`rx`/`ok`) et fraîcheur de la dernière trame. La page
**Logs** (12/12) ferme le cycle.

---

## Fonctionnalités avancées et API web

### Interface web
- Accès à l’interface web via l’adresse http://<nom>.local (mDNS) ou IP locale.
- Menu principal à quatre entrées : **Tableau de bord**, **Statistiques**, **Historique**, **Système**. Les outils Fichiers et Logs sont accessibles depuis la page Système.

### Gestion des fichiers (LittleFS/SD)
- Navigation, téléchargement, suppression, upload de fichiers via l’interface web (onglet Fichiers), y compris dans les sous-dossiers.
- Accès aux fichiers SD si une carte est insérée et reconnue.
- API : `/api/files/list`, `/api/files/download`, `/api/files/delete`, `/api/files/upload` (paramètre `fs=sd` ou `fs=littlefs`).

### Historique et statistiques
- Page **Historique** : visualisation graphique de l’historique (température `°C`, humidité `Hu%`, pression `hPa`). Elle est volontairement simple - **voir** les données ; l'analyse approfondie est le rôle de morfAnalytics.
  - **Source** : *Extérieur (OUT)*, *Intérieur (IN)*, ou *Intérieur + Extérieur* (les deux tracés ensemble, sur le même axe de temps, pour comparer d'un coup d'œil).
  - **Sélection de période** : dernières 24 h / 48 h / 7 jours / 30 jours, « aujourd’hui », ou une plage **personnalisée** (champs *du* / *au*). L'échelle de temps horizontale s'adapte à la période choisie. Les périodes longues sont reconstituées à partir des fichiers binaires journaliers de la carte SD.
  - **Échelle** : mode *Fixe* / *Dynamique* / *Mixte*. En mode *Mixte*, le curseur **Zoom** (0 → 100 %) interpole entre l’échelle complète configurée (0 %, courbe quasi plate) et l’amplitude exacte des données (100 %, courbe pleine hauteur).
  - **Synthèse** (option, masquée par défaut) : une ligne au-dessus du graphe résume la période affichée pour chaque grandeur - variation (flèche ▲/▼/=), minimum, maximum et moyenne (en vue Intérieur + Extérieur, l'écart moyen OUT − IN est aussi rappelé).
  - **Temps réel** (case à côté de « Synthèse », activée par défaut) : active/désactive le rafraîchissement automatique, calé sur la cadence d'enregistrement (~5 min), pas sur un intervalle court.
- API : `/api/history`
  - Fenêtre glissante : `window`, `interval`, `points`.
  - Plage absolue : `from`, `to` (secondes Unix) et `interval` optionnel, plus `ctx=in|out` pour choisir la source (utilisée par la page Historique ; lecture SD + RAM, tranches vides renvoyées à `null`). La réponse indique aussi `measurement_interval_s` (cadence d'enregistrement).
- API : `/api/history/summary?from=&to=` - synthèse pré-calculée d'une plage (min/max/moyenne + variation par grandeur) reconstruite à partir des fichiers `.stats` journaliers, sans relire les mesures.
- Statistiques 24h via `/api/stats` (min, max, moyenne). Ces statistiques sont calculées avec un **filtre robuste (médiane/MAD)** qui écarte les valeurs aberrantes, y compris en série, pour rester représentatives.
- Page **Statistiques** : une bascule **« Mise à jour en temps réel »** (activée par défaut) active/désactive le rafraîchissement automatique des tableaux ; un bouton **« Actualiser »** permet un rafraîchissement manuel, et l'heure de dernière mise à jour est indiquée.

#### Stockage de l'historique (carte SD)
- L'historique est stocké au **format binaire compact**, découpé par contexte puis par jour, l'intérieur et l'extérieur ayant la **même arborescence** : `/history/indoor/AAAA/MM/AAAA-MM-JJ.bin` et `/history/outdoor/AAAA/MM/AAAA-MM-JJ.bin` (horodatage + température/humidité/pression). Ce format est plusieurs fois plus petit que le CSV et permet un accès direct à une mesure sans relire tout le fichier.
- Chaque `.bin` débute par un **en-tête** (magic `MTHB`, version de format, tailles, capteurs présents, nombre de mesures, premier/dernier relevé). Cet en-tête rend le format **pérenne** : de futurs capteurs pourront agrandir l'enregistrement sans imposer de convertir les anciens fichiers (compatibilité ascendante).
- **Qualité des données** : à l'exploitation (côté serveur, sur les mesures brutes avant agrégation), les valeurs manifestement aberrantes sont automatiquement **écartées des graphiques et des statistiques**, par grandeur (température, humidité et pression traitées indépendamment) :
  - pour les **graphes** (`/api/history`), un filtre de **cohérence temporelle** écarte un pic/creux ponctuel incohérent avec ses deux voisins (pas de seuil fixe), les points valides étant reliés directement ;
  - pour les **statistiques** (`/api/stats`), un filtre **robuste médiane/MAD** écarte les valeurs aberrantes même en série (ex. plusieurs mesures à 0 d'affilée après un incident capteur).
  - Les **mesures brutes restent conservées** dans les fichiers : seule leur exploitation est adaptée.
- À côté de chaque `.bin`, un fichier `.stats` conserve les statistiques du jour déjà calculées (min/max/moyenne, nombre de mesures, première/dernière mesure), mises à jour au fil des acquisitions - d'où l'affichage quasi instantané de la synthèse.
- Le **CSV** reste disponible **uniquement comme format d'export** (pour Excel/LibreOffice).
- Les anciens fichiers CSV présents sur la carte sont **convertis automatiquement au binaire au premier démarrage** (puis renommés en `.csv.bak`).
- Page Statistiques : tendances détaillées sur 1h, 12h, 24h et 48h (température, humidité, pression), ainsi qu’une tendance générale qui croise la direction de la pression sur ces fenêtres pour dégager une véritable évolution (amélioration/dégradation durable, stable, ou variable). La fenêtre 48h nécessite une carte SD avec l’historique journalier (fichier binaire de J-2) ; elle s’affiche « N/D » si indisponible.

### Logs et diagnostic
- Accès aux logs système via l’interface web (depuis la page **Système** → « Logs système ») et API `/api/logs`.
- Diagnostic système via `/api/system` (informations matérielles, firmware, SD, uptime, etc.).
- Les logs sont également émis sur le **port série** (miroir) et diffusés par **UDP** sur le réseau (voir ci-dessous).

### Monitoring des logs par UDP (sans câble série, ex. avec Tabby)

MeteoHub peut diffuser ses logs sur le réseau local en UDP, ce qui permet de les suivre à distance sans être branché physiquement au port série. Sont diffusés à la fois les logs applicatifs (capteurs, SD, migration, etc.) **et** ceux du cœur ESP (WiFi, erreurs I2C, watchdog…).

**1. Configuration (`include/config.h`)** puis reflashage :
- `UDP_LOG_ENABLED` : `1` pour activer (défaut), `0` pour désactiver.
- `UDP_LOG_PORT` : port UDP d'écoute (défaut `5005`).
- `UDP_LOG_HOST` : `"255.255.255.255"` pour un **broadcast** sur le réseau local (aucune IP à connaître), ou l'**IP du PC** récepteur pour un envoi direct (plus fiable si le broadcast est filtré). Dans ce dernier cas, il est conseillé de réserver/fixer l'IP du PC.

**2. Réception côté PC.** Tabby ne sait pas écouter l'UDP nativement : on ouvre un **onglet « terminal local »** dans Tabby et on y lance un écouteur UDP.
- **Linux / macOS** (recommandé, gère le broadcast) :
  ```bash
  socat -u UDP-RECV:5005 STDOUT
  ```
  Alternative avec netcat (selon la version) : `nc -u -l -k 5005`.
- **Windows** : installer **Ncat** (fourni avec Nmap), puis :
  ```powershell
  ncat -u -l 5005
  ```
  (ou utiliser `socat` sous WSL).

**3. Profil Tabby dédié (optionnel, pour un accès en un clic).** Dans Tabby : *Settings → Profiles & connections → New profile → Local terminal*, et renseigner comme commande l'écouteur ci-dessus (ex. `socat -u UDP-RECV:5005 STDOUT`). Le profil ouvre alors directement le flux de logs.

**4. Points d'attention :**
- Autoriser le **port UDP** (5005 par défaut) en entrée dans le pare-feu du PC.
- Le **broadcast** est le plus simple (pas besoin de connaître l'IP du PC) mais nécessite que le réseau laisse passer les paquets broadcast (généralement le cas sur un LAN domestique). En cas de souci, préférer l'envoi direct vers l'IP du PC (`UDP_LOG_HOST`).
- MeteoHub et le PC doivent être sur le **même réseau local**.
- **Logs de démarrage** : la capture est active dès le début du `setup()` et les logs de boot sont mis en tampon puis **rejoués dès la connexion WiFi**. Le moniteur UDP reçoit donc l'intégralité des logs (applicatifs et cœur ESP : montage SD et **fréquence SPI retenue**, init capteurs, événements WiFi, erreurs I2C…). Seules les toutes premières lignes du **bootloader ROM** et de l'init du cœur Arduino (émises avant `setup()`) restent visibles uniquement sur le port série physique - c'est une limite matérielle inhérente.

### Page Système
Le menu principal ne comporte que quatre entrées : **Tableau de bord**, **Statistiques**, **Historique** et **Système**. La page Système regroupe :
- **Luminosité de la LED** : réglage de la NeoLED (0-255), appliqué immédiatement et conservé au redémarrage (persisté en NVS). API : `GET`/`POST /api/led`.
- **Export** :
  - Historique au format **CSV** (dernières 24 h / 7 j / 30 j / tout), pour Excel/LibreOffice - API `GET /api/history/export.csv?from=&to=` ;
  - **Configuration** effective au format JSON - API `GET /api/config/export`.
- **Mise à jour du firmware (OTA)** : upload d’un fichier `.bin`, reboot automatique après succès. API : `/api/ota/update`. (L’ancienne URL `/ota.html` redirige vers `/system.html`.)
- **Historique** : bouton « Vider tout l'historique » (double confirmation) qui efface toutes les mesures enregistrées, intérieures et extérieures, en mémoire interne et sur la carte SD, pour repartir sur des mesures propres. API : `POST /api/history/clear`.
- **Outils** : accès au **gestionnaire de fichiers** (`/files.html`) et aux **logs système** (`/logs`), qui ne figurent plus dans le menu principal.

### Acquisition capteur (AHT20 + BMP280, bus I2C)
- Une mesure est enregistrée à la **cadence configurée** (5 minutes par défaut, `INDOOR_MEASUREMENT_INTERVAL_SECONDS` dans `include/config.h`), la même pour l'intérieur et l'extérieur pour des séries homogènes. L'**affichage** reste en temps réel (il relit la dernière mesure sans créer d'entrée d'historique). Chaque lecture est **vérifiée** (succès de la communication I2C + plausibilité) : en cas d'échec, le cycle est **sauté** plutôt que d'enregistrer une valeur erronée.
- **Robustesse** : jusqu'à 3 tentatives par lecture (les erreurs I2C sont souvent transitoires) et **récupération automatique du bus I2C** après plusieurs échecs consécutifs. En temps réel, la dernière valeur valide est affichée si une lecture échoue.
- Un badge signale une lecture capteur invalide sur le tableau de bord.
- En cas d'erreurs I2C fréquentes (`i2cRead returned Error -1` dans les logs), voir la section correspondante de [Maintenance et dépannage](maintenance_and_troubleshooting.md).

### Analyse avancée (service externe, optionnel)
> Guide pas à pas destiné aux débutants : **[Analyse avancée](analyse_avancee.md)**.

- MeteoHub reste **entièrement autonome** : mesures, historique, graphiques et exports fonctionnent sans aucun autre composant.
- Un service d'analyse présent sur le réseau local est détecté **passivement** via son annonce morfBeacon (UDP `45454`). La page **Système** indique alors « Analyse avancée disponible » ; sinon « indisponible », sans que le comportement nominal ne change.
- **Détection par capacité, jamais par nom.** MeteoHub cherche un service annonçant la capacité `advanced_analysis`, et n'utilise le nom annoncé que comme **libellé** affiché. Le projet étant sous licence GPL, chacun peut renommer son service : une détection fondée sur le nom cesserait de fonctionner au premier renommage.
- Quand un service est détecté, une entrée portant **son nom annoncé** apparaît dans le menu, juste avant « Système ». Elle s'ouvre dans le **même onglet** - la navigation reste fluide et n'accumule pas d'onglets ; le service propose en retour un lien « Retour à MeteoHub ».
- **Adresse manuelle de secours.** Si la découverte automatique ne passe pas (réseau segmenté, VPN, isolation des clients Wi-Fi), une adresse peut être saisie dans la page **Système**. Elle est persistée en NVS et **prend le pas** sur la découverte. Vider le champ rétablit le mode automatique.
- API : `GET /api/analytics` (`{available, mode, effective_url, capability, manual_url, detected:{found,name,version,host,status_port,last_seen_s}}`) et `POST /api/analytics/config` (`manual_url`). Réglages dans `include/config.h` (`ANALYTICS_BEACON_*`).
- Principe : **MeteoHub écrit, morfAnalytics lit - jamais l'inverse**. MeteoHub demeure la source de vérité ; morfAnalytics travaille sur une copie de l'historique et ne renvoie que des résultats synthétiques.

#### Alerte « accu de la sonde à changer » (via morfNotify)
- Si un service de notification (morfNotify ≥ 0.6.0) est présent sur le réseau, MeteoHub prévient **avant** que l'accu de la sonde extérieure ne coupe : un avertissement à **3,40 V** (« à remplacer dans les prochains jours »), une alerte à **3,20 V** (« à remplacer maintenant »), puis une confirmation quand un accu neuf est vu (**≥ 3,80 V**).
- Seuils en **tension**, pas en pourcentage : le % de la sonde est linéaire entre 2,6 et 4,2 V, alors qu'un accu Li-ion s'effondre sous ~3,4 V. À 20 %, soit 2,9 V, il serait déjà presque vide (coupure de protection à 2,4 V).
- **Anti-rebond** : un palier n'est franchi qu'après 3 mesures consécutives (~15 min) ; une retransmission ou un doublon ne compte pas. **Une seule notification par palier**, même après un redémarrage du hub (palier mémorisé en NVS).
- morfNotify est trouvé **par sa capacité** `notification` (annonce morfBeacon), jamais par son nom. MeteoHub n'indique aucune destination : morfNotify applique ses destinations par défaut (`/etc/morfsystem/alert-targets` sur le Pi).
- L'envoi se fait juste après une trame de la sonde, pendant qu'elle dort : il ne gêne jamais la radio. Un échec est retenté à la trame suivante. Sans morfNotify, rien ne part, mais l'OLED et la page web signalent toujours la pile faible (dès le même seuil).
- État dans `GET /api/live` (bloc extérieur) : `battery_alert` (0 = OK, 1 = faible, 2 = critique), `battery_alert_pending` (notification due, pas encore acceptée), `notify_available` (morfNotify détecté). Seuils dans `include/config.h` (`BATTERY_ALERT_*`).

#### Recopie de l'historique par le serveur d'analyse
- Deux API **en lecture seule** permettent au serveur de recopier l'historique sans jamais rien modifier sur MeteoHub :
  - `GET /api/history/days` - liste des journées présentes sur la carte SD, avec pour chacune le nombre de mesures enregistrées (`{day, nrec, first_ts, last_ts}`) ; accepte `ctx=in|out` (intérieur par défaut) ;
  - `GET /api/history/raw?day=AAAAMMJJ&index=N&limit=M` - mesures brutes de la journée, à partir de la position `N`, au format compact `[horodatage, température, humidité, pression]` ; accepte aussi `ctx=in|out` ;
  - `GET /api/forecast/history` - prévisions « du lendemain » archivées (une par jour cible), pour l'analyse « prévu vs observé » de morfAnalytics.
- Une mesure est repérée par sa **position dans le fichier du jour**, et non par son horodatage. Les fichiers étant écrits en ajout seul, cette position ne change jamais, alors qu'un horodatage peut reculer lors d'un recalage d'horloge ou se répéter lors du passage à l'heure d'hiver. Le serveur d'analyse retient donc le couple (jour, position) et ne redemande que ce qui manque : aucune mesure n'est transférée deux fois.
- Ces API ne sont utiles qu'à un serveur d'analyse. Pour un export manuel, préférer le CSV (`GET /api/history/export.csv`), directement exploitable dans un tableur.

### Alertes météo et tendances
- Affichage des alertes météo (niveau, type, description, couleur) sur l’interface et l’OLED.
- Traduction automatique des alertes (anglais → français).
- Synthèse de tendance météo (amélioration, pluie, perturbation, etc.).

### Maintenance avancée
- Appui long sur le bouton BOOT au démarrage : formatage du système de fichiers interne (LittleFS) et redémarrage.

Pour plus de détails sur chaque API, voir la documentation technique ou le code source (web_manager.cpp).
