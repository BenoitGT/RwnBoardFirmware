---
tags: [sdd, dcc-control-app, domaine]
banner: "[[microchip_banner.png]]"
banner_y: 70
---

# Modèle de domaine & configuration

Voir [[03-Convention-Nommage]] pour la distinction `RawConfig*` / modèle interne / `*Dto`.

## NetworkConfig (anciennement HardwareConfig)

Renommage délibéré pour intégrer une vision plus large que le seul matériel : **décodeurs, locomotives, accessoires, et à terme une `NetworkMapDto`** (futur TCO, rôle Manager).

```typescript
export interface NetworkConfigDto {
  decoders: DecoderDto[];
  locomotives: LocomotiveDto[];
  accessories: AccessoryDto[];
  networkMap?: NetworkMapDto;   // futur, optionnel
}
```

Chargé depuis un fichier `network-config-<accessPointId>.json`, en dur pour le POC (pas d'édition UI pour l'instant — prévue côté rôle Admin plus tard).

## Decoder

Entité DCC adressable physique — séparée de l'élément métier qu'elle pilote (voir plus bas, découplage important).

```typescript
interface Decoder {
  id: string;
  family: 'multifunction' | 'basicAccessory' | 'extendedAccessory';
  address: number;
  addressing?: 'short' | 'long';   // pertinent pour multifunction
  outputsCount?: number;            // 4 en général, parfois 8 selon modèle — voir note NMRA ci-dessous
}
```

> [!note] Détail protocole NMRA (S9.2.1 §2.4.1)
> La "règle des 4 sorties" pour un basic accessory decoder n'est pas explicite dans le texte du §2.4.1 lui-même — elle découle de la structure des bits (2 bits de sélection = 4 combinaisons), documentée plus clairement dans S9.2.2 (CV33 : statut des 4 paires de sorties). Les décodeurs "8 sorties" ne sont pas une extension du protocole : ils combinent 2 blocs de 4, ou utilisent le mode d'adressage "Output Address" (CV29 bit 6) où chaque sortie a sa propre adresse DCC complète plutôt qu'un partage d'adresse de base.

## Découplage important : décodeur ↔ élément métier

Deux couplages trop rigides identifiés et corrigés en cours de conception :

1. **Un `Turnout` n'est pas forcément piloté par un `basicAccessory`** : un aiguillage à servo-moteur avec positions pré-paramétrées peut être géré par un `extendedAccessory` — les positions sont calibrées dans les CV du décodeur, le paquet DCC envoie juste l'index d'aspect correspondant.
2. **Un décodeur `multifunction` ne représente pas forcément une `Locomotive`** : peut être un module stationnaire (ex: effets lumineux via F0-F28, sans notion de mouvement).

### Stratégie de contrôle découplée (`AccessoryControl`)

```typescript
type AccessoryControl = BasicOutputControl | ExtendedAspectControl;

interface BasicOutputControl {
  kind: 'basicOutput';
  outputIndex: number;   // 0..3
}

interface ExtendedAspectControl {
  kind: 'extendedAspect';
  aspectMap: Record<string, number>;   // état métier (id) → valeur d'aspect (0-31)
}
```

### Rôle découplé pour les décodeurs multifonction

```typescript
interface MultifunctionEntityBase {
  id: string;
  label: string;
  decoderId: string;
  functions: LocoFunction[];
}

interface Locomotive extends MultifunctionEntityBase {
  role: 'locomotive';
  speedSteps: 14 | 28 | 128;
  direction: 'forward' | 'reverse';
  currentSpeed: number;
}

interface StationaryFunctionModule extends MultifunctionEntityBase {
  role: 'stationary';   // pas de vitesse/direction pertinents
}
```

## Fonctions loco : indépendantes vs logiques (combinatoires)

Cas réel qui a motivé ce découpage : 2 fils (FL + F1) multiplexés donnant 4 états logiques :
`00=Éteint, 01=Feux blanc sens marche, 10=Feux blanc/rouge selon sens, 11=Feux Rouge AV/AR`.

```typescript
interface IndependentFunction {
  kind: 'independent';
  index: number;        // F0(FL), F1, F2...
  label: string;
  state: boolean;
}

interface LogicalFunctionState {
  id: string;
  label: string;
  physicalPattern: boolean[];   // un bit par entrée de physicalOutputs, même ordre
}

interface LogicalFunction {
  kind: 'logical';
  id: string;
  label: string;
  physicalOutputs: number[];     // indices F concernés, dans l'ordre du pattern
  states: LogicalFunctionState[];
  currentStateId: string;
}

type LocoFunction = IndependentFunction | LogicalFunction;
```

Résolution logique → bits physiques (fonction pure) :

```typescript
function resolveLogicalFunctionBits(fn: LogicalFunction, targetStateId: string): Map<number, boolean> {
  const state = fn.states.find(s => s.id === targetStateId);
  if (!state) throw new Error(`État "${targetStateId}" inconnu pour "${fn.label}"`);
  const bits = new Map<number, boolean>();
  fn.physicalOutputs.forEach((fIndex, i) => bits.set(fIndex, state.physicalPattern[i]));
  return bits;
}
```

UI : une `IndependentFunction` = toggle simple. Une `LogicalFunction` = sélecteur à état unique (radio/select), pas des toggles indépendants (incohérence possible sinon).

## Zone

```typescript
export type ETrackDirection = 'increasing' | 'decreasing' | 'bidirectional';
export type EOccupancyState = 'free' | 'occupied' | 'reserved';

export interface ZoneDto {
  id: string;           // "z011"
  label?: string;         // ex: "Voie 1"
  direction: ETrackDirection;
  occupancy: EOccupancyState;
}
```

- `bidirectional` pour les voies de garage, sans sens de circulation fixé.
- `occupancy` : pas de commande utilisateur directe (contrairement à `Accessory`) — change uniquement via évènement poussé par la gateway (retour capteur, plus tard gestion de parcours). `reserved` n'a de sens que le jour où la gestion d'itinéraires existera.

## Sector

```typescript
export interface SectorDto {
  id: string;
  label?: string;
}
```

Secteur d'aimantation — regroupe des zones alimentées ensemble. **La relation zone↔secteur n'est volontairement PAS portée par `ZoneDto`/`SectorDto`** (pas de `sectorId`/`zoneIds` croisés) : c'est une donnée de configuration **Admin**, gérée via des messages WebSocket dédiés (`admin:*`), jamais exposée dans les DTO consultés par les autres rôles. Deux UX envisagées côté Admin :
1. Depuis la map : sélectionner une zone → liste des secteurs disponibles → association.
2. Depuis un secteur : deux listes de zones (ajout/retrait).

Voir [[06-Topologie-Graphe]] pour la relation accessoire ↔ zones alimentées (`feedsZoneIds`, distincte de zone↔secteur).

## Résolution d'adresse accessoire (exemple, à valider selon le firmware réel)

```typescript
function resolveAccessoryAddress(decoder: Decoder, output: number): { address: number; subOutput: number } {
  if (decoder.family === 'basicAccessory') {
    return { address: decoder.address, subOutput: output % 4 };
  }
  return { address: decoder.address + output, subOutput: 0 };
}
```
> ⚠️ Convention à valider contre le firmware réel — varie selon les fabricants/implémentations.
