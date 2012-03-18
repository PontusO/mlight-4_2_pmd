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

#ifndef cycle_mgr_H_INCLUDED
#define cycle_mgr_H_INCLUDED

#include "pt.h"
#include "ramp_ctrl.h"

/* Internal signal definitions */
#define CYC_SIG_NONE          0x00    /** No signal */
#define CYC_SIG_START         0x01    /** Start cycle */
#define CYC_SIG_RESTART       0x02    /** Restart cycle */
#define CYC_SIG_STOP          0x03    /** Stop cycle */

/* Defines the behaviour of the ramp */
#define CYC_MODE_SINGLE_RAMP   0x00    /** Only do a intro ramp */
#define CYC_MODE_DUAL_RAMP     0x01    /** Do both intro and end ramp */

/**
 * Rule data definition
 */
typedef struct {
  char channel;         /** The channel the ramping shall take place on */
  pwm_perc_t rampto;    /** The value that it should ramp to */
  u16_t rate;           /** Using tick rate.. */
  unsigned char step;   /** and steps per tick */
  unsigned int time;    /** The time the light should be on */
  u8_t mode;            /** Indicates the mode of a ramp */
} act_cycle_data_t;

/*
 * State of the cycle manager
 */
enum cycle_state_t {
  CYCLE_STATE_UNDEFINED = 0x00,
  CYCLE_STATE_DORMANT,
  CYCLE_STATE_RAMPING_UP,
  CYCLE_STATE_RAMPING_DN,
  CYCLE_STATE_WAITING,
};
/*
 * Data types used by the cycle_mgr
 */
typedef struct {
  struct pt pt;
  ramp_ctrl_t *rctrl;
  u8_t signal;
  u8_t state;
  u8_t tmr;
  pwm_perc_t intensity;
  act_cycle_data_t cdata;
} cycle_mgr_t;

void init_cycle_mgr(cycle_mgr_t *cycle_mgr) __reentrant __banked;
PT_THREAD(handle_cycle_mgr(cycle_mgr_t *cycle_mgr) __reentrant __banked);

#endif // cycle_mgr_H_INCLUDED
