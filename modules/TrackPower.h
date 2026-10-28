/**
 * TrackPower.h : Track Power module 
 *
 * Role:  Realy Booster for DCC on Track. Can also be use for PWM train controle, A Legacy approche for non DCC locomotive
 * Availble with Railway Network Software and isolated block architectured track network 
 *
 * No physical pins here: the hardware profile (HardwareProfile.h) must define
 *   DCC_PORT       
 *   DCC_TRIS        
 *   DCC_MASK
 * otherwise compilation stops (#error).
 *
 * Stack:
 */

#ifndef TRACKPOWER_H
#define TRACKPOWER_H

#include "HardwareProfile.h"
#include "core/DccDecoder.h"

#if !defined(DCC_PORT) || !defined(DCC_TRIS) || !defined(DCC_MASK)
#error "HardwareProfile.h must define RWNBUS_DE and RWNBUS_DE_TRIS"
#endif


extern DccPacket_t *DccCurrentPacket;
extern __bit TrkPwr_TXEN;
extern __bit TrkPwr_TRMT;

#define TRKPWR_TX_BUSY()      (TrkPwr_TRMT)

// TrackPower module interface
void trackPower_Init(void);
void trackPower_SetDCCMode(unsigned char dccMode);
inline unsigned char trackPower_IsDCCMode(void);
/**
 * Task interrupt for track power management
 * This should be called from the Timer2 interrupt service routine to handle DCC signal generation.
 * It manages the timing and state transitions for sending DCC packets on RB5.
 * Note: Ensure that the Timer2 interrupt is enabled and that the ISR calls this function.
 */
void trackPower_TaskInterrupt(void); 
#endif // TRACKPOWER_H
