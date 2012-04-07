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
//#define PRINT_A     // Enable A prints

#include "system.h"
#include "iet_debug.h"
#include "cycle_mgr.h"
#include "event_switch.h"
#include "swtimers.h"

#if 1
#define CHECK_AND_RESTART()   if (cycle_mgr->signal == CYC_SIG_STOP) { \
                                    cycle_mgr->signal = CYC_SIG_NONE; \
                                    goto restart; \
                              }
#else
#define CHECK_AND_RESTART()
#endif

/* Function prototypes */
void cycle_mgr_trigger (struct rule *rule) __reentrant;

/* The cycle manager action handle */
static action_mgr_t  cycleaction;
static const char *cycle_name = "Light Ramp";

static cycle_mgr_t *cycle_mgr_tab[CFG_NUM_PWM_DRIVERS];

/*
 * Initialize the cycle_mgr pthread
 */
void init_cycle_mgr(cycle_mgr_t *cycle_mgr) __reentrant __banked
{
  static u8_t first = 1;

  PT_INIT(&cycle_mgr->pt);
  cycle_mgr->signal = CYC_SIG_NONE;
  cycle_mgr->state = CYCLE_STATE_DORMANT;

  cycleaction.base.type = EVENT_ACTION_MANAGER;
  cycleaction.base.name = action_base_name_dimmable;
  cycleaction.type = ATYPE_CYCLE_ACTION;
  cycleaction.action_name = (char*)cycle_name;
  cycleaction.vt.trigger_action = cycle_mgr_trigger;

  if (cycle_mgr->cdata.channel < CFG_NUM_PWM_DRIVERS) {
    cycle_mgr_tab[cycle_mgr->cdata.channel] = cycle_mgr;
  } else {
    A_(printf (__AT__ "Channel is incorrect !\n");)
  }

  if (first && evnt_register_handle(&cycleaction) < 0) {
    first = 0;
    A_(printf (__AT__ "Could not register cycle manager !\n");)
    return;
  }
  first = 0;
}

/*
 * A stop function. This can for instance be used by an absolute manager
 * to stop any ramping that is going on on a specific channel before updating
 * the absolute value.
 */
char cycle_mgr_stop_channel (u8_t channel) __reentrant __banked
{
  static cycle_mgr_t *cm;

  if (channel < CFG_NUM_PWM_DRIVERS) {
    cm = cycle_mgr_tab[channel];
    cm->signal = CYC_SIG_STOP;
    return 0;
  } else {
    return -1;
  }
}

/**
 * Trigger function
 */
void cycle_mgr_trigger (struct rule *rule) __reentrant
{
  cycle_mgr_t *cmgr;
  act_cycle_data_t *cycdat = (act_cycle_data_t *)rule->action_data;
  rule_data_t *rdata = rule->r_data;

  A_(printf (__AT__ "Entered the trigger function (channel %d) in state: ", cycdat->channel);)

  if (cycdat->channel <= CFG_NUM_PWM_DRIVERS) {
    /* Get manager instance */
    cmgr = cycle_mgr_tab[(cycdat->channel - 1)];
    if (cmgr->state == CYCLE_STATE_WAITING) {
      A_(printf ("CYCLE_STATE_WAITING\n");)
      /* If the ramp is in a waiting state we either restart or continue */
      if (rule->r_data->command == EVENT_USE_CONTINUE)
        cmgr->signal = CYC_SIG_CONTINUE;
      else
        cmgr->signal = CYC_SIG_RESTART;
    } else if (cmgr->state == CYCLE_STATE_DORMANT) {
      A_(printf ("CYCLE_STATE_DORMANT\n");)
      /* If not, we can start normally. */
      cmgr->cdata.channel = cycdat->channel - 1;
      cmgr->cdata.rampto = cycdat->rampto;
      cmgr->cdata.rate = cycdat->rate;
      cmgr->cdata.step = cycdat->step;
      cmgr->cdata.time = cycdat->time;
      cmgr->cdata.mode = cycdat->mode;
      cmgr->signal = CYC_SIG_START;
      cmgr->intensity = ledlib_get_light_percentage(cmgr->cdata.channel);
    } else if (cmgr->state == CYCLE_STATE_RAMPING_DN) {
      A_(printf ("CYCLE_STATE_RAMPING_DN\n");)
      cmgr->cdata.channel = cycdat->channel - 1;
      cmgr->cdata.rampto = cycdat->rampto;
      cmgr->cdata.rate = cycdat->rate;
      cmgr->cdata.step = cycdat->step;
      cmgr->cdata.time = cycdat->time;
      cmgr->cdata.mode = cycdat->mode;
      cmgr->signal = CYC_SIG_RESTART;
    } else if (CYCLE_STATE_RAMPING_UP) {
      cmgr->signal = CYC_SIG_CONTINUE;
    }
  }
}

