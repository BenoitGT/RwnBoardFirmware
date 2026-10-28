#include "SystemLEDIndicator.h"
#include <xc.h>



void systemLEDIndicator__init(void){
    /*activate OUTPUT mode*/
    SLI_STATUS0_TRIS =0;
    SLI_STATUS1_TRIS =0;
}

// Set RA0 and RA1 outputs per state
void systemLEDIndicator_setState(SystemLEDIndicatorState state)
{
    SLI_STATUS0 = (state & 0x01);
    SLI_STATUS1 = ((state >> 1) & 0x01);
}

// SystemLEDIndicator module implementation
