---
tags:
  - sdd
  - dcc-control-app
  - domaine
banner: "[[microchip_banner.png]]"
banner_y: 70.0%
---

# Modules

## Core modules
### QueueBuffer
 _[tested on PC: randomized model, sizes 1..255]_

- `head .. commit .. tail`. `Enqueue` publishes immediately; `EnqueueHold` keeps bytes invisible until `Commit`, or discards them with `Rollback`. Capacity = size − 1.
- API: Init, Enqueue, Dequeue, IsEmpty, IsFull, EnqueueHold, PendingCount, PeekPending, Commit, Rollback.
- Do not mix `Enqueue` and `EnqueueHold` while held bytes exist.


### DccDecoder
_[tested on PC]_

- API: `dccDecoder_Init()`, `dccDecoder_Task(pkt, n)` returning `DCC_RESULT_IGNORED/DONE/ACK`; globals `dccDecoder_Address`, `dccDecoder_ResetMode`.
- Hooks (only for enabled capabilities): `FnGrp1(mask)`, `FnGrp2(firstFn 5|9, mask)`, `FnExpan(firstFn 13|21, mask)`, `Speed(forward, step)` (128 steps), `Accessory(output, activate)`, `AccessoryAspect(head, aspect)`, `CvRead(cv)`, `CvWrite(cv, value)` (CV numbers 1..1024).
- Filtering: own address only; broadcast (0x00 MF, 0x1FF accessory) accepted only for CV access while in reset mode; idle (0xFF) and 14-bit addresses ignored.
- CV long form: verify byte, write byte, bit manipulation (built from the two CV hooks).

## Drivers modules

### ResetBtn
_[tested on PC]_
### SLI 
_[tested on PC]_
### Mx Driver 
_[tested on PC]_


### RwnBus 
_[tested on PC]_

- Data: `RwnPacketQueue`, `rwnBus_Packet[6]`, `rwnBus_PacketLength`, flags `rwnBus_rdy`, `rwnBus_txb`; macros `RWNBUS_FRAME_READY/RELEASE/TX_BUSY`.
- API: `rwnBus_Init()`, `rwnBus_ReceiveTaskInterrupt()`, `rwnBus_Task()`, `rwnBus_Send(p, n)` (n = 2..5, returns 0 if busy or a packet is being received).
- Parameters: queue 16, Timer0 prescaler 1:32 (32 µs/step), silence min 40 (1280 µs), max 160 (5120 µs), `SPBRG = 12` with `BRGH = 1` (19231 baud, 0.16 % error).
- Behaviour: the ISR strips the header and holds payload bytes; it ignores its own transmission and framing errors, and restarts the receiver on overrun. `rwnBus_Task` waits for silence, checks length and XOR, then commits or rolls back. Stop-and-wait: one packet in flight.
### BusI2C8xIO 
_[tested on PC with a PCF8574 + 74HC573 model; not compiled with XC8]_

- Hardware: PCF8574 (address 0x20) drives an 8-bit bus shared by an output 74HC573 (LE ← DIR) and an input 74HC573 (OE ← DIR). DIR (RA4) is open drain: H = PCF drives and outputs follow, L = input latch on the bus and outputs locked.
- Data: `busI2C8xIO_DataOut` (wanted outputs, 0 = safe/inactive), `busI2C8xIO_DataIn` (last read).
- API: `busI2C8xIO_Init()` (writes 0 to the PCF immediately), `busI2C8xIO_Task()` (at most one I2C transaction per call, ~0.3 ms).
- Cycle: writes `DataOut` when it changes; every 4 Timer0 overflows (~33 ms, configurable) it runs a read cycle: DIR = L, write 0xFF, read, write `DataOut`, DIR = H. A `DataOut` change during the cycle is applied at the end. No ACK means retry at the next tick, never a blocking wait.
- Proposed mapping (hooks, not in the module): `DataOut` bit n = F(5+n) via `FnGrp2`; CV21 = state of F5–F8, CV22 = state of F9–F12.

> [!danger] **Hardware safety (I/O block) - IA warnning**
> The PCF8574 powers up at 0xFF, so outputs may be active until `Init` writes 0 (and if the PCF disappears, the port floats high). Possible mitigations to evaluate: a high-value pull-down on the 8 lines. During a read, the PCF and the input latch both drive the bus for ~0.3 ms: series resistors on the input latch outputs (already present on one side), and a possible ~1 µs glitch on outputs when returning to normal.