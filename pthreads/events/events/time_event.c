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

#pragma codeseg APP_BANK
#define PRINT_A     // Enable A prints

#include "system.h"
#include "flash.h"
#include "iet_debug.h"
#include "rtc.h"
#include "time_event.h"
#include "string.h"

char *time_event_name = "Time Event";

/*
 * Initialize the time_event pthread
 */
void init_time_event(time_event_t *time_event) __reentrant banked
{
  memset (time_event, 0, sizeof *time_event);
  /* Initialize the event data */
  time_event->event.base.type = EVENT_EVENT_PROVIDER;
  time_event->event.base.name = time_event_name;
  time_event->event.priv = NULL;  /* For, now. Should point to the real event data */
  time_event->event.signal = 0;

  PT_INIT(&time_event->pt);
}

/*
 * Look for a free time event entry in the list.
 */
time_spec_t *get_first_free_time_event_entry(void) __reentrant __banked
{
  time_spec_t *ptr;
  u8_t i;

  ptr = &sys_cfg.time_events[0];

  for (i=0 ; i<NMBR_TIME_EVENTS ; i++) {
    if (!(ptr->status & TIME_EVENT_ENTRY_USED))
      return ptr;
    ptr++;
  }
  return NULL;
}

/*
 * Add a new event to the list
 *
 * Return 0 if operation was ok, otherwise -1
 */
char add_time_event (time_spec_t *ts)
{
  time_spec_t *ptr;

  ptr = get_first_free_time_event_entry();
  if (!ptr) {
    A_(printf (__FILE__ " No free entries left !\n");)
    return -1;
  }
  memcpy (ptr, ts, sizeof ts);
  ptr->status |= TIME_EVENT_ENTRY_USED;
  return 0;
}

PT_THREAD(handle_time_event(time_event_t *time_event) __reentrant banked)
{
  PT_BEGIN(&time_event->pt);

  A_(printf (__FILE__ " Starting time_event pthread!\n");)
  /* Register us as a event provider */
  evnt_register_handle(&time_event->event);

  while (1)
  {
    PT_WAIT_UNTIL(&time_event->pt, RTC_SECOND_EVENT != time_event->sd);
    time_event->sd = RTC_SECOND_EVENT;
    {
      struct time_param tp;
      u8_t i;

      /* get current global time */
      tp.b_time = get_g_time();
      /* Convert to human format */
      binary_to_dat(&tp);
      time_event->time_spec = &sys_cfg.time_events[0];
      for (i=0;i<NMBR_TIME_EVENTS;i++) {
        if (time_event->time_spec->status & (TIME_EVENT_ENABLED | TIME_EVENT_ENTRY_USED) &&
            time_event->time_spec->hrs == tp.time.hrs &&
            time_event->time_spec->min == tp.time.min &&
            tp.time.sec == 0) {
          A_(printf (__FILE__ " Handling a time event !\n");)
            /* "Sound the alarm", note that the event need to be initialized before
             * being sent */
            // time_event->event.signal = 1;
        }
        time_event->time_spec++;
      }
    }
  }

  PT_END(&time_event->pt);
}

/* EOF */
