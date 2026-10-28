---
tags: [sdd, dcc-control-app, frontend]
banner: "[[microchip_banner.png]]"
banner_y: 70
---

# Frontend Angular

## Choix techniques

- **Standalone components**, pas de NgModules.
- **Angular Signals** pour tout le state applicatif — **pas de RxJS** (`Subject`/`BehaviorSubject` + `async` pipe) pour ce besoin. Erreur rencontrée en pratique : un template avec `accessPoints$ | async` sur une propriété qui est en réalité un `Signal` (pas un `Observable`) — les deux primitives ne s'interopèrent pas directement sans `toObservable`/`toSignal` (`@angular/core/rxjs-interop`), inutile ici.
- Templates avec la nouvelle syntaxe `@for`/`@if`/`@empty`.

## Service central : `RwnBridgeConnectionService`

(renommé depuis `GatewayConnectionService` — "gateway" devenu "backend" dans le vocabulaire projet)

```typescript
@Injectable({ providedIn: 'root' })
export class RwnBridgeConnectionService {
  private ws?: WebSocket;
  private pendingScan = false;

  readonly connectionState = signal<'disconnected' | 'connecting' | 'connected' | 'error'>('disconnected');
  readonly accessPoints = signal<AccessPointDto[]>([]);
  readonly scanning = signal(false);
  readonly accessories = signal<AccessoryDto[]>([]);
  readonly locomotives = signal<LocomotiveDto[]>([]);
  readonly pendingAccessoryIds = signal<Set<string>>(new Set());

  connect(gatewayUrl: string): void {
    this.ws = new WebSocket(gatewayUrl);
    this.ws.onopen = () => {
      this.connectionState.set('connected');
      if (this.pendingScan) { this.send({ type: 'discovery:scan' }); this.pendingScan = false; }
    };
    this.ws.onmessage = (evt) => this.handleMessage(JSON.parse(evt.data));
  }

  scanAccessPoints(): void {
    if (this.ws?.readyState === WebSocket.OPEN) this.send({ type: 'discovery:scan' });
    else this.pendingScan = true;
  }

  requestConnection(accessPointId: string): void {
    this.send({ type: 'connection:request', stationId: accessPointId });
  }

  setAccessoryValue(accessoryId: string, update: AccessoryValueUpdateDto): void {
    this.pendingAccessoryIds.update(s => new Set(s).add(accessoryId));
    this.send({ type: 'command:setAccessory', accessoryId, update });
  }

  private handleMessage(msg: RwnServerMessage): void {
    switch (msg.type) {
      case 'discovery:started': this.accessPoints.set([]); this.scanning.set(true); break;
      case 'discovery:found': this.accessPoints.update(l => [...l, msg.accessPoint]); break;
      case 'discovery:completed': this.scanning.set(false); break;
      case 'connection:state': this.connectionState.set(msg.state); break;
      case 'config:accessoryFound': this.accessories.update(l => [...l, msg.accessory]); break;
      case 'config:locomotiveFound': this.locomotives.update(l => [...l, msg.locomotive]); break;
      case 'event:accessoryValueChanged':
        this.accessories.update(l => l.map(a => a.id === msg.accessoryId ? { ...a, value: msg.update } as AccessoryDto : a));
        this.clearPending(msg.accessoryId);
        break;
      // ...
    }
  }

  private clearPending(accessoryId: string): void {
    this.pendingAccessoryIds.update(s => { const n = new Set(s); n.delete(accessoryId); return n; });
  }

  private send(msg: RwnClientMessage): void {
    if (this.ws?.readyState !== WebSocket.OPEN) { console.warn('WS non connecté, message ignoré:', msg.type); return; }
    this.ws.send(JSON.stringify(msg));
  }
}
```

> [!warning] Piège rencontré
> `send()` appelé juste après `new WebSocket(url)` échoue (`Still in CONNECTING state`) — toujours passer par le callback `onopen`, ou bufferiser (`pendingScan`) comme ci-dessus si l'appel peut arriver avant la connexion effective.

## Composants

### `RwnNetworkListComponent` (liste des points d'accès, écran d'accueil)

- `isClickable(ap)` : filtre sur `status` (`Available`/`Nominal`/`Degraded` cliquables, pas `Unknown`/`Error`) et absence de connexion en cours.
- Navigation déclenchée par un **`effect()`** réagissant à `connectionState() === 'connected'`, **pas** par un `routerLink` statique sur l'élément de liste (bug rencontré : un `routerLink` fixe déclenchait la navigation indépendamment de la garde `isClickable`, en plus et sans rapport avec le `(click)` — à ne pas laisser cohabiter les deux mécanismes sur le même élément).

```typescript
constructor() {
  effect(() => {
    const state = this.connectionState();
    if (!this.selectedAccessPointId()) return;
    if (state === 'connected') this.router.navigate(['/accessories-list']);
    else if (state === 'error') this.selectedAccessPointId.set(null);
  });
}
```

Garde côté template, avec filet de sécurité CSS :
```html
<li [class.disabled]="!isClickable(ap)" (click)="isClickable(ap) && onItemClicked(ap)">
```
```css
.disabled { pointer-events: none; opacity: 0.5; }
```

### `RwnAccessoriesListComponent` (locos + accessoires après connexion)

```typescript
items = computed<DisplayItem[]>(() => [
  ...this.rwnBridge.accessories().map(a => ({ id: a.id, label: a.label, iconClass: this.iconFor(a.family) })),
  ...this.rwnBridge.locomotives().map(l => ({ id: l.id, label: l.label, iconClass: 'icon-locomotive' })),
]);
```

> [!note] Adresse décodeur retirée de l'affichage
> Un affichage initial montrait l'adresse DCC brute ("200", "116"...) à titre informatif — décision prise de **ne jamais exposer l'adresse DCC côté frontend**, aucune valeur opérationnelle pour l'utilisateur. Remplacée à terme par une position lisible (ex: "Sur la voie 2", via `currentZoneId`, optionnel tant qu'il n'y a pas de détection d'occupation).

### Composants de contrôle génériques (voir [[05-Accessory-Generique]])

```typescript
@Component({ selector: 'app-enum-control', standalone: true })
export class EnumControlComponent {
  @Input({ required: true }) value!: Extract<AccessoryValueDto, { valueKind: 'enum' }>;
  @Output() valueChange = new EventEmitter<AccessoryValueUpdateDto>();
  select(optionId: string): void { this.valueChange.emit({ valueKind: 'enum', selectedId: optionId }); }
}
```

Composant "item" (rôle de controller MVC) :
```typescript
onValueChange(update: AccessoryValueUpdateDto): void {
  this.rwnBridge.setAccessoryValue(this.accessory.id, update);
}
```

## Configuration serveur de dev

```bash
ng serve --host 0.0.0.0   # écoute sur toutes les IP, pas juste localhost
```
`--disable-host-check` disponible si accès réseau bloqué par la protection anti-DNS-rebinding d'Angular CLI — déconseillé en usage prolongé/exposé, acceptable en test ponctuel sur LAN de confiance.

## Génération initiale du projet

```bash
npx @angular/cli@latest new frontend --directory=frontend --skip-install --skip-tests
```
