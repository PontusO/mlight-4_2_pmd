/*
 * Copyright (c) 2008, Invector Embedded Technologies.
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 * Author: Pontus Oldberg <pontus@invector.se>
 *
 */
#pragma codeseg UIP_BANK

//#define PRINT_A

#include "system.h"
#include "uip.h"
#include "httpd-param.h"
#include "httpd-cgi.h"
#include "flash.h"
#include "rtc.h"
#include "iet_debug.h"

#include <string.h>
#include <stdlib.h>
#define PARAM_FUNC(x) \
        static void x(struct httpd_state *s, char *buffer) __reentrant

/*---------------------- Local data types -----------------------------------*/
struct parameter_table {
  const char *param;
  void (*function)(struct httpd_state *s, char *buffer) __reentrant;
};

/*---------------------- Local data ----------------------------------------*/
/* Form parameters that needs to be parsed */
static const struct parameter_table parmtab[] = {
    {
        "user",
        set_username
    },
    {
        "password",
        set_password
    },
    {
        "node",
        set_device_id
    },
    {
        "hostip",
        set_ip
    },
    {
        "netmask",
        set_netmask
    },
    {
        "gateway",
        set_gateway
    },
    {
        "webport",
        set_webport
    },
    {
        "resettime",
        reset_time
    },
    {
        "entim",
        enable_time
    },
    {
        "timip",
        set_time
    },
    {
        "timprt",
        set_timeport
    },
    {
        "intrvl",
        set_interval
    },
    {
        "timval",
        set_timevalue
    },
    {
        "datval",
        set_datevalue
    },
    {
        "tz",
        set_timezone
    },
	/* Parameters used in xcgi commands */
	{
        "channel",
        cgi_set_channel
    },
    {
        "level",
        cgi_set_level
    },
    {
        "rampto",
        cgi_set_rampto
    },
    {
        "rate",
        cgi_set_rate
    },
    {
        "step",
        cgi_set_step
    },
    /* This is the last parameter in the list of html parameters.
     * It's used to trigger the real flash save function
     */
    {
        "submit",
        set_save
    },
    {
        "*",
        0
    }
};

static u16_t year;
static u8_t month;
static u8_t day;
static u8_t hrs;
static u8_t min;
static bit need_time_update = FALSE;
static bit need_reset = FALSE;

/*---------------------------------------------------------------------------*/
static char *skip_to_char(char *buf, char chr) __reentrant
{
  while (*buf != chr)
    buf++;
  buf++;

  return buf;
}

/*---------------------------------------------------------------------------*/
static void parse_ip(char *buf, uip_ipaddr_t *ip)
{
  static u8_t octet[4];

  buf = skip_to_char(buf, '=');
  octet[0] = atoi(buf);
  buf = skip_to_char(buf, '.');
  octet[1] = atoi(buf);
  buf = skip_to_char(buf, '.');
  octet[2] = atoi(buf);
  buf = skip_to_char(buf, '.');
  octet[3] = atoi(buf);
  buf = skip_to_char(buf, '.');

  uip_ipaddr(ip, octet[0],octet[1],octet[2],octet[3]);
}
/*---------------------------------------------------------------------------*/
PARAM_FUNC (set_device_id)
{
  u8_t *ptr = sys_cfg.device_id;
  char i = 8;

  IDENTIFIER_NOT_USED (s);

  buffer = skip_to_char(buffer, '=');

  while ((*buffer != ISO_and) && (i >= 0)) {
    *ptr++ = *buffer++;
    i--;
  }
  *buffer = 0x00;
}

/*---------------------------------------------------------------------------*/
PARAM_FUNC (set_ip)
{
  static uip_ipaddr_t ip;

  IDENTIFIER_NOT_USED (s);

  parse_ip(buffer, &ip);

  /* Pack the result in the global parameter structure */
  sys_cfg.ip_addr[0] = htons(ip[0]) >> 8;
  sys_cfg.ip_addr[1] = htons(ip[0]) & 0xff;
  sys_cfg.ip_addr[2] = htons(ip[1]) >> 8;
  sys_cfg.ip_addr[3] = htons(ip[1]) & 0xff;
  need_reset = TRUE;
}

