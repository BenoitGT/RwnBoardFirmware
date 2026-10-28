#ifndef RWNBUS_H
#define RWNBUS_H

/*
 * RwnBus : RS485 half-duplex link between the command station and the decoders.
 *
 * Frame on the wire (19200 bauds, 8N1):
 *      '>' '>' '>'  address  instruction  [data ...]  xor
 *  - header = 3 x '>' (0x3E), stripped by the receiver (never stored in the queue)
 *  - packet = DCC packet, 3 to 6 bytes, no length field: XOR of all the bytes, check byte
 *    included, is 0 when the packet is valid
 *  - end of packet = silence on the line (RWNBUS_SILENCE_MIN)
 *
 * Reception
 *  - rwnBus_ReceiveTaskInterrupt() runs in the interrupt vector: strips the header and puts
 *    the packet bytes in RwnPacketQueue as HELD bytes (invisible to the consumer).
 *  - rwnBus_Task() runs in the main loop: after the silence it checks the packet (length,
 *    XOR). Valid  : copy in rwnBus_Packet, flag rwnBus_rdy, bytes COMMITTED in the queue
 *    (TrackPower or main can dequeue them). Invalid: bytes dropped (rollback).
 *
 * Assumptions
 *  - stop-and-wait: the sender waits for an ACK, so one packet at a time is in flight and the
 *    next one does not start before the previous one was consumed (RWNBUS_RELEASE()).
 *  - the main loop runs much faster than RWNBUS_SILENCE_MIN (period < ~700 us).
 *  - the queue is shared with TrackPower (DCC regeneration): in analog mode nobody drains it
 *    but main, which must dequeue the bytes it does not forward.
 *  - bytes lost (queue full, framing error) are caught by the XOR / length check.
 *
 * The meaning of the packet (DCC) is not handled here: see DccDecoder.h.
 *
 * Call order: rwnBus_Init(), then GIE/PEIE by main, then rwnBus_Task() in the main loop.
 * Hardware names (RWNBUS_DE, RWNBUS_DE_TRIS, RWNBUS_SPBRG) come from HardwareProfile.h.
 */

#include "HardwareProfile.h"

#include "core/DccDecoder.h"


/* ---- tunable parameters (override in HardwareProfile.h before the include) ------------- */
#ifndef RWNBUS_BUFFER_SIZE
#define RWNBUS_BUFFER_SIZE     16        /* queue bytes (usable: size - 1); room for 2 packets */
#endif
#ifndef RWNBUS_RX_IRQ
#define RWNBUS_RX_IRQ         1         /* 1: USART RX interrupt (RCIE); 0: ISR calls the receive function itself */
#endif
#ifndef RWNBUS_TIMER0_OPTION
#define RWNBUS_TIMER0_OPTION  0x04      /* OPTION_REG bits 5:0 : Timer0 on Fosc/4, prescaler 1:32 -> 32 us per step */
#endif
#ifndef RWNBUS_SILENCE_MIN
#define RWNBUS_SILENCE_MIN    40        /* steps of 32 us: 1280 us (> 2 bytes at 19200 bauds) = end of packet */
#endif
#ifndef RWNBUS_SILENCE_MAX
#define RWNBUS_SILENCE_MAX    160       /* steps of 32 us: 5120 us = a new frame is starting (max 255) */
#endif

#define RWN_HEADER_CHAR       0x3E      /* '>' */
#define RWN_HEADER_SIZE       3
#define RWN_PACKETS_MIN_SIZE  3         /* address, instruction, xor */
#define RWN_PACKETS_MAX_SIZE  6


/* ---- module data ------------------------------------------------------------------- */
extern DccPacket_t *pRwnBus_RxPacket; /* last valid packet (raw bytes, xor included), valid while RWNBUS_FRAME_READY() */
extern unsigned char RwnBus_RxPacketLength;
extern DccPacket_t *RwnBus_TxPacket; /* packet (raw bytes, xor included) to send, do NOT acces while RWNBUS_TX_BUSY()  */
extern unsigned char RwnBus_TxPacketLength;

extern __bit RwnBus_FERR;      /*Framing Error*/
extern __bit RwnBus_OERR;       /*Over Run Error*/
extern __bit RwnBus_CREN;      /*Continious Receptio ENable*/
extern __bit RwnBus_RRDY;       /* a valid packet is in rwnBus_Packet */
extern __bit RwnBus_TRMT;      /* transmission buffer full/empty */
extern __bit RwnBus_TXEN;

#define RWNBUS_FRAME_READY()  (RwnBus_RRDY)
#define RWNBUS_RELEASE()      (RwnBus_RRDY = 0)   /* rwnBus_Packet consumed: next packet can be accepted */
#define RWNBUS_TX_BUSY()      (RwnBus_TRMT)

/* ---- RwnBus module interface ----------------------------------------------------------- */
void rwnBus_init(void);                 // Call once from main (USART, Timer0, queue)
void rwnBus_receive_taskInterrupt(void); // Call from the interrupt vector (no-op when RCIF is 0)
void rwnBus_task(void);                 // Call from every main loop pass: end of packet detection and transmission
/* Send a packet: p = payload (address, instruction, data...), n = 2..5 bytes, the xor byte and
 * the header are added. Returns 1 if accepted, 0 if busy (transmission or reception in
 * progress) or invalid length. */
unsigned char rwnBus_send(const unsigned char *p, unsigned char n);


#endif // RWNBUS_H