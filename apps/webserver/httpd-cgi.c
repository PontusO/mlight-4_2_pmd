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
#include "channel_map.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Network CGI's */
HTTPD_CGI_CALL(L_get_device_id, "get-id", get_device_id);
HTTPD_CGI_CALL(L_get_tv_id, "get-tv", get_tv_id);
HTTPD_CGI_CALL(L_get_ip, "get-ip", get_ip);
HTTPD_CGI_CALL(L_get_mask, "get-mask", get_mask);
HTTPD_CGI_CALL(L_get_router, "get-router", get_router);
/* Time CGI's */
HTTPD_CGI_CALL(L_get_time_ena, "get-time-ena", get_time_ena);
HTTPD_CGI_CALL(L_get_time_ip, "get-time-ip", get_time_ip);
HTTPD_CGI_CALL(L_get_timeport, "get-tim-port", get_timeport);
HTTPD_CGI_CALL(L_get_interval, "get-interval", get_interval);
HTTPD_CGI_CALL(L_get_current_time, "get-cur-tim", get_current_time);
HTTPD_CGI_CALL(L_get_current_date, "get-cur-date", get_current_date);
HTTPD_CGI_CALL(L_get_tz_options, "get-tz-opt", get_tz_options);
HTTPD_CGI_CALL(L_get_time_tz, "get-time-tz", get_time_tz);
HTTPD_CGI_CALL(L_get_rtc, "get-rtc", get_rtc);
/* UI Settings */
HTTPD_CGI_CALL(L_get_ch_lock, "get-ch-lock", get_ch_lock);
HTTPD_CGI_CALL(L_get_login_to, "get-login-to", get_login_to);
HTTPD_CGI_CALL(L_get_login_end, "get-login-end", get_login_end);
HTTPD_CGI_CALL(L_get_login_time, "get-login-time", get_login_time);
/* System Settings cgi calls */
HTTPD_CGI_CALL(L_get_rpt_tim, "get-rpt-tim", get_rpt_tim);
HTTPD_CGI_CALL(L_get_rpt_rte, "get-rpt-rte", get_rpt_rte);
HTTPD_CGI_CALL(L_get_discr_tim, "get-discr-tim", get_discr_tim);
HTTPD_CGI_CALL(L_get_button_map, "get-button-map", get_button_map);
HTTPD_CGI_CALL(L_get_user_name, "get-user-name", get_user_name);
HTTPD_CGI_CALL(L_get_password, "get-password", get_password);
/* Upgrade settings */
HTTPD_CGI_CALL(L_get_upgrade_ena, "get-upgrade-ena", get_upgrade_ena);
HTTPD_CGI_CALL(L_get_upgrade_ip, "get-upgrade-ip", get_upgrade_ip);
/* Channel Mapping */
HTTPD_CGI_CALL(L_get_channel,         "get-channel", get_channel);
HTTPD_CGI_CALL(L_get_chan_sel,        "get-chan-sel", get_chan_sel);
/* File wrapped cgi calls */
HTTPD_CGI_CALL(L_clear_entries,       "clear-entries", clear_entries);
HTTPD_CGI_CALL(L_get_chan_map,        "get-chan-map", get_chan_map);
HTTPD_CGI_CALL(L_get_time_ena_cgi,    "get-tim-en-cgi", get_time_ena_cgi);
HTTPD_CGI_CALL(L_get_upgrade_ena_cgi, "cgi-get-upgrade-ena", get_upgrade_ena_cgi);
HTTPD_CGI_CALL(L_get_button_map_cgi,  "cgi-get-button-map", get_button_map_cgi);

/* Other */
HTTPD_CGI_CALL(L_set_param, "set-param", set_param);

static const struct httpd_cgi_call *calls[] = {
  &L_get_device_id,
  &L_get_tv_id,
  &L_get_ip,
  &L_get_mask,
  &L_get_router,
  &L_get_time_ena,
  &L_get_time_ip,
  &L_get_timeport,
  &L_get_interval,
  &L_get_current_time,
  &L_get_current_date,
  &L_get_tz_options,
  &L_get_time_tz,
  &L_get_rtc,
  &L_get_ch_lock,
  &L_get_login_to,
  &L_get_login_end,
  &L_get_login_time,
  &L_get_rpt_tim,
  &L_get_rpt_rte,
  &L_get_discr_tim,
  &L_get_button_map,
  &L_get_upgrade_ena,
  &L_get_upgrade_ip,
  &L_get_channel,
  &L_get_chan_sel,
  &L_get_user_name,
  &L_get_password,
  /* File wrapped cgi calls */
  &L_clear_entries,
  &L_get_chan_map,
  &L_get_time_ena_cgi,
  &L_get_upgrade_ena_cgi,
  &L_get_button_map_cgi,
  &L_set_param,
  NULL
};

/*
 * Definition of parameters that are passed in the URL
 */
u16_t gcgi_start;
u16_t gcgi_end;
u8_t  gcgi_channel;

static int i;
static int gi;

static const char *ip_string = "%d.%d.%d.%d";

/*
 * Error codes
 */
