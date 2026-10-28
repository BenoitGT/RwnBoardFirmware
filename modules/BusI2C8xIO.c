#include "BusI2C8xIO.h"
#include <xc.h>

#if !defined(BUSI2C8XIO_SDA_TRIS) || !defined(BUSI2C8XIO_SCL_TRIS) || !defined(BUSI2C8XIO_DIR_TRIS) || \
    !defined(BUSI2C8XIO_SDA_PIN) || !defined(BUSI2C8XIO_PORT) || !defined(BUSI2C8XIO_PORT_MASK)
#error "HardwareProfile.h must define BUSI2C8XIO_SDA_TRIS, _SCL_TRIS, _DIR_TRIS, _SDA_PIN, _PORT and _PORT_MASK"
#endif
#ifndef _XTAL_FREQ
#error "HardwareProfile.h must define _XTAL_FREQ (needed by __delay_us)"
#endif

unsigned char busI2C8xIO_DataOut;
unsigned char busI2C8xIO_DataIn;

/* ---- open drain lines: TRIS input = released (pull-up), TRIS output = low (latch is 0) ------ */
#define SDA_LOW()       (BUSI2C8XIO_SDA_TRIS = 0)
#define SDA_REL()       (BUSI2C8XIO_SDA_TRIS = 1)
#define SCL_LOW()       (BUSI2C8XIO_SCL_TRIS = 0)
#define SCL_REL()       (BUSI2C8XIO_SCL_TRIS = 1)
#define DIR_LOW()       (BUSI2C8XIO_DIR_TRIS = 0)   /* DIR = L : inputs on the bus, outputs locked */
#define DIR_REL()       (BUSI2C8XIO_DIR_TRIS = 1)   /* DIR = H : PCF drives the bus, outputs follow */
#define BIT_DELAY()     __delay_us(BUSI2C8XIO_DELAY_US)

/* ---- state of the cycle (main side only) ---------------------------------------------------- */
#define ST_OUT      0       /* DIR = H : normal state */
#define ST_READ     1       /* DIR = L, PCF released : read the bus */
#define ST_RESTORE  2       /* DIR = L : write DataOut, then DIR = H */

static unsigned char state;
static unsigned char ticks;     /* Timer0 overflows left before the next read cycle */
static unsigned char lastOut;   /* last value written in the PCF */
static unsigned char fail;      /* a transaction got no ACK: wait for the next tick */
static unsigned char i2cOk;     /* result of the last transaction: 1 = every byte acknowledged */

/*
 * One complete I2C transaction with the PCF8574, a single leaf function (one stack level).
 *  rw = 0 : START, address + W, v, STOP
 *  rw = 1 : START, address + R, one byte read (answered with a NACK), STOP ; returns that byte
 * i2cOk tells whether the PCF acknowledged. Nothing waits for the bus: no ACK = give up.
 */
static unsigned char i2cXfer(unsigned char rw, unsigned char v){
    unsigned char b, i, n, r;

    i2cOk = 1;
    r = 0;

    SDA_REL();                          /* START: SDA falls while SCL is high */
    SCL_REL();
    BIT_DELAY();
    SDA_LOW();
    BIT_DELAY();
    SCL_LOW();

    b = (unsigned char)((BUSI2C8XIO_ADDRESS << 1) | rw);
    n = rw ? 1 : 2;                     /* bytes to send: address, then data for a write */
    for(;;){
        for(i = 0; i < 8; i++){         /* MSB first, SDA changes while SCL is low */
            if(b & 0x80){
                SDA_REL();
            } else {
                SDA_LOW();
            }
            b <<= 1;
            BIT_DELAY();
            SCL_REL();
            BIT_DELAY();
            SCL_LOW();
        }
        SDA_REL();                      /* ninth clock: the PCF pulls SDA low to acknowledge */
        BIT_DELAY();
        SCL_REL();
        BIT_DELAY();
        if(BUSI2C8XIO_SDA_PIN){
            i2cOk = 0;
        }
        SCL_LOW();
        n--;
        if(!i2cOk || n == 0){
            break;
        }
        b = v;
    }

    if(rw && i2cOk){
        for(i = 0; i < 8; i++){         /* SDA released, the PCF drives the bits (P7 first) */
            BIT_DELAY();
            SCL_REL();
            BIT_DELAY();
            r <<= 1;
            if(BUSI2C8XIO_SDA_PIN){
                r |= 1;
            }
            SCL_LOW();
        }
        BIT_DELAY();                    /* NACK: SDA stays released during the ninth clock */
        SCL_REL();
        BIT_DELAY();
        SCL_LOW();
    }

    SDA_LOW();                          /* STOP: SDA rises while SCL is high */
    BIT_DELAY();
    SCL_REL();
    BIT_DELAY();
    SDA_REL();
    BIT_DELAY();
    return r;
}

void busI2C8xIO_Init(void){
    BUSI2C8XIO_PORT &= (unsigned char)~BUSI2C8XIO_PORT_MASK;    /* latches of SDA, SCL, DIR at 0: one single write */
    SDA_REL();
    SCL_REL();
    DIR_REL();

    busI2C8xIO_DataOut = 0;             /* safe state: every output inactive */
    busI2C8xIO_DataIn = 0;
    state = ST_OUT;
    fail = 0;
    ticks = BUSI2C8XIO_POLL_TICKS;

    i2cXfer(0, 0);                      /* the PCF powers up at 0xFF: put it to the safe state at once */
    lastOut = i2cOk ? 0 : 0xFF;         /* no answer: Task retries (0xFF differs from DataOut = 0) */
}

void busI2C8xIO_Task(void){
    unsigned char v;

    if(T0IF){                           /* Timer0 overflow: 8.192 ms time base */
        T0IF = 0;
        fail = 0;
        if(ticks != 0){
            ticks--;
        }
    }
    if(fail){
        return;                         /* last transaction failed: wait for the next tick */
    }

    switch(state){
    case ST_READ:
        v = i2cXfer(1, 0);
        if(i2cOk){
            busI2C8xIO_DataIn = v;
        } else {
            fail = 1;
        }
        state = ST_RESTORE;             /* always go back: never leave the PCF released */
        break;

    case ST_RESTORE:
        v = busI2C8xIO_DataOut;         /* the latest value, it may have changed during the cycle */
        i2cXfer(0, v);
        if(i2cOk){
            lastOut = v;
            DIR_REL();                  /* PCF drives the bus first, then the outputs follow it */
            state = ST_OUT;
            ticks = BUSI2C8XIO_POLL_TICKS;
        } else {
            fail = 1;                   /* DIR stays L: outputs stay locked */
        }
        break;

    default:                            /* ST_OUT */
        if(busI2C8xIO_DataOut != lastOut){
            v = busI2C8xIO_DataOut;
            i2cXfer(0, v);
            if(i2cOk){
                lastOut = v;
            } else {
                fail = 1;
            }
        } else if(ticks == 0){
            DIR_LOW();                  /* outputs locked, input 573 on the bus */
            i2cXfer(0, 0xFF);           /* the PCF releases the bus */
            state = ST_READ;
            if(!i2cOk){
                fail = 1;
            }
        }
        break;
    }
}
