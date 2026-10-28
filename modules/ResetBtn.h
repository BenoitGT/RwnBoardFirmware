 /**
 * ResetBtn.h : Reset module 
 *
 * Role: Allow Reset Decoder at Power UP and entring in broadcast programming mode (from 0x00 addressing)
 *
 * No physical pins here: the hardware profile (HardwareProfile.h) must define
 *   RESET_BT        input pin 
 *   RESTE_BT_TRIS   its direction register
 * otherwise compilation stops (#error).
 *
 * 
 */


#ifndef RESETBTN_H
#define RESETBTN_H

#include "HardwareProfile.h"

#if !defined(RESET_BT) || !defined(RESET_BT_TRIS)
#error "HardwareProfile.h must define RESET_BT and RESET_BT_TRIS"
#endif

// ResetBtn module interface
void    resetBtn_init(void);
unsigned char isResetBtnPressed(void);

#endif // RESETBTN_H
