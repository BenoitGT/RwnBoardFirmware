/*
 * HardwareProfile.h : hardware profile for the "multifunction PIC16F628A" decoder
 *
 * THE ONLY file to adapt when the decoder changes (different pins, different functions).
 * The modules (isr, rs485, ...) only use the logical names defined here and stop with
 * #error if one of them is missing: no silent compilation on the wrong pin.
 *
 *   modules (dcc / rs485 / servo)  -> logical names (RS485_DE, DCC_MASK, SERVO_PIN ...)
 *   HardwareProfile.h  (decoder)  -> pins, available functions, throughput
 *   mcu_pic16f628a.h   (MCU)      -> clock, DCC timings, required USART pins
 *
 * Wiring of this decoder:
 *   [TBD]  = DCC Interface 
 *   [TDB]  = 9 Pins Signal Interface
 *   [TBD] = SLI          
 *   RA5 = reset
 *
 * Required configuration bits: INTOSCIO, LVP=OFF, MCLRE=ON.  CMCON = 0x07.
 * 
 *                                  ________
 *                                  | Sig  |
 *                                  |  A   |
 *                                  |  S   |      
 *  ____       _______________      |  VL  |
 * |SLI ?? ---|  PIC16F628A   |     |  M   |
 * |____?? ---|_______________|-x8->| C/Cv |
 *   ________    ///  _|_           | R30  |
 *  |DCC    ?? -///  |RA5|          | R60  |
 *  |Bus IF ?? -//   |RST|          |  EO  |
 *  |_______`|       |___|     Vcc--|_Comn_|
 *                                
 */
#ifndef HARDWARE_PROFILE_H
#define HARDWARE_PROFILE_H

#include "mcu/mcu_pic16f628a.h"

/* ---- functions available on this decoder (1 = compiled) --------------------------- */


/* ---- SLI ---------------------------------------------------------------------- */


/* ---- RWNBUS ---------------------------------------------------------------------- */
#define RWNBUS_DE        RB0       /* R/W: 1 = transmit */
#define RWNBUS_DE_TRIS   TRISB0
#define RWNBUS_SPBRG     12        /* 19200 baud @ 4 MHz, BRGH = 1 (error 0.2%) */
#define RX_SIZE         16        /* RX ring (power of 2) */

/* ---- DCC booster: two complementary outputs on the same port ---------------------- */
#define DCC_PORT        PORTB
#define DCC_TRIS        TRISB
#define DCC_MASK        0x30      /* RB4 | RB5 */
#define DCC_LEVEL0      0x10      /* initial state: RB4 = 1, RB5 = 0 */



/* ---- servo: one pulse signal + 2 address bits to a demultiplexer ------------------ */


#endif /* HARDWARE_PROFILE_H */