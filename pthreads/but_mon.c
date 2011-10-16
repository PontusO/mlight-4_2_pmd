/*
 * Copyright (c) 2011, Pontus Oldberg.
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

//#define PRINT_A     // Enable A prints
#include <system.h>
#include <but_mon.h>
#include "iet_debug.h"
#include <string.h>
#include <stdlib.h>

#define   KEY1      0x40
#define   KEY2      0x20
#define   ALL_KEYS  (KEY1 | KEY2)

/*
 * Initialize the button monitor
 */
void init_but_mon(but_mon_t *but_mon) __reentrant __banked
{
  memset (but_mon, 0, sizeof *but_mon);
  PT_INIT(&but_mon->pt);
}

PT_THREAD(handle_but_mon(but_mon_t *but_mon) __reentrant __banked)
{
  PT_BEGIN(&but_mon->pt);

  /* Set initial values for LED outputs */
  P1_4 = !P1_5;
  P1_3 = !P1_6;

  while (1)
  {
    PT_WAIT_UNTIL (&but_mon->pt, (P1 & ALL_KEYS) != but_mon->prev_but_val);
    but_mon->but_val = P1 & ALL_KEYS;
    B_(syslog ("Switch changed %d\n", but_mon->but_val);)
    P1_4 = !P1_5;
    P1_3 = !P1_6;
    but_mon->prev_but_val = but_mon->but_val;
  }
  PT_END(&but_mon->pt);
}
