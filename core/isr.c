/*
 * isr.c : unique interrupt vector of the decoder
 *
 * Structure: each feature provides an XXX_ISR_BLOCK() block (here as macros; to be moved
 * later into the .h of each module). isr() only assembles them.
 * No physical pins in this file: logical names from HardwareProfile.h.
 * Only the isr_init() section is MCU-specific (PIC16F628A registers).
 *
 * Budget: Timer2 interrupts every 58 us (bit 1) or 100 us (bit 0), 1 cycle = 1 us.
 * Each pass must last LESS THAN 58 CYCLES, including compiler prologue/epilogue,
 * otherwise the next edge jitters. Hence the split:
 *   1st half of the bit: light work  (PR2 = d_pr already computed, RX polling)
 *   2nd half of the bit: compute the duration of the next bit (heavier work)
 * TO MEASURE (MPLAB SIM, stopwatch): aim for < 45 cycles in both cases. Keep ISR
 * variables in bank 0: verify in the .lst that there is no bsf STATUS,RP0.
 */
#include "isr.h"

#include "HardwareProfile.h"
#include "modules/MultiplexedServo.h"
#include "modules/RwnBus.h"
#include "modules/TrackPower.h"



/* ---- MCU layer: PIC16F628A registers ----------------------------------------------- */
void isr_init(void)
{
   PEIE = 1;
    GIE = 1;
}

void __interrupt() isr(void)
{

#if defined (FEAT_RWNBUS_RX)
    rwnBus_receive_taskInterrupt();
#endif
#if defined (FEAT_DCC_GEN)
  //  trackPower_TaskInterrupt();
#endif
#if defined (FEAT_MX_SERVO)
   multiplexedServo_taskInterrupt();
#endif

}