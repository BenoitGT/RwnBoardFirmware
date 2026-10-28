---
banner: "[[electronic_banner.png]]"
banner_y: 100.0%
---
# 1. Identity
The Powered RS485 block drives up to 4 Servo motors (basic version) using a multiplexed bus, the current revison is 2.0 , wich is a stable design version. The designis provided by :
- rwnServoMxBlock.kicad_sch
- rwnServoMxBlock.kicad_pcb

# 2. Overview
![[rwnResetBt.png]]
==🔴*All my schematics are NOT professional skills based. I not responsable for electronics issues*==
Schéma bloc simplifié (entrées, sorties, alimentations) et place du bloc dans le système (relié à quel autre bloc).


# 3. Electrical interface (Block API)
This is the block Pins API table with hierachical signal label 

| Broche | Signal | Dir. | Type      | Niveau/plage | Courant max | Remarques  |
| ------ | ------ | ---- | --------- | ------------ | ----------- | ---------- |
| 1      | VCC_5V | out  | alim      | 5 V ±5 %     | 1.2 A       |            |
| 2      | RX     | out  | numérique | 0/5 V        | —           |            |
| 3      | TX     | in   | numérique | 0/5 V        | —           |            |
| 4      | RWRS   | in   | numérique | 0/5 V        | —           | actif haut |
|        |        |      |           |              |             |            |


# 4. Required supply
Rails utilisés, tension, consommation typique et maximale, protections (diode, fusible), découplage.

# 5. Fonctional specification

- **C1** : Helps to prevent bouncing effect.


# 6. Schéma et composants clés
Capture de la feuille, liste des composants principaux avec valeurs et rôles, calculs importants (résistances de polarisation, filtres).

# 7. PCB Implantaion

![[rwnResetBt.png]]
- Dimensions : 15x15mm
- Connectors :
	-   2 pins to drives up to 3 LEDs . the GND is provided by PCB bottom surface
	

# 8. Firmware integration
The "" has to be define in`HardwareProfile.h` to link the RWRS(DE) Pin. RC and TX are automatically associated to `usart` module of the MCU.


# 9. Setting to Work and Tests
Mesures à faire, valeurs attendues, procédure de vérification, pannes classiques.

# 10. Changes history  

- 2.0 - 2026-10 : fix supply leak, change LM78LS05 for LM7805 to support up to 1.0 A
- 1.0  - 2018-01 : Initial version