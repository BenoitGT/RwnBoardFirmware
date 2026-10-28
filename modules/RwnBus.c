#include "RwnBus.h"
//#include <xc.h>
//#include "core/DccDecoder.h"
#include "modules/SystemLEDIndicator.h"


#if !defined (FEAT_RWNBUS_RX ) && !defined (FEAT_RWNBUS_TX)
#error "HardwareProfile.h must at laest define FEAT_RWNBUS_RX  or FEAT_RWNBUS_TX"
#endif

#if !defined(RWNBUS_DE) || !defined(RWNBUS_DE_TRIS) || !defined(RWNBUS_SPBRG)
#error "HardwareProfile.h must define RWNBUS_DE, RWNBUS_DE_TRIS and RWNBUS_SPBRG"
#endif
#if RWNBUS_SILENCE_MIN >= RWNBUS_SILENCE_MAX
#error "RWNBUS_SILENCE_MIN must be lower than RWNBUS_SILENCE_MAX"
#endif
#if RWNBUS_SILENCE_MAX > 255
#error "RWNBUS_SILENCE_MAX must fit the 8-bit Timer0"
#endif



DccPacket_t * pRwnBus_RxPacket;
unsigned char  RwnBus_RxPacketLength;

 __bit RwnBus_FERR;      /*Framing Error*/
__bit RwnBus_OERR;       /*Over Run Error*/
__bit RwnBus_CREN;      /*Continious Receptio ENable*/
__bit RwnBus_RRDY;       /* a valid packet is in rwnBus_Packet */
__bit RwnBus_TRMT;      /* transmission buffer full/empty */
__bit RwnBus_TXEN;



/**Half Duplex Buffer */
static unsigned char rwnPacketData[RWNBUS_BUFFER_SIZE];

/* ---- reception state (ISR side) --------------------------------------------------- */
static unsigned char rxLastTime;        /* TMR0 at the last received byte */

static unsigned char BuffHead;             /* header characters seen: 0..RWN_HEADER_SIZE (= payload) */
static unsigned char BuffTail;  
static unsigned char PacketStartCount;


/* ---- transmission state (main side) ------------------------------------------------ */
static unsigned char txBuf[RWN_PACKETS_MAX_SIZE];   /* payload + xor */
static unsigned char txLen;             /* total bytes to send: header + payload + xor */
static unsigned char txIdx;

void rwnBus_init(void){
    OPTION_REG = (unsigned char)((OPTION_REG & 0xC0) | RWNBUS_TIMER0_OPTION);   /* Timer0 as time base, bits 7:6 untouched */

    SPBRG = RWNBUS_SPBRG;
    BRGH = 1;
    SYNC = 0;                           /* asynchronous */
    SPEN = 1;
    RX9D =0;
#ifdef MCU_USART_RX_TRIS
    MCU_USART_RX_TRIS = 1;              /* pins imposed by the MCU */
#endif
#ifdef MCU_USART_TX_TRIS
    MCU_USART_TX_TRIS = 1;
#endif
   CREN = 1;
#if FEAT_RWNBUS_TX  
    TXEN = 1;
#endif
  RWNBUS_DE = 0;                      /* idle: receive */
    RWNBUS_DE_TRIS = 0;

  
    rxLastTime = TMR0;
    BuffHead = 0;
    BuffTail = 0;
    RwnBus_RRDY= 0;
    RwnBus_TRMT = 0;
    PacketStartCount = 0;
    RwnBus_RxPacketLength=0;
    pRwnBus_RxPacket=(DccPacket_t*)rwnPacketData;
#if RWNBUS_RX_IRQ
    RCIE = 1;                           /* PEIE and GIE are enabled by main */
#endif
}

