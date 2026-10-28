---
tags: [sdd, dcc-control-app, journal]
banner: "[[microchip_banner.png]]"
banner_y: 70
---

# Journal des décisions (chronologique)

> Format court façon ADR — une ligne par décision significative, dans l'ordre où elle a émergé. Pour le détail, voir la note liée entre crochets.

- **Structuration Angular du projet** → architecture monorepo gateway/frontend plutôt qu'un seul projet front. [[02-Modular Architecture]]
- **Multi-protocoles (LocoNet, NMRA, XpressNet) + multi-connexions (LAN, Série)** dès la conception initiale. [[01-Vision-et-Contexte]]
- **Pas de connexion simultanée** à plusieurs centrales — scan puis sélection unique. [[08-Contrat-WebSocket]]
- **Premier jalon : POC bas niveau NMRA en série**, sans gestion CV. [[01-Vision-et-Contexte]]
- **Framing custom** (préfixe/suffixe) car le firmware maison envoie des paquets NMRA sans préambule ni bits de séparation. [[07-Communication-Serie]]
- **Decoder découplé de l'élément métier** : un turnout peut être sur extended accessory (servo), un multifunction decoder peut ne pas être une loco. [[04-Modele-Domaine-Config]]
- **Pas d'édition UI pour l'instant** — configuration en dur via fichier JSON. [[01-Vision-et-Contexte]]
- **Fonctions loco combinatoires** (logical vs independent) pour gérer le multiplexage de sorties physiques. [[04-Modele-Domaine-Config]]
- **Modèle Zone/Sector** avec direction de circulation et regroupement électrique. [[04-Modele-Domaine-Config]]
- **Convention direction : `increasing`/`decreasing`/`bidirectional`** (pas `toParis`/`toProvince`, pas `normal`/`reverse`). [[03-Convention-Nommage]]
- **Relation zone↔secteur hors DTO** — gérée côté Admin via messages dédiés. [[04-Modele-Domaine-Config]]
- **Convention `*Dto` systématique** côté shared-contracts. [[03-Convention-Nommage]]
- **Discriminant `family`** (pas `type`, pas `kind`) pour accessoires/décodeurs. [[03-Convention-Nommage]]
- **Modèle en graphe** pour la topologie (zones=arêtes, signaux/turnouts=nœuds, toe/heel). [[06-Topologie-Graphe]] — *non implémenté*
- **Architecture Transport/Protocol/FrameCodec finale** : Protocol pilote Transport, pas d'ExchangePolicy séparée. [[07-Communication-Serie]]
- **Framing : START 3 octets, pas de END** — fiabilisation expérimentale. [[07-Communication-Serie]]
- **Modèle d'accessoire générique** (`range`/`enum`/`flags`) pour extensibilité sans recompilation. [[05-Accessory-Generique]]
- **Lumière = enum à 2 options**, pas un `valueKind: 'boolean'` dédié. [[05-Accessory-Generique]]
- **Fusion schéma + valeur courante** dans `AccessoryValueDto` (élimine la redondance et les états invalides). [[05-Accessory-Generique]]
- **`RawConfig*` / modèle interne / `*Dto`** — convention à trois niveaux pour clarifier qui manipule quoi. [[03-Convention-Nommage]]
- **Streaming au lieu de snapshot bloquant** pour discovery et config (anticipation d'un scan LAN long). [[08-Contrat-WebSocket]]
- **Statut `AccessPoint` enrichi** : `Unknown`/`Available`/`Nominal`/`Degraded`/`Error` plutôt qu'un booléen `available`. [[08-Contrat-WebSocket]]
- **Sérialisation des commandes + état "pending" frontend** pour gérer la latence protocolaire. [[10-Asynchronicite-et-Robustesse]]

## Pièges techniques rencontrés (hors décisions de conception, mais à garder en mémoire)

Voir la section dédiée dans [[02-Modular Architecture]] — extensions `.js` manquantes (NodeNext), `__dirname` en ESM, exports oubliés dans `index.ts`, `workspace:*` vs `*` (npm vs pnpm), `routerLink` statique en conflit avec une garde `(click)`, `| async` utilisé par erreur sur un Signal.

## Sujets ouverts / reportés

- Gestion de rôles (Admin/Manager/Operator) par client IP/MAC — évoqué, non traité.
- Scan réseau LAN réel (actuellement fichier statique uniquement).
- `LocoNetProtocol`, `XpressNetProtocol`, `TcpTransport` — non implémentés.
- Modèle de graphe topologique — discuté, non codé.
- Gestion de parcours / réservation d'itinéraires.
- `NetworkMapDto` (futur TCO) — champ optionnel posé, contenu non défini.
- Validation stricte de config (type zod ou équivalent) — principe posé, pas systématiquement implémenté.
- Reconnexion automatique WebSocket frontend↔gateway (distincte de la reconnexion série gateway↔firmware).
