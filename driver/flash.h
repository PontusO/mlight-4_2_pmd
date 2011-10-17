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

#ifndef _FLASH_H_
#define _FLASH_H_
#include "system.h"


void flash_write_config(void);
/* write flash */
void write_config_to_flash(void);

struct sys_config
{
  struct uip_eth_addr mac_addr;   /* MAC address of the node */
  char device_id[8];              /* Device ID, allocate 8 bytes for this */
  char tv_number;                 /* Which device in a household */
  u8_t ip_addr[4];                /* IP address of the node */
  u8_t netmask[4];                /* network mask of the node */
  u8_t gw_addr[4];                /* the default gateway of the node */
  u16_t http_port;                /* Port for the webserver */
  u8_t enable_time;               /* 0 = Do not use time server, 1 = Use time server */
  u8_t time_server[4];            /* The systems time server address */
  u16_t time_port;                /* The port of the time server */
  u16_t update_interval;          /* number of hours between rtc updates */
  char time_zone;                 /* The offset from GMT for our displayed time */
  u8_t remote_repeat_time;        /* A timeout used to determine remote key repeat */
  u8_t remote_repeat_rate;        /* How fast the repeat function is */
  u16_t channel_lock;             /* Timeout before a channel selection locks */
  u16_t login_time_out;           /* Time before the PMD starts to request login */
  u16_t login_allow;              /* Time allowed for the user to login */
  u16_t login_time;               /* How long the user shall be logged in */
  u16_t discr_time;               /* Timeout used to qualify a valid channel selection */
  u8_t color_key_map[4];          /* Mapping of color keys */
  u8_t enable_upgrades;           /* Determines wether automtic upgrades are enabled or not */
  u8_t multicast_group[4];        /* This holds the current multicast group */
  u8_t channelmap[100];           /* Array that holds the channel map */
  u8_t remote_long_press;         /* Time out for detecting long presses on a remote button */
  char username[9];               /* Authentication user name */
  char password[9];               /* Authentication password */
};

#define CONFIG_MEM_SIZE ((u16_t)sizeof(struct sys_config))

#define LAST_PAGE_ADDRESS 0xFC00

extern struct sys_config sys_cfg;
extern const struct sys_config default_cfg;

/* load the system config from the flash when the system is powered on. */
void load_sys_config(void);
void load_default_config(void);
void load_network_params(void);
u8_t validate_config_flash(void);
void sys_getethaddr(struct uip_eth_addr *addr);
void erase_config_area(xdata u8_t *erase_ptr, u8_t bank);
void write_to_flash(xdata u8_t *flash_ptr, u8_t *ram_ptr, u16_t len, u8_t bank);

#endif
