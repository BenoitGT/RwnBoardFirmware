#include "EEPROM.h"
#include <xc.h>

unsigned char eeprom_read(unsigned char address)
{
    EEADR = address;
    EECON1bits.RD = 1;    // Start read
    return EEDATA;
}

void eeprom_write(unsigned char address, unsigned char data)
{
    // Required sequence per PIC16F628A datasheet
    EEADR = address;
    EEDATA = data;
    EECON1bits.WREN = 1;  // Enable write
    INTCONbits.GIE = 0;   // Disable interrupts
    EECON2 = 0x55;        // Write sequence
    EECON2 = 0xAA;
    EECON1bits.WR = 1;    // Start write
    INTCONbits.GIE = 1;   // Re-enable interrupts
    while(EECON1bits.WR); // Wait for write to finish
    EECON1bits.WREN = 0;  // Disable write
}
