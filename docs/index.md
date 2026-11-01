---
tags:
  - sdd
  - moc
  - dcc-control-app
banner: "[[microchip_banner.png]]"
banner_y: 70
---

# Sub-Systems Design Document 

> [!info] About
> Structured sumary of sevral topics arounds **Railway Network Firmware Boards**  project , a Microchip MCU based project.  Boards and firmware ares based on a moduluard conception for reuse purpose.
>
> Firmware parts have been reviewed whith *Claude* (Anthropic) for code optitimisation approche. Exchanges and choices validated by this IA help are notice in theses notes.   

# Notes Map 

### Fondations
- [01-Project-Vision](firmware/01-Project-Vision.md) — Objectives, Perimeter, constraints limitations
- [02-Modular-Architecture](firmware/02-Modular-Architecture.md) —  Repo and code organisation
- [03-Conventions-Rules](firmware/03-Conventions-Rules.md) — Naming , use of #error ,stack use 

### Project Domain
-  [04-Modules](firmware/04-Modules.md)— Hardware block driver, functionals modules

### Interfaces
-  [05-Serial-Communication](firmware/05-Serial-Communication.md) — Railway Network side interfaces, development facilities
-  [06-Sharing-Buffer-Process](firmware/06-Sharing-Buffer-Process.md) — Dispatching instructions

### Hardware
- [SignalBoard](hardware/boards/SignalBoard.md)
- [StarterBoard](hardware/boards/StarterBoard.md)
### Transverses
- [07-Decisions -Journal-Logs](firmware/07-Decisions%20-Journal-Logs.md) 

## Overview

