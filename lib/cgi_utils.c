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
#pragma codeseg   APP_BANK

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "uip.h"
#include "cgi_utils.h"
#include "httpd-param.h"
#include "flash.h"

static char i;
static char tstr[32];

/*---------------------------------------------------------------------------*/
static char *skip_to_char(char *buf, char chr) __reentrant
{
  while (*buf != chr)
    buf++;
  buf++;

  return buf;
}

/*---------------------------------------------------------------------------*/
PT_THREAD(get_tz_options_util(struct httpd_state *s) __reentrant __banked)
{
  PSOCK_BEGIN(&s->sout);

  for (i = -11 ; i<12 ; i++) {
    sprintf(uip_appdata, "<option value='%d'", i);
    /* Mark the selected option */
    if (i == sys_cfg.time_zone) {
      sprintf(tstr, " selected>GMT");
    } else {
      sprintf(tstr, ">GMT");
    }
    strcat(uip_appdata, tstr);
    /* GMT has no values */
    if (i == 0)
      sprintf(tstr, "</option>");
    else if (i < 0)
      sprintf(tstr, " %d hrs</option>", i);
    else
      sprintf(tstr, " +%d hrs</option>", i);
    strcat(uip_appdata, tstr);
    PSOCK_SEND_STR(&s->sout, uip_appdata);
  }

  PSOCK_END(&s->sout);
}

/* Parameter functions */
/*---------------------------------------------------------------------------*/
void x_set_mapcmd(struct httpd_state *s, char *buffer) __reentrant __banked
{
  u32_t lst=1;
  u8_t i;
  rule_t *rp = &sys_cfg.rules[0];

  buffer = skip_to_char(buffer, '=');

  if (strncmp("delete", buffer, 6) == 0) {
    /* Go through all time events */
    for (i=0; i<MAX_NR_RULES; i++) {
      /* Check only entries in the marklist */
      if (rp->status != RULE_STATUS_FREE) {
        if ( s->parms.marklist & lst) {
          memset (rp, 0, sizeof *rp);
          rp->status = RULE_STATUS_FREE;
        }
        /* Move to next entry in the marklist */
        lst <<= 1;
      }
      /* Move to next entry in the list of rules */
      rp++;
    }
    /* Write new configuration to flash */
    write_config_to_flash();
  } else if (strncmp("modify", buffer, 6) == 0) {
    printf ("Setting modify in mapcmd\n");
    s->parms.modify = TRUE;
    /* Look for the entry to modify */
    for (i=0; i<MAX_NR_RULES; i++) {
      /* Check only used entries */
      if (rp->status != RULE_STATUS_FREE) {
        /* Check if this entry is in the marklist */
        if (s->parms.marklist & lst) {
          /* Set the ts entry to modify */
          s->parms.rp = rp;
          /* We use the first entry we find, so break here */
          break;
        }
        /* Move to next entry */
        lst <<= 1;
      }
      /* Move to next active entry in the list */
      rp++;
    }
  }
}

/*---------------------------------------------------------------------------*/
void x_set_tscmd(struct httpd_state *s, char *buffer) __reentrant __banked
{
  u16_t lst=1;
  u8_t i;
  time_spec_t *ts = &sys_cfg.time_events[0];

  buffer = skip_to_char(buffer, '=');

  if (strncmp("delete", buffer, 6) == 0) {
    /* Go through all time events */
    for (i=0; i<NMBR_TIME_EVENTS; i++) {
      /* Check only used entries */
      if (ts->status & TIME_EVENT_ENTRY_USED) {
        /* Check if this entry is on the delete list */
        if (s->parms.marklist & lst)
          ts->status &= ~TIME_EVENT_ENTRY_USED;
        /* Move to next entry */
        lst <<= 1;
      }
      /* Move to next active entry in the list */
      ts++;
    }
    /* Write new configuration to flash */
    write_config_to_flash();
  } else if (strncmp("modify", buffer, 6) == 0) {
    printf ("Setting modify in tscmd\n");
    s->parms.modify = TRUE;
    /* Look for the entry to modify */
    for (i=0; i<NMBR_TIME_EVENTS; i++) {
      /* Check only used entries */
      if (ts->status & TIME_EVENT_ENTRY_USED) {
        /* Check if this entry is in the marklist */
        if (s->parms.marklist & lst) {
          /* Set the ts entry to modify */
          s->parms.ts = ts;
          /* We only use the first entry so break here */
          break;
        }
        /* Move to next entry */
        lst <<= 1;
      }
      /* Move to next active entry in the list */
      ts++;
    }
  }
}

/* EOF */
