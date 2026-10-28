/**
 * MultiplexedServo.h : Multiplexed Servo module 
 *
 * Role: Manage up to 4 Servo command
 *
 * No physical pins here: the hardware profile (HardwareProfile.h) must define
 *   MX_SERVO_PIN       Pulse pin
 *   MX_SERVO_PIN_TRIS  its direction register
 *   MX_SERVO_SEL_INIT()  Init pins on Port for Multiplexed Addressing control
 *   MX_SERVO_SEL_WRITE  its pins SEL write marco
 * otherwise compilation stops (#error).
 *
 * 
 * Stack: 
 */


#ifndef MULTIPLEXEDSERVO_H
#define MULTIPLEXEDSERVO_H

#include "HardwareProfile.h"


#if !defined(MX_SERVO_PIN) || !defined(MX_SERVO_PIN_TRIS) 
#error "HardwareProfile.h must define MX_SERVO_PIN, MX_SERVO_PIN_TRIS"
#endif

#if !defined(MX_SERVO_SEL_SH_PORT) || !defined(MX_SERVO_SEL_PORT) || !defined(MX_SERVO_SEL_MASK) || !defined(MX_SERVO_SEL_SHIFT) 
#error "HardwareProfile.h must define MX_SERVO_SEL_PORT, MX_SERVO_SEL_MASK, MX_SERVO_SEL_SHIFT and MX_SERVO_SEL_SH_PORT"
#endif

// MultiplexedServo module interface
#define NUM_SERVOS 4

#define MX_SERVO_SEL_INIT() { MX_SERVO_SEL_TRIS &= (uint8_t)~MX_SERVO_SEL_MASK; } 
/* sel = address already positioned in SERVO_SEL_MASK (0x00, 0x40, 0x80, 0xC0) */
/*sh_porta = (uint8_t)(PORTA & ~MX_SERVO_SEL_MASK) | (uint8_t)(sel<<MX_SERVO_SEL_SHIFT); */ 

#define MX_SERVO_SEL_WRITE(sel) { \
       MX_SERVO_SEL_SH_PORT= (uint8_t)MX_SERVO_SEL_PORT; \
       MX_SERVO_SEL_SH_PORT &= ~ MX_SERVO_SEL_MASK; \
       MX_SERVO_SEL_SH_PORT |=  (uint8_t)(sel<<MX_SERVO_SEL_SHIFT); \
       MX_SERVO_SEL_PORT = MX_SERVO_SEL_SH_PORT;  \
    } 


/**
 * Initialize the multiplexed servo module. Sets up TIMER1 and necessary pins.
 * Call this at startup.
 * Note: Ensure the TIMER1 interrupt is enabled and the ISR calls multiplexedServo_TaskInterrupt
 */
void multiplexedServo_init(void);

/**
 * This function should be called from the TIMER1 interrupt service routine to handle the servo pulse timing.
 * It manages the state of the current servo pulse and schedules the next pulse accordingly.
 * Note: Ensure that the TIMER1 interrupt is enabled and that the ISR calls this function.
 */
void multiplexedServo_taskInterrupt(void);



/**
 *  positionStep: 0-127 corresponding to 1ms-2ms pulse width
 */
void multiplexedServo_setPosition(unsigned char servoIndex, unsigned char positionStep);


#endif // MULTIPLEXEDSERVO_H


