/**
 * \addtogroup apps
 * @{
 */

/**
 * \defgroup httpd Web server
 * @{
 * The uIP web server is a very simplistic implementation of an HTTP
 * server. It can serve web pages and files from a read-only ROM
 * filesystem, and provides a very small scripting language.

 */

/**
 * \file
 *         Web server
 * \author
 *         Adam Dunkels <adam@sics.se>
 */


/*
 * Copyright (c) 2004, Adam Dunkels.
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
 * Author: Adam Dunkels <adam@sics.se>
 *
 * $Id: httpd.c,v 1.2 2006/06/11 21:46:38 adam Exp $
 */
#pragma codeseg UIP_BANK

#include "uip.h"
#include "httpd.h"
#include "flash.h"
#include "base64.h"
#include "httpd-fs.h"
#include "httpd-cgi.h"
#include "httpd-param.h"
#include "http-strings.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#define STATE_WAITING     0
#define STATE_OUTPUT      1

#define ISO_nl            0x0a
#define ISO_space         0x20
#define ISO_bang          0x21
#define ISO_percent       0x25
#define ISO_period        0x2e
#define ISO_slash         0x2f
#define ISO_colon         0x3a

#define USERNAME_LENGTH   8
#define PASSWORD_LENGTH   8

char auth_password[16] = "dolk";

