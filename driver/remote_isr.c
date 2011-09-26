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
#include "system.h"
#include "pt.h"
#include "remote.h"

extern unsigned char remote_state;
extern xdata u16_t time_array[256];
extern unsigned char array_ptr;
extern unsigned char length;

/*
 * 0x1b Timer 1 overflow
 */
void Timer1_ISR (void) interrupt TF1_VECTOR using 0
{
  /* Stop the timer */
  TR1 = STOP;
  /* Reset the remote receiver state machine */
  remote_state = REMOTE_DORMANT;
  /* trigger the decoder process to decode the received ir command */
  SIG_REMOTE_PROCESS = 1;
  /* Update the packet length */
  length = array_ptr;
}

/*
 * PCA Interrupt service routine
 */
void PCA_ISR (void) interrupt PCA_VECTOR using 0
{
  static unsigned int temp;

  switch(remote_state)
  {
    case REMOTE_DORMANT:
      remote_state = REMOTE_STARTBIT;
      PCA0H = 0;
      PCA0L = 0;
      time_array[0] = 0;
      array_ptr = 2;
      /* Start the bit timeout timer */
      TL1 = 0xff;
      TH1 = 0x05;
      TR1 = RUN;
      break;

    case REMOTE_STARTBIT:
      remote_state = REMOTE_MEASURING;
      /* calculate the length of the start bit, multiply by 4 and use it as a time
       * out to detect the end of a remote key press.
       * Remember to store the length of the start bit as well */
      time_array[1] = PCA0CPL0 | (PCA0CPH0 << 8);
      temp = -((time_array[1] - time_array[0]) * 4);
      /* Reset bit timeout timer */
      TR1 = STOP;
      TL1 = temp & 0xff;
      TH1 = temp >> 8;
      TR1 = RUN;
      break;

    case REMOTE_MEASURING:
      /* Store everything in the evaluation buffer */
      /* FIXME: take care of array_ptr overflowing */
      time_array[array_ptr++] = PCA0CPL0 | (PCA0CPH0 << 8);
      /* Reset bit timeout timer */
      TR1 = STOP;
      TL1 = temp & 0xff;
      TH1 = temp >> 8;
      TR1 = RUN;
      break;

    default:
      break;
  }

  CCF0 = 0;
}
