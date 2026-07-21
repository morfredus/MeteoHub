# Roadmap — MeteoHub S3

MeteoHub S3 est une station météo autonome sur ESP32-S3 : acquisition capteurs,
historisation locale sur carte SD, tableau de bord OLED et interface Web.

Sa contrainte fondatrice est celle de l'écosystème : **l'équipement reste
propriétaire de ses données**. MeteoHub écrit, les autres lisent. morfAnalytics
recopie son historique dans un cache de travail pour y mener des analyses
lourdes, sans jamais écrire en retour.

## Pistes

- **Exposer la version sur le réseau.** La version vit désormais dans le
  fichier `VERSION` à la racine, injecté à la compilation par
  `scripts/version.py`. Elle est donc lisible par un outil d'inventaire du parc
  — reste à l'exposer sur une route HTTP dédiée pour qu'elle soit interrogeable
  à distance comme celle des services C++.

- **Annonce de présence morfBeacon.** MeteoHub est aujourd'hui découvert par
  sonde réseau (test d'ouverture d'un port TCP déclaré dans `morfsystem.json`),
  ce qui suppose de connaître son nom mDNS à l'avance. Une annonce UDP le
  rendrait découvrable sans configuration, comme les services du parc. Le
  protocole `morfbeacon/1` est volontairement minimal et n'exige pas Qt.

- **Collecte incrémentale plus robuste.** Le curseur de morfAnalytics repose sur
  le couple (jour, index) plutôt que sur un horodatage, précisément parce qu'un
  ESP32 dérive. Documenter ce contrat côté MeteoHub éviterait qu'une évolution
  du format binaire le casse silencieusement.

- **Tests des formules et du format binaire.** L'en-tête de fichier assure déjà
  une compatibilité ascendante ; un test de relecture d'un fichier de référence
  figerait cette garantie.

## Non-objectifs

- **Pas d'analyse longue durée à bord.** Les statistiques lourdes, corrélations
  et tendances saisonnières sont le rôle de morfAnalytics, qui travaille sur une
  copie et dispose de bien plus de ressources.

- **Pas de dépendance au cloud.** L'appareil fonctionne seul sur le réseau
  local. Les prévisions récupérées depuis Internet sont un confort, jamais une
  condition de fonctionnement.

- **Pas de notification.** Détecter un seuil et prévenir l'utilisateur relève de
  morfNotify, qui est le point unique de diffusion de l'écosystème.

- **Pas d'écriture par un tiers.** Aucun service extérieur ne modifie
  l'historique : c'est la contrepartie du principe qui fait de l'équipement le
  propriétaire de ses données.
