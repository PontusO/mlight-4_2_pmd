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

#include "system.h"
#include "iet_debug.h"
#include "ramp_mgr.h"
#include "pca.h"
#include "string.h"
#include "stdlib.h"
#include "lightlib.h"
#include "swtimers.h"

/*
 * Locally used data
 */
static ramp_mgr_t *ramp_mgr_tab[CFG_NUM_PWM_DRIVERS];
static u8_t num_mgrs = 0;

/*
 * Initialize the ramp manager
 */
void init_ramp_mgr(ramp_mgr_t *rmgr) __reentrant banked
{
  u8_t channel = rmgr->channel;

  if (channel < CFG_NUM_PWM_DRIVERS) {
    A_(printf (__FILE__ " Initializing ramp mgr %p on channel %d\n",
               rmgr, channel);)
    memset (rmgr, 0, sizeof *rmgr);
    PT_INIT(&rmgr->pt);
    rmgr->channel = channel;
    /* Also put a pointer to the instance in our local instance table */
    ramp_mgr_tab[channel] = rmgr;
    num_mgrs++;
  } else {
    A_(printf ("Incorrect channel %d!\n", channel);)
  }
}

ramp_mgr_t *get_ramp_mgr (u8_t channel) __reentrant banked
{
  if (channel >= CFG_NUM_PWM_DRIVERS)
    return NULL;

  return ramp_mgr_tab[channel];
}

/* Debug lines for catching compiler generated failures.
 * If you think that values are being trashed in this function
 * during runtime, this macro can be enabled and trace the required
 * variable.
 */
#define TMR_GUARD if (ramp->timer != tmptim) { A_(printf (__FILE__ " Timer Error line %d: expected %d found %d, %p\n", __LINE__, (int)tmptim, (int)ramp->timer, ramp);) ; goto exit; }
//#define TMR_GUARD

static u8_t tmptim;
PT_THREAD(do_ramp(ramp_t *ramp) __reentrant)
{
  PT_BEGIN (&ramp->pt);
  {
    ramp->timer = alloc_timer();
    tmptim = ramp->timer;
    ramp->signal = RAMP_CMD_RESET;

    A_(printf (__FILE__ " allocated timer %d for ramp manager %p\n", ramp->timer, ramp);)

    do {
      TMR_GUARD
      set_timer (ramp->timer, ramp->rate, NULL);

      TMR_GUARD
      if (ramp->step >= 0) {
        TMR_GUARD
        if (ramp->intensity > ramp->rampto) {
          TMR_GUARD
          ramp->intensity = ramp->rampto;
        }
      } else {
        TMR_GUARD
        if (ramp->intensity < ramp->rampto) {
          TMR_GUARD
          ramp->intensity = ramp->rampto;
        }
      }

      TMR_GUARD
      ledlib_set_light_percentage_log (ramp->channel, ramp->intensity);
      TMR_GUARD
      PT_WAIT_UNTIL (&ramp->pt, (get_timer(ramp->timer) == 0) ||
                                 ramp->signal == RAMP_CMD_STOP);
      TMR_GUARD
      ramp->intensity += ramp->step;
      TMR_GUARD
    } while ((ramp->intensity != ramp->rampto) &&
             (ramp->signal == RAMP_CMD_RESET));

    if (ramp->signal == RAMP_CMD_RESET) {
      A_(printf (__FILE__ " Set light: channel %d, intensity %d, target %d\n",
                (int)ramp->channel, (int)ramp->intensity, (int)ramp->rampto);)
      ledlib_set_light_percentage_log (ramp->channel, ramp->intensity);
    }
exit:
    free_timer(ramp->timer);
  }
  PT_END (&ramp->pt);
}

PT_THREAD(handle_ramp_mgr(ramp_mgr_t *rmgr) __reentrant banked)
{
  PT_BEGIN(&rmgr->pt);

  while (1)
  {
    /* Wait for a ramp command to arrive */
    PT_WAIT_UNTIL (&rmgr->pt, rmgr->signal);
    if (rmgr->signal == RAMP_CMD_START) {
      A_(printf (__FILE__ " Received a start ramp request on channel %d\n",
                 rmgr->channel);)
      /* Reset the signal */
      rmgr->signal = RAMP_CMD_RESET;
      /* Setup and start a ramp job */
      /* Get current light intensity */
      rmgr->intensity = ledlib_get_light_percentage(rmgr->channel);
      A_(printf (__FILE__ " Start intensity %d\n", rmgr->intensity);)
      /* Check difference between new and current */
      rmgr->cnt = rmgr->rampto - rmgr->intensity;
      A_(printf (__FILE__ " Ramping to %d\n", rmgr->rampto);)

      if (rmgr->cnt < 0) {
        rmgr->step *= -1;
        rmgr->cnt = abs(rmgr->cnt);
      }

      rmgr->ramp.channel = rmgr->channel;
      rmgr->ramp.intensity = rmgr->intensity;
      rmgr->ramp.rate = rmgr->rate;
      rmgr->ramp.step = rmgr->step;
      rmgr->ramp.rampto = rmgr->rampto;

      PT_SPAWN (&rmgr->pt, &rmgr->ramp.pt, do_ramp(&rmgr->ramp));
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
