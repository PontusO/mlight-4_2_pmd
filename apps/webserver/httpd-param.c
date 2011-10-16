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

#include "system.h"
#include "uip.h"
#include "httpd-param.h"
#include "httpd-cgi.h"
#include "flash.h"
#include "rtc.h"
#include "iet_debug.h"

#include <string.h>
#include <stdlib.h>

/*---------------------- Local data types -----------------------------------*/
struct parameter_table {
  const char *param;
  void (*function)(char *buffer) __reentrant;
};

/*---------------------- Local function prototypes --------------------------*/
static void set_ip(char *buffer) __reentrant;
static void set_netmask(char *buffer) __reentrant;
static void set_gateway(char *buffer) __reentrant;

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
        "devid",
        set_device_id
    },
    {
        "tvid",
        set_tv_id
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
    {
        "rdrt",   /* Remote detect repeat timeout*/
        set_rdrt
    },
    {
        "rdrr",   /* Remote repeat rate */
        set_rdrr
    },
    {
        "discr",  /* Channel commitment time */
        set_discr
    },
    {
        "red",  /* Channel commitment time */
        set_red
    },
    {
        "green",  /* Channel commitment time */
        set_green
    },
    {
        "yellow",  /* Channel commitment time */
        set_yellow
    },
    {
        "blue",  /* Channel commitment time */
        set_blue
    },
    {
        "chlock",  /* Channel commitment time */
        set_chlock
    },
    {
        "loginto",  /* Channel commitment time */
        set_loginto
    },
    {
        "loginend",  /* Channel commitment time */
        set_loginend
    },
    {
        "logintime",  /* Channel commitment time */
        set_logintime
    },
    {
        "enableup",  /* Channel commitment time */
        set_enable_upgrade
    },
    {
        "resetup",  /* Channel commitment time */
        set_reset_upgrade
    },
    {
        "mcast-ip",  /* Multi cast address */
        set_mcast_ip
    },
    {
        "reload",   /* Reload parameter for the channels.shtml page */
        set_reload
    },
    {
        "interval",   /* Reload parameter for the channels.shtml page */
        set_channel_interval
    },
    {
        "channel",    /* Channel field, used by get-chan-map.cgi */
        set_channel_channel
    },
    {
        "button",    /* Button field, used by get-button-map-cgi cgi */
        set_button
    },

    {
        "ch",         /* Channel field, used by channels.shtml */
        set_channel_ch
    },
    /* This is the last parameter in the list of html parameters.
     * It's used to trigger the real flash save function
     */
    {
        "submit",
        set_save
    },
    /* This entry is only used by the channels page due to a conflict with
     * the objects on the webpage. */
    {
        "send",
        set_save
    },
    /*
     * Below are cgi parameters
     */
    {
        "start",
        cgi_start
    },
    {
        "end",
        cgi_end
    },
    {
        "*",
        0
    }
};

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
static void set_device_id(char *buffer) __reentrant
{
  u8_t *ptr = sys_cfg.device_id;
  char i = 8;

  buffer = skip_to_char(buffer, '=');

  while ((*buffer != ISO_and) && (i >= 0)) {
    *ptr++ = *buffer++;
    i--;
  }
  *buffer = 0x00;
}

/*---------------------------------------------------------------------------*/
static void set_tv_id(char *buffer) __reentrant
{
  buffer = skip_to_char(buffer, '=');

  sys_cfg.tv_number = atoi(buffer);
  if (sys_cfg.tv_number > 5)
    sys_cfg.tv_number  = 5;
}

/*---------------------------------------------------------------------------*/
static void set_ip(char *buffer) __reentrant
{
  static uip_ipaddr_t ip;

  parse_ip(buffer, &ip);

  /* Pack the result in the global parameter structure */
  sys_cfg.ip_addr[0] = htons(ip[0]) >> 8;
  sys_cfg.ip_addr[1] = htons(ip[0]) & 0xff;
  sys_cfg.ip_addr[2] = htons(ip[1]) >> 8;
  sys_cfg.ip_addr[3] = htons(ip[1]) & 0xff;
}

