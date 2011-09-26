/*
 * Copyright (c) 2008, Pontus Oldberg.
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

#ifndef THEAPP_H_INCLUDED
#define THEAPP_H_INCLUDED

#include "flash.h"
#include "time_stamp_mgr.h"

/* This timeout is used when selecting a channel with digits */
#define CHANNEL_SET_TIMEOUT   sys_cfg.channel_lock
/* Timeout used after a channel has been selected and the user is required
 * to log in */
#define USER_LOGIN_TIMEOUT    sys_cfg.login_time_out
/* Timeout before giving up on the user */
#define USER_FINAL_WARNING    sys_cfg.login_allow
/* The total time a user will remain logged in */
#define USER_LOGIN_TIME       sys_cfg.login_time

#define NUMBER_OF_USERS       4

enum USER_STATES {
  USER_LOGGED_OUT = 0x00,       /* Indicating a user is logged out */
  USER_BEING_LOGGED_OUT,        /* Indicates a user is being logged out */
  USER_LOGGED_IN                /* User is logged in */
};

enum POWER_STATE {
  POWER_OFF = 0x00,
  POWER_ON = 0x01
};

struct user_entry {
  u8_t logged_in;               /* 2 = User logged in, 1 = User being logged out,
                                 * 0 = User logged out */
  u8_t timer;                   /* Timer used to count login time */
  u16_t upper_time;             /* Used to hold the high word of the time out */
  u16_t reload_time;            /* Holds the timer part of the login time */
};

struct theapp {
  struct pt theapp_pt;
  char pressed_key;             /* Last pressed key */
  u8_t current_channel;         /* This is the currently selected channel */
  u8_t timer;                   /* Timer used when entering channel with digits */
  u8_t req_timer;               /* Timer used requesting a user log in */
  u8_t num_users;               /* Indicates how many users are logged in */
  u8_t logging_out;             /* Inidicates that a user (any) is in a log out state */
  struct user_entry users[4];   /* Array of users */
  char ledstr[3];               /* Array for displaying LED values */
  u8_t power_state;             /* Indicates wether the power is on or off */
};

struct seg7 {
  struct pt pt;
  u8_t timer;                   /* Timer used to delay and let the user see who needs to log in */
  u8_t i;                       /* Instance loop variable */
};

extern struct theapp theapp;
extern struct testapp testapp;
extern struct seg7 Seg7;
extern bit LOGIN_USER_7SEG;

void init_theapp(void) banked;
u8_t suggest_channel(u8_t new_channel) banked;
void set_time_data(struct time_data *td) banked;
u8_t user_logging_out(void) banked;
PT_THREAD(handle_theapp(struct theapp *theapp) __reentrant banked);
PT_THREAD(handle_7seg(struct seg7 *seg7) __reentrant banked);

#endif // THEAPP_H_INCLUDED