/* Interrupt side. Stack: vector -> here -> queueBuffer_EnqueueHold = 3 levels. */
void rwnBus_receive_taskInterrupt(void){

    unsigned char b, now;
    

    if(!RCIF){
        return;
    }
    if(OERR){                           /* overrun: flush the FIFO and restart the receiver */
        b = RCREG;
        b = RCREG;
        CREN = 0;                   /* clear overrun  error */
        NOP();
        CREN = 1;                 /* restart the receiver */
        BuffHead = 0;
        return;
    }
    RwnBus_FERR = FERR;                /* FERR belongs to the byte about to be read */
    b = RCREG;

    now = TMR0;

#if FEAT_RWNBUS_TX 
    if(RwnBus_TRMT){                     /* own transmission (or line echo): ignored */
        rxLastTime = now;
        return;
    }
#endif
    if((unsigned char)(now - rxLastTime) >= RWNBUS_SILENCE_MAX){
        BuffHead = BuffTail + DCC_PACKETS_MAX_SIZE;      /* long silence: a new frame starts */
        if (BuffHead >= RWN_PACKETS_MAX_SIZE) /*End of buffer ? */
        { 
            BuffHead = 0; /* Back to Start*/
            if (BuffTail == 0) RwnBus_FERR = 1;   /*Oups*/
        }
    }
    rxLastTime = now;

    if(RwnBus_FERR){
        return;                         /* corrupted byte: the packet will fail the length / xor check */
    }
    /* searching the header */
   
    if(PacketStartCount < RWN_HEADER_SIZE){   
        if(b == RWN_HEADER_CHAR ){
            PacketStartCount++;
      
        } else {
            PacketStartCount = 0;
             
        }
    } else {
       // *(rwnPacketData + BuffHead)= b;
       rwnPacketData[ BuffHead] =b;
        BuffHead++;
    }
    
}

void rwnBus_task(void){
    unsigned char n, silence, i, x;
     
#if FEAT_RWNBUS_TX 
    /* ---- transmission: one byte per pass, release the bus when the last one has left ---- */
    if(rwnBus_txb){
        if(txIdx < txLen){
            if(TXIF){
                if(txIdx < RWN_HEADER_SIZE){
                    TXREG = RWN_HEADER_CHAR;
                } else {
                    TXREG = txBuf[txIdx - RWN_HEADER_SIZE];
                }
                txIdx++;
            }
        } else if(TRMT){
            RWNBUS_DE = 0;
            rwnBus_txb = 0;
            rxCloseCount++;             /* restart the header search */
        }
        return;
    }
#endif
#if FEAT_RWNBUS_RX

    /*check for end*/
    silence = (unsigned char)(TMR0 - rxLastTime);
    if(silence < RWNBUS_SILENCE_MIN){
        return;                         /* packet still in progress */
    }

   
    /* check Packet */  
    n = BuffHead - BuffTail  ;
    if(n >= RWN_PACKETS_MIN_SIZE){
        x = 0;
        for(i = BuffTail ; i < BuffHead; i++){
             x ^= *(rwnPacketData + i);
        }
        if(x == 0){        
           pRwnBus_RxPacket = (DccPacket_t*)(rwnPacketData+ BuffTail);              /* valid */
      
            RwnBus_RxPacketLength=n;
            RwnBus_RRDY= 1;
            PacketStartCount=0; 
            BuffTail+= DCC_PACKETS_MAX_SIZE;
            if (BuffTail >= RWNBUS_BUFFER_SIZE) BuffTail=0; /** back to start */
            return;
        }
    }
    if(silence >= RWNBUS_SILENCE_MAX){  /* incomplete or invalid, and no more bytes coming: drop */
        /** reset read */
        BuffHead =BuffTail;
         PacketStartCount=0; 
    }
  
#endif
}

unsigned char rwnBus_Send(const unsigned char *p, unsigned char n){
    unsigned char i, x = 0;

    if(RwnBus_TRMT || n < RWN_PACKETS_MIN_SIZE - 1 || n > RWN_PACKETS_MAX_SIZE - 1){
        return 0;
    }
   // if(queueBuffer_PendingCount(&RwnPacketQueue) != 0){
        return 0;                       /* a packet is being received */
    //}
    for(i = 0; i < n; i++){
        txBuf[i] = p[i];
        x ^= p[i];
    }
    txBuf[n] = x;                       /* xor of the payload: the whole packet xors to 0 */
    txLen = (unsigned char)(RWN_HEADER_SIZE + n + 1);
    txIdx = 0;
    RWNBUS_DE = 1;                      /* transmit mode, first byte on the next Task pass */
    RwnBus_TRMT = 1;
    return 1;
}