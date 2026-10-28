/**
 * @file main.c
 * @author benoit
 * @date 2026-10-10
 * @brief Main function
 */

 #include "HardwareProfile.h"

 //modules
 #include "modules/SystemLEDIndicator.h"
 #include "modules/RwnBus.h"
 #include "modules/ResetBtn.h"
 #include "modules/MultiplexedServo.h"
 #include "modules/TrackPower.h"
 #include "modules/EEPROM.h"
 #include "core/DccDecoder.h"
 #include "core/isr.h"


 /** CV cache Managment */
__EEPROM_DATA(DCC_DEFAULT_ADDR,0x01,0x70,0x70,0x70,0x70); //Default Data for MCU flash
unsigned char CVCcache[MAX_CV];



typedef enum {
   START_STATE = 0,
   OPE_STATE =1,
   PRG_STATE = 2,
   RESET_STATE = 3
}EDecoderState;

EDecoderState decoderState;
DccAddress dccDecoder_Address;
__bit dccDecoder_ResetMode = 0;
uint8_t sh_porta;




//hook for DCC
void dccDecoder_Speed(unsigned char forward, unsigned char step){
   return ;
}

void dccDecoder_FnGrp1(unsigned char mask){
   unsigned char idx = 4;
   unsigned char divergin;
   unsigned char cmdDiverginCycle;
   systemLEDIndicator_setState(BUSY_STATE);
   while (idx){
      idx--;
      cmdDiverginCycle=0;
      divergin=mask && (1 << idx);
      if (divergin) cmdDiverginCycle = 100;/* *(CVCcache + CV_SERVO_IDX + idx);*/
      multiplexedServo_setPosition(idx,cmdDiverginCycle);
 
   } 
   systemLEDIndicator_setState(READY_STATE);
}
/*

*/
//#define _XTAL_FREQ 4000000       // pour __delay_ms()



// main.c : ordonnanceur coopératif
int main(void){


    //init System
   hal_init();
   systemLEDIndicator__init();
   rwnBus_init();
   multiplexedServo_init();
 
   
    //trackPower_Init();
   // i2c_init(); 
    dccDecoder_Init();


   /**         Init Doecoder */

   //Load CV
   unsigned char idx=MAX_CV;
   do{
      idx--;
      CVCcache[idx]=eeprom_read(idx);
   }while (idx);


   //Test Reset
   if(isResetBtnPressed()){
         //soft reset
         dccDecoder_Address = DCC_DEFAULT_ADDR;
         dccDecoder_ResetMode =1;
         decoderState= RESET_STATE;

          //write CV
   }

   //starting...
   isr_init();
   systemLEDIndicator_setState(READY_STATE);
   while (1) {
     

    

    
    //if (RwnBus_OERR || RwnBus_FERR) systemLEDIndicator_setState(ERROR_STATE);
     rwnBus_task();
     
      if ( RWNBUS_FRAME_READY()) {
         
         /** Decoder JOB */
         dccDecoder_Task(pRwnBus_RxPacket,RwnBus_RxPacketLength);
             
         /**DCC Transmimison */
       /*  DccCurrentPacket = pRwnBus_RxPacket;
         TrkPwr_TRMT=1 ; //Start Transming 
         while (TrkPwr_TRMT) ;*/
         RWNBUS_RELEASE()   ; //Allow forward in buffer
      }
   }
   return 0;
}
