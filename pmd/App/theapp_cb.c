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

#include "system.h"
#include "theapp.h"
#include "swtimers.h"
#include "sound.h"
#include "led.h"
#include "time_stamp_mgr.h"
#include "iet_debug.h"

static u8_t tmr;

/*
 * Callback function for when the user is considered logged out
 */
void user_being_logged_out_cb(u8_t timer) __reentrant
{
  u8_t i;

  /* This is so stupid, that we have to search what user entry caused the cb */
  for (i=0 ; i<NUMBER_OF_USERS ; i++) {
    if ((timer == theapp.users[i].timer) &&
        (theapp.users[i].logged_in == USER_BEING_LOGGED_OUT))
      break;
  }
  free_timer(timer);

  if (i == NUMBER_OF_USERS) {
    A_(printf("Seriuos error, a non valid user (%d) was detected\r\n", i);)
    return;
  }

  A_(printf("User %d has not responded in time, logging him out (current status %d)!\r\n", i, theapp.users[i].logged_in);)

  /* Log out user */
  theapp.users[i].logged_in = USER_LOGGED_OUT;
  /* Save logout entry */
  set_time_data(&direct_time_data);
  direct_time_data.entry_type = ENTRY_LOGIN;
  direct_time_data.entry_command = CMD_LOGIN_OUT;
  direct_time_data.channel = theapp.current_channel;
  direct_time_data.user = i+1;
  TSM_EVENT_WRITE_DIRECT_TIME_STAMP = 1;

  theapp.num_users--;
  /* If the number of user that are logged in are zero, we shut the LED down */
  if (!theapp.num_users) {
    set_led(OFF, 0);
    LOGIN_USER_7SEG = 0;
  } else if (!user_logging_out()) {
    set_led(ON, 0);
    LOGIN_USER_7SEG = 0;
  }
}
/*
 * Callback function for the user login procedure
 * These are now called in the main loop so there are no real
 * restrictions (other than normally) on runtime here.
 */
void user_login_timeout_cb(u8_t timer) __reentrant
{
  u8_t i;

  for (i=0 ; i<NUMBER_OF_USERS ; i++) {
    if ((timer == theapp.users[i].timer) &&
        (theapp.users[i].logged_in == USER_LOGGED_IN))
      break;
  }

  A_(printf("User timer %d fired !\r\n", i);)

  if (!theapp.users[i].upper_time) {
    A_(printf("Warning user %d that he is being logged out!\r\n", i);)
    /* User is about to be logged out, show warning */
    beep(5000, 15);
    set_led(BLINK, 50);
    LOGIN_USER_7SEG = 1;
    set_timer(timer, USER_FINAL_WARNING * 100, user_being_logged_out_cb);
    theapp.users[i].logged_in = USER_BEING_LOGGED_OUT;
  } else {
    A_(printf("Timeout not reached, restarting timer !\r\n");)
    /* Decrement the high part of the timeout counter and restart timer */
    theapp.users[i].upper_time--;
    set_timer_cnt(timer, theapp.users[i].reload_time);
  }
}

void user_final_login_timeout_cb(u8_t timer) __reentrant
{
  IDENTIFIER_NOT_USED(timer);

  A_(printf("user_final_login_timeout_cb\r\n");)
  if (!user_logging_out())
    set_led(OFF, 0);
}

void user_wait_for_login_cb(u8_t timer) __reentrant
{
  set_led(BLINK, 50);
  beep(5000, 15);
  set_timer(timer, USER_FINAL_WARNING * 100, user_final_login_timeout_cb);
}

/* End of file */
