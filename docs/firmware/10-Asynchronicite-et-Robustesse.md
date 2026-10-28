---
tags: [sdd, dcc-control-app, robustesse]
banner: "[[microchip_banner.png]]"
banner_y: 70
---

# Asynchronicité et robustesse

## Problème identifié

Une mise à jour de valeur (`onSetAccessory`) implique un ou plusieurs échanges protocolaires, potentiellement lents (série, et plus tard scrutation XpressNet). Trois sous-problèmes distincts à traiter séparément.

## 1. Frontend : état "pending" explicite

Le frontend n'attend déjà rien de bloquant (WebSocket `send()` est fire-and-forget), mais **rien n'indiquait à l'utilisateur qu'une commande était en cours** — besoin d'un état visuel entre le clic et la confirmation (`event:accessoryValueChanged` ou `error`).

```typescript
readonly pendingAccessoryIds = signal<Set<string>>(new Set());
```
Le composant grise/désactive le contrôle tant que `pendingAccessoryIds().has(accessory.id)`. Voir [[09-Frontend-Angular]] pour l'implémentation complète. Nécessite de corréler `error` à l'`accessoryId` concerné (`{ type: 'error'; message: string; accessoryId?: string }`).

## 2. Backend : sérialisation des commandes (pas de parallélisme sur une session)

Node n'est pas bloqué par un `await` (l'event loop continue), donc "rendre la main" au niveau serveur n'est pas le vrai risque. Le vrai risque : **deux commandes concurrentes sur la même session** (clic rapide, double clic) peuvent s'exécuter en parallèle — problématique pour tout protocole avec état partagé (XpressNet scrutation, LocoNet collision).

```typescript
class RwnSession {
  private queue: Promise<unknown> = Promise.resolve();

  sendCommand(cmd: OutgoingCommand): Promise<void> {
    const result = this.queue.then(() => this.doSendCommand(cmd));
    this.queue = result.catch(() => undefined);   // une erreur ne bloque pas la suite de la file
    return result;
  }

  private async doSendCommand(cmd: OutgoingCommand): Promise<void> {
    /* encode → frame → transport.send() */
  }
}
```

> [!note] Cohérent avec la décision d'architecture finale
> Voir [[07-Communication-Serie]] — `RwnSession.sendCommand()` délègue en réalité à `Protocol.sendCommand()`. La sérialisation par file d'attente devrait donc probablement vivre **dans `Protocol`** plutôt que dans `RwnSession`, pour rester cohérente avec "le protocole pilote le transport" et "ExchangePolicy intégrée au Protocol". **Point à ajuster** par rapport à l'esquisse initiale ci-dessus (écrite avant la décision finale d'architecture).

## 3. Filet de sécurité : timeout global

Si le transport physique se bloque (port série figé), une commande peut ne jamais résoudre — gèlerait toute la file derrière elle.

```typescript
function withTimeout<T>(p: Promise<T>, ms: number): Promise<T> {
  return Promise.race([
    p,
    new Promise<T>((_, reject) => setTimeout(() => reject(new Error('Timeout commande')), ms)),
  ]);
}
```
À appliquer autour de l'envoi réel, en filet au-dessus (pas en remplacement) d'un éventuel retry/timeout déjà interne à un protocole comme XpressNet.

## Robustesse de configuration (déjà appliqué, pattern à réutiliser)

Pattern retenu pour tout chargement de fichier de config : **logger précisément l'entrée fautive et continuer**, plutôt que de faire planter tout le chargement pour une seule entrée mal formée.

```typescript
for (const [i, entry] of entries.entries()) {
  try { yield toSomething(entry, i); }
  catch (err) { console.error(`entries[${i}]:`, (err as Error).message); }
}
```
Appliqué à `access_points.json`, à généraliser à `network-config-*.json` (accessories/locomotives).

## Validation référentielle au chargement (principe posé, pas totalement implémenté)

Vérifier au chargement — pas seulement à l'usage — que chaque `decoderId` référencé par un accessoire/loco existe bien dans la liste des décodeurs, et que la cohérence `decoder.family` ↔ `control.kind` est respectée. Objectif : détecter une erreur de config à froid plutôt qu'au moment où l'utilisateur actionne l'aiguillage sur la maquette.
