
 /**
 * SystemLEDIndicator.h : SLI module 
 *
 * Role: Display module Status using multiplexed Led wired
|S1 | S0 || Status | LEDs |
|---|----||--------|------|
| 0 | 0  ||  Init  |  -   |
| 0 | 1  || Busy   |Y/G,R*| 
| 1 | 0  || Ready  |  G   |
| 1 | 1  || Error  |  R   |
---------------------------
*Green and Red leds replace Yellow one if not existe

 * No physical pins here: the hardware profile (HardwareProfile.h) must define
 *   SLI_STATUS0        pin 1
 *   SLI_STATUS0_TRIS   its direction register
 *   SLI_STATUS1        pin 2
 *   SLI_STATUS1_TRIS   its direction register
 * otherwise compilation stops (#error).
 */

#ifndef SYSTEMLEDINDICATOR_H
#define SYSTEMLEDINDICATOR_H

#include "HardwareProfile.h"

#if !defined(SLI_STATUS0) || !defined(SLI_STATUS0_TRIS) \
    || !defined(SLI_STATUS1) || !defined( SLI_STATUS1_TRIS) 
#error "HardwareProfile.h must define SLI_STATUS0, SLI_STATUS0_TRIS, SLI_STATUS1 and  SLI_STATUS1_TRIS"
#endif


// SystemLEDIndicator module interface
// Define state enum
typedef enum {
    INIT_STATE = 0,
    BUSY_STATE = 3,
    READY_STATE = 1,
    ERROR_STATE = 2
} SystemLEDIndicatorState;

void systemLEDIndicator__init(void);
void systemLEDIndicator_setState(SystemLEDIndicatorState state);
#endif // SYSTEMLEDINDICATOR_H