/*---------------------------------------------------------------------------*/
static void set_netmask(char *buffer) __reentrant
{
  static uip_ipaddr_t ip;

  parse_ip(buffer, &ip);

  /* Pack the result in the global parameter structure */
  sys_cfg.netmask[0] = htons(ip[0]) >> 8;
  sys_cfg.netmask[1] = htons(ip[0]) & 0xff;
  sys_cfg.netmask[2] = htons(ip[1]) >> 8;
  sys_cfg.netmask[3] = htons(ip[1]) & 0xff;
}
/*---------------------------------------------------------------------------*/
static void set_gateway(char *buffer) __reentrant
{
  static uip_ipaddr_t ip;

  parse_ip(buffer, &ip);

  /* Pack the result in the global parameter structure */
  sys_cfg.gw_addr[0] = htons(ip[0]) >> 8;
  sys_cfg.gw_addr[1] = htons(ip[0]) & 0xff;
  sys_cfg.gw_addr[2] = htons(ip[1]) >> 8;
  sys_cfg.gw_addr[3] = htons(ip[1]) & 0xff;
}
/*---------------------------------------------------------------------------*/
static void reset_time(char *buffer) __reentrant
{
  IDENTIFIER_NOT_USED(buffer);

  sys_cfg.enable_time = 0;
}
/*---------------------------------------------------------------------------*/
static void enable_time(char *buffer) __reentrant
{
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
static void set_time(char *buffer) __reentrant
{
  static uip_ipaddr_t ip;

  parse_ip(buffer, &ip);

  /* Pack the result in the global parameter structure */
  sys_cfg.time_server[0] = htons(ip[0]) >> 8;
  sys_cfg.time_server[1] = htons(ip[0]) & 0xff;
  sys_cfg.time_server[2] = htons(ip[1]) >> 8;
  sys_cfg.time_server[3] = htons(ip[1]) & 0xff;
}
/*---------------------------------------------------------------------------*/
static void set_timeport(char *buffer) __reentrant
{
  buffer = skip_to_char(buffer, '=');
  if (*buffer != ISO_and)
    sys_cfg.time_port = atoi(buffer);
}
/*---------------------------------------------------------------------------*/
static void set_interval(char *buffer) __reentrant
{
  buffer = skip_to_char(buffer, '=');
  if (*buffer != ISO_and)
    sys_cfg.update_interval = atoi(buffer);
}
/*---------------------------------------------------------------------------*/
static u8_t hrs;
static u8_t min;
static bit  need_time_update = FALSE;
static void set_timevalue(char* buffer) __reentrant
{
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
static u16_t year;
static u8_t month;
static u8_t day;
/*---------------------------------------------------------------------------*/
static void set_datevalue(char* buffer) __reentrant
{
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
static void set_timezone(char *buffer) __reentrant
{
  static u8_t tz;

  buffer = skip_to_char(buffer, '=');
  tz = atoi(buffer);

  sys_cfg.time_zone = tz;
}
/*---------------------------------------------------------------------------*/
static void set_rdrt(char *buffer) __reentrant
{
  static u8_t rdrt;

  buffer = skip_to_char(buffer, '=');
  rdrt = atoi(buffer);

  sys_cfg.remote_repeat_time = rdrt;
}
/*---------------------------------------------------------------------------*/
static void set_rdrr(char *buffer) __reentrant
{
  static u8_t rdrr;

  buffer = skip_to_char(buffer, '=');
  rdrr = atoi(buffer);

  sys_cfg.remote_repeat_rate = rdrr;
}
/*---------------------------------------------------------------------------*/
static void set_discr(char *buffer) __reentrant
{
  static u16_t discr;

  buffer = skip_to_char(buffer, '=');
  discr = atoi(buffer);

  sys_cfg.discr_time = discr;
}
/*---------------------------------------------------------------------------*/
static void set_red(char *buffer) __reentrant
{
  buffer = skip_to_char(buffer, '=');
  sys_cfg.color_key_map[0] = atoi(buffer);
}
/*---------------------------------------------------------------------------*/
static void set_green(char *buffer) __reentrant
{
  buffer = skip_to_char(buffer, '=');
  sys_cfg.color_key_map[1] = atoi(buffer);
}
/*---------------------------------------------------------------------------*/
static void set_yellow(char *buffer) __reentrant
{
  buffer = skip_to_char(buffer, '=');
  sys_cfg.color_key_map[2] = atoi(buffer);
}
/*---------------------------------------------------------------------------*/
static void set_blue(char *buffer) __reentrant
{
  buffer = skip_to_char(buffer, '=');
  sys_cfg.color_key_map[3] = atoi(buffer);
}
/*---------------------------------------------------------------------------*/
static void set_chlock(char *buffer) __reentrant
{
  buffer = skip_to_char(buffer, '=');
  sys_cfg.channel_lock = atoi(buffer);
}
/*---------------------------------------------------------------------------*/
static void set_loginto(char *buffer) __reentrant
{
  buffer = skip_to_char(buffer, '=');
  sys_cfg.login_time_out = atoi(buffer);
}
/*---------------------------------------------------------------------------*/
static void set_loginend(char *buffer) __reentrant
{
  buffer = skip_to_char(buffer, '=');
  sys_cfg.login_allow = atoi(buffer);
}
/*---------------------------------------------------------------------------*/
static void set_logintime(char *buffer) __reentrant
{
  buffer = skip_to_char(buffer, '=');
  sys_cfg.login_time = atoi(buffer);
}
/*---------------------------------------------------------------------------*/
static void set_reset_upgrade(char *buffer) __reentrant
{
  IDENTIFIER_NOT_USED(buffer);
  sys_cfg.enable_upgrades = 0;
}
/*---------------------------------------------------------------------------*/
static void set_enable_upgrade(char *buffer) __reentrant
{
  buffer = skip_to_char(buffer, '=');
  if (strncmp(buffer, "on", 2) == 0) {
    sys_cfg.enable_upgrades = 1;
  } else if (strncmp(buffer, "off", 3) == 0) {
    sys_cfg.enable_upgrades = 0;
  }
}
/*---------------------------------------------------------------------------*/
static void set_mcast_ip(char *buffer) __reentrant
{
  static uip_ipaddr_t ip;

  parse_ip(buffer, &ip);

  /* Pack the result in the global parameter structure */
  sys_cfg.multicast_group[0] = htons(ip[0]) >> 8;
  sys_cfg.multicast_group[1] = htons(ip[0]) & 0xff;
  sys_cfg.multicast_group[2] = htons(ip[1]) >> 8;
  sys_cfg.multicast_group[3] = htons(ip[1]) & 0xff;
}

u8_t channels_reload;
/*---------------------------------------------------------------------------*/
static void set_reload(char *buffer) __reentrant
{
  buffer = skip_to_char(buffer, '=');
  channels_reload = atoi(buffer);
}

u8_t channels_interval;
/*---------------------------------------------------------------------------*/
static void set_channel_interval(char *buffer) __reentrant
{
  buffer = skip_to_char(buffer, '=');
  channels_interval = atoi(buffer);
}

u8_t cgi_button;
/*---------------------------------------------------------------------------*/
static void set_button(char *buffer) __reentrant
{
  buffer = skip_to_char(buffer, '=');
  cgi_button = atoi(buffer);
}

/*---------------------------------------------------------------------------*/
static void set_channel_ch(char *buffer) __reentrant
{
  u8_t ch, val;

  /* Only update the when we are doing a real save */
  if (!channels_reload) {
    /* Get the channel number from the field ID */
    ch = atoi(buffer) + (channels_interval * 10);
    buffer = skip_to_char(buffer, '=');
    /* Get mapped channel value that we should store */
    val = atoi(buffer);
    sys_cfg.channelmap[ch] = val;
  }
}

/*---------------------------------------------------------------------------*/
static void set_channel_channel(char *buffer) __reentrant
{
  buffer = skip_to_char(buffer, '=');
  gcgi_channel = atoi(buffer);
  /* Check limits */
  if (gcgi_channel > 99)
    gcgi_channel = 99;
}

/*---------------------------------------------------------------------------*/
struct time_param tp;
static void set_save(char *buffer) __reentrant
{
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
  }
}

/*---------------------------------------------------------------------------*/
static void cgi_start(char *buffer) __reentrant
{
  buffer = skip_to_char(buffer, '=');
  gcgi_start = atoi(buffer);
}

/*---------------------------------------------------------------------------*/
static void cgi_end(char *buffer) __reentrant
{
  buffer = skip_to_char(buffer, '=');
  gcgi_end = atoi(buffer);
}

/*---------------------------------------------------------------------------*/
static u8_t parse_expr(char *buf)
{
  struct parameter_table *tptr = parmtab;

  while (*tptr->param != '*')
  {
    if (strncmp(buf, tptr->param, strlen(tptr->param)) == 0)
    {
      /* Call the setter */
      tptr->function(buf);
      return 1;
    }
    tptr++;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void parse_input(char *buf) banked
{
  static char token[128];
  char *tok;

  /* Search for query or end of line */
  while (*buf != ISO_query &&
         *buf != ISO_nl &&
         *buf != ISO_space)
    buf++;

  /* Return if no query is present */
  if (*buf != ISO_query)
    return;

  while (*buf != ISO_space)
  {
    buf++;
    tok = token;
    while (*buf != ISO_space && *buf != ISO_and)
      *tok++ = *buf++;
    *tok = 0;
    /* Here we simply try to parse the tokenized parameter
     * If the parameter does not exist, we simply discard it silently */
    A_(printf("Parsing token %s\r\n", token);)
    parse_expr(token);
  }
}

/*---------------------------------------------------------------------------*/
static void set_username(char *buffer) __reentrant
{
  u8_t *ptr = sys_cfg.username;
  char i = 8;

  buffer = skip_to_char(buffer, '=');

  while ((*buffer != ISO_and) && (i >= 0)) {
    *ptr++ = *buffer++;
    i--;
  }
  *buffer = 0x00;
}

/*---------------------------------------------------------------------------*/
static void set_password(char *buffer) __reentrant
{
  u8_t *ptr = sys_cfg.password;
  char i = 8;

  buffer = skip_to_char(buffer, '=');

  while ((*buffer != ISO_and) && (i >= 0)) {
    *ptr++ = *buffer++;
    i--;
  }
  *buffer = 0x00;
}

/* End of File */
