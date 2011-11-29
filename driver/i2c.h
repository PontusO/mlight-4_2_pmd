/*
 * Copyright (c) 2008, Pontus Oldberg.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef I2C_H_INCLUDED
#define I2C_H_INCLUDED

//------------------------------------------------------------------------------------
// Data types
//------------------------------------------------------------------------------------
struct i2c {
  struct pt pt;
  u8_t device;
  u16_t address;
  u8_t address_size;
  u8_t *buffer;
  u8_t len;
  u8_t ptr;
  u8_t error;
  u8_t busy;
};

extern struct i2c i2c;

//------------------------------------------------------------------------------------
// Global CONSTANTS
//------------------------------------------------------------------------------------

#define I2C_WRITE   0x00            // SMBus WRITE command
#define I2C_READ    0x01            // SMBus READ command

#define EEPROM1     0xA0
#define EEPROM2     0xA1

// I2C buss errors
enum I2cBussError {
  I2C_ERR_INVALID_I2C_OPERATION = 0x01,
  I2C_ERR_WRITE_DATA_NACKED,
  I2C_ERR_MT_ATBITRATION_LOST,
  I2C_ERR_SLAVE_NOT_RESPONDING,
};

// SMBus states:
// MT = Master Transmitter
// MR = Master Receiver
#define  SMB_BUS_ERROR  0x00        // (all modes) BUS ERROR
#define  SMB_START      0x08        // (MT & MR) START transmitted
#define  SMB_RP_START   0x10        // (MT & MR) repeated START
#define  SMB_MTADDACK   0x18        // (MT) Slave address + W transmitted;
                                    //  ACK received
#define  SMB_MTADDNACK  0x20        // (MT) Slave address + W transmitted;
                                    //  NACK received
#define  SMB_MTDBACK    0x28        // (MT) data byte transmitted; ACK rec'vd
#define  SMB_MTDBNACK   0x30        // (MT) data byte transmitted; NACK rec'vd
#define  SMB_MTARBLOST  0x38        // (MT) arbitration lost
#define  SMB_MRADDACK   0x40        // (MR) Slave address + R transmitted;
                                    //  ACK received
#define  SMB_MRADDNACK  0x48        // (MR) Slave address + R transmitted;
                                    //  NACK received
#define  SMB_MRDBACK    0x50        // (MR) data byte rec'vd; ACK transmitted
#define  SMB_MRDBNACK   0x58        // (MR) data byte rec'vd; NACK transmitted

//------------------------------------------------------------------------------------
// Function PROTOTYPES
//------------------------------------------------------------------------------------

void init_i2c(void) __banked;
PT_THREAD(SM_Send(struct i2c *i2c) __banked);
PT_THREAD(SM_Receive(struct i2c *i2c) __banked);
u8_t nos_i2c_read(struct i2c *i2c) __reentrant __banked;
u8_t nos_i2c_write(struct i2c *i2c) __reentrant __banked;
u8_t is_smbus_busy(void) __banked;

#endif /* I2C_H_INCLUDED */

/* EOF */
