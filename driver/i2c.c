//------------------------------------------------------------------------------------
//
// Copyright 2001 Cygnal Integrated Products, Inc.
//
// FILE NAME      : i2c.c
// TARGET DEVICE  : C8051F020
// CREATED ON     : 2/20/01
// CREATED BY     : JS
// REVISION HISTORY :
//
// 2009-04-12   Pontus Oldberg
//  Imported i2c.c and i2c.h into the pmd project and started conversion
//  to follow invector architecture.
//
// 2005-04-27   Pontus Oldberg
//   Created a new version from the original Silabs application note to fit
//   the IP Avenger. First try. Original comments left in here.
//   This first version was very nastily converted to support only 1 byte
//   address transfers. Should be improved to be more flexible.
//
//
// Example code for interfacing a C8051F0xx to three EEPROMs via the SMBus.
// Code assumes that three 16-bit address space EEPROMs are connected
// on the SCL and SDA lines, and configured so that their slave addresses
// are as follows:
// CHIP_A = 1010000
// CHIP_B = 1010001
// CHIP_C = 1010010
//
// Slave and arbitration states are not defined.  Assume the CF000 is the only
// master in the system.
// Functions: SM_Send performs a 1-byte write to the specified EEPROM
// SM_Receive performs a 1-byte read of the specified EEPROM address (both include
// memory address references).
//
// Includes test code section.
#pragma codeseg APP_BANK

//------------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------------
#include <string.h>

#include "system.h"                 // SFR declarations
#include "i2c.h"

//-----------------------------------------------------------------------------------
//Global VARIABLES
//-----------------------------------------------------------------------------------
char I2cCommand;            // Holds the slave address + R/W bit for
                            // use in the SMBus ISR.

char I2cWord;               // Holds data to be transmitted by the SMBus
                            // OR data that has just been received.

char I2cByteNumber;         // Used by ISR to check what data has just been
                            // sent - High address byte, Low byte, or data
                            // byte

char I2cDataLen;            // Used by the ISR to keep track of incoming
                            // data bytes

char I2cBuffer[32];         // This buffer holds the incomming data from
                            // the I2C interface.

char I2cPtr;                // This pointer points to where the data is stored
                            // in the buffer above.

char I2cError;              // Holds any error after the transmission is ready.

unsigned char I2cHighAdd;   // High byte for EEPROM memory address
unsigned char I2cLowAdd;    // Low byte for EEPROM memory address

volatile bit SM_BUSY;       // This flag is set when a send or receive
                            // is started. It is cleared by the
                            // ISR when the operation is finished.

struct i2c i2c;             // Instance data


/**
 * SMBusInit
 *
 * Initialize the SMBus interface.
 */
void init_i2c(void) banked
{
#ifndef USE_UART_INSTEAD_OF_SMB
  /* Initialize the oscillator and SMBus control register */
  SFRPAGE   = SMB0_PAGE;
  SMB0CN    = 0x44;
  SMB0CR    = 0xE9;
  EIE1 |= 0x02;                    // SMBus interrupt enable
  SM_BUSY = 0;                     // Free SMBus for first transfer.
#else
  /* Make the SMBus always busy when the UART is enabled */
  SM_BUSY = 1;
#endif
  PT_INIT(&i2c.pt);
}


/**
 * SM_Send
 *
 * This protothread will write a number of data bytes to the I2C bus
 * according to the description in the i2c structure
 */
PT_THREAD(SM_Send(struct i2c *i2c) banked)
{
  PT_BEGIN(&i2c->pt);

  i2c->busy = TRUE;

  /* In case the I2c interface is busy we must wait for it */
  PT_WAIT_WHILE(&i2c->pt, SM_BUSY);
  SM_BUSY = 1;                                            // Occupy SMBus (set to busy)

  SFRPAGE = SMB0_PAGE;
  SMB0CN = 0x44;                                          // SMBus enabled,
  // ACK on acknowledge cycle

  I2cCommand = (i2c->device | I2C_WRITE);                 // Chip select + WRITE
  I2cByteNumber = i2c->address_size;                      // 2 address bytes
  if (I2cByteNumber == 2) {
    I2cHighAdd = (unsigned char)(i2c->address >> 8);      // Upper 8 address bits
    I2cLowAdd = (unsigned char)(i2c->address & 0x00FF);   // Lower 8 address bits
  } else {
    I2cHighAdd = (unsigned char)(i2c->address & 0x00FF);  // Use only lower 8 bits in this case
  }
  I2cDataLen = i2c->len;                                  // Tell the driver how much data
  I2cPtr = 0;                                             // Start writing from the beginning
  I2cError = 0;

  /* Copy application data to the driver buffer */
  memcpy(I2cBuffer, i2c->buffer, I2cDataLen);

  STO = 0;
  STA = 1;                                                // Start transfer
  SFRPAGE = LEGACY_PAGE;

  /* Wait for the transaction to complete */
  PT_WAIT_UNTIL(&i2c->pt, (SM_BUSY == 0));

  i2c->error = I2cError;
  i2c->busy = FALSE;

  PT_END(&i2c->pt);
}

