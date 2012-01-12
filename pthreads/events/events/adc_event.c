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
#pragma codeseg APP_BANK
#define PRINT_A     // Enable A prints

#include <stdlib.h>

#include "system.h"
#include "iet_debug.h"
#include "adc_event.h"
#include "event_switch.h"
#include "absval_mgr.h"
#include "adc.h"

#define CUT_OUT_SPAN      4
#define KNEE_LEVEL        512
#define ADC_RESOLUTION    10
#define VALUE_SHIFT       (16 - ADC_RESOLUTION)

/* Local prototypes */
void *adc_event_get_dptr(void);

/* Event handle */
static event_prv_t adcevents[4];
static const char *adc_names[4] =
  { "Potentiometer Channel 1", "Potentiometer Channel 2",
    "Potentiometer Channel 3", "Potentiometer Channel 4" };

/*
 * Initialize the adc_event pthread
 */
void init_adc_event(adc_event_t *adc_event) __reentrant __banked
{
  char i;

  PT_INIT(&adc_event->pt);

  /* Initialize the event data */
  for (i=0; i<CFG_NUM_POTS; i++) {
    adcevents[i].base.type = EVENT_EVENT_PROVIDER;
    adcevents[i].type = ETYPE_POTENTIOMETER_EVENT;
    adcevents[i].base.name = (char*)adc_names[i];
//    adcevents[i].evt_data = (adc_input_t *)&adc_input[i];
    adcevents[i].signal = 0;
    adcevents[i].vt.get_edptr = adc_event_get_dptr;
  }

  /* This will ensure that the light settings will update on start */
  for (i=0;i<CFG_NUM_POTS;i++)
    adc_event->prev_pot_val[i] = 100;
}

PT_THREAD(handle_adc_event(adc_event_t *adc_event) __reentrant __banked)
{
  char i;

  PT_BEGIN(&adc_event->pt);

  /* Register the event provider */
  for (i=0; i<CFG_NUM_POTS; i++)
    evnt_register_handle(&adcevents[i]);

  A_(printf (__FILE__ " Starting adc_event pthread, handle ptr %p!\n", &adcevents);)

  while (1)
  {
    PT_WAIT_UNTIL (&adc_event->pt, SIG_NEW_ADC_VALUE_RECEIVED != -1);
    adc_event->channel = SIG_NEW_ADC_VALUE_RECEIVED;
    SIG_NEW_ADC_VALUE_RECEIVED = -1;
    adc_event->pot_val = adc_get_average(adc_event->channel);
    if (abs(adc_event->pot_val - adc_event->prev_pot_val[adc_event->channel]) > 4) {
      u16_t temp = adc_event->pot_val;
      /* To create a flicker free lights out ^*/
      if (adc_event->pot_val < CUT_OUT_SPAN)
        adc_event->pot_val = 0;
      else {
#if 0
        if (adc_event->pot_val < KNEE_LEVEL)
          adc_event->pot_val = adc_event->pot_val - CUT_OUT_SPAN;
        else
          adc_event->pot_val = ((adc_event->pot_val - KNEE_LEVEL) << VALUE_SHIFT) + KNEE_LEVEL;
#else
        adc_event->pot_val = adc_event->pot_val << VALUE_SHIFT;
#endif
      }
      /* Make sure maximum value is maxed out */
      if (adc_event->pot_val >= 0xff00)
        adc_event->pot_val = 0xffff;
      A_(printf (__FILE__ " pot_val = %04x\n", adc_event->pot_val);)

      /* Get the related data pointer from the event system */
      adc_event->dptr = (adc_input_t*)event_get_dptr (&adcevents[i]);

      adc_event->dptr.
      put[adc_event->channel].abs_data.value = adc_event->pot_val;
      adcevents[adc_event->channel].signal = 1;
      adc_event->prev_pot_val[adc_event->channel] = temp;
    }
  }

  PT_END(&adc_event->pt);
}

/* EOF */
