/*
 * isr.h : ISR <-> main interface
 *
 * No physical pins here: everything comes from HardwareProfile.h (logical names).
 *
 * Principles
 *  - ONE single interrupt vector (isr.c), WITHOUT function calls: only one stack level.
 *  - Real time is handled in the ISR (DCC generator, servo pulse end). The rest is polled
 *    by main. RS485 reception is polled INSIDE the DCC ISR (no dedicated RX interrupt:
 *    it would make DCC edges jitter).
 *
 * Resource split (MCU layer, see isr_init)
 *  Timer2: DCC half-bit     Timer0: servo pulse end (1:8 = 8 us/step)
 *  Timer1: free time base without interruption (TICK_NOW(), see mcu_pic16f628a.h)
 */
#ifndef ISR_H
#define ISR_H


/* ---- API ------------------------------------------------------------------------ */
void isr_init(void);          /* I/O, timers, USART, then GIE = 1 */


#endif /* ISR_H */