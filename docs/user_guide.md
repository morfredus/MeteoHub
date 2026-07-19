# Guide utilisateur

**Débutant ?** Voir le [Guide Débutant](beginner/index.md)

Version minimale valide : 1.9.0

- Tourner l’encodeur pour naviguer entre les pages.
- Le clic encodeur ouvre le menu.
- Back sort du menu/des confirmations.
- Confirm valide les actions contextuelles.


Pages principales :
- Météo
- Prévisions
- Graphes (température, humidité, pression)
- Réseau
- Système
- Logs

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
- Page **Historique** : visualisation graphique de l’historique (température `°C`, humidité `Hu%`, pression `hPa`).
  - **Sélection de période** : dernières 24 h / 48 h / 7 jours / 30 jours, « aujourd’hui », ou une plage **personnalisée** (champs *du* / *au*). Les périodes au-delà de ~24 h sont reconstituées à partir des fichiers binaires journaliers de la carte SD.
  - **Comparaison de deux périodes** : « aucune », « période précédente » (même durée, juste avant la période affichée) ou « autre période… » (début libre, durée identique à la période principale). La période comparée (B) est tracée en pointillés et alignée sur la période principale (A) ; une ligne d’information rappelle les plages exactes.
  - **Échelle** : mode *Fixe* / *Dynamique* / *Mixte*. En mode *Mixte*, le curseur **Zoom** (0 → 100 %) interpole entre l’échelle complète configurée (0 %, courbe quasi plate) et l’amplitude exacte des données (100 %, courbe pleine hauteur).
  - **Synthèse** (option, masquée par défaut) : une ligne au-dessus du graphe résume la période affichée pour chaque grandeur — variation sur la période (flèche ▲/▼/=), minimum, maximum et moyenne — pour une lecture rapide sans analyser les courbes.
  - **Temps réel** (case à côté de « Synthèse », activée par défaut) : active/désactive le rafraîchissement automatique du graphe (limité aux périodes relatives ≤ 48 h sans comparaison).
- API : `/api/history`
  - Fenêtre glissante : `window`, `interval`, `points` (utilisée par le tableau de bord).
  - Plage absolue : `from`, `to` (secondes Unix) et `interval` optionnel (utilisée par la page Historique ; lecture SD + RAM, tranches vides renvoyées à `null`).
- API : `/api/history/summary?from=&to=` — synthèse pré-calculée d'une plage (min/max/moyenne + variation par grandeur) reconstruite à partir des fichiers `.stats` journaliers, sans relire les mesures.
- Statistiques 24h via `/api/stats` (min, max, moyenne). Ces statistiques sont calculées avec un **filtre robuste (médiane/MAD)** qui écarte les valeurs aberrantes, y compris en série, pour rester représentatives.
- Page **Statistiques** : une bascule **« Mise à jour en temps réel »** (activée par défaut) active/désactive le rafraîchissement automatique des tableaux ; un bouton **« Actualiser »** permet un rafraîchissement manuel, et l'heure de dernière mise à jour est indiquée.

#### Stockage de l'historique (carte SD)
- L'historique est stocké au **format binaire compact**, découpé par jour : `/history/AAAA/MM/AAAA-MM-JJ.bin` (enregistrements de 16 octets : horodatage + température/humidité/pression). Ce format est plusieurs fois plus petit que le CSV et permet un accès direct à une mesure sans relire tout le fichier ; une consultation 24 h ne lit qu'un fichier.
- Chaque `.bin` débute par un **en-tête** (magic `MTHB`, version de format, tailles, capteurs présents, nombre de mesures, premier/dernier relevé). Cet en-tête rend le format **pérenne** : de futurs capteurs pourront agrandir l'enregistrement sans imposer de convertir les anciens fichiers (compatibilité ascendante).
- **Qualité des données** : à l'exploitation (côté serveur, sur les mesures brutes avant agrégation), les valeurs manifestement aberrantes sont automatiquement **écartées des graphiques et des statistiques**, par grandeur (température, humidité et pression traitées indépendamment) :
  - pour les **graphes** (`/api/history`), un filtre de **cohérence temporelle** écarte un pic/creux ponctuel incohérent avec ses deux voisins (pas de seuil fixe), les points valides étant reliés directement ;
  - pour les **statistiques** (`/api/stats`), un filtre **robuste médiane/MAD** écarte les valeurs aberrantes même en série (ex. plusieurs mesures à 0 d'affilée après un incident capteur).
  - Les **mesures brutes restent conservées** dans les fichiers : seule leur exploitation est adaptée.
- À côté de chaque `.bin`, un fichier `.stats` conserve les statistiques du jour déjà calculées (min/max/moyenne, nombre de mesures, première/dernière mesure), mises à jour au fil des acquisitions — d'où l'affichage quasi instantané de la synthèse.
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
- **Logs de démarrage** : la capture est active dès le début du `setup()` et les logs de boot sont mis en tampon puis **rejoués dès la connexion WiFi**. Le moniteur UDP reçoit donc l'intégralité des logs (applicatifs et cœur ESP : montage SD et **fréquence SPI retenue**, init capteurs, événements WiFi, erreurs I2C…). Seules les toutes premières lignes du **bootloader ROM** et de l'init du cœur Arduino (émises avant `setup()`) restent visibles uniquement sur le port série physique — c'est une limite matérielle inhérente.

### Page Système
Le menu principal ne comporte que quatre entrées : **Tableau de bord**, **Statistiques**, **Historique** et **Système**. La page Système regroupe :
- **Luminosité de la LED** : réglage de la NeoLED (0-255), appliqué immédiatement et conservé au redémarrage (persisté en NVS). API : `GET`/`POST /api/led`.
- **Export** :
  - Historique au format **CSV** (dernières 24 h / 7 j / 30 j / tout), pour Excel/LibreOffice — API `GET /api/history/export.csv?from=&to=` ;
  - **Configuration** effective au format JSON — API `GET /api/config/export`.
- **Mise à jour du firmware (OTA)** : upload d’un fichier `.bin`, reboot automatique après succès. API : `/api/ota/update`. (L’ancienne URL `/ota.html` redirige vers `/system.html`.)
- **Outils** : accès au **gestionnaire de fichiers** (`/files.html`) et aux **logs système** (`/logs`), qui ne figurent plus dans le menu principal.

### Acquisition capteur (AHT20 + BMP280, bus I2C)
- Une mesure est enregistrée toutes les minutes. Chaque lecture est **vérifiée** (succès de la communication I2C + plausibilité) : en cas d'échec, la minute est **sautée** plutôt que d'enregistrer une valeur erronée.
- **Robustesse** : jusqu'à 3 tentatives par lecture (les erreurs I2C sont souvent transitoires) et **récupération automatique du bus I2C** après plusieurs échecs consécutifs. En temps réel, la dernière valeur valide est affichée si une lecture échoue.
- Un badge signale une lecture capteur invalide sur le tableau de bord.
- En cas d'erreurs I2C fréquentes (`i2cRead returned Error -1` dans les logs), voir la section correspondante de [Maintenance et dépannage](maintenance_and_troubleshooting.md).

### Alertes météo et tendances
- Affichage des alertes météo (niveau, type, description, couleur) sur l’interface et l’OLED.
- Traduction automatique des alertes (anglais → français).
- Synthèse de tendance météo (amélioration, pluie, perturbation, etc.).

### Maintenance avancée
- Appui long sur le bouton BOOT au démarrage : formatage du système de fichiers interne (LittleFS) et redémarrage.

Pour plus de détails sur chaque API, voir la documentation technique ou le code source (web_manager.cpp).
