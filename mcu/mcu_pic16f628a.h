/*
 * mcu_pic16f628a.h : PIC16F628A MCU-specific definitions (nothing specific to the decoder)
 *
 * MCU layer: clock, resulting timings, pins imposed by the silicon.
 * If the decoder changes pins, it updates HardwareProfile.h, NOT this file.
 * Another MCU -> a new mcu_xxx.h, included by the relevant HardwareProfile.h.
 */
#ifndef MCU_PIC16F628A_H
#define MCU_PIC16F628A_H

#include <xc.h>
#include <stdint.h>

/* ---- clock: INTOSC 4 MHz, 1 instruction cycle = 1 us -------------------------------- */
#define _XTAL_FREQ  4000000UL
#if _XTAL_FREQ != 4000000UL
#error "DCC / servo / tick timings are calculated for 1 us per cycle (4 MHz)"
#endif

/* ---- DCC generator: Timer2, period = PR2 + 1 us ------------------------------------- */
#define DCC_PR_ONE    57      /* half-bit '1' = 58 us */
#define DCC_PR_ZERO   99      /* half-bit '0' = 100 us */

/* ---- time base: free Timer1 at 1 us/step, 1024 us step (0..63) ---------------------- */
#define TICK_NOW()    ((uint8_t)(TMR1H >> 2))

/* ---- hardware USART: pins IMPOSED by the MCU (must stay as inputs) ---------------- */
#define MCU_USART_RX_TRIS   TRISB1    /* RB1 = RX */
#define MCU_USART_TX_TRIS   TRISB2    /* RB2 = TX */

#pragma config FOSC = INTOSCIO   // oscillateur interne 4 MHz, RA6/RA7 en E/S
#pragma config WDTE = OFF        // chien de garde coupé (sinon le PIC redémarre)
#pragma config PWRTE = ON
#pragma config MCLRE = OFF       // RA5 en entrée simple, comme sur ta carte
#pragma config BOREN = ON
#pragma config LVP = OFF         // RB4 reste une E/S
#pragma config CPD = OFF
#pragma config CP = OFF

#endif /* MCU_PIC16F628A_H */