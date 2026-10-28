#include "MultiplexedServo.h"
#include <xc.h>
#include "SystemLEDIndicator.h"

#define SERVO_MIN_US 600   // 0.6ms
#define SERVO_MAX_US 2400   // 2.4ms
#define SERVO_PERIOD_US 20000 // 20ms
#define SERVO_PERIODS 100     //100xPERIOD_US  => servo move = 2s
#ifndef SERVO_SPEED_SCALE
#define SERVO_SPEED_SCALE 0 
#endif
//#define SERVO_STEP_US ((SERVO_MAX_US-SERVO_MIN_US)/128)

volatile unsigned char servoPulseSteps[NUM_SERVOS] = {0}; // 0=0.6ms..225=~2,4ms
volatile unsigned char currentServo = 0;
volatile unsigned char isPulse = 0;
volatile unsigned char RemainPeriods = 0;

// Step  to cycle timer 1  :  128 steps -> 1000 µs : 1 step = 7.8125 µs (approx 8µs) -> 125 steps = 1000µs
#define STEP_TO_US(step) (SERVO_MIN_US + (unsigned int)(step<< 3)) // step*8µs

// --- Helper: TIMER1 preload for us duration ---
inline void set_timer1_us(unsigned int us)
{
    // Assuming Fosc = 4MHz -> 1us instruction cycle, prescaler 1:1
    // TMR1 is 16bit, so preload = 65536 - us
    unsigned int preload = 0xFFFF - us;
    TMR1H = (preload >> 8) & 0xFF;
    TMR1L = preload & 0xFF;
}

// TIMER1 interrupt vector (adjust name for compiler if needed)
void multiplexedServo_taskInterrupt(void)
{
    if (! PIR1bits.TMR1IF ) return;
    PIR1bits.TMR1IF = 0;
    // Set TIMER1 for pulse ON duration: Steps to microseconds
    unsigned int pulse_us = STEP_TO_US(servoPulseSteps[currentServo]); 

       
    if (!isPulse) {
        // Start pulse for current servo
        // Set selection pins 
        MX_SERVO_SEL_WRITE(currentServo);

    
        // Set PWM output
        MX_SERVO_PIN  = 1;
        // Set TIMER1 for pulse ON duration: SERVO_MIN_US + step*SERVO_STEP_US
       
       // set_timer1_us(pulse_us); // User-defined inline for TIMER1 preload
        isPulse = 1;
    } else {
        // End pulse
        MX_SERVO_PIN = 0;
        // Set TIMER1 for OFF duration: SERVO_PERIOD_US/4 - pulse_us
        pulse_us = ((SERVO_PERIOD_US/NUM_SERVOS)<< SERVO_SPEED_SCALE) - pulse_us;
        // Next servo
        currentServo++;
        if ( currentServo == NUM_SERVOS){
            currentServo = 0;
            RemainPeriods --;
        } 
        isPulse = 0;
    }
    // User-defined inline for TIMER1 preload
    set_timer1_us(pulse_us); // User-defined inline for TIMER1 preload
    
    if (! RemainPeriods )  PIE1bits.TMR1IE = 0; //stop Servo moving
}



// --- Initialization (call at startup) ---
void multiplexedServo_init(void)
{
    // Set IFs as outputs
    MX_SERVO_SEL_INIT() // SEL Addresse pins output
    MX_SERVO_PIN_TRIS = 0 ; // Pulse pin output

    // Initialize TIMER1
    T1CON = 0b00000001; // TMR1 ON, prescaler 1:1
    TMR1H = 0xFF;// preload initial
    TMR1L = 0x00;
    PIE1bits.TMR1IE = 0;
    PIR1bits.TMR1IF = 0;
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1;
}

void multiplexedServo_setPosition(unsigned char servoIndex, unsigned char positionStep)
{
    if (servoIndex < NUM_SERVOS /*&& (positionStep <= 127*/) 
    {
        servoPulseSteps[servoIndex] = positionStep;
        RemainPeriods = SERVO_PERIODS;
        PIE1bits.TMR1IE = 1; //start Servo moving
    }
}