---
banner: "[[electronic_banner.png]]"
banner_y: 100
---
# 1. Identity
The MxSerov block drives up to 4 Servo motors (basic version) using a multiplexed bus, the current revison is 2.0 , wich is a stable design version. The designis provided by :
- rwnServoMxBlock.kicad_sch
- rwnServoMxBlock.kicad_pcb

# 2. Overview
Schéma bloc simplifié (entrées, sorties, alimentations) et place du bloc dans le système (relié à quel autre bloc).


**3. Interface électrique (« API électronique »)**  
Un tableau des broches de la feuille, avec une ligne par étiquette hiérarchique :

| Broche | Signal   | Dir. | Type      | Niveau/plage | Courant max | Côté MCU | Remarques  |
| ------ | -------- | ---- | --------- | ------------ | ----------- | -------- | ---------- |
| 1      | VCC_5V   | in   | alim      | 5 V ±5 %     | 1.2 A       | —        |            |
| 2      | RS485_RX | out  | numérique | 0/5 V        | —           | RB1      |            |
| 3      | RS485_DE | in   | numérique | 0/5 V        | —           | RB0      | actif haut |

Ajouter ici les connecteurs physiques (repère, brochage, sens d’enfichage) et les adresses de bus (par exemple I2C).

**4. Alimentation et consommation**  
Rails utilisés, tension, consommation typique et maximale, protections (diode, fusible), découplage.

**5. Caractéristiques de fonctionnement**  
Protocole et débit (par exemple RS485 à 19200 bauds), temporisations critiques, seuils, limites.

**6. Schéma et composants clés**  
Capture de la feuille, liste des composants principaux avec valeurs et rôles, calculs importants (résistances de polarisation, filtres).

**7. Implantation PCB**

![[rwnServoMxBlock_v2.0.png]]
- Dimensions : 18x76mm
- Connectors :
	-  Power on the left (2 pins)
	-  Mx Bus and cmd at the bottom (3 pins)

**8. Lien avec le firmware**  
Macros de `HardwareProfile.h` utilisées (par exemple `RWNBUS_DE`), capacités DCC activées, fonctions de rappel (« hooks ») à fournir, ressources du MCU consommées (broches, timer, RAM).

**9. Mise en service et tests**  
Mesures à faire, valeurs attendues, procédure de vérification, pannes classiques.

**10. Historique et notes**  

- 2.0 fix pull down reqiered behind invertor (RN2) to provide 5V on servo command pins