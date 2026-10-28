# Rapport firmware : 16F628A

Source : `mem.map` + `StarterBoard.hex`

## 1. Occupation

| Zone | Utilise | Capacite | Libre | Taux |
|---|---:|---:|---:|---:|
| Flash (mots de 14 bits) | 634 | 2048 | 1414 | 31.0 % |
| RAM COMMON | 16 | 16 | 0 | 100.0 % |
| RAM BANK0 | 71 | 80 | 9 | 88.8 % |
| RAM BANK1 | 0 | 80 | 80 | 0.0 % |
| RAM BANK2 | 0 | 48 | 48 | 0.0 % |
| **RAM totale (octets)** | 87 | 224 | 137 | 38.8 % |

> **Attention** : COMMON est pleine (16/16 octets).

## 2. Flash par module et par fonction

| Module | Fonctions | Mots |
|---|---:|---:|
| RwnBus.c | 3 | 203 |
| MultiplexedServo.c | 4 | 156 |
| DccDecoder.c | 2 | 102 |
| main.c | 3 | 88 |
| SystemLEDIndicator.c | 2 | 37 |
| isr.c | 2 | 14 |
| shared (runtime) | 1 | 9 |
| hal.c | 1 | 5 |
| startup (reset, interruption, init, effacement RAM) |  | 20 |

```mermaid
pie showData title Flash par module (mots)
    "RwnBus.c" : 203
    "MultiplexedServo.c" : 156
    "DccDecoder.c" : 102
    "main.c" : 88
    "SystemLEDIndicator.c" : 37
    "isr.c" : 14
    "shared" : 9
    "hal.c" : 5
    "startup" : 20
```

Les 12 plus grosses fonctions :

| Fonction | Module | Adresse | Mots |
|---|---|---:|---:|
| _multiplexedServo_taskInterrupt | MultiplexedServo.c | 0x018 | 103 |
| _dccDecoder_Task | DccDecoder.c | 0x07F | 96 |
| _rwnBus_receive_taskInterrupt | RwnBus.c | 0x0DF | 91 |
| _rwnBus_task | RwnBus.c | 0x13A | 71 |
| _dccDecoder_FnGrp1 | main.c | 0x181 | 60 |
| _rwnBus_init | RwnBus.c | 0x1BD | 41 |
| _systemLEDIndicator_setState | SystemLEDIndicator.c | 0x1E6 | 32 |
| _main | main.c | 0x206 | 27 |
| _multiplexedServo_setPosition | MultiplexedServo.c | 0x236 | 21 |
| _multiplexedServo_init | MultiplexedServo.c | 0x221 | 21 |
| _isr | isr.c | 0x24B | 11 |
| _set_timer1_us | MultiplexedServo.c | 0x256 | 11 |

Flash libre : 0x27D-0x7FF (1411 mots)

## 3. RAM : variables globales

| Variable | Zone | Adresse | Octets |
|---|---|---:|---:|
| rwnPacketData | bssBANK0 | 0x044 | 16 |
| txBuf | bssBANK0 | 0x026 | 12 |
| servoPulseSteps | bssBANK0 | 0x032 | 4 |
| trackPowerState | bssBANK0 | 0x036 | 3 |
| currentServo | bssBANK0 | 0x040 | 2 |
| txIdx | bssBANK0 | 0x039 | 1 |
| txLen | bssBANK0 | 0x03A | 1 |
| PacketStartCount | bssBANK0 | 0x03B | 1 |
| BuffTail | bssBANK0 | 0x03C | 1 |
| BuffHead | bssBANK0 | 0x03D | 1 |
| RemainPeriods | bssBANK0 | 0x03E | 1 |
| isPulse | bssBANK0 | 0x03F | 1 |
| RwnBus_RxPacketLength | bssBANK0 | 0x042 | 1 |
| sh_porta | bssBANK0 | 0x043 | 1 |
| rxLastTime | nvBANK0 | 0x063 | 1 |
| pRwnBus_RxPacket | nvBANK0 | 0x064 | 1 |
| dccDecoder_Address | nvBANK0 | 0x065 | 1 |

Taille deduite de l'ecart entre symboles consecutifs (un remplissage eventuel est compte dans la variable qui precede).

Variables d'un bit : `RwnBus_FERR` (0x07D bit 5), `RwnBus_RRDY` (0x07D bit 6), `RwnBus_TRMT` (0x07D bit 2), `dccDecoder_ResetMode` (0x07D bit 7)

