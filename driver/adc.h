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
#ifndef ADC_H_INCLUDED
#define ADC_H_INCLUDED

#define MAX_SAMPLES   32

extern char SIG_NEW_ADC_VALUE_RECEIVED;

enum {
  ADC_OK = 0x00,
  ADC_ERROR_WRONG_CHANNEL,
};

struct adc {
  u16_t last_sample;
  u16_t latest_average;
  u8_t  channel;
  u16_t values[MAX_SAMPLES];
  u8_t  r_ptr;
  u8_t  w_ptr;
  u8_t  n;
};

void adc_init(void);
void adc_start_conversion(u8_t channel) __reentrant banked;
u16_t adc_get_average(u8_t channel) __reentrant banked;
u16_t adc_get_last_sample(u8_t channel) __reentrant banked;
int get_temperature(u8_t channel) __reentrant banked;

#endif // ADC_H_INCLUDED
