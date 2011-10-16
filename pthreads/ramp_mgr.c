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
#include <system.h>
#include "iet_debug.h"
#include <ramp_mgr.h>
#include <pca.h>
#include <string.h>
#include <stdlib.h>
#include <lightlib.h>
#include <swtimers.h>

/*
 * Locally used data
 */
static ramp_mgr_t *ramp_mgr_tab[CFG_NUM_PWM_DRIVERS];
static u8_t num_mgrs = 0;

/*
 * Initialize the ramp manager
 */
void init_ramp_mgr(ramp_mgr_t *rmgr, u8_t channel) __reentrant banked
{
  if (channel < CFG_NUM_PWM_DRIVERS) {
    memset (rmgr, 0, sizeof *rmgr);
    PT_INIT(&rmgr->pt);
    rmgr->channel = channel;
    /* Also put a pointer to the instance in our local instance table */
    ramp_mgr_tab[channel] = rmgr;
    num_mgrs++;
  } else {
    A_(printf ("Incorrect channel !\n");)
  }
}

ramp_mgr_t *get_ramp_mgr (u8_t channel) __reentrant banked
{
  if (channel >= CFG_NUM_PWM_DRIVERS)
    return NULL;

  return ramp_mgr_tab[channel];
}

PT_THREAD(handle_ramp_mgr(ramp_mgr_t *rmgr) __reentrant banked)
{

  PT_BEGIN(&rmgr->pt);

  rmgr->timer = alloc_timer();

  while (1)
  {
    /* Wait for a ramp command to arrive */
    PT_WAIT_UNTIL (&rmgr->pt, rmgr->signal);
    if (rmgr->signal == RAMP_CMD_START) {
      /* Reset the signal */
      rmgr->signal = RAMP_CMD_RESET;
      /* Setup and start a ramp job */
      /* Get current light intensity */
      rmgr->intensity = ledlib_get_light_percentage(rmgr->channel);
      /* Check difference between new and current */
      rmgr->cnt = rmgr->rampto - rmgr->intensity;
      if (rmgr->cnt < 0) {
        rmgr->step = -1;
        rmgr->cnt *= -1;
      } else
        rmgr->step = 1;

      while (rmgr->cnt) {
        ledlib_set_light_percentage_log (rmgr->channel, rmgr->intensity);
        set_timer (rmgr->timer, rmgr->rate, NULL);

        PT_WAIT_UNTIL (&rmgr->pt, (get_timer(rmgr->timer) == 0));
        rmgr->intensity = rmgr->intensity + rmgr->step;
        rmgr->cnt = rmgr->cnt - 1;
      }
    }
  }

  PT_END(&rmgr->pt);
}

#if 0
/*
 * The timer 2 interrupt routine
 */
void TIMER2_ISR (void) __interrupt TF2_VECTOR __using 2
{
  TF2 = 0;
  putchar ('.');
}
#endif
/* EOF */
