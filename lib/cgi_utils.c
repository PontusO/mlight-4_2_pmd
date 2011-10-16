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
#pragma codeseg   UIP_BANK

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "uip.h"
#include "cgi_utils.h"
#include "httpd-param.h"
#include "flash.h"

static char i;
static char tstr[32];

PT_THREAD(get_tz_options_util(struct httpd_state *s) __reentrant banked)
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

/* EOF */
