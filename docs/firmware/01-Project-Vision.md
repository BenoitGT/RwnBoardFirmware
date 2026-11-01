---
tags: [sdd, dcc-control-app]
banner: "[[microchip_banner.png]]"
banner_y: 70
---

# Project vision

## Context

- Hobby electronics project to improve personal electronics skill :   a **modular multifunction / accessory DCC decoder** for a model railway network (RwnBoard / RailWay Network).
- Target: **PIC16F628A**, 4 MHz internal oscillator, **XC8 v3.10**, MPLAB extensions for VS Code on Linux, programmed with a **PICkit 2** through `pk2cmd` (the VS Code extension does not drive PICkit 2).
- The command station talks to the decoders over **RS485 half-duplex, 19200 baud**, using **RwnBus**: a framing layer that carries DCC packets.
- Hard limits: 2 K-word flash (last report: 1525 used), 224 B RAM (COMMON RAM 16/16 full), **8-level hardware stack** (the ISR uses one), no on-chip debug hardware, no MSSP (I2C is bit-banged).

## Objectives

- Split **transport/framing** (RwnBus) from **DCC packet processing** (DccDecoder) so the DCC code can be reused with the historical signal source (58/100 µs).
- Select capabilities **at compile time** (multifunction vs accessory), with decoded parameters passed to hooks.
- Keep the **call depth and RAM** minimal; measure them with a map/stack report.

- Add an **8-in / 8-out I/O expansion** (PCF8574 + 74HC573) for zone activation (via DCC function group 2) and presence feedback (via CV21/CV22).

## Stack

- **Hardware** :  Microchip based boards (Arduino chip compatibility is possible). Boards are designed with block group approche.
- **Firmware**: Follow the modular block design with sevrals block driver code 
- **Transport & Protocols** : This project provide serial et DCC Interfaces to manage NMRA protocol or adaptation (serial)
- **Tools** : VSCode (with Microchip plugins set), Linux environment, Kid software, and obsidian/zensical for documentation

## Engineering process

All feature will not be delivered in once time. Hardware blocks will be integrated one by one. frimaware and external control application follow this sprint engineering model .

## Scope limitation

- TrackPower rework (DCC regeneration/booster), main-loop orchestration (analog-mode queue drain, ACK after drain), ACK frame format.
- CV list (NMRA), “write byte needs two identical packets” rule, verification of the extended accessory 11-bit address mapping.
- Servo pulse stop, DCC hooks for servos / EEPROM, SignalDecoder configuration (shares StarterBoard’s properties).
- Replacing `bit` flags by the author’s `rwnBusInfo` bitfield struct (author’s task).
- I2C error recovery (stuck bus), PC unit-test folder `tests/`, pinned CI workflow and `requirements.txt`.
- Hardware safety points (section 7).
- _[open]_ Running the MPLAB simulator debug session: unresolved.

## long term Vision 

-  Servo pulse : A prefixed cycles number will manage auto stop. a periode modulation (`SPEED_SCALE`) should adjust servo speed motion.
