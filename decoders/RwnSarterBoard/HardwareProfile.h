/*
 * HardwareProfile.h : hardware profile for the "multifunction PIC16F628A" decoder
 *
 * THE ONLY file to adapt when the decoder changes (different pins, different functions).
 * The modules (isr, rs485, ...) only use the logical names defined here and stop with
 * #error if one of them is missing: no silent compilation on the wrong pin.
 *
 *   modules (dcc / rs485 / servo)  -> logical names (RWNBUS_DE, DCC_MASK, SERVO_PIN ...)
 *   HardwareProfile.h  (decoder)  -> pins, available functions, throughput
 *   mcu_pic16f628a.h   (MCU)      -> clock, DCC timings, required USART pins
 *
 * Wiring of this decoder:
 *   RB0 = RWNBUS R/W        RB1 = RX (USART)      RB2 = TX (USART)
 *   RB4/RB5 = DCC booster (complementary)      RB7 = servo signal
 *   RA6/RA7 = servo address (74xx138/238)     RA2/RA3/RA4 = I2C (PCF8574)
 *   RA0/RA1 = SLI           RA5 = reset
 *
 * Required configuration bits: INTOSCIO, LVP=OFF, MCLRE=ON.  CMCON = 0x07.
 * 
 * __________                    ________
 * |Servo  RB7--\           /-RB5  Track |
 * |       RA6--\\         //-RB4  Power |
 * |_______RA7 -\\\       //     |_______|
 *  ____       __\\\_____//___
 * |SLI RA0---|  PIC16F628A   |
 * |____RA1---|_______________|
 *   ________    ///  _|_  \\\     _______
 *  |Bus    RA2-///  |RA5|  \\\-RB2 Rwn   |
 *  | I2C   RA3-//   |RST|   \\-RB1 Bus IF|
 *  |_______RA4-/    |___|    \-RB0 ______|
 */
#ifndef HARDWARE_PROFILE_H
#define HARDWARE_PROFILE_H

#include "mcu/mcu_pic16f628a.h"

/* ---- functions available on this decoder (1 = compiled) --------------------------- */
#define FEAT_RWNBUS_RX   1
//#define FEAT_RWNBUS_TX 1
#define FEAT_DCC_GEN    1
#define FEAT_MX_SERVO    1

/* ---- SLI---------------------------------------------------------------------- */
#define SLI_STATUS0        RA0 /* pin 1 */
#define SLI_STATUS0_TRIS   TRISA0
#define SLI_STATUS1        RA1 /* pin 2*/
#define SLI_STATUS1_TRIS   TRISA1

/* ---- RESEt BT---------------------------------------------------------------------- */
#define RESET_BT          RA5
#define RESET_BT_TRIS    TRISA5

/* ---- RWNBUS ---------------------------------------------------------------------- */
#define RWNBUS_DE        RB0       /* R/W: 1 = transmit */
#define RWNBUS_DE_TRIS   TRISB0
#define RWNBUS_SPBRG     12        /* 19200 baud @ 4 MHz , BRGH = 1 (error 0.2%) */
//#define RWNBUS_SPBRG     25        /* 9600 baud @ 4 MHz, BRGH = 1 (error 0.2%) */
#define RX_SIZE         16        /* RX ring (power of 2) */

/* ---- TRACk_POWER booster: two complementary outputs on the same port ---------------------- */
#define DCC_PORT        PORTB
#define DCC_TRIS        TRISB
#define DCC_MASK        0x30      /* RB4 | RB5 */
#define DCC_LEVEL0      0x10      /* initial state: RB4 = 1, RB5 = 0 */

#define MAX_CV 6
#define CV1_IDX 0
#define CV29_IDX 1
#define CV_SERVO_IDX 2
#define CV7_VALUE 0x01 //Manufacturer Version No.
#define CV8_VALUE 0x13 //Manufactured ID = Public Custom

extern unsigned char CVCcache[MAX_CV];
#define DCC_DECODER_ADDR CVCcache[CV1_IDX]
#define DCC_DECODER_SERVO_CV(idx) CVCcache[CV_SERVO_IDX + idx] 

/* ---- PHAL API definition */
extern void hal_init(void);
extern uint8_t sh_porta;


/* ---- MX_SERVO : one pulse signal + 2 address bits to a demultiplexer ------------------ */
#define MX_SERVO_PIN       RB7
#define MX_SERVO_PIN_TRIS  TRISB7
#define MX_SERVO_SEL_MASK  0xC0      /* RA7:RA6 */
#define MX_SERVO_SEL_SHIFT 6    
#define MX_SERVO_SEL_PORT  PORTA
#define MX_SERVO_SEL_TRIS  TRISA
#define MX_SERVO_SEL_SH_PORT sh_porta
//#define SERVO_SPEED_SCALE 2  //2^2 =4 = servos move duration x4 (4s)



/* ---- DCC CAPABILITIES ------------------ */

#define DCC_MF_FN_GRP1_CAPABILITY
#define DCC_MF_ADV_OPE_CAPABILITY


#endif /* HARDWARE_PROFILE_H */