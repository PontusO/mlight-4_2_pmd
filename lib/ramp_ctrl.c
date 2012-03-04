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

/* Ramp controller
 *
 * This module implements the LED ramp_ctrl controller. This is the thread that
 * does the actual ramping of the LED.
 */
#pragma codeseg APP_BANK
//#define PRINT_A     // Enable A prints

#include "system.h"
#include "iet_debug.h"
#include "ramp_ctrl.h"
#include "lightlib.h"
#include "swtimers.h"

/**
 * Local table containing the registered ramp_ctrl managers
 */
static ramp_ctrl_t *ramp_ctrl_tab[CFG_NUM_PWM_DRIVERS];
char *ramp_ctrl_states_str[] = {"Steady", "Increasing", "Decreasing"};

/*
 * Initialize the ramp_ctrl pthread
 */
void init_ramp_ctrl(ramp_ctrl_t *ramp_ctrl) __reentrant __banked
{
  u8_t channel = ramp_ctrl->channel;
  ramp_ctrl->signal = RAMP_SIG_NONE;
  ramp_ctrl->state = RAMP_STATE_DORMANT;
  PT_INIT(&ramp_ctrl->pt);

  if (ramp_ctrl->channel < CFG_NUM_PWM_DRIVERS) {
    ramp_ctrl_tab[ramp_ctrl->channel] = ramp_ctrl;
  } else {
    A_(printf (__FILE__ " Channel is incorrect !\n");)
  }
}

/**
 * ramp_ctrl_get_ramp_ctrl (u8_t channel)
 *
 * Retrieves the ramp_ctrl instance pointer for the specified channel
 *
 */
ramp_ctrl_t *ramp_ctrl_get_ramp_ctrl (u8_t channel) __reentrant __banked
{
  if (channel < CFG_NUM_PWM_DRIVERS)
    return ramp_ctrl_tab[channel];
  return NULL;
}

/**
 * ramp_ctrl_send_start
 *
 * Sends a start signal to the specified ramp control manager
 *
 */
void ramp_ctrl_send_start (ramp_ctrl_t *rcmgr) __reentrant __banked
{
  rcmgr->signal = RAMP_SIG_START;
}

/**
 * ramp_ctrl_send_stop
 *
 * Sends a stop signal to the specified ramp control manager
 *
 */
void ramp_ctrl_send_stop (ramp_ctrl_t *rcmgr) __reentrant __banked
{
  rcmgr->signal = RAMP_SIG_STOP;
}

/**
 * ramp_ctrl_get_state
 *
 * Gets the current state of the ramp controller
 *
 */
u8_t ramp_ctrl_get_state (ramp_ctrl_t *rcmgr) __reentrant __banked
{
  return rcmgr->state;
}

/*
 * return a textual representation of the current state of the controller
 */
char *ramp_ctrl_get_state_str (ramp_ctrl_t *rcmgr) __reentrant __banked
{
  if (rcmgr->state == RAMP_STATE_DORMANT)
    return ramp_ctrl_states_str[0];
  else
    if (rcmgr->step > 0)
      return ramp_ctrl_states_str[1];
    else
      return ramp_ctrl_states_str[2];
}


/**
 * The mainloop for the ramp controller.
 *
 * Description:
 * The ramp controller is responsible for ramping the light intensity on a
 * specified LED channel. If the system contains more than one LED channel
 * this thread should be instantiated for as many times as there are LED
 * channels.
 *
 * The controller will listen for incoming signals (use ramp_ctrl_send_signal
 * for that) and do the ramp.
 */
#pragma save
#pragma nogcse
PT_THREAD(handle_ramp_ctrl(ramp_ctrl_t *ramp_ctrl) __reentrant __banked)
{
  PT_BEGIN(&ramp_ctrl->pt);

  /* Allocate a timer for this thread */
  ramp_ctrl->timer = alloc_timer();
  A_(printf (__FILE__ " Starting ramp controller %p on channel %d\n",
             ramp_ctrl, ramp_ctrl->channel);)

  while (1) {
    PT_WAIT_UNTIL (&ramp_ctrl->pt, ramp_ctrl->signal == RAMP_SIG_START);
    /* Reset signal and set state to ramping */
    ramp_ctrl->signal = RAMP_SIG_NONE;
    ramp_ctrl->state = RAMP_STATE_RAMPING;

    A_(printf (__FILE__ " Starting a ramp on channel %d\n",
               ramp_ctrl->channel);)

    do {
      set_timer (ramp_ctrl->timer, ramp_ctrl->rate, NULL);

      if (ramp_ctrl->step >= 0) {
        if (ramp_ctrl->intensity > ramp_ctrl->rampto) {
          ramp_ctrl->intensity = ramp_ctrl->rampto;
        }
      } else {
        if (ramp_ctrl->intensity < ramp_ctrl->rampto) {
          ramp_ctrl->intensity = ramp_ctrl->rampto;
        }
      }

      ramp_ctrl->lp.channel = ramp_ctrl->channel;
      ramp_ctrl->lp.level_percent = ramp_ctrl->intensity;
      ledlib_set_light_percentage_log (&ramp_ctrl->lp);
      /* Time wait */
      PT_WAIT_UNTIL (&ramp_ctrl->pt, (get_timer(ramp_ctrl->timer) == 0) ||
                                 ramp_ctrl->signal == RAMP_SIG_STOP);
      ramp_ctrl->intensity += ramp_ctrl->step;

    } while ((ramp_ctrl->intensity != ramp_ctrl->rampto) &&
             (ramp_ctrl->signal == RAMP_SIG_NONE));

    /* Set the final value calculated in the loop above */
    if (ramp_ctrl->signal == RAMP_SIG_NONE) {
      ramp_ctrl->lp.channel = ramp_ctrl->channel;
      ramp_ctrl->lp.level_percent = ramp_ctrl->intensity;
      ledlib_set_light_percentage_log (&ramp_ctrl->lp);
    } else {
      ramp_ctrl->signal = RAMP_SIG_NONE;
    }
    /* Make the manager dormant */
    ramp_ctrl->state = RAMP_STATE_DORMANT;
  }
  PT_END(&ramp_ctrl->pt);
}
#pragma restore

/* EOF */
