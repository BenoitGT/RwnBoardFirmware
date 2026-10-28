#ifndef BUSI2C8XIO_H
#define BUSI2C8XIO_H

/*
 * BusI2C8xIO : 8 inputs / 8 outputs through a PCF8574 (I2C, bit-banged) and two 74HC573.
 *
 * Hardware
 *  - PCF8574 port P0..P7 = one 8-bit bus shared by two 74HC573:
 *      output 573 : D <- bus, LE <- DIR, OE <- GND  (DIR = H: transparent, DIR = L: latched)
 *      input  573 : Q -> bus, LE <- Vcc, OE <- DIR  (DIR = L: drives the bus, DIR = H: hi-Z)
 *  - DIR is an OPEN DRAIN pin (RA4 on the PIC16F628A): "H" = pin released, needs an external pull-up.
 *  - SDA / SCL are open drain too: driven only through TRISx (input = released, output with a
 *    latch at 0 = low). Their PORT latch bits must stay at 0 (see "PORT latch" below).
 *  - the PCF8574 powers up with its port at 0xFF.
 *
 * Software model: two cache bytes, written / read by the rest of the program
 *      busI2C8xIO_DataOut : value the outputs must take (bit n -> output n). 0 = safe state.
 *      busI2C8xIO_DataIn  : last value read from the inputs (bit n <- input n).
 *  busI2C8xIO_Task() runs in the main loop and moves the caches to / from the PCF8574, with at
 *  most ONE I2C transaction per call (about 0.3 ms at 4 MHz, see BUSI2C8XIO_DELAY_US).
 *
 * Normal state: DIR = H, the PCF drives the bus, the outputs follow DataOut (written as soon as
 * it changes). Every BUSI2C8XIO_POLL_TICKS Timer0 overflows (4 x 8.192 ms = 32.8 ms), a read cycle:
 *      1. DIR = L  (outputs locked, input 573 drives the bus) + write 0xFF (the PCF releases the bus)
 *      2. read the bus -> DataIn
 *      3. write DataOut (the PCF drives the bus again), then DIR = H
 *  A DataOut change during a read cycle is taken into account at step 3. The outputs never see
 *  the released (0xFF) bus: they are locked while the PCF is released.
 *
 * Errors: no ACK -> the transaction is abandoned (no wait loop) and retried at the next Timer0
 *  tick; the data are simply up to date one pass later. If the PCF stops answering in the middle
 *  of a read cycle, DIR stays at L (outputs locked at their last value) until it answers again.
 *
 * Call order: busI2C8xIO_Init() once at start (it writes DataOut = 0 immediately), then
 *  busI2C8xIO_Task() in every main loop pass. Needs Timer0 running (set by rwnBus_Init()).
 *  Everything runs in the main context: the interrupt must not use these variables or the
 *  SDA / SCL / DIR pins.
 *
 * Stack: main -> busI2C8xIO_Task -> one leaf function = 2 levels.
 *
 * PORT latch: this module never writes the PORT bits of SDA, SCL and DIR except once in Init,
 *  where it clears them. If the main program writes the whole port (shadow register), the bits
 *  of these three pins must be 0 in the shadow, otherwise the pin would be driven high.
 *
 * ---- to define in HardwareProfile.h (example for RA2 = SDA, RA3 = SCL, RA4 = DIR) ------------
 *      #define _XTAL_FREQ                 4000000
 *      #define BUSI2C8XIO_SDA_TRIS        TRISA2
 *      #define BUSI2C8XIO_SCL_TRIS        TRISA3
 *      #define BUSI2C8XIO_DIR_TRIS        TRISA4
 *      #define BUSI2C8XIO_SDA_PIN         RA2          // read of the SDA pin
 *      #define BUSI2C8XIO_PORT            PORTA        // port holding the three pins
 *      #define BUSI2C8XIO_PORT_MASK       0x1C         // their bits in that port
 *  ---- optional (default values) ------------------------------------------------------------
 *      BUSI2C8XIO_ADDRESS       0x20   7-bit I2C address of the PCF8574 (A0 = A1 = A2 = GND)
 *      BUSI2C8XIO_POLL_TICKS    4      Timer0 overflows (8.192 ms each) between two reads; 24 = 200 ms
 *      BUSI2C8XIO_DELAY_US      4      delay between two line changes, in us (about 60 kHz at 4 us, not measured; 100 kHz max)
 */

#include "HardwareProfile.h"

#ifndef BUSI2C8XIO_ADDRESS
#define BUSI2C8XIO_ADDRESS     0x20
#endif
#ifndef BUSI2C8XIO_POLL_TICKS
#define BUSI2C8XIO_POLL_TICKS  4
#endif
#ifndef BUSI2C8XIO_DELAY_US
#define BUSI2C8XIO_DELAY_US    4
#endif

/* ---- module data ------------------------------------------------------------------- */
extern unsigned char busI2C8xIO_DataOut;    /* wanted outputs (written by the program), 0 at start */
extern unsigned char busI2C8xIO_DataIn;     /* last inputs read (read by the program) */

/* ---- BusI2C8xIO module interface -------------------------------------------------------- */
void busI2C8xIO_Init(void);     /* pins released, DataOut = 0 written to the PCF at once */
void busI2C8xIO_Task(void);     /* every main loop pass: at most one I2C transaction */

#endif // BUSI2C8XIO_H
