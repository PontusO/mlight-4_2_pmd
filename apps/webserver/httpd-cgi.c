/**
 * \addtogroup httpd
 * @{
 */

/**
 * \file
 *         Web server script interface
 * \author
 *         Adam Dunkels <adam@sics.se>
 *
 */

/*
 * Copyright (c) 2001-2006, Adam Dunkels.
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
 * This file is part of the uIP TCP/IP stack.
 *
 * $Id: httpd-cgi.c,v 1.2 2006/06/11 21:46:37 adam Exp $
 *
 */
//#define PRINT_A

#include "uip.h"
#include "psock.h"
#include "httpd.h"
#include "httpd-cgi.h"
#include "cgi_utils.h"
#include "httpd-param.h"
#include "httpd-fs.h"
#include "flash.h"
#include "rtc.h"
#include "i2c.h"
#include "iet_debug.h"

#include "lightlib.h"
#include "ramp_mgr.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Time CGI's */
HTTPD_CGI_CALL(L_get_time_ena, "get-time-ena", get_time_ena);
HTTPD_CGI_CALL(L_get_time_ip, "get-time-ip", get_time_ip);
HTTPD_CGI_CALL(L_get_timeport, "get-tim-port", get_timeport);
HTTPD_CGI_CALL(L_get_current_time, "get-cur-tim", get_current_time);
HTTPD_CGI_CALL(L_get_current_date, "get-cur-date", get_current_date);
HTTPD_CGI_CALL(L_get_tz_options, "get-tz-opt", get_tz_options);
HTTPD_CGI_CALL(L_get_time_tz, "get-time-tz", get_time_tz);
HTTPD_CGI_CALL(L_get_rtc, "get-rtc", get_rtc);
/* Other */
HTTPD_CGI_CALL(L_set_param, "set-param", set_param);
/* Generic data type CGI's */
HTTPD_CGI_CALL(f_get_ip_num, "get-ip-num", get_ip_num);
HTTPD_CGI_CALL(f_get_check_box, "get-check", get_check_box);
HTTPD_CGI_CALL(f_get_string, "get-string", get_string);
HTTPD_CGI_CALL(f_get_int, "get-int", get_int);
HTTPD_CGI_CALL(f_get_time_events, "te-get-time-events", get_time_events);

HTTPD_CGI_CALL(f_set_level, "set-level", set_level);
HTTPD_CGI_CALL(f_get_level, "get-level", get_level);
HTTPD_CGI_CALL(f_start_ramp, "start-ramp", start_ramp);
HTTPD_CGI_CALL(f_stop_ramp, "stop-ramp", stop_ramp);

static const struct httpd_cgi_call *calls[] = {
  &f_get_ip_num,
  &f_get_check_box,
  &f_get_string,
  &f_get_int,
  &L_get_time_ena,
  &L_get_time_ip,
  &L_get_timeport,
  &L_get_current_time,
  &L_get_current_date,
  &L_get_tz_options,
  &L_get_time_tz,
  &L_get_rtc,
  &L_set_param,
  &f_get_time_events,
  &f_set_level,
  &f_get_level,
  &f_start_ramp,
  &f_stop_ramp,
  NULL
};

static char *ip_format = "%d.%d.%d.%d";
static char *error_string = "<ERROR ";

