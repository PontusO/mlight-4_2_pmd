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
#pragma codeseg APP_BANK

#include "system.h"
#include "pt.h"
#include "theapp.h"
#include "led.h"
#include "remote.h"
#include "sound.h"
#include "swtimers.h"
#include "led.h"
#include "i2c.h"
#include "time_stamp_mgr.h"
#include "rtc.h"
#include "flash.h"
#include "iet_debug.h"
#include "channel_map.h"

#include <stdio.h>
#include <string.h>

extern void user_login_timeout_cb(u8_t timer) __reentrant;
extern void user_wait_for_login_cb(u8_t timer) __reentrant;
void user_final_login_timeout_cb(u8_t timer) __reentrant;

struct theapp theapp;
struct seg7 Seg7;

bit LOGIN_USER_7SEG;

/*
 * Initialize the main appplication thread
 */
void init_theapp(void) banked
{
  u8_t i;
  /* At reset, all is cleared */
  memset(&theapp, 0, sizeof(struct theapp));
  LOGIN_USER_7SEG = 0;

  PT_INIT(&theapp.theapp_pt);
  PT_INIT(&Seg7.pt);
}

/*
 * Local wrapper for the get_remote_key function
 */
static char get_key(struct theapp *app) __reentrant
{
  app->pressed_key = get_remote_key();
  return app->pressed_key;
}

/*
 * Fill the time_data structure with sensible values.
 */
void set_time_data(struct time_data *td) banked
{
  if (!td)
    return;

  td->time_stamp = get_g_time();
}

/*
 * A function external applications can use to suggest a new channel.
 */
u8_t suggest_channel(u8_t new_channel) banked
{
  if (theapp.power_state) {
    /* For now, we accept all suggestions but this might change */
    theapp.current_channel = new_channel;

    sprintf(theapp.ledstr, "%02d", theapp.current_channel);
    led_out(theapp.ledstr);
  }

  return 0;
}

/*
 * Function for calculating long time
 */
void calc_long_time(struct theapp *app, u8_t user)
{
  unsigned long ltim;

  ltim = (unsigned long)USER_LOGIN_TIME * 100 * 60;
  app->users[user].upper_time = (u16_t)(ltim >> 16);
  app->users[user].reload_time = (u16_t)(ltim & 0xffff);
}

/*
 * This function checks to see if any user is currently being logged out.
 * This is used to determine if we can poke the LED state or not.
 */
u8_t user_logging_out(void) banked
{
  u8_t i;

  for (i=0 ; i<NUMBER_OF_USERS ; i++) {
    if (theapp.users[i].logged_in == USER_BEING_LOGGED_OUT)
      return 1;
  }

  return 0;
}

/*
 * This function handles shutting the PMD unit down. It will log out all users
 * and cancel all pendig requests for the same.
 */
void power_down(void)
{
  u8_t i;

  if (get_timer(theapp.req_timer)) {
    stop_timer(theapp.req_timer);
  }

  for (i=0 ; i<NUMBER_OF_USERS ; i++) {
    /* Log out all users, not matter what */
    theapp.users[i].logged_in = USER_LOGGED_OUT;
    if (get_timer(theapp.users[i].timer))
      free_timer(theapp.users[i].timer);
  }

  /* Clear the number of logged in users counter */
  theapp.num_users = 0;

  set_led(OFF, 0);
}

/*
 * The main application thread
 */
