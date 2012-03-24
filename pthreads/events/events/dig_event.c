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
//#define PRINT_A     // Enable A prints

#include <stdlib.h>

#include "system.h"
#include "flash.h"
#include "iet_debug.h"
#include "event_switch.h"
#include "dig_event.h"
#include "swtimers.h"

#include "comparator.h"
#include "dac.h"

/* Event handle */
static event_prv_t digevent[NUMBER_OF_DIG_INPUTS];
static const char *base_name = "Digital Input";
static const char *dig_names[] = { "Push Button 1", "Push Button 2" };

/*
 * Initialize the dig_event pthread
 */
void init_dig_event(dig_event_t *dig_event) __reentrant __banked
{
  char i;

  PT_INIT(&dig_event->pt);

  /* Initialize the event data */
  for (i=0;i<NUMBER_OF_DIG_INPUTS;i++) {
    digevent[i].base.type = EVENT_EVENT_PROVIDER;
    digevent[i].base.name = base_name;
    digevent[i].type = ETYPE_DIG_INPUT_EVENT;
    digevent[i].event_name = (char*)dig_names[i];
  }
}

/*
 * The pir main thread
 */
PT_THREAD(handle_dig_event(dig_event_t *dig_event) __reentrant __banked)
{
  char i;

  PT_BEGIN(&dig_event->pt);

  /* Register the event provider */
  for (i=0; i<NUMBER_OF_DIG_INPUTS; i++)
    evnt_register_handle(&digevent[i]);

  while (1)
  {
    PT_YIELD (&dig_event->pt);
  }

  PT_END(&dig_event->pt);
}

/* EOF */
