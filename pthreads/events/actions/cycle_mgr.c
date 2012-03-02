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

/* Light Cycle Manager
 *
 * This module is part of the event system.
 * It is an action manager that will provide the system with the
 * ability to perform a on/off cycle of light.
 *
 */
#pragma codeseg APP_BANK
#define PRINT_A     // Enable A prints

#include "system.h"
#include "iet_debug.h"
#include "cycle_mgr.h"
#include "event_switch.h"

/*
 * Initialize the cycle_mgr pthread
 */
void init_cycle_mgr(cycle_mgr_t *cycle_mgr) __reentrant __banked
{
  PT_INIT(&cycle_mgr->pt);
}

/**
  * Stop function
  */
void cycle_mgr_stop (void) __reentrant
{
  A_(printf (__FILE__ " Entered the stop function !\n");)
}

/**
 * Trigger function
 */
void cycle_mgr_trigger (void *input) __reentrant
{
  (void)input;
  A_(printf (__FILE__ " Entered the trigger function !\n");)
}

PT_THREAD(handle_cycle_mgr(cycle_mgr_t *cycle_mgr) __reentrant __banked)
{
  PT_BEGIN(&cycle_mgr->pt);

  printf ("Starting cycle_mgr pthread!\n");

  while (1)
  {
    PT_YIELD (&cycle_mgr->pt);
  }

  PT_END(&cycle_mgr->pt);
}

/* EOF */