/**
 * SM_Receive
 *
 * This protothread will read a number of data bytes from the I2C bus
 * according to the description in the i2c structure
 */
PT_THREAD(SM_Receive(struct i2c *i2c) banked)
{
  PT_BEGIN(&i2c->pt);

  i2c->busy = TRUE;

  PT_WAIT_WHILE(&i2c->pt, SM_BUSY);
  SM_BUSY = 1;                                            // Occupy SMBus (set to busy)

  SFRPAGE = SMB0_PAGE;
  SMB0CN = 0x44;                                          // SMBus enabled, ACK on acknowledge cycle

  I2cCommand = (i2c->device | I2C_READ);                  // Chip select + READ
  I2cByteNumber = i2c->address_size;                      // 2 address bytes
  if (I2cByteNumber == 2) {
    I2cHighAdd = (unsigned char)(i2c->address >> 8);      // Upper 8 address bits
    I2cLowAdd = (unsigned char)(i2c->address & 0x00FF);   // Lower 8 address bits
  } else {
    I2cHighAdd = (unsigned char)(i2c->address & 0x00FF);  // Use only lower 8 bits in this case
  }
  I2cDataLen = i2c->len;                                  // Tell the driver how much data
  I2cPtr = 0;                                             // Start writing from the beginning
  I2cError = 0;

  STO = 0;                                                // Send a stop
  STA = 1;                                                // Start transfer
  SFRPAGE = LEGACY_PAGE;

  PT_WAIT_UNTIL(&i2c->pt, (SM_BUSY == 0));

  /* Now copy data from I2c buffer to application buffer */
  memcpy(i2c->buffer, I2cBuffer, i2c->len);
  i2c->error = I2cError;
  i2c->busy = FALSE;

  PT_END(&i2c->pt);
}

/**
 * nos_i2c_write
 *
 * This protothread will write a number of data bytes to the I2C bus
 * according to the description in the i2c structure,
 * It does not use protothreading making it usefull during startup or in places
 * where it is impossible to use protothreads.
 */
u8_t nos_i2c_write(struct i2c *i2c) __reentrant banked
{
  if (SM_BUSY)
    return 1;

  SM_BUSY = 1;                                            // Occupy SMBus (set to busy)

  SFRPAGE = SMB0_PAGE;
  SMB0CN = 0x44;                                          // SMBus enabled,
  // ACK on acknowledge cycle

  I2cByteNumber = 2;                                      // 2 address bytes
  I2cCommand = (i2c->device | I2C_WRITE);                 // Chip select + WRITE
  I2cHighAdd = (unsigned char)(i2c->address >> 8);        // Upper 8 address bits
  I2cLowAdd = (unsigned char)(i2c->address & 0x00FF);     // Lower 8 address bits
  I2cDataLen = i2c->len;                                  // Tell the driver how much data
  I2cPtr = 0;                                             // Start writing from the beginning
  I2cError = 0;

  /* Copy application data to the driver buffer */
  memcpy(I2cBuffer, i2c->buffer, i2c->len);

  STO = 0;
  STA = 1;                                                // Start transfer
  SFRPAGE = LEGACY_PAGE;

  /* Wait for the transaction to complete */
  while(SM_BUSY);

  return I2cError;
}

/**
 * nos_i2c_read
 *
 * This function will read a number of bytes form the i2c interface.
 * It does not use protothreading making it usefull during startup or in places
 * where it is impossible to use protothreads.
 */
u8_t nos_i2c_read(struct i2c *i2c) __reentrant banked
{
  if(SM_BUSY)                                             // If busy, return
    return 1;

  SM_BUSY = 1;                                            // Occupy SMBus (set to busy)

  SFRPAGE = SMB0_PAGE;
  SMB0CN = 0x44;                                          // SMBus enabled, ACK on acknowledge cycle

  I2cByteNumber = 2;                                      // 2 address bytes
  I2cCommand = (i2c->device | I2C_READ);                  // Chip select + READ
  I2cHighAdd = (unsigned char)(i2c->address >> 8);        // Upper 8 address bits
  I2cLowAdd = (unsigned char)(i2c->address & 0x00FF);     // Lower 8 address bits
  I2cDataLen = i2c->len;                                  // Tell the driver how much data
  I2cPtr = 0;                                             // Start writing from the beginning
  I2cError = 0;

  STO = 0;                                                // Send a stop
  STA = 1;                                                // Start transfer
  SFRPAGE = LEGACY_PAGE;

  while(SM_BUSY);                                         // Wait here, while busy

  /* Now copy data from I2c buffer to application buffer */
  memcpy(i2c->buffer, I2cBuffer, i2c->len);
  i2c->error = I2cError;

  return I2cError;
}

/**
 * is_smbus_busy
 *
 * Returns wether the smbus is busy or not
 */
u8_t is_smbus_busy(void) banked
{
  if (SM_BUSY)
    return 1;
  return 0;
}

/* End Of File */