Variables locales (pile compilee, partagees entre fonctions qui ne s'appellent pas) : cstackCOMMON = 13 octets, cstackBANK0 = 15 octets.

## 4. Graphe d'appels et pile materielle

| Contexte | Niveaux de pile | Chemin le plus profond |
|---|---:|---|
| main (boucle principale) | 3 | main -> dccDecoder_Task -> dccDecoder_FnGrp1 -> systemLEDIndicator_setState |
| interruption (1 niveau pris par l'interruption + appels) | 3 | interruption -> isr -> multiplexedServo_taskInterrupt -> set_timer1_us |
| demarrage (avant la boucle, interruptions coupees) | 3 | reset_vec -> end_init -> cinit -> main -> dccDecoder_Task -> dccDecoder_FnGrp1 -> systemLEDIndicator_setState |

**Pire cas = main (3) + interruption (3) = 6 / 8 niveaux : OK** (l'interruption peut survenir au point le plus profond de main).

```mermaid
flowchart LR
    interrupt_entry["interrupt_entry<br/>0x004 · 8 mots"]
    _multiplexedServo_taskInterrupt["multiplexedServo_taskInterrupt<br/>0x018 · 103 mots"]
    _dccDecoder_Task["dccDecoder_Task<br/>0x07F · 96 mots"]
    _rwnBus_receive_taskInterrupt["rwnBus_receive_taskInterrupt<br/>0x0DF · 91 mots"]
    _rwnBus_task["rwnBus_task<br/>0x13A · 71 mots"]
    _dccDecoder_FnGrp1["dccDecoder_FnGrp1<br/>0x181 · 60 mots"]
    _rwnBus_init["rwnBus_init<br/>0x1BD · 41 mots"]
    _systemLEDIndicator_setState["systemLEDIndicator_setState<br/>0x1E6 · 32 mots"]
    _main["main<br/>0x206 · 27 mots"]
    _multiplexedServo_init["multiplexedServo_init<br/>0x221 · 21 mots"]
    _multiplexedServo_setPosition["multiplexedServo_setPosition<br/>0x236 · 21 mots"]
    _isr["isr<br/>0x24B · 11 mots"]
    _set_timer1_us["set_timer1_us<br/>0x256 · 11 mots"]
    _dccDecoder_Init["dccDecoder_Init<br/>0x269 · 6 mots"]
    _systemLEDIndicator__init["systemLEDIndicator__init<br/>0x26F · 5 mots"]
    _hal_init["hal_init<br/>0x274 · 5 mots"]
    _isr_init["isr_init<br/>0x279 · 3 mots"]
    _dccDecoder_Speed["dccDecoder_Speed<br/>0x27C · 1 mots"]
    interrupt_entry -.->|goto| _isr
    _multiplexedServo_taskInterrupt --> _set_timer1_us
    _dccDecoder_Task --> _dccDecoder_FnGrp1
    _dccDecoder_Task --> _dccDecoder_Speed
    _dccDecoder_FnGrp1 --> _systemLEDIndicator_setState
    _dccDecoder_FnGrp1 --> _multiplexedServo_setPosition
    _main --> _hal_init
    _main --> _systemLEDIndicator__init
    _main --> _rwnBus_init
    _main --> _multiplexedServo_init
    _main --> _dccDecoder_Init
    _main --> _isr_init
    _main --> _systemLEDIndicator_setState
    _main --> _rwnBus_task
    _main --> _dccDecoder_Task
    _isr --> _rwnBus_receive_taskInterrupt
    _isr --> _multiplexedServo_taskInterrupt
    classDef isr fill:#fde2e2,stroke:#c0392b
    classDef main fill:#e2edfd,stroke:#2e5fa3
    classDef both fill:#f3e8fd,stroke:#7a3fa3
    class _main main
    class _rwnBus_receive_taskInterrupt isr
    class _multiplexedServo_taskInterrupt isr
    class _dccDecoder_Speed main
    class _rwnBus_task main
    class interrupt_entry isr
    class _multiplexedServo_init main
    class _hal_init main
    class _systemLEDIndicator__init main
    class _rwnBus_init main
    class _isr isr
    class _dccDecoder_Init main
    class _multiplexedServo_setPosition main
    class _dccDecoder_FnGrp1 main
    class _dccDecoder_Task main
    class _systemLEDIndicator_setState main
    class _isr_init main
    class _set_timer1_us isr
```

Bleu : appele depuis `main`. Rouge : appele depuis l'interruption. Violet : les deux. Trait pointille : `goto` (saut de fin de fonction, ne consomme pas de pile).

| Fonction | Niveaux de pile sous elle | Appelle |
|---|---:|---|
| main | 3 | dccDecoder_Init, dccDecoder_Task, hal_init, isr_init, multiplexedServo_init, rwnBus_init, rwnBus_task, systemLEDIndicator__init, systemLEDIndicator_setState |
| interrupt_entry | 2 | isr |
| isr | 2 | multiplexedServo_taskInterrupt, rwnBus_receive_taskInterrupt |
| dccDecoder_Task | 2 | dccDecoder_FnGrp1, dccDecoder_Speed |
| multiplexedServo_taskInterrupt | 1 | set_timer1_us |
| dccDecoder_FnGrp1 | 1 | multiplexedServo_setPosition, systemLEDIndicator_setState |
| rwnBus_receive_taskInterrupt | 0 | - |
| dccDecoder_Speed | 0 | - |
| rwnBus_task | 0 | - |
| multiplexedServo_init | 0 | - |
| hal_init | 0 | - |
| systemLEDIndicator__init | 0 | - |
| rwnBus_init | 0 | - |
| dccDecoder_Init | 0 | - |
| multiplexedServo_setPosition | 0 | - |
| systemLEDIndicator_setState | 0 | - |
| isr_init | 0 | - |
| set_timer1_us | 0 | - |

Methode : les instructions `CALL` et `GOTO` du .hex sont rattachees aux fonctions par leurs adresses (.map). Un `GOTO` vers une autre fonction n'ajoute pas de niveau. Les appels par pointeur ou par table (PCL) ne sont pas suivis.
