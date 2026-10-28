#include "DccDecoder.h"
#include "modules/SystemLEDIndicator.h"




DccAddress dccDecoder_Address;
__bit dccDecoder_ResetMode;

void dccDecoder_Init(void){
    dccDecoder_Address = DCC_DEFAULT_ADDR;
    dccDecoder_ResetMode = 0;
}

// One single function on purpose: every extra level of call costs a hardware stack level.
unsigned char dccDecoder_Task(const DccPacket_t *pkt, unsigned char n){
#if DCC_ADDRESS_MF
    unsigned char instrByte1;                   // second byte: instruction (MF) 
 #endif   
    
    //or address / data (accessory)
    unsigned char r = DCC_RESULT_IGNORED;
#if DCC_CV_ACCESS
    unsigned char cvIdx = 0;            // index of the 1110CCVV byte in pkt, 0 = no CV instruction
    unsigned char t, d, cur, mask;
    unsigned int cv;
#endif
#if DCC_ADDRESS_ACC
    DccAddress addr;
#if DCC_CV_ACCESS
    unsigned char bcast = 0;            // broadcast address: only CV access, only in reset mode
#endif
#endif

    if(n < DCC_PACKETS_MIN_SIZE){
        return DCC_RESULT_IGNORED;
    }
    


    /* ================= address filter and instruction decoding ================= */


#if DCC_ADDRESS_MF
    instrByte1 = pkt->rawData[1];

    if(pkt->mfip.addr == DCC_BROADCAST_ADDR){   // checked first: address 0 is never "own"
#if DCC_CV_ACCESS
        if(!dccDecoder_ResetMode || DCC_INSTR_TYPE(i1) != DCC_MF_CV_ACCESS_INSTR){
            return DCC_RESULT_IGNORED;
        }
#else
        return DCC_RESULT_IGNORED;
#endif
    } else if(pkt->mfip.addr != dccDecoder_Address){
        return DCC_RESULT_IGNORED;      // other decoder, idle (0xFF), accessory, long address
    }
    switch(DCC_INSTR_TYPE(instrByte1)){
#ifdef DCC_MF_FN_GRP1_CAPABILITY
    case DCC_MF_FN_GRP1:
        if(n == 3){
            dccDecoder_FnGrp1(DCC_INSTR_DATA(instrByte1 ));
            r = DCC_RESULT_DONE;
        }
        break;
#endif
#ifdef DCC_MF_FN_GRP2_CAPABILITY
    case DCC_MF_FN_GRP2:
        if(n == 3){
            dccDecoder_FnGrp2((i1 & 0x10) ? 5 : 9, (unsigned char)(i1 & 0x0F));
            r = DCC_RESULT_DONE;
        }
        break;
#endif
#ifdef DCC_MF_FEATURE_EXPAN_CAPABILITY
    case DCC_MF_FEATURE_EXPAN_INSTR:
        if(n == 4 && (i1 == 0xDE || i1 == 0xDF)){
            dccDecoder_FnExpan((i1 == 0xDE) ? 13 : 21, pkt[2]);
            r = DCC_RESULT_DONE;
        }
        break;
#endif
#ifdef DCC_MF_ADV_OPE_CAPABILITY
    case DCC_MF_ADV_OPE_INSTR:
        if(n == 4 && instrByte1 == 0x3F){       // 128 speed steps
            dccDecoder_Speed((unsigned char)((pkt->rawData[2] & DCC_MF_DIR_MASK) != 0), (unsigned char)(pkt->rawData[2] & DCC_MF_SPEED_MASK));
            r = DCC_RESULT_DONE;
        }
        break;
#endif
#ifdef DCC_MF_CV_ACCESS_CAPABILITY
    case DCC_MF_CV_ACCESS_INSTR:
        if(n == 5 && (i1 & 0x10) == 0){ // long form 1110CCVV
            cvIdx = 1;
        }
        break;
#endif
    default:
        break;
    }
#endif /* DCC_ADDRESS_MF */

#if DCC_ADDRESS_ACC
    if((pkt[0] & 0xC0) != 0x80){
        return DCC_RESULT_IGNORED;      // not an accessory packet (loco, idle, long address)
    }
    // 9-bit address: 6 low bits in the address byte, 3 high bits (ones' complement) in the second byte
    addr = (DccAddress)((pkt[0] & 0x3F) | ((unsigned int)(((unsigned char)~i1 >> 4) & 0x07) << 6));
    if(addr == DCC_BROADCAST_ADDR){
#if DCC_CV_ACCESS
        if(!dccDecoder_ResetMode || n != 6){
            return DCC_RESULT_IGNORED;
        }
        bcast = 1;
#else
        return DCC_RESULT_IGNORED;
#endif
    } else if(addr != dccDecoder_Address){
        return DCC_RESULT_IGNORED;
    }

    if(i1 & 0x80){                      // basic: 10AAAAAA 1AAACDDD
#ifdef DCC_ACC_BASIC_CAPABILITY
        if(n == 3){
            dccDecoder_Accessory((unsigned char)(i1 & 0x07), (unsigned char)((i1 >> 3) & 0x01));
            r = DCC_RESULT_DONE;
        }
#endif
#ifdef DCC_ACC_CV_ACCESS_CAPABILITY
        if(n == 6 && (pkt[2] & 0xF0) == 0xE0){
            cvIdx = 2;
        }
#endif
    } else if((i1 & 0x09) == 0x01){     // extended: 10AAAAAA 0AAA0AA1
#ifdef DCC_ACC_EXTENDED_CAPABILITY
        if(n == 4){
            dccDecoder_AccessoryAspect((unsigned char)((i1 >> 1) & 0x03), pkt[2]);
            r = DCC_RESULT_DONE;
        }
#endif
#ifdef DCC_ACC_CV_ACCESS_CAPABILITY
        if(n == 6 && (pkt[2] & 0xF0) == 0xE0){
            cvIdx = 2;
        }
#endif
    }
#if DCC_CV_ACCESS
    if(bcast && cvIdx == 0){
        return DCC_RESULT_IGNORED;
    }
#endif
#endif /* DCC_ADDRESS_ACC */

    /* ================= CV access (long form), common to both families ================= */
#if DCC_CV_ACCESS
    if(cvIdx != 0){
        t = pkt[cvIdx];                                 // 1110CCVV
        cv = (unsigned int)(((unsigned int)(t & 0x03) << 8) | pkt[cvIdx + 1]) + 1;
        d = pkt[cvIdx + 2];                             // data
        switch(t & 0x0C){
        case 0x04:                                      // verify byte
            r = (dccDecoder_CvRead(cv) == d) ? DCC_RESULT_ACK : DCC_RESULT_DONE;
            break;
        case 0x0C:                                      // write byte
            dccDecoder_CvWrite(cv, d);
            r = DCC_RESULT_ACK;
            break;
        case 0x08:                                      // bit manipulation, d = 111CDBBB
            if((d & 0xE0) == 0xE0){
                cur = dccDecoder_CvRead(cv);
                mask = (unsigned char)(1u << (d & 0x07));
                if(d & 0x10){                           // C = 1: write bit
                    if(d & 0x08){
                        cur |= mask;
                    } else {
                        cur &= (unsigned char)~mask;
                    }
                    dccDecoder_CvWrite(cv, cur);
                    r = DCC_RESULT_ACK;
                } else {                                // C = 0: verify bit
                    r = (((cur & mask) != 0) == ((d & 0x08) != 0)) ? DCC_RESULT_ACK : DCC_RESULT_DONE;
                }
            }
            break;
        default:                                        // 00: reserved
            break;
        }
    }
#endif
    return r;
}