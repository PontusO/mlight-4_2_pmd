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

#include "system.h"
#include "iet_debug.h"
#include "ramp_mgr.h"
#include "pca.h"
#include "string.h"
#include "stdlib.h"
#include "lightlib.h"
#include "swtimers.h"

/* Function prototypes */
void ramp_stop (void);
void ramp_trigger (void *ldata);

/*
 * Locally used data
 */
static ramp_mgr_t *ramp_mgr_tab[CFG_NUM_PWM_DRIVERS];
static u8_t num_mgrs = 0;
/* The action manager handle */
static action_mgr_t  rampmgr;
static const char *ramp_name = "Light Ramp";

/*
 * Initialize the ramp manager
 */
void init_ramp_mgr(ramp_mgr_t *rmgr) __reentrant __banked
{
  static first = 1;
  u8_t channel = rmgr->channel;

  rampmgr.base.type = EVENT_ACTION_MANAGER;
  rampmgr.base.name = action_base_name_dimmable;
  rampmgr.type = ATYPE_RAMP_ACTION;
  rampmgr.action_name = ramp_name;
  rampmgr.vt.stop_action = ramp_stop;
  rampmgr.vt.trigger_action = ramp_trigger;

  if (channel < CFG_NUM_PWM_DRIVERS) {
    A_(printf (__AT__ " Initializing ramp mgr %p on channel %d\n",
               rmgr, channel);)
    memset (rmgr, 0, sizeof *rmgr);
    PT_INIT(&rmgr->pt);
    rmgr->channel = channel;
    /* Also put a pointer to the instance in our local instance table */
    ramp_mgr_tab[channel] = rmgr;
    num_mgrs++;
  } else {
    A_(printf ("Incorrect channel %d!\n", channel);)
    return;
  }

  if (first && evnt_register_handle(&rampmgr) < 0) {
    first = 0;
    A_(printf (__AT__ " Could not register ramp manager !\n");)
    return;
  }
  first = 0;
}

/* No thread (yet) to interupt so we don't do anything here */
void ramp_stop (void) __reentrant
{
}

/*
 * This is the trigger function that is called from the event switch.
 */
void ramp_trigger (void *input) __reentrant
{
  char rmgr;
  act_ramp_data_t *rampdata = (act_ramp_data_t *)input;

  A_(printf(__AT__ " Starting ramp on channel %d\n",
          (int)rampdata->channel);)
  rmgr = rampdata->channel - 1;

  /* Only update ramp managers if rmgr is within valid range */
  if (rmgr < CFG_NUM_PWM_DRIVERS) {
    ramp_mgr_tab[rmgr]->rampto = rampdata->rampto;
    ramp_mgr_tab[rmgr]->rate = rampdata->rate;
    ramp_mgr_tab[rmgr]->step = rampdata->step;
    ramp_mgr_tab[rmgr]->signal = 1;

    B_(printf (__AT__ " Ramp Data:\n");)
    B_(printf (__AT__ "  Ramp to   : %d\n", rampdata->rampto);)
    B_(printf (__AT__ "  Ramp rate : %d\n", rampdata->rate);)
    B_(printf (__AT__ "  Ramp step : %d\n", rampdata->step);)
  }
}

ramp_mgr_t *get_ramp_mgr (u8_t channel) __reentrant banked
{
  ramp_mgr_t *ptr;

  if (channel >= CFG_NUM_PWM_DRIVERS)
    return NULL;

  ptr = ramp_mgr_tab[channel];
  A_(printf (__AT__ " Returning requested ramp manager for channel %d @ %p\n",
             ptr->channel, ptr);)

  return ptr;
}

/*
 * This thread performs the actual ramping of the light value
 */
#pragma save
#pragma nogcse
PT_THREAD(handle_ramp_mgr(ramp_mgr_t *rmgr) __reentrant __banked)
{
  PT_BEGIN(&rmgr->pt);

  A_(printf (__AT__ " Starting ramp manager %p on channel %d\n",
             rmgr, rmgr->channel);)

  while (1)
  {
    /* Wait for a ramp command to arrive */
    PT_WAIT_UNTIL (&rmgr->pt, rmgr->signal);
    if (rmgr->signal == RAMP_CMD_START) {
      A_(printf (__AT__ " Received a start ramp request on channel %d\n",
                 rmgr->channel);)
      /* Reset the signal */
      rmgr->signal = RAMP_CMD_RESET;

      /* Get the proper ramp control instance */
      rmgr->rctrl = ramp_ctrl_get_ramp_ctrl (rmgr->channel);
      /* If the controller is running, send a stop signal and wait for it to
       * stop. */
      if (ramp_ctrl_get_state (rmgr->rctrl) == RAMP_STATE_RAMPING) {
        ramp_ctrl_send_stop (rmgr->rctrl);
        PT_WAIT_UNTIL (&rmgr->pt,
            ramp_ctrl_get_state (rmgr->rctrl) == RAMP_STATE_DORMANT);
      }

      /* Get current light intensity */
      rmgr->intensity = ledlib_get_light_percentage(rmgr->channel);
      A_(printf (__AT__ " Start intensity %d\n", rmgr->intensity);)
      if (rmgr->intensity != rmgr->rampto) {
        /* Check difference between new and current */
        if ((rmgr->rampto - rmgr->intensity) < 0) {
          rmgr->step *= -1;
        }
        A_(printf (__AT__ " Ramping to %d\n", rmgr->rampto);)

        /* Copy instance data */
        rmgr->rctrl->channel = rmgr->channel;
        rmgr->rctrl->intensity = rmgr->intensity;
        rmgr->rctrl->rate = rmgr->rate;
        rmgr->rctrl->step = rmgr->step;
        rmgr->rctrl->rampto = rmgr->rampto;

        ramp_ctrl_send_start (rmgr->rctrl);
        /* First we need to wait for the controller to acknowledge the signal */
        PT_WAIT_UNTIL (&rmgr->pt,
            ramp_ctrl_get_state (rmgr->rctrl) == RAMP_STATE_RAMPING);
        /* Now we wait while the ramp controller is processing */
        PT_WAIT_WHILE (&rmgr->pt,
            ramp_ctrl_get_state (rmgr->rctrl) == RAMP_STATE_RAMPING);
      }
    }
  }

  PT_END(&rmgr->pt);
}
#pragma restore

/* EOF */
