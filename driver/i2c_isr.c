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
#include "system.h"                 // SFR declarations
#include "i2c.h"

extern char I2cCommand;
extern char I2cWord;
extern char I2cByteNumber;
extern char I2cDataLen;
extern char I2cBuffer[32];
extern char I2cPtr;
extern char I2cError;
extern unsigned char I2cHighAdd;
extern unsigned char I2cLowAdd;
extern bit SM_BUSY;

//------------------------------------------------------------------------------------
// Interrupt Service Routine
//------------------------------------------------------------------------------------
void SMBus_ISR (void) interrupt SMB0_VECTOR using 0
{
  switch (SMB0STA){                   // Status code for the SMBus (SMB0STA register)

      // Master Transmitter/Receiver: START condition transmitted.
      // The R/W bit of the COMMAND word sent after this state will
      // always be a zero (W) because for both read and write,
      // the memory address must be written first.
    case SMB_START:
      SMB0DAT = (I2cCommand & 0xFE);   // Load address of the slave to be accessed.
      STA = 0;                      // Manually clear START bit
      break;

      // Master Transmitter/Receiver: Repeated START condition transmitted.
      // This state should only occur during a read, after the memory address has been
      // sent and acknowledged.
    case SMB_RP_START:
      SMB0DAT = I2cCommand;            // COMMAND should hold slave address + R.
      STA = 0;
      break;

      // Master Transmitter: Slave address + WRITE transmitted.  ACK received.
    case SMB_MTADDACK:
      SMB0DAT = I2cHighAdd;           // Load high byte of memory address
      // to be written.
      break;

      // Master Transmitter: Slave address + WRITE transmitted.  NACK received.
      // The slave is not responding. if we are communicating with an EEPROM this
      // could mean that it is busy writing, so we will try again.
      // TODO: Implement a counter here so that we can get a timeout if we're in
      // an endless loop.
    case SMB_MTADDNACK:
//      I2cError = I2C_ERR_SLAVE_NOT_RESPONDING;
      STO = 1;                      // Reset communication.
      STA = 1;                      // Try again
      break;

      // Master Transmitter: Data byte transmitted.  ACK received.
      // This state is used in both READ and WRITE operations.  Check BYTE_NUMBER
      // for memory address status - if only HIGH_ADD has been sent, load LOW_ADD.
      // If LOW_ADD has been sent, check COMMAND for R/W value to determine
      // next state.
    case SMB_MTDBACK:
      switch (I2cByteNumber){
        case 2:                     // If BYTE_NUMBER=2, only HIGH_ADD
          SMB0DAT = I2cLowAdd;      // has been sent.
          I2cByteNumber--;          // Decrement for next time around.
          break;
        case 1:                     // If BYTE_NUMBER=1, LOW_ADD was just sent.
          if (I2cCommand & 0x01){   // If R/W=READ, sent repeated START.
            STO = 0;
            STA = 1;
          } else {
            SMB0DAT = I2cBuffer[I2cPtr++];
            I2cByteNumber--;        // Next state is 0
          }
          break;
        case 0:                     // If BYTE_NUMBER=0, transfer is finished.
          I2cDataLen--;
          if (I2cDataLen)
            SMB0DAT = I2cBuffer[I2cPtr++];
          else {
            STO = 1;
            SM_BUSY = 0;            // Free SMBus
          }
      }
      break;


      // Master Transmitter: Data byte transmitted.  NACK received.
      // Slave not responding or write protected.
    case SMB_MTDBNACK:
      I2cError = I2C_ERR_WRITE_DATA_NACKED;
      STO = 1;                      // Reset communication.
      SM_BUSY = 0;
      break;

      // Master Transmitter: Arbitration lost.
      // Should not occur.  If so, restart transfer.
    case SMB_MTARBLOST:
      I2cError = I2C_ERR_MT_ATBITRATION_LOST;
      STO = 1;
      SM_BUSY = 0;
      break;

      // Master Receiver: Slave address + READ transmitted.  ACK received.
      // Depending on how much data there is left we either set AA = ACK
      // or we clear it which will indicate that the next byte is our last.
    case SMB_MRADDACK:
      I2cDataLen--;
      if (I2cDataLen)
        AA = 1;
      else
        AA = 0;                       // NACK sent on acknowledge cycle.
      break;

      // Master Receiver: Slave address + READ transmitted.  NACK received.
      // Slave not responding.  Send repeated start to try again.
    case SMB_MRADDNACK:
      I2cError = I2C_ERR_SLAVE_NOT_RESPONDING;
      STO = 1;
      SM_BUSY = 0;
      break;

      // Data byte received.  ACK transmitted.
      // This is were we end up when we have received a data byte after the
      // first data byte.
    case SMB_MRDBACK:
      I2cDataLen--;
      I2cBuffer[I2cPtr++] = SMB0DAT;
      if (I2cDataLen)
        AA = 1;
      else
        AA = 0;
      break;

      // Data byte received.  NACK transmitted.
      // Read operation has completed.  Read data register and send STOP.
    case SMB_MRDBNACK:
      I2cBuffer[I2cPtr] = SMB0DAT;
      STO = 1;
      SM_BUSY = 0;                  // Free SMBus
      break;

      // All other status codes meaningless in this application. Reset communication.
    default:
      I2cError = I2C_ERR_INVALID_I2C_OPERATION;
      STO = 1;                      // Reset communication.
      SM_BUSY = 0;
      break;
  }

  SI=0;                               // clear interrupt flag
}

// EOF