static
PT_THREAD(set_level(struct httpd_state *s, char *ptr) __reentrant)
{
  IDENTIFIER_NOT_USED(ptr);

  PSOCK_BEGIN(&s->sout);
  /* Make sure only 2 parameters were passed to this cgi */
  if (s->parms.num_parms == 2) {
    /* Make sure only the correct parameters were passed */
    if (s->parms.channel_updated && s->parms.level_updated) {
      /* Validate parameters */
      if (s->parms.channel < CFG_NUM_LIGHT_DRIVERS &&
          s->parms.level <= 100) {
        ld_param_t led_params;
        led_params.channel = s->parms.channel;
        led_params.level_percent = s->parms.level;
        ledlib_set_light_percentage_log (&led_params);
        sprintf((char *)uip_appdata, "<OK>");
      } else {
        sprintf((char *)uip_appdata, "%s01>", error_string);
      }
    } else {
      sprintf((char *)uip_appdata, "%s02>", error_string);
    }
  } else {
      sprintf((char *)uip_appdata, "%s03>", error_string);
  }
  PSOCK_SEND_STR(&s->sout, uip_appdata);
  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_level(struct httpd_state *s, char *ptr) __reentrant)
{
  IDENTIFIER_NOT_USED(ptr);

  PSOCK_BEGIN(&s->sout);
  /* Make sure only 1 parameter was passed to this cgi */
  if (s->parms.num_parms == 1) {
    /* Make sure only the correct parameters were passed */
    if (s->parms.channel_updated) {
      /* Validate parameters */
      if (s->parms.channel < CFG_NUM_LIGHT_DRIVERS) {
        u8_t val = ledlib_get_light_percentage (s->parms.channel);
        sprintf((char *)uip_appdata, "<%d>", val);
      } else {
        sprintf((char *)uip_appdata, "%s01>", error_string);
      }
    } else {
      sprintf((char *)uip_appdata, "%s02>", error_string);
    }
  } else {
      sprintf((char *)uip_appdata, "%s03>", error_string);
  }
  PSOCK_SEND_STR(&s->sout, uip_appdata);
  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(start_ramp(struct httpd_state *s, char *ptr) __reentrant)
{
  IDENTIFIER_NOT_USED(ptr);

  PSOCK_BEGIN(&s->sout);
  {
    /* Make sure we have at least 3 parameters
     * channel, rate and rampto are mandatory parameters.
     * step is optional
     */
    if (s->parms.num_parms >= 3 &&
        s->parms.channel_updated &&
        s->parms.rate_updated &&
        s->parms.rampto_updated)
    {
      ramp_mgr_t *rmptr = get_ramp_mgr (s->parms.channel);
      if (!rmptr) {
        A_(printf (__FILE__ " %p is not a valid ramp manager !\n", rmptr);)
        sprintf((char *)uip_appdata, "%s04>", error_string);
      } else {
        /* Assert a signal to the ramp manager to start a ramp */
        A_(printf (__FILE__ " Starting ramp (%p) on channel %d\n",
                   rmptr, (int)s->parms.channel);)
        rmptr->rate = s->parms.rate;
        if (!s->parms.step_updated)
          rmptr->step = 1;
        else
          rmptr->step = s->parms.step;
        rmptr->rampto = s->parms.rampto;
        rmptr->signal = RAMP_CMD_START;
        /* Return Ok status to the web client */
        sprintf((char *)uip_appdata, "<OK>");
      }
    } else {
      sprintf((char *)uip_appdata, "%s01>", error_string);
    }
    PSOCK_SEND_STR(&s->sout, uip_appdata);
  }
  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(stop_ramp(struct httpd_state *s, char *ptr) __reentrant)
{
  IDENTIFIER_NOT_USED(ptr);

  PSOCK_BEGIN(&s->sout);
  {
    if (s->parms.num_parms == 1 &&
        s->parms.channel_updated) {
      ramp_mgr_t *rmptr = get_ramp_mgr (s->parms.channel);
      /* Assert a signal to the ramp manager to start a ramp */
      A_(printf (__FILE__ " Stopping ramp (%p) on channel %d\n",
                rmptr, (int)s->parms.channel);)
      rmptr->ramp.signal = RAMP_CMD_STOP;
      /* For now, this will always return OK status */
      sprintf((char *)uip_appdata, "<OK>");
    } else {
      sprintf((char *)uip_appdata, "%s03>", error_string);
    }
    PSOCK_SEND_STR(&s->sout, uip_appdata);
  }
  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(nullfunction(struct httpd_state *s, char *ptr) __reentrant)
{
  IDENTIFIER_NOT_USED(ptr);

  PSOCK_BEGIN(&s->sout);

  sprintf((char*)uip_appdata, "Invalid CGI encountered !");
  PSOCK_SEND_STR(&s->sout, uip_appdata);

  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
httpd_cgifunction
httpd_cgi(char *name) __reentrant
{
  const struct httpd_cgi_call **f;

  /* Find the matching name in the table, return the function. */
  for (f = calls; *f != NULL; ++f) {
    if (strncmp((*f)->name, name, strlen((*f)->name)) == 0) {
      return (*f)->function;
    }
  }
  return nullfunction;
}

/* ****************************************************************************
 * CGI function template
 */
#if 0
/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_rtc(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  /* Do stuff here */

  PSOCK_END(&s->sout);
}
#endif
/* ***************************************************************************/

/*---------------------------------------------------------------------------*/
/*
 * Create a list of the currently configured time events in the system
 * The output is modeled after this static html code.
 * <tr>
 *   <td><input type="checkbox" name="cb1"></td>
 *   <td>Evening</td>
 *   <td>Time 21:40</td>
 *   <td>Weekdays: Mon-Tue-Wed-Thu-Fri</td>
 * </tr>
 *
 */
static char *weekdays[] =
  {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static unsigned short generate_time_events(void *arg) __reentrant
{
  struct httpd_state *s = (struct httpd_state *)arg;
  time_spec_t *ts;
  int i;
  u8_t j, day_mask;

  ts = &sys_cfg.time_events[s->cgiv.i];
  i = sprintf((char *)uip_appdata, "<tr><td><input type=\"checkbox\" name=\"cb%d\" ", s->cgiv.i);
  if (ts->status & TIME_EVENT_ENABLED)
    i += sprintf((char *)uip_appdata+i, "Checked");
  i += sprintf((char *)uip_appdata+i, "></td>");
  i += sprintf((char *)uip_appdata+i, "<td>%s</td>", ts->name);
  i += sprintf((char *)uip_appdata+i, "<td>Time %02d:%02d</td>", ts->hrs, ts->min);
  i += sprintf((char *)uip_appdata+i, "<td>Weekdays: ");
  day_mask = 0x40;
  for (j=0;j<7;j++) {
    if (ts->weekday & day_mask) {
      i += sprintf((char *)uip_appdata+i, "%s ", weekdays[j]);
    }
    day_mask >>= 1;
  }
  sprintf((char *)uip_appdata+i, "</td></tr>");
  printf ((char*)uip_appdata);
  putchar ('\n');

  return strlen(uip_appdata);
}

#pragma save
#pragma nogcse
static
PT_THREAD(get_time_events(struct httpd_state *s, char *ptr) __reentrant)
{
  IDENTIFIER_NOT_USED(ptr);

  PSOCK_BEGIN(&s->sout);

  for(s->cgiv.i=0;s->cgiv.i < NMBR_TIME_EVENTS;++s->cgiv.i) {
    time_spec_t *ts;

    ts = &sys_cfg.time_events[s->cgiv.i];
    if (ts->status & TIME_EVENT_ENTRY_USED) {
      PSOCK_GENERATOR_SEND (&s->sout, generate_time_events, s);
    }
  }

  PSOCK_END(&s->sout);
}
#pragma restore

/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_current_time(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  print_time_formated(uip_appdata);
  PSOCK_SEND_STR(&s->sout, uip_appdata);

  PSOCK_END(&s->sout);
}

/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_current_date(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  print_date_formated(uip_appdata);
  PSOCK_SEND_STR(&s->sout, uip_appdata);

  PSOCK_END(&s->sout);
}

/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_tz_options(struct httpd_state *s, char *ptr) __reentrant)
{
  PT_BEGIN(&s->utilpt);
  IDENTIFIER_NOT_USED(ptr);
  PT_WAIT_THREAD(&s->utilpt, get_tz_options_util(s));
  PT_END(&s->utilpt);
}

/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_rtc(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  print_datetime_formated(uip_appdata);
  PSOCK_SEND_STR(&s->sout, uip_appdata);

  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_time_ena(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  if (sys_cfg.enable_time) {
    sprintf((char *)uip_appdata, "checked");
    PSOCK_SEND_STR(&s->sout, uip_appdata);
  }

  PSOCK_END(&s->sout);
}

/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_time_ena_cgi(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  if (sys_cfg.enable_time)
    sprintf((char *)uip_appdata, "on");
  else
    sprintf((char *)uip_appdata, "off");

  PSOCK_SEND_STR(&s->sout, uip_appdata);

  PSOCK_END(&s->sout);
}

/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_time_ip(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  sprintf((char *)uip_appdata, ip_format,
          sys_cfg.time_server[0], sys_cfg.time_server[1],
          sys_cfg.time_server[2], sys_cfg.time_server[3]);

  PSOCK_SEND_STR(&s->sout, uip_appdata);

  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_timeport(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  sprintf((char *)uip_appdata, "%u", (u16_t)sys_cfg.time_port);

  PSOCK_SEND_STR(&s->sout, uip_appdata);

  PSOCK_END(&s->sout);
}

/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_time_tz(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  sprintf(uip_appdata, "%d", sys_cfg.time_zone);
  PSOCK_SEND_STR(&s->sout, uip_appdata);

  PSOCK_END(&s->sout);
}

/*---------------------------------------------------------------------------*/
static
PT_THREAD(set_param(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  /* This cgi was meant to actually verify that parameters being written are
   * parameters that are allowed to be written. This is not implemented yet
   * though. And as cgi's are executed after parameter parsing we need
   * something that does the validation ahead of parsing the parameters and
   * then reports this to this function.
   * Tricky stuff =)
   */
  sprintf(uip_appdata, "<OK>");
  PSOCK_SEND_STR(&s->sout, uip_appdata);

  PSOCK_END(&s->sout);
}

/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_user_name(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  sprintf(uip_appdata, sys_cfg.username);
  PSOCK_SEND_STR(&s->sout, uip_appdata);

  PSOCK_END(&s->sout);
}

/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_password(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  sprintf(uip_appdata, sys_cfg.password);
  PSOCK_SEND_STR(&s->sout, uip_appdata);

  PSOCK_END(&s->sout);
}

/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_ip_num(struct httpd_state *s, char *ptr) __reentrant)
{
  char ip_group;

  PSOCK_BEGIN(&s->sout);

  while (*ptr != ' ')
    ptr++;
  ptr++;
  ip_group = atoi(ptr);

  switch(ip_group)
  {
    case 1:
      /* Host IP */
      sprintf((char *)uip_appdata, ip_format,
        sys_cfg.ip_addr[0], sys_cfg.ip_addr[1],
        sys_cfg.ip_addr[2], sys_cfg.ip_addr[3]);
      break;

    case 2:
      /* Netmask */
      sprintf((char *)uip_appdata, ip_format,
        sys_cfg.netmask[0], sys_cfg.netmask[1],
        sys_cfg.netmask[2], sys_cfg.netmask[3]);
      break;

    case 3:
      /* Deafult router */
      sprintf((char *)uip_appdata, ip_format,
        sys_cfg.gw_addr[0], sys_cfg.gw_addr[1],
        sys_cfg.gw_addr[2], sys_cfg.gw_addr[3]);
      break;

    case 5:
      /* HTTP Port */
      sprintf((char *)uip_appdata, "%u", sys_cfg.http_port);
      break;

    default:
      sprintf((char *)uip_appdata, "Invalid IP group !");
      break;
  }

  PSOCK_SEND_STR(&s->sout, uip_appdata);

  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_check_box(struct httpd_state *s, char *ptr) __reentrant)
{
  char check_box;

  PSOCK_BEGIN(&s->sout);

  while (*ptr != ' ')
    ptr++;
  ptr++;
  check_box = atoi(ptr);

  switch(check_box)
  {
#if 0
    case 0:
      check_box = sys_cfg.authenabled;
      break;

    case 1:
      check_box = sys_cfg.enable_stop_larm;
      break;

    case 2:
      check_box = sys_cfg.enable_fall_tube_alarm;
      break;

    case 3:
      check_box = sys_cfg.enable_sonar_larm;
      break;
#endif
    default:
      check_box = 0;
      break;
  }

#if 0
  if (check_box)
    sprintf((char *)uip_appdata, "checked");
  else
    sprintf((char *)uip_appdata, " ");
#endif

  PSOCK_SEND_STR(&s->sout, uip_appdata);
  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_int(struct httpd_state *s, char *ptr) __reentrant)
{
  char intno;
  int myint = 0;

  PSOCK_BEGIN(&s->sout);

  while (*ptr != ' ')
    ptr++;
  ptr++;
  intno = atoi(ptr);

  switch(intno)
  {
    /* Update interval for the timeset web page */
    case 1:
      myint = (u16_t)sys_cfg.update_interval;
      break;

    case 10:
    case 11:
    case 12:
    case 13:
      myint = (u16_t)ledlib_get_light_percentage(intno - 10);
      break;
  }

  sprintf((char *)uip_appdata, "%d", myint);

  PSOCK_SEND_STR(&s->sout, uip_appdata);
  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_string(struct httpd_state *s, char *ptr) __reentrant)
{
  char stringno;
  char *string = NULL;

  PSOCK_BEGIN(&s->sout);

  while (*ptr != ' ')
    ptr++;
  ptr++;
  stringno = atoi(ptr);

  switch(stringno)
  {
    /* Node Name */
    case 1:
      string = sys_cfg.device_id;
      break;

    case 10:
    case 11:
    case 12:
    case 13: {
        ramp_mgr_t *rmgr = get_ramp_mgr(stringno-10);
        if (rmgr) {
          string = get_ramp_state(rmgr);
        } else {
          string = "Invalid string !";
        }
      }
      break;
  }

  if (string)
    sprintf((char *)uip_appdata, "%s", string);

  PSOCK_SEND_STR(&s->sout, uip_appdata);
  PSOCK_END(&s->sout);
}

/** @} */
