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
#pragma codeseg APP_BANK
//#define PRINT_A     // Enable A prints

#include <system.h>
#include "iet_debug.h"
#include <adc_mon.h>
#include <adc.h>
#include <pca.h>
#include <string.h>
#include <stdlib.h>
#include "lightlib.h"

#define CUT_OUT_SPAN      4
#define KNEE_LEVEL        512
#define ADC_RESOLUTION    10
#define VALUE_SHIFT       (16 - ADC_RESOLUTION)

/*
 * Initialize the adc monitor
 */
void init_adc_mon(adc_mon_t *adc_mon) __reentrant banked
{
  u8_t i;
  memset (adc_mon, 0, sizeof *adc_mon);
  PT_INIT(&adc_mon->pt);

  /* This will ensure that the light settings will update on start */
  for (i=0;i<CFG_NUM_POTS;i++)
    adc_mon->prev_pot_val[i] = 100;
}

PT_THREAD(handle_adc_mon(adc_mon_t *adc_mon) __reentrant banked)
{
  PT_BEGIN(&adc_mon->pt);

  printf ("Starting the adc monitor !\n");

  while (1)
  {
    /* Wait for a new adc value to arrive o channel 0 */
    PT_WAIT_UNTIL (&adc_mon->pt, SIG_NEW_ADC_VALUE_RECEIVED != -1);
    adc_mon->channel = SIG_NEW_ADC_VALUE_RECEIVED;
    SIG_NEW_ADC_VALUE_RECEIVED = -1;
    adc_mon->pot_val = adc_get_average(adc_mon->channel);
    if (abs(adc_mon->pot_val - adc_mon->prev_pot_val[adc_mon->channel]) > 4) {
      u16_t temp = adc_mon->pot_val;
      /* To create a flicker free lights out ^*/
      if (adc_mon->pot_val < CUT_OUT_SPAN)
        adc_mon->pot_val = 0;
      else {
#if 0
        if (adc_mon->pot_val < KNEE_LEVEL)
          adc_mon->pot_val = adc_mon->pot_val - CUT_OUT_SPAN;
        else
          adc_mon->pot_val = ((adc_mon->pot_val - KNEE_LEVEL) << VALUE_SHIFT) + KNEE_LEVEL;
#else
        adc_mon->pot_val = adc_mon->pot_val << VALUE_SHIFT;
#endif
      }
      /* Make sure maximum value is maxed out */
      if (adc_mon->pot_val >= 0xff00)
        adc_mon->pot_val = 0xffff;
      if (!adc_mon->channel)
        printf ("pot_val = %04x\n", adc_mon->pot_val);
      ledlib_set_light_abs (adc_mon->channel, (u16_t)adc_mon->pot_val);
      adc_mon->prev_pot_val[adc_mon->channel] = temp;
    }
  }
  PT_END(&adc_mon->pt);
}

/* EOF */
