#include "ResetBtn.h"
#include <xc.h>
#define RESETBTN_DEBOUNCE_DELAY_US 1000

void    resetBtn_init(void){
    RESET_BT_TRIS =0;
}

unsigned char isResetBtnPressed(void){
    if (RESET_BT == 0) {
        // Simple debounce: short delay then check again
        for (volatile unsigned int i = 0; i < RESETBTN_DEBOUNCE_DELAY_US; i++);
        if (RESET_BT == 0) {
            return 1;
        }
    }
    return 0;
}