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
#include "absval_mgr.h"
#include "event_switch.h"
#include "lightlib.h"

/* Function prototypes */
void absval_stop (void);
void absval_trigger (void *ldata);

/* The action manager handle */
static action_mgr_t  absvalmgr;
static const char *absval_name = "Set absolute light level";

/*
 * Initialize the absval_mgr pthread
 */
void init_absval_mgr(absval_mgr_t *absval_mgr) __reentrant __banked
{
  PT_INIT(&absval_mgr->pt);

  absvalmgr.base.type = EVENT_ACTION_MANAGER;
  absvalmgr.type = ATYPE_ABSOLUTE_ACTION;
  absvalmgr.base.name = (char*)absval_name;
//  absvalmgr.props = ACT_PRP_ABSOLUTE_VALUE;
  absvalmgr.vt.stop_action = absval_stop;
  absvalmgr.vt.trigger_action = absval_trigger;
}

/* No thread (yet) to interupt so we don't do anything here */
void absval_stop (void)
{
}

/* Set new data */
void absval_trigger (void *input)
{
  ld_param_t led_params;
  act_absolute_data_t *absdata = (act_absolute_data_t *)input;

  A_(printf(__FILE__ " Channel %d, Value %04x\n",
          (int)absdata->channel,
          absdata->value);)
  led_params.channel = absdata->channel;
  led_params.level_absolute = absdata->value;
  ledlib_set_light_abs (&led_params);
}

PT_THREAD(handle_absval_mgr(absval_mgr_t *absval_mgr) __reentrant __banked)
{
  PT_BEGIN(&absval_mgr->pt);

  evnt_register_handle(&absvalmgr);
  A_(printf(__FILE__ " Starting absval_mgr pthread!\n");)

  while (1)
  {
    PT_YIELD(&absval_mgr->pt);
  }

  PT_END(&absval_mgr->pt);
}

/* EOF */
