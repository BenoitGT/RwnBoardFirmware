
#include "HardwareProfile.h"

uint8_t sh_porta;

/**
Default Hardware Abstract Layer Init
*/
void hal_init(void){
   CMCON = 0x07;  //Disable comparator
}