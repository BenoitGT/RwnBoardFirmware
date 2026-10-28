---
tags: [sdd, dcc-control-app]
banner: "[[microchip_banner.png]]"
banner_y: 70
---

# Vision et contexte

## Objectif

Développer une application personnelle de contrôle d'une centrale DCC (modélisme ferroviaire), avec :
- support **multi-protocoles** : NMRA-DCC, LocoNet, XpressNet
- support **multi-connexions** : Série, LAN
- pas de connexion simultanée à plusieurs centrales : l'appli scanne les points d'accès disponibles, l'utilisateur en choisit un

## Stack

- **Frontend** : Angular (standalone components, Signals — pas de RxJS pour le state applicatif)
- **Backend ("gateway")** : Node.js + TypeScript, ESM natif (`NodeNext`)
- **Transport applicatif** : WebSocket entre frontend et gateway
- **Éditeur** : VSCode, environnement Linux

## Démarche adoptée

Plutôt que d'attaquer directement l'application complète, la démarche a été incrémentale :

1. **POC bas niveau** : test série + protocole NMRA pour commander locos et accessoires (basic/extended), **sans gestion des CV** dans un premier temps — cf. norme NMRA S9.2.1.
2. **Architecture en couches** découplées (voir [[07-Communication-Serie]]) pour pouvoir brancher LocoNet/XpressNet plus tard sans tout réécrire.
3. **Configuration en dur** (fichiers JSON) avant toute UI d'édition — l'édition via GUI (rôle Admin) est un sujet reporté.
4. Le **matériel réel** utilisé pour le POC : firmware µC maison, parlant un sous-ensemble des paquets NMRA (S9.2.1) sans préambule ni bits de séparation — le framing est une couche ajoutée par-dessus, propre à ce firmware (voir [[07-Communication-Serie]]).

## Hors périmètre (pour l'instant)

- Gestion des CV (programmation décodeur)
- Édition de configuration via UI (prévue pour un rôle **Admin** futur)
- Gestion de rôles/permissions par client (**Admin / Manager / Operator**, identification par IP/MAC) — évoquée, non implémentée
- Gestion de parcours / réservation d'itinéraires (la notion de zone `reserved` existe dans le modèle mais sans logique derrière)
- Scan réseau LAN réel (le `DiscoveryService` actuel ne lit qu'un fichier de config statique, le scan réseau proprement dit reste à écrire)

## Vision à plus long terme (évoquée, non détaillée)

- **TCO (Tableau de Contrôle Optique)** futur pour le rôle Manager, avec une `NetworkMapDto` — notion posée dans le modèle (`NetworkConfig` incluant `networkMap`) mais non développée.
- Gestion de rôles par client pour restreindre l'accès (ex: un Operator ne voit que les locomotives, pas les accessoires d'infrastructure).
