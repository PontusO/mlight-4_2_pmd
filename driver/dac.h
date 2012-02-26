/*
 * Copyright (c) 2011, Pontus Oldberg.
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
#ifndef DAC_H_INCLUDED
#define DAC_H_INCLUDED

#define DAC_NUM_DACS        0x02

/* Control registers default values */
#define DACXCN_DEFAULT_VALUE  0x02

#define DAC_ENABLE          0x80
#define DAC_UPDATE_DAC_H    0x00
#define DAC_UPDATE_TIMER3   0x08
#define DAC_UPDATE_TIMER4   0x10
#define DAC_UPDATE_TIMER2   0x18
#define DAC_MODE_0          0x00
#define DAC_MODE_1          0x01
#define DAC_MODE_2          0x02
#define DAC_MODE_3          0x03
#define DAC_MODE_4          0x04

#define DAC_MAX_SCALE       4095
#define DAC_MID_SCALE       (DAC_MAX_SCALE >> 1)

enum dac_status_t {
  DAC_FREE = 0x00,
  DAC_ALLOCATED  = 0x01,
};

enum dac_errors_t {
  DAC_ERR_OK = 0x00,
  DAC_ERR_ALREADY_IN_USE = 0x01,
  DAC_ERR_NO_SUCH_DEVICE = 0x02,
};

void init_dacs (void);
char allocate_dac (u8_t dac);
char free_dac (u8_t dac);
char write_dac (u8_t dac, u16_t value);

#endif // DAC_H_INCLUDED
