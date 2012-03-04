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
#ifndef RAMP_MGR_H_INCLUDED
#define RAMP_MGR_H_INCLUDED

#include "pt.h"
#include "ramp_ctrl.h"

#define RAMP_CMD_RESET  0x00
#define RAMP_CMD_START  0x01
#define RAMP_CMD_STOP   0x02

/*
 * This data structure defines the data required by this action manager.
 */
typedef struct {
  char channel;
  unsigned char rampto;
  unsigned char rate;
  unsigned char step;
} act_ramp_data_t;

enum ramp_state {
  STEADY = 0,
  INCREASING,
  DECREASING
};

typedef struct {
  struct pt pt;
  ramp_ctrl_t *rctrl;
  u8_t channel;
  u8_t signal;
  char intensity;
  char rampto;
  char step;
  u8_t rate;
  char state;
} ramp_mgr_t;

void init_ramp_mgr(ramp_mgr_t *rmgr) __reentrant __banked;
ramp_mgr_t *get_ramp_mgr (u8_t channel) __reentrant __banked;
char *get_ramp_state (ramp_mgr_t *rmgr) __reentrant __banked;
PT_THREAD(handle_ramp_mgr(ramp_mgr_t *rmgr) __reentrant __banked);

#endif // RAMP_MGR_H_INCLUDED
