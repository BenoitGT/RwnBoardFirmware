---
banner: "[[microchip_banner.png]]"
banner_y: 70
---
# Serial Communication


> [!important] Development tools
> **UART bring-up:** the USB-serial adapter (Prolific PL2303 on DB9) outputs **RS-232 levels** (−8.8 V idle: inverted polarity and over-voltage for the PIC), which explained the 0x00 ↔ 0xFF swap. A MAX202E (0.1 µF capacitors) fixed it; 0x3E reception then worked. On the final RS485 bus, a swapped A/B pair would look the same.