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

#ifndef RAMP_CTRL_H_INCLUDED
#define RAMP_CTRL_H_INCLUDED

#include "pt.h"
#include "lightlib.h"

/* Signal definitions */
#define RAMP_SIG_NONE      0x00   /** No signal */
#define RAMP_SIG_START     0x01   /** Start signal */
#define RAMP_SIG_STOP      0x02   /** Stop signal */

/* State definitions */
#define RAMP_STATE_DORMANT  0x00  /** Dormant state */
#define RAMP_STATE_RAMPING  0x01  /** The ramp is running */
#define RAMP_STATE_STOPPING 0x02  /** The ramp is stopping */

enum ramp_state {
  STEADY = 0,
  INCREASING,
  DECREASING
};

/*
 * Data types used by the ramp_ctrl
 */
typedef struct {
  struct pt pt;
  u8_t signal;
  u8_t state;
  u8_t channel;
  u16_t rate;
  char rampto;
  char intensity;
  char step;
  u8_t timer;
  ld_param_t lp;
} ramp_ctrl_t;

void init_ramp_ctrl(ramp_ctrl_t *ramp_ctrl) __reentrant __banked;
PT_THREAD(handle_ramp_ctrl(ramp_ctrl_t *ramp_ctrl) __reentrant __banked);
ramp_ctrl_t *ramp_ctrl_get_ramp_ctrl (u8_t channel) __reentrant __banked;
void ramp_ctrl_send_start (ramp_ctrl_t *rcmgr) __reentrant __banked;
void ramp_ctrl_send_stop (ramp_ctrl_t *rcmgr) __reentrant __banked;
u8_t ramp_ctrl_get_state (ramp_ctrl_t *rcmgr) __reentrant __banked;
char *ramp_ctrl_get_state_str (ramp_ctrl_t *rcmgr) __reentrant __banked;

#endif // ramp_ctrl_H_INCLUDED