/**
 * Retrieve the current ramp manager state
 */
u8_t cycle_mgr_get_state (u8_t channel) __reentrant
{
  return cycle_mgr_tab[channel]->state;
}

#pragma save
#pragma nogcse
PT_THREAD(handle_cycle_mgr(cycle_mgr_t *cycle_mgr) __reentrant __banked)
{
  PT_BEGIN(&cycle_mgr->pt);

  A_(printf (__AT__ "Starting cycle_mgr %p on channel %d !\n",
             cycle_mgr, cycle_mgr->cdata.channel);)

  cycle_mgr->tmr = alloc_timer ();

  while (1)
  {
restart:
    /* Make sure no ramp is running at the moment */
    ramp_ctrl_send_stop (cycle_mgr->rctrl);
    PT_WAIT_UNTIL (&cycle_mgr->pt,
        ramp_ctrl_get_state (cycle_mgr->rctrl) == RAMP_STATE_DORMANT);

    /* Inform everyone the we're not doin much */
    cycle_mgr->state = CYCLE_STATE_DORMANT;

    /* Wait to be triggered */
    PT_WAIT_UNTIL (&cycle_mgr->pt, cycle_mgr->signal != CYC_SIG_NONE);

    /* Check for unexcpected signals */
    if (cycle_mgr->signal != CYC_SIG_START) {
      cycle_mgr->signal = CYC_SIG_NONE;
      goto restart;
    }

    /* Indicate that the ramp is going up. */
    cycle_mgr->state = CYCLE_STATE_RAMPING_UP;

    A_(printf (__AT__ "Received a cycle event start trigger !\n");)

    /* Get the proper ramp control instance */
    cycle_mgr->rctrl = ramp_ctrl_get_ramp_ctrl (cycle_mgr->cdata.channel);

    A_(printf (__AT__ "Start intensity %d\n", cycle_mgr->intensity);)

    /* Only do a ramp if the the requested intensity differs from the current */
    if (cycle_mgr->intensity != cycle_mgr->cdata.rampto) {
      A_(printf (__AT__ "Ramping to %d\n", cycle_mgr->cdata.rampto);)

      /* Copy instance data for first ramp */
      if (cycle_mgr->signal == CYC_SIG_START) {
//        cycle_mgr->rctrl->intensity = cycle_mgr->intensity;
        cycle_mgr->rctrl->intensity =
            ledlib_get_light_percentage(cycle_mgr->cdata.channel);
      }
      cycle_mgr->rctrl->channel = cycle_mgr->cdata.channel;
      cycle_mgr->rctrl->rate = cycle_mgr->cdata.rate;
      cycle_mgr->rctrl->rampto = cycle_mgr->cdata.rampto;
      /* What direction do we need to go */
      if (cycle_mgr->rctrl->rampto > cycle_mgr->rctrl->intensity)
        cycle_mgr->rctrl->step = cycle_mgr->cdata.step;
      else
        cycle_mgr->rctrl->step = cycle_mgr->cdata.step * -1;

      /* Reset the signal */
      cycle_mgr->signal = CYC_SIG_NONE;

      /* Start the ramp */
      ramp_ctrl_send_start (cycle_mgr->rctrl);

      /* First we need to wait for the controller to acknowledge the signal */
      PT_WAIT_UNTIL (&cycle_mgr->pt,
          ramp_ctrl_get_state (cycle_mgr->rctrl) == RAMP_STATE_RAMPING);
      /* Now we wait while the ramp controller is processing the on ramp and we
       * have not been interrupted */
      PT_WAIT_WHILE (&cycle_mgr->pt,
          ramp_ctrl_get_state (cycle_mgr->rctrl) == RAMP_STATE_RAMPING &&
                               cycle_mgr->signal == CYC_SIG_NONE);

      CHECK_AND_RESTART();

      /* If we only are doing the intro ramp, return to start again */
      if (cycle_mgr->cdata.mode == CYC_MODE_SINGLE_RAMP) {
        /* Clear signal and state */
        cycle_mgr->signal = CYC_SIG_NONE;
        cycle_mgr->state = CYCLE_STATE_DORMANT;
        /* Go wait for the next signal */
        A_(printf (__AT__ "Restarting ramp loop !\n");)
        goto restart;
      }

      /* If we weren't interrupted during the up ramp we should wait now */
      if (cycle_mgr->signal == CYC_SIG_NONE) {
        /* Clear any pending signals */
        cycle_mgr->signal = CYC_SIG_NONE;
        /* Doing the wait, set the state to reflect that */
        cycle_mgr->state = CYCLE_STATE_WAITING;
again:
        /* Set the waiting time to the requested timeout in minutes */
        set_timer (cycle_mgr->tmr,
                  (timer_time_t)(cycle_mgr->cdata.time * 100 * 60), NULL);
        /* Wait for the timer to reach 0 */
        A_(printf (__AT__ "Starting Timer %lu !\n", get_timer (cycle_mgr->tmr));)
        PT_WAIT_UNTIL (&cycle_mgr->pt, !get_timer (cycle_mgr->tmr) ||
                       cycle_mgr->signal != CYC_SIG_NONE);

        CHECK_AND_RESTART();

        /* Check why we left the waiting loop ! */
        if (cycle_mgr->signal == CYC_SIG_RESTART) {
          cycle_mgr->signal = CYC_SIG_NONE;
          goto again;
        }
      } else { /* if (cycle_mgr->signal == CYC_SIG_NONE) */
        /* If the ramp controller is running, send a stop signal and wait
         * for it to stop before starting a new ramp. */
        if (ramp_ctrl_get_state (cycle_mgr->rctrl) == RAMP_STATE_RAMPING) {
          ramp_ctrl_send_stop (cycle_mgr->rctrl);
          PT_WAIT_UNTIL (&cycle_mgr->pt,
              ramp_ctrl_get_state (cycle_mgr->rctrl) == RAMP_STATE_DORMANT);
        }
      }

      CHECK_AND_RESTART();

      cycle_mgr->signal = CYC_SIG_NONE;
      /* Indicate that we are rampng down again */
      cycle_mgr->state = CYCLE_STATE_RAMPING_DN;
      A_(printf (__AT__ "Timer Done !\n");)

      /* Copy instance data for second ramp */
      cycle_mgr->rctrl->intensity =
          ledlib_get_light_percentage(cycle_mgr->cdata.channel);
      cycle_mgr->rctrl->channel = cycle_mgr->cdata.channel;
      cycle_mgr->rctrl->rate = cycle_mgr->cdata.rate;
      cycle_mgr->rctrl->rampto = cycle_mgr->intensity;
      /* Evaluate the correct direction to step */
      if (cycle_mgr->rctrl->rampto > cycle_mgr->rctrl->intensity)
        cycle_mgr->rctrl->step = cycle_mgr->cdata.step;
      else
        cycle_mgr->rctrl->step = cycle_mgr->cdata.step * -1;

      A_(printf (__AT__ "Start intensity %d\n",
          cycle_mgr->rctrl->intensity);)
      A_(printf (__AT__ "Rampto %d\n",
          cycle_mgr->rctrl->rampto);)

      /* Start the ramp again */
      ramp_ctrl_send_start (cycle_mgr->rctrl);
      /* Wait for the controller to acknowledge the signal */
      PT_WAIT_UNTIL (&cycle_mgr->pt,
          ramp_ctrl_get_state (cycle_mgr->rctrl) == RAMP_STATE_RAMPING);
      /* Wait for the ramp to finnish or break on new command */
      PT_WAIT_UNTIL (&cycle_mgr->pt,
          ramp_ctrl_get_state (cycle_mgr->rctrl) == RAMP_STATE_DORMANT ||
                               cycle_mgr->signal != CYC_SIG_NONE);

      cycle_mgr->state = CYCLE_STATE_DORMANT;
    } else {
      /* Reset the signal */
      cycle_mgr->signal = CYC_SIG_NONE;
      A_(printf (__AT__ "Condition is not met !\n");)
    }
  }

  PT_END(&cycle_mgr->pt);
}
#pragma restore

/* EOF */
