#ifndef DCCDECODER_H
#define DCCDECODER_H

/*
 * DccDecoder : DCC packet processing, independent of the packet source.
 *
 * Sources (RwnBus over RS485, historical 58/100 us signal decoder, ...) hand over a VALID
 * packet (length and XOR already checked by the source): address, instruction, data..., xor.
 * The main loop orchestrates:
 *      if (RWNBUS_FRAME_READY()) { dccDecoder_Task(rwnBus_Packet, rwnBus_PacketLength); RWNBUS_RELEASE(); }
 *
 * dccDecoder_Task() filters the address, decodes the instruction once and calls the
 * decoder specific hooks with ready-to-use values. Everything is selected at compile time
 * in HardwareProfile.h, a block that is not enabled generates no code and requires no hook.
 *
 * ---- addressing type (fixed at compile time) ------------------------------------------
 *  Multi-function decoder (7-bit address, unsigned char) : define any DCC_MF_*_CAPABILITY
 *  Accessory decoder (9-bit address, unsigned int)       : define any DCC_ACC_*_CAPABILITY
 *  Both families in one build are refused.
 *
 * ---- capabilities and the hooks to implement elsewhere (link error if one is missing) ----
 *  DCC_MF_FN_GRP1_CAPABILITY      void dccDecoder_FnGrp1(unsigned char mask);
 *        bit0..3 = F1..F4, bit4 = F0
 *  DCC_MF_FN_GRP2_CAPABILITY      void dccDecoder_FnGrp2(unsigned char firstFn, unsigned char mask);
 *        firstFn = 5 or 9, bit0 = F(firstFn) .. bit3 = F(firstFn + 3)
 *  DCC_MF_FEATURE_EXPAN_CAPABILITY void dccDecoder_FnExpan(unsigned char firstFn, unsigned char mask);
 *        firstFn = 13 or 21, bit0 = F(firstFn) .. bit7 = F(firstFn + 7)
 *  DCC_MF_ADV_OPE_CAPABILITY      void dccDecoder_Speed(unsigned char forward, unsigned char step);
 *        128 speed steps: step 0 = stop, 1 = emergency stop, 2..127 = speed
 *  DCC_ACC_BASIC_CAPABILITY       void dccDecoder_Accessory(unsigned char output, unsigned char activate);
 *        output 0..7 (bits 2:1 = pair, bit 0 = output of the pair), activate = 0 / 1
 *  DCC_ACC_EXTENDED_CAPABILITY    void dccDecoder_AccessoryAspect(unsigned char head, unsigned char aspect);
 *        head 0..3 (signal head of the decoder address), aspect 0..31
 *  DCC_MF_CV_ACCESS_CAPABILITY / DCC_ACC_CV_ACCESS_CAPABILITY (long form: verify byte, write byte, bit)
 *                                 unsigned char dccDecoder_CvRead(unsigned int cv);
 *                                 void dccDecoder_CvWrite(unsigned int cv, unsigned char value);
 *        cv = CV number 1..1024. Verify, write and bit operations are built from these two hooks.
 *
 * ---- address filter ---------------------------------------------------------------------
 *  A packet is processed when its address equals dccDecoder_Address (global, loaded from the
 *  CV by main). The broadcast address (0x00 for MF, 0x1FF for accessories) is accepted ONLY
 *  for CV access and ONLY while dccDecoder_ResetMode is 1 (set by main from the reset button):
 *  this lets the address CV be programmed on a decoder that was just reset.
 *  The idle packet (0xFF) and the 14-bit addresses are ignored.
 *
 * Stack: main -> dccDecoder_Task -> hook -> hook callee. dccDecoder_Task calls no other
 * function of its own, keep the hooks shallow.
 */

#include "HardwareProfile.h"



/* ---- capability checks -------------------------------------------------------------- */
#if defined(DCC_MF_FN_GRP1_CAPABILITY) || defined(DCC_MF_FN_GRP2_CAPABILITY) || \
    defined(DCC_MF_FEATURE_EXPAN_CAPABILITY) || defined(DCC_MF_ADV_OPE_CAPABILITY) || \
    defined(DCC_MF_CV_ACCESS_CAPABILITY)
#define DCC_ADDRESS_MF   1
#else
#define DCC_ADDRESS_MF   0
#endif
#if defined(DCC_ACC_BASIC_CAPABILITY) || defined(DCC_ACC_EXTENDED_CAPABILITY) || \
    defined(DCC_ACC_CV_ACCESS_CAPABILITY)
#define DCC_ADDRESS_ACC  1
#else
#define DCC_ADDRESS_ACC  0
#endif
#if DCC_ADDRESS_MF && DCC_ADDRESS_ACC
#error "HardwareProfile.h: DCC_MF_* and DCC_ACC_* capabilities cannot be mixed (one addressing type per build)"
#endif
#if !DCC_ADDRESS_MF && !DCC_ADDRESS_ACC
#error "HardwareProfile.h must define at least one DCC_MF_*_CAPABILITY or DCC_ACC_*_CAPABILITY"
#endif
#if defined(DCC_MF_CV_ACCESS_CAPABILITY) || defined(DCC_ACC_CV_ACCESS_CAPABILITY)
#define DCC_CV_ACCESS    1
#else
#define DCC_CV_ACCESS    0
#endif