/*---------------------------------------------------------------------------*/
static unsigned short
generate_part_of_file(struct httpd_state *s) __reentrant
{
  if (s->file.len > uip_mss()) {
    s->len = uip_mss();
  } else {
    s->len = s->file.len;
  }
  memcpy(uip_appdata, s->file.fsdata, s->len);

  return s->len;
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(send_file(struct httpd_state *s) __reentrant)
{
  PSOCK_BEGIN(&s->sout);

  do {
    PSOCK_GENERATOR_SEND(&s->sout, (void*)generate_part_of_file, s);
    s->file.len -= s->len;
    s->file.fsdata += s->len;
  } while (s->file.len > 0);

  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(send_part_of_file(struct httpd_state *s) __reentrant)
{
  PSOCK_BEGIN(&s->sout);

  PSOCK_SEND(&s->sout, s->file.fsdata, s->len);

  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static void
next_scriptstate(struct httpd_state *s) __reentrant
{
  char *p;
  p = strchr(s->scriptptr, ISO_nl) + 1;
  s->scriptlen -= (unsigned short)(p - s->scriptptr);
  s->scriptptr = p;
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(handle_script(struct httpd_state *s) __reentrant)
{
  char *ptr;

  PT_BEGIN(&s->scriptpt);


  while (s->file.len > 0) {

    /* Check if we should start executing a script. */
    if (*s->file.fsdata == ISO_percent &&
        *(s->file.fsdata + 1) == ISO_bang) {
      s->scriptptr = s->file.fsdata + 3;
      s->scriptlen = s->file.len - 3;
      if (*(s->scriptptr - 1) == ISO_colon) {
        httpd_fs_open(s->scriptptr + 1, &s->file);
        PT_WAIT_THREAD(&s->scriptpt, send_file(s));
      } else {
        /* In case the cgi's uses any utility functions */
        PT_INIT(&s->utilpt);
        PT_WAIT_THREAD(&s->scriptpt,
                       httpd_cgi(s->scriptptr)(s, s->scriptptr));
      }
      next_scriptstate(s);

      /* The script is over, so we reset the pointers and continue
      sending the rest of the file. */
      s->file.fsdata = s->scriptptr;
      s->file.len = s->scriptlen;
    } else {
      /* See if we find the start of script marker in the block of HTML
      to be sent. */

      if (s->file.len > uip_mss()) {
        s->len = uip_mss();
      } else {
        s->len = s->file.len;
      }

      if (*s->file.fsdata == ISO_percent) {
        ptr = strchr(s->file.fsdata + 1, ISO_percent);
      } else {
        ptr = strchr(s->file.fsdata, ISO_percent);
      }
      if (ptr != NULL &&
          ptr != s->file.fsdata) {
        s->len = (int)(ptr - s->file.fsdata);
        if (s->len >= uip_mss()) {
          s->len = uip_mss();
        }
      }
      PT_WAIT_THREAD(&s->scriptpt, send_part_of_file(s));
      s->file.fsdata += s->len;
      s->file.len -= s->len;

    }
  }

  PT_END(&s->scriptpt);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(send_headers(struct httpd_state *s,
                       const char *statushdr,
                       int dofil) __reentrant)
{
  char *ptr;

  PSOCK_BEGIN(&s->sout);

  PSOCK_SEND_STR(&s->sout, statushdr);

  if (dofil) {
    ptr = strchr(s->filename, ISO_period);
    if (ptr == NULL) {
      PSOCK_SEND_STR(&s->sout, http_content_type_binary);
    } else if (strncmp(http_html, ptr, 5) == 0 ||
               strncmp(http_shtml, ptr, 6) == 0) {
      PSOCK_SEND_STR(&s->sout, http_content_type_html);
    } else if (strncmp(http_css, ptr, 4) == 0) {
      PSOCK_SEND_STR(&s->sout, http_content_type_css);
    } else if (strncmp(http_png, ptr, 4) == 0) {
      PSOCK_SEND_STR(&s->sout, http_content_type_png);
    } else if (strncmp(http_gif, ptr, 4) == 0) {
      PSOCK_SEND_STR(&s->sout, http_content_type_gif);
    } else if (strncmp(http_jpg, ptr, 4) == 0) {
      PSOCK_SEND_STR(&s->sout, http_content_type_jpg);
  } else if (strncmp(http_cgi, ptr, 4) == 0) {
    PSOCK_SEND_STR(&s->sout, http_content_type_plain);
    } else {
      PSOCK_SEND_STR(&s->sout, http_content_type_plain);
    }
  }
  PSOCK_END(&s->sout);
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(handle_output(struct httpd_state *s) __reentrant)
{
  char *ptr;

  PT_BEGIN(&s->outputpt);

  if (*auth_password) {
    if (!(s->is_authorized)) {
      PT_WAIT_THREAD(&s->outputpt,
                     send_headers(s, http_header_401, 0));
      goto endhandleoutput;
    }
  }
  if (!httpd_fs_open(s->filename, &s->file)) {
    httpd_fs_open(http_404_html, &s->file);
    strcpy(s->filename, http_404_html);
    PT_WAIT_THREAD(&s->outputpt,
                   send_headers(s, http_header_404, 1));
    PT_WAIT_THREAD(&s->outputpt,
                   send_file(s));
  } else {
    PT_WAIT_THREAD(&s->outputpt,
                   send_headers(s, http_header_200, 1));
    ptr = strchr(s->filename, ISO_period);
    if (ptr != NULL && (strncmp(ptr, http_shtml, 6) == 0 ||
                        strncmp(ptr, http_cgi, 4) == 0)) {
      PT_INIT(&s->scriptpt);
      PT_WAIT_THREAD(&s->outputpt, handle_script(s));
    } else {
      PT_WAIT_THREAD(&s->outputpt,
                     send_file(s));
    }
  }
endhandleoutput:
  PSOCK_CLOSE(&s->sout);
  PT_END(&s->outputpt);
}
/*---------------------------------------------------------------------------*/
static int check_authorization(const char *base64pass)
{
  int authed, len;
  static char mypass64[USERNAME_LENGTH*2 + PASSWORD_LENGTH*2 + 2] = { 0 };

  if (!(*mypass64)) {
    char mypass[USERNAME_LENGTH + PASSWORD_LENGTH + 2];
    strncpy (mypass, sys_cfg.username, 8);
    strcat  (mypass, ":");
    strncat (mypass, sys_cfg.password, 8);
    encode64(mypass, mypass64);
  }
  len = strlen(mypass64);
  authed = (strncmp(base64pass, mypass64, len) == 0);
  if ((base64pass[len] != 0) && (!isspace(base64pass[len])))
    authed = 0;
  return authed;
}

/*---------------------------------------------------------------------------*/
static
PT_THREAD(handle_input(struct httpd_state *s) __reentrant)
{
  PSOCK_BEGIN(&s->sin);

  PSOCK_READTO(&s->sin, ISO_space);


  if (strncmp(s->inputbuf, http_get, 4) != 0) {
    PSOCK_CLOSE_EXIT(&s->sin);
  }
  PSOCK_READTO(&s->sin, ISO_space);

  if (s->inputbuf[0] != ISO_slash) {
    PSOCK_CLOSE_EXIT(&s->sin);
  }

  parse_input(s, s->inputbuf);

  if (s->inputbuf[1] == ISO_space) {
    strncpy(s->filename, http_index_html, sizeof(s->filename));
  } else {
    s->inputbuf[PSOCK_DATALEN(&s->sin) - 1] = 0;
    strncpy(s->filename, &s->inputbuf[0], sizeof(s->filename));
  }

  for (;;) {
    PSOCK_READTO(&s->sin, ISO_nl);

    if ((s->inputbuf[0] == '\n') || (s->inputbuf[0] == '\r'))
      break;
    if (strncmp(s->inputbuf, http_content_length, sizeof(http_content_length)-1) == 0)
      s->content_length = atoi(&s->inputbuf[sizeof(http_content_length)-1]);
    if (strncmp(s->inputbuf, http_authorization, sizeof(http_authorization)-1) == 0)
      s->is_authorized = check_authorization(&s->inputbuf[ sizeof(http_authorization)-1 ]);
  }
  /*  httpd_log_file(uip_conn->ripaddr, s->filename);*/

  s->state = STATE_OUTPUT;

  while (1) {
    PSOCK_READTO(&s->sin, ISO_nl);

    if (strncmp(s->inputbuf, http_referer, 8) == 0) {
      s->inputbuf[PSOCK_DATALEN(&s->sin) - 2] = 0;
      /*      httpd_log(&s->inputbuf[9]);*/
    }
  }

  PSOCK_END(&s->sin);
}
/*---------------------------------------------------------------------------*/
static void
handle_connection(struct httpd_state *s) __reentrant
{
  handle_input(s);
  if (s->state == STATE_OUTPUT) {
    handle_output(s);
  }
}
/*---------------------------------------------------------------------------*/
void
httpd_appcall(void) __banked
{
  struct httpd_state *s = (struct httpd_state *)&(uip_conn->appstate);

  if (uip_closed() || uip_aborted() || uip_timedout()) {
  } else if (uip_connected()) {
    PSOCK_INIT(&s->sin, s->inputbuf, sizeof(s->inputbuf) - 1);
    PSOCK_INIT(&s->sout, s->inputbuf, sizeof(s->inputbuf) - 1);
    PT_INIT(&s->outputpt);
    s->state = STATE_WAITING;
    /*    timer_set(&s->timer, CLOCK_SECOND * 100);*/
    s->timer = 0;
    s->content_length = 0;
    s->is_authorized = 0;
    handle_connection(s);
  } else if (s != NULL) {
    if (uip_poll()) {
      ++s->timer;
      if (s->timer >= 20) {
        uip_abort();
      }
    } else {
      s->timer = 0;
    }
    handle_connection(s);
  } else {
    uip_abort();
  }
}
/*---------------------------------------------------------------------------*/
/**
 * \brief      Initialize the web server
 *
 *             This function initializes the web server and should be
 *             called at system boot-up.
 */
void
httpd_init(void) __banked
{
  uip_listen(htons(sys_cfg.http_port));
}
/*---------------------------------------------------------------------------*/
/** @} */
