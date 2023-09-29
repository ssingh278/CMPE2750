// I2C library, ATmega328P Version
// Simon Walker, NAIT
// Revision History:
// March 18 2022 - Initial Build

#define I2C_STOP 1
#define I2C_NOSTOP 0
#define I2C_READ 1
#define I2C_WRITE 0
#define I2C_ACK 1
#define I2C_NACK 0

// enum for desired I2C bus rate
typedef enum
{
	I2CBus100,  // I2C bus @ 100 kHz
	I2CBus400   // I2C bus @ 400 kHz
} I2C_BusRate;

// initialize the TWI bus for use
int I2C_Init (unsigned long ulBusRate, I2C_BusRate sclRate);

// start a transaction with intent to read or write
int I2C_Start (unsigned char uc7Addr, int bRead);

// read an 8-bit device register (complete transaction)
//int I2C_ReadRegister8 (unsigned char uc7Addr, unsigned char ucRegister, unsigned char * ucValue);

// write an 8-bit device register (complete transaction)
// read a 16-bit device register (complete transaction)
// write a 16-bit device register (complete transaction)
// read n-bytes from a device, starting at register (complete transaction)
// write n-bytes to a device, starting at register (complete transaction)

// scan all 7-bit addresses and report ones found on the bus
// requires 128-byte buffer for results
void I2C_Scan (unsigned char * results);

// private(ish)helper methods:
// write a byte to an open transaction
int I2C_Write8 (unsigned char ucData, int bStop);

// read a byte from an open transaction
int I2C_Read8 (unsigned char *ucData, int bAck, int bStop);
// end helper methods