static char *error = "Error %u";

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
static
PT_THREAD(get_device_id(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  sprintf(uip_appdata, "%s", sys_cfg.device_id);
  PSOCK_SEND_STR(&s->sout, uip_appdata);

  PSOCK_END(&s->sout);
}

/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_tv_id(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  sprintf(uip_appdata, "%d", sys_cfg.tv_number);
  PSOCK_SEND_STR(&s->sout, uip_appdata);

  PSOCK_END(&s->sout);
}

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
PT_THREAD(get_ip(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  sprintf((char *)uip_appdata, ip_string,
          sys_cfg.ip_addr[0], sys_cfg.ip_addr[1],
          sys_cfg.ip_addr[2], sys_cfg.ip_addr[3]);

  PSOCK_SEND_STR(&s->sout, uip_appdata);

  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_mask(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  sprintf((char *)uip_appdata, ip_string,
          sys_cfg.netmask[0], sys_cfg.netmask[1],
          sys_cfg.netmask[2], sys_cfg.netmask[3]);

  PSOCK_SEND_STR(&s->sout, uip_appdata);

  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_router(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  sprintf((char *)uip_appdata, ip_string,
          sys_cfg.gw_addr[0], sys_cfg.gw_addr[1],
          sys_cfg.gw_addr[2], sys_cfg.gw_addr[3]);

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

  sprintf((char *)uip_appdata, ip_string,
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
PT_THREAD(get_interval(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  sprintf((char *)uip_appdata, "%u", (u16_t)sys_cfg.update_interval);

  PSOCK_SEND_STR(&s->sout, uip_appdata);

  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(clear_entries(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);

  IDENTIFIER_NOT_USED(ptr);

  PSOCK_SEND_STR(&s->sout, "<OK>");

  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_rpt_tim(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  sprintf(uip_appdata, "%d", sys_cfg.remote_repeat_time);
  PSOCK_SEND_STR(&s->sout, uip_appdata);

  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_rpt_rte(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  sprintf(uip_appdata, "%d", sys_cfg.remote_repeat_rate);
  PSOCK_SEND_STR(&s->sout, uip_appdata);

  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_discr_tim(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  sprintf(uip_appdata, "%d", sys_cfg.discr_time);
  PSOCK_SEND_STR(&s->sout, uip_appdata);

  PSOCK_END(&s->sout);
}

u8_t color_index;
/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_button_map(struct httpd_state *s, char *ptr) __reentrant)
{
  PT_BEGIN(&s->utilpt);

  while (*ptr != ISO_space)
    ptr++;
  ptr++;
  /* The index of the color is in the webpage, go get it */
  color_index = atoi(ptr)-1;
  PT_WAIT_THREAD(&s->utilpt, get_button_map_util(s));

  PT_END(&s->utilpt);
}

extern u8_t cgi_button;
/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_button_map_cgi(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  /* The index of the color is specified in the cgi call (?button=0) */
  if (cgi_button >= 4)
    sprintf(uip_appdata, error, 304);
  else
    sprintf(uip_appdata, "%u", sys_cfg.color_key_map[cgi_button]);

  PSOCK_SEND_STR(&s->sout, uip_appdata);

  PSOCK_END(&s->sout);
}

/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_ch_lock(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  sprintf(uip_appdata, "%d", sys_cfg.channel_lock);
  PSOCK_SEND_STR(&s->sout, uip_appdata);

  PSOCK_END(&s->sout);
}

/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_login_to(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  sprintf(uip_appdata, "%u", sys_cfg.login_time_out);
  PSOCK_SEND_STR(&s->sout, uip_appdata);

  PSOCK_END(&s->sout);
}

/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_login_end(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  sprintf(uip_appdata, "%u", sys_cfg.login_allow);
  PSOCK_SEND_STR(&s->sout, uip_appdata);

  PSOCK_END(&s->sout);
}

/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_login_time(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  sprintf(uip_appdata, "%u", sys_cfg.login_time);
  PSOCK_SEND_STR(&s->sout, uip_appdata);

  PSOCK_END(&s->sout);
}

/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_upgrade_ena(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  if (sys_cfg.enable_upgrades) {
    sprintf((char *)uip_appdata, "checked");
    PSOCK_SEND_STR(&s->sout, uip_appdata);
  }

  PSOCK_END(&s->sout);
}

/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_upgrade_ena_cgi(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  if (sys_cfg.enable_upgrades)
    sprintf((char *)uip_appdata, "on");
  else
    sprintf((char *)uip_appdata, "off");

  PSOCK_SEND_STR(&s->sout, uip_appdata);

  PSOCK_END(&s->sout);
}

/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_upgrade_ip(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  sprintf((char *)uip_appdata, ip_string,
          sys_cfg.multicast_group[0], sys_cfg.multicast_group[1],
          sys_cfg.multicast_group[2], sys_cfg.multicast_group[3]);

  PSOCK_SEND_STR(&s->sout, uip_appdata);

  PSOCK_END(&s->sout);
}

/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_channel(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);

  {
    u8_t chn;

    while (*ptr != ISO_space)
      ptr++;
    ptr++;

    /* Index of channel that we need to get */
    chn = atoi(ptr);
    /* Check if we are changing channel interval from the web page */
    chn = channels_interval * 10 + chn;
    sprintf(uip_appdata, "%02d", map_channel(chn));
  }

  PSOCK_SEND_STR(&s->sout, uip_appdata);
  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_chan_sel(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  /* Create the option list */
  for (i=0;i<10;i++) {
    sprintf(uip_appdata, "<option value='%d'", i);
    PSOCK_SEND_STR(&s->sout, uip_appdata);
    if (channels_interval == i)
      sprintf(uip_appdata, " selected>%d0-%d9</option>", i, i);
    else
      sprintf(uip_appdata, ">%d0-%d9</option>", i, i);
    PSOCK_SEND_STR(&s->sout, uip_appdata);
  }

  PSOCK_END(&s->sout);
}

/*---------------------------------------------------------------------------*/
static
PT_THREAD(get_chan_map(struct httpd_state *s, char *ptr) __reentrant)
{
  PSOCK_BEGIN(&s->sout);
  IDENTIFIER_NOT_USED(ptr);

  sprintf(uip_appdata, "%d", sys_cfg.channelmap[gcgi_channel]);
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
/** @} */