/* ---- constants ----------------------------------------------------------------------- */
#define DCC_PACKETS_MIN_SIZE  3         /* address, instruction, xor */
#define DCC_PACKETS_MAX_SIZE  6
typedef union {
    struct 
    {
        unsigned char addr;
        unsigned char instrType :3;        
    } mfip;
    unsigned char rawData [DCC_PACKETS_MAX_SIZE];   /* buffer for a source that builds a packet */
} DccPacket_t;


#define DCC_DEFAULT_ADDR 0x03
#if DCC_ADDRESS_MF
#define DCC_BROADCAST_ADDR    0x00
typedef unsigned char DccAddress;       /* 7-bit address */
#else
#define DCC_BROADCAST_ADDR    0x1FF
typedef unsigned int  DccAddress;       /* 9-bit decoder address */
#endif

/* ---- address of the packet (first byte) ---------------------------------------------- */
#define IS_MF_ADDR(addr)            (((addr) & 0x80) == 0x00)   /* 7-bit address: multi-function decoder */
#define IS_ACC_ADDR(addr)           (((addr) & 0xC0) == 0x80)   /* 10AAAAAA: accessory decoder */
#define IS_IDLE_ADDR(addr)          ((addr) == 0xFF)            /* idle packet */
#define IS_NOT_SUPPORTED_ADDR(addr) (!(IS_IDLE_ADDR(addr) || IS_MF_ADDR(addr) || IS_ACC_ADDR(addr)))   /* 14 bits address and advanced extended packet */

/* ---- instruction byte of a multi-function packet: 3 bits type + 5 bits data ------------ */
#define DCC_INSTR_TYPE(b)           ((unsigned char)(b) >> 5)
#define DCC_INSTR_DATA(b)           ((unsigned char)(b) & 0x1F)

//3Bits Instruction Type
#define DCC_MF_ADV_OPE_INSTR        0x1
#define DCC_MF_FN_GRP1              0x4
#define DCC_MF_FN_GRP2              0x5
#define DCC_MF_FEATURE_EXPAN_INSTR  0x6
#define DCC_MF_CV_ACCESS_INSTR      0x7

/* ---- instruction Speed and Direction of a multi-function packet: 1 bits direction + 7 bits speed ------------ */
#define DCC_INSTR_DIR(b)           ((unsigned char)(b) >> 7)
#define DCC_INSTR_SPEED(b)         ((unsigned char)(b) & 0x7F)

/* ---- result of dccDecoder_Task ------------------------------------------------------- */
#define DCC_RESULT_IGNORED  0           /* not for this decoder, or not supported */
#define DCC_RESULT_DONE     1           /* processed */
#define DCC_RESULT_ACK      2           /* processed, an acknowledge is expected (verify ok, CV written) */

/* ---- module data ------------------------------------------------------------------- */
extern DccAddress dccDecoder_Address;   /* own address, loaded from the CV by main */
extern __bit dccDecoder_ResetMode;        /* 1 while the decoder is in reset mode (broadcast CV access allowed) */


/* ---- hooks, to implement by the decoder (only those of the enabled capabilities) -------- */
#ifdef DCC_MF_FN_GRP1_CAPABILITY
void dccDecoder_FnGrp1(unsigned char mask);
#endif
#ifdef DCC_MF_FN_GRP2_CAPABILITY
void dccDecoder_FnGrp2(unsigned char firstFn, unsigned char mask);
#endif
#ifdef DCC_MF_FEATURE_EXPAN_CAPABILITY
void dccDecoder_FnExpan(unsigned char firstFn, unsigned char mask);
#endif
#ifdef DCC_MF_ADV_OPE_CAPABILITY
void dccDecoder_Speed(unsigned char forward, unsigned char step);
#endif
#ifdef DCC_ACC_BASIC_CAPABILITY
void dccDecoder_Accessory(unsigned char output, unsigned char activate);
#endif
#ifdef DCC_ACC_EXTENDED_CAPABILITY
void dccDecoder_AccessoryAspect(unsigned char head, unsigned char aspect);
#endif
#if DCC_CV_ACCESS
unsigned char dccDecoder_CvRead(unsigned int cv);
void dccDecoder_CvWrite(unsigned int cv, unsigned char value);
#endif

/* ---- DccDecoder module interface ------------------------------------------------------- */
void dccDecoder_Init(void);             // address = 0 (no own address until main loads the CV), reset mode off
/* pkt = valid packet (address, instruction, data..., xor), n = its length (3..6). Returns DCC_RESULT_*. */
unsigned char dccDecoder_Task(const DccPacket_t *pkt, unsigned char n);


#endif // DCCDECODER_H