/*---------------------------------------------------------------------------*/
PARAM_FUNC (set_netmask)
{
  static uip_ipaddr_t ip;

  IDENTIFIER_NOT_USED (s);

  parse_ip(buffer, &ip);

  /* Pack the result in the global parameter structure */
  sys_cfg.netmask[0] = htons(ip[0]) >> 8;
  sys_cfg.netmask[1] = htons(ip[0]) & 0xff;
  sys_cfg.netmask[2] = htons(ip[1]) >> 8;
  sys_cfg.netmask[3] = htons(ip[1]) & 0xff;
  need_reset = TRUE;
}
/*---------------------------------------------------------------------------*/
PARAM_FUNC (set_gateway)
{
  static uip_ipaddr_t ip;

  IDENTIFIER_NOT_USED (s);

  parse_ip(buffer, &ip);

  /* Pack the result in the global parameter structure */
  sys_cfg.gw_addr[0] = htons(ip[0]) >> 8;
  sys_cfg.gw_addr[1] = htons(ip[0]) & 0xff;
  sys_cfg.gw_addr[2] = htons(ip[1]) >> 8;
  sys_cfg.gw_addr[3] = htons(ip[1]) & 0xff;
  need_reset = TRUE;
}
/*---------------------------------------------------------------------------*/
PARAM_FUNC (set_webport)
{
  IDENTIFIER_NOT_USED (s);

  buffer = skip_to_char(buffer, '=');
  if (*buffer != ISO_and) {
    sys_cfg.http_port = atoi(buffer);
    /* Block non http ports below 80 */
    if (sys_cfg.http_port < 80) {
      sys_cfg.http_port = 80;
    }
    need_reset = TRUE;
  }
}
/*---------------------------------------------------------------------------*/
PARAM_FUNC (reset_time)
{
  IDENTIFIER_NOT_USED (buffer);
  IDENTIFIER_NOT_USED (s);

  sys_cfg.enable_time = 0;
}
/*---------------------------------------------------------------------------*/
PARAM_FUNC (enable_time)
{
  IDENTIFIER_NOT_USED (s);

  buffer = skip_to_char(buffer, '=');
  if (strncmp(buffer, "on", 2) == 0) {
    /* Make sure we try to get a new time */
    RTC_GET_TIME_EVENT = 1;
    sys_cfg.enable_time = 1;
  } else if (strncmp(buffer, "off", 3) == 0) {
    sys_cfg.enable_time = 0;
  }
}
/*---------------------------------------------------------------------------*/
PARAM_FUNC (set_time)
{
  static uip_ipaddr_t ip;

  IDENTIFIER_NOT_USED (s);

  parse_ip(buffer, &ip);

  /* Pack the result in the global parameter structure */
  sys_cfg.time_server[0] = htons(ip[0]) >> 8;
  sys_cfg.time_server[1] = htons(ip[0]) & 0xff;
  sys_cfg.time_server[2] = htons(ip[1]) >> 8;
  sys_cfg.time_server[3] = htons(ip[1]) & 0xff;
}
/*---------------------------------------------------------------------------*/
PARAM_FUNC (set_timeport)
{
  IDENTIFIER_NOT_USED (s);

  buffer = skip_to_char(buffer, '=');
  if (*buffer != ISO_and)
    sys_cfg.time_port = atoi(buffer);
}
/*---------------------------------------------------------------------------*/
PARAM_FUNC (set_interval)
{
  IDENTIFIER_NOT_USED (s);

  buffer = skip_to_char(buffer, '=');
  if (*buffer != ISO_and) {
    sys_cfg.update_interval = atoi(buffer);
    /* Update Interval can never be 0, so adjust */
    if (!sys_cfg.update_interval)
      sys_cfg.update_interval = 1;
  }
}
/*---------------------------------------------------------------------------*/
PARAM_FUNC (set_timevalue)
{
  IDENTIFIER_NOT_USED (s);
  /* Here we need to parse the time value and store it before creating a 32 bit
    * binary time value
    * Note that if the option to use a time server we do not parse these values
    */
  if (!sys_cfg.enable_time) {
    buffer = skip_to_char(buffer, '=');
    if (*buffer != ISO_and) {
      hrs = atoi(buffer);
      /* We should be checking for a colon here but since it has been converted
       * to web characters it is now '%3A'. So we check for a percent and
       * skip ahead two characters. Crude, but it works =)
       */
      buffer = skip_to_char(buffer, '%');
      buffer += 2;
      min = atoi(buffer);
      need_time_update = TRUE;
    }
  }
}
/*---------------------------------------------------------------------------*/
PARAM_FUNC (set_datevalue)
{
  IDENTIFIER_NOT_USED (s);
  /* Here we need to parse the date value and store it before creating a 32 bit
    * binary time value
    * Note that if the option to use a time server we do not parse these values
    */
  if (!sys_cfg.enable_time) {
    buffer = skip_to_char(buffer, '=');
    if (*buffer != ISO_and) {
      year=atoi(buffer);
      buffer = skip_to_char(buffer, '-');
      month = atoi(buffer);
      buffer = skip_to_char(buffer, '-');
      day = atoi(buffer);
      need_time_update = TRUE;
    }
  }
}
/*---------------------------------------------------------------------------*/
PARAM_FUNC (set_timezone)
{
  static u8_t tz;

  IDENTIFIER_NOT_USED (s);

  buffer = skip_to_char(buffer, '=');
  tz = atoi(buffer);

  sys_cfg.time_zone = tz;
}
/*---------------------------------------------------------------------------*/
struct time_param tp;
PARAM_FUNC (set_save)
{
  IDENTIFIER_NOT_USED (s);

  buffer = skip_to_char(buffer, '=');
  if (strncmp("save", buffer, 4) == 0) {
    /* Write new configuration to flash */
    write_config_to_flash();
    /* In case we are setting the time manually */
    if (need_time_update) {
      need_time_update = 0;
      tp.time.year = year;
      tp.time.month = month;
      tp.time.day = day;
      tp.time.hrs = hrs;
      tp.time.min = min;
      tp.time.sec = 0;
      dat_to_binary(&tp);
      set_g_time(&tp);
      /* Signal the time client to update the hw rtc */
      RTC_SET_HW_RTC = &tp;
    }
    if (need_reset) {
      A_(printf (__FILE__ " Performing a planned software reset !\n");)
      RSTSRC |= 0x10; // Force a software reset
    }
  }
}

/*---------------------------------------------------------------------------*/
PARAM_FUNC (set_username)
{
  u8_t *ptr = sys_cfg.username;
  char i = 8;

  IDENTIFIER_NOT_USED (s);

  buffer = skip_to_char(buffer, '=');

  while ((*buffer != ISO_and) && (i >= 0)) {
    *ptr++ = *buffer++;
    i--;
  }
  *buffer = 0x00;
}

/*---------------------------------------------------------------------------*/
PARAM_FUNC (set_password)
{
  u8_t *ptr = sys_cfg.password;
  char i = 8;

  IDENTIFIER_NOT_USED (s);

  buffer = skip_to_char(buffer, '=');

  while ((*buffer != ISO_and) && (i >= 0)) {
    *ptr++ = *buffer++;
    i--;
  }
  *buffer = 0x00;
}

/*---------------------------------------------------------------------------*/
PARAM_FUNC (cgi_set_channel)
{
  buffer = skip_to_char(buffer, '=');
  s->parms.channel = atoi(buffer);
  s->parms.channel_updated = 1;
  s->parms.num_parms++;
}
/*---------------------------------------------------------------------------*/
PARAM_FUNC (cgi_set_level)
{
  buffer = skip_to_char(buffer, '=');
  s->parms.level = atoi(buffer);
  s->parms.level_updated = 1;
  s->parms.num_parms++;
}
/*---------------------------------------------------------------------------*/
PARAM_FUNC (cgi_set_rampto)
{
  buffer = skip_to_char(buffer, '=');
  s->parms.rampto = atoi(buffer);
  s->parms.rampto_updated = 1;
  s->parms.num_parms++;
}
/*---------------------------------------------------------------------------*/
PARAM_FUNC (cgi_set_rate)
{
  buffer = skip_to_char(buffer, '=');
  s->parms.rate = atoi(buffer);
  s->parms.rate_updated = 1;
  s->parms.num_parms++;
}
/*---------------------------------------------------------------------------*/
PARAM_FUNC (cgi_set_step)
{
  buffer = skip_to_char(buffer, '=');
  s->parms.step = atoi(buffer);
  s->parms.step_updated = 1;
  s->parms.num_parms++;
}

/*---------------------------------------------------------------------------*/
static u8_t parse_expr(struct httpd_state *s, char *buf)
{
  struct parameter_table *tptr = (struct parameter_table *)parmtab;

  while (*tptr->param != '*')
  {
    if ((buf[strlen(tptr->param)] == '=') &&
       (strncmp(buf, tptr->param, strlen(tptr->param)) == 0))
    {
      /* Call the parameter set function */
      tptr->function(s, buf);
      return 1;
    }
    tptr++;
  }
  return 0;
}/*---------------------------------------------------------------------------*/
void parse_input(struct httpd_state *s, char *buf) banked
{
  static char token[128];
  char *tok;

  /* Search for query or end of line */
  while (*buf != ISO_query &&
         *buf != ISO_nl &&
         *buf != ISO_space)
    buf++;

  /* Return if no query is present */
  if (*buf == ISO_nl || *buf == ISO_space)
    return;

  /* Clear the cgi control parameter structure */
  memset (&s->parms, 0, sizeof s->parms);

  while (*buf != ISO_space)
  {
    buf++;
    tok = token;
    while (*buf != ISO_space && *buf != ISO_and)
      *tok++ = *buf++;
    *tok = 0;
    /* Here we simply try to parse the tokenized parameter
     * If the parameter does not exist, we simply discard it silently */
    A_(printf("Parsing token %s\n", token);)
    parse_expr(s, token);
  }
}

/* End of File */