PT_THREAD(handle_theapp(struct theapp *app) __reentrant banked)
{
  PT_BEGIN(&app->theapp_pt);

  /* As in power off */
  sprintf(app->ledstr, "  ");
  led_out(app->ledstr);

  app->num_users = 0;
  app->power_state = POWER_OFF;
  /* request a static timer */
  app->req_timer = alloc_timer();

  while (1)
  {
    PT_WAIT_UNTIL(&app->theapp_pt, (get_key(app) != -1));

    if ((app->pressed_key == DIGIT_RED ||
        app->pressed_key == DIGIT_GREEN ||
        app->pressed_key == DIGIT_YELLOW ||
        app->pressed_key == DIGIT_BLUE) &&
        app->power_state)
    {
      u8_t user;

      /* What user was pressed */
      user = app->pressed_key - DIGIT_RED;

      A_(printf("User %d logging in.\r\n", user);)

      /* Login beep */
      beep(5000, 15);

      /* Check if this user is already logged in */
      if (app->users[user].logged_in) {
        A_(printf("User %d was already logged in, renewing timeout !\r\n", user);)
        /* User was logged in so we simply renew his time out */
        calc_long_time(app, user);
        /* Just in case we were in the process of being logged out we
         * renew the call back function as well */
        set_timer(app->users[user].timer,
                  app->users[user].reload_time,
                  user_login_timeout_cb);
        /* Renew status in case we were being logged out */
        app->users[user].logged_in = USER_LOGGED_IN;
        if (!user_logging_out())
          set_led(ON, 0);
      } else {
        if (get_timer(app->req_timer) != 0) {
          /* If the login requestor is running we need to stop the count down */
          stop_timer(app->req_timer);
          if (!user_logging_out())
            set_led(ON, 0);
        }
        A_(printf("User %d just logged in.\r\n", user);)
        /* The user has just logged in, so set up time out and
         * save time of logg in */
        app->num_users++;
        app->users[user].timer = alloc_timer();
        calc_long_time(app, user);
        set_timer(app->users[user].timer,
                  app->users[user].reload_time, user_login_timeout_cb);
        app->users[user].logged_in = USER_LOGGED_IN;
        A_(printf("User timer = %d\r\n", app->users[user].timer);)
        A_(printf("User logged in = %d\r\n", app->users[user].logged_in);)

        /* Save login entry */
        set_time_data(&direct_time_data);
        direct_time_data.entry_type = ENTRY_LOGIN;
        direct_time_data.entry_command = CMD_LOGIN_IN;
        direct_time_data.channel = app->current_channel;
        direct_time_data.user = user+1;
        TSM_EVENT_WRITE_DIRECT_TIME_STAMP = 1;
        if (!user_logging_out())
          set_led(ON, 0);
      }
    } else if ((app->pressed_key <= DIGIT_9 ||
               app->pressed_key == DIGIT_UP ||
               app->pressed_key == DIGIT_DOWN) &&
               app->power_state) {
      if (app->pressed_key <= DIGIT_9)
      {
        /* This context takes care of when the users enters the channel
         * with numeric buttons */
        app->current_channel = app->pressed_key;
        sprintf(app->ledstr, "-%01d", app->pressed_key);
        led_out(app->ledstr);

        /* Allocate a timer for the key timeout */
        app->timer = alloc_timer();
        set_timer(app->timer, CHANNEL_SET_TIMEOUT, NULL);

        /* Wait for next key press or timeout */
        PT_WAIT_UNTIL(&app->theapp_pt,
                      ((get_key(app) != -1) ||
                       (get_timer(app->timer) == 0)));
        free_timer(app->timer);

        /* Check if the timer timed out */
        if (!get_timer(app->timer) == 0)
        {
          /* Was it a valid digit that was pressed */
          if (app->pressed_key <= DIGIT_9)
          {
            /* Calculate new channel number */
            app->current_channel = app->current_channel * 10 + app->pressed_key;
          }
        }
      }
      else if (app->pressed_key == DIGIT_UP)
      {
        if (app->current_channel < 99)
        {
          app->current_channel++;
        }
        else
        {
          app->current_channel = 0;
        }
      }
      else if (app->pressed_key == DIGIT_DOWN)
      {
        if (app->current_channel > 0)
        {
          app->current_channel--;
        }
        else
        {
          app->current_channel = 99;
        }
      }
      /* print the new channel number */
      sprintf(app->ledstr, "%02d", app->current_channel);
      led_out(app->ledstr);
      /*
       * So, now we need to tell the time stamp manager that he should attempt saving
       * this new channel.
       */
      set_time_data(&time_data);
      time_data.entry_type = ENTRY_CHANNEL;
      time_data.entry_command = CMD_CHANNEL_CHANGE;
      time_data.channel = map_channel(app->current_channel);
      time_data.user = 0;
      TSM_EVENT_WRITE_TIME_STAMP = 1;
      /*
       * Check here if a user is already logged in.
       * If not we should wait for while and then remind the user gently to log in.
       */
      if (!app->num_users)
      {
        /* First we wait to give the user time to login */
        set_timer(app->req_timer, USER_LOGIN_TIMEOUT, user_wait_for_login_cb);
      }
    } else if (app->pressed_key == DIGIT_PWR) {
      if (!app->power_state) {
        /* need to turn power on */
        app->power_state = POWER_ON;
        sprintf(app->ledstr, "%02d", app->current_channel);
        led_out(app->ledstr);

        /* Save Power On entry */
        set_time_data(&direct_time_data);
        direct_time_data.entry_type = ENTRY_POWER;
        direct_time_data.entry_command = CMD_POWER_ON;
        direct_time_data.channel = app->current_channel;
        direct_time_data.user = 0;
        TSM_EVENT_WRITE_DIRECT_TIME_STAMP = 1;

      } else {
        app->power_state = POWER_OFF;
        sprintf(app->ledstr, "  ");
        led_out(app->ledstr);

        /* Make sure all users are logged out */
        power_down();

        /* Save Power Off entry */
        set_time_data(&direct_time_data);
        direct_time_data.entry_type = ENTRY_POWER;
        direct_time_data.entry_command = CMD_POWER_OFF;
        direct_time_data.channel = app->current_channel;
        direct_time_data.user = 0;
        TSM_EVENT_WRITE_DIRECT_TIME_STAMP = 1;
      }
    }
  }
  PT_END(&app->theapp_pt);
}

/**
 * This simple thread will, when activated alternate between the current TV channel
 * and any user needs to log in.
 */
static char *users[] = {"rd", "6r", "4l", "bl"};
u8_t bt;
PT_THREAD(handle_7seg(struct seg7 *seg7) __reentrant banked)
{
  PT_BEGIN(&seg7->pt);

  while (1) {
    PT_WAIT_UNTIL(&seg7->pt, LOGIN_USER_7SEG);

    for (bt=0 ; bt<NUMBER_OF_USERS ; bt++) {
      if (theapp.users[bt].logged_in == USER_BEING_LOGGED_OUT) {
        sprintf(theapp.ledstr, "%s", users[bt]);
        led_out(theapp.ledstr);
        seg7->timer = alloc_timer();
        set_timer(seg7->timer, 75, NULL);
        PT_WAIT_UNTIL(&seg7->pt, get_timer(seg7->timer) == 0 ||
                                 !LOGIN_USER_7SEG);
        free_timer(seg7->timer);
        if (!LOGIN_USER_7SEG) {
          sprintf(theapp.ledstr, "%02d", theapp.current_channel);
          led_out(theapp.ledstr);
          PT_RESTART(&seg7->pt);
        }
      }
    }
    /* Display the channel number briefly */
    sprintf(theapp.ledstr, "%02d", theapp.current_channel);
    led_out(theapp.ledstr);
    seg7->timer = alloc_timer();
    set_timer(seg7->timer, 200, NULL);
    PT_WAIT_UNTIL(&seg7->pt, get_timer(seg7->timer) == 0 ||
                             !LOGIN_USER_7SEG);
    free_timer(seg7->timer);
  }
  PT_END(&seg7->pt);
}

/* End of file */
