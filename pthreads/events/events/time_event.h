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
#ifndef TIME_EVENT_H_INCLUDED
#define TIME_EVENT_H_INCLUDED

#include "event_switch.h"

#define NMBR_TIME_EVENTS    16
#define TIME_EVENT_NAME_LEN 8

/**
 * This is a bit field with the following functions
 *
 * Bit 0 - Indicates if the user has enabled or disabled the time event.
 *         0 means that the event is disabled
 *         1 means that the event is enabled
 * Bit 1 - Indicates if this entry holds a valid time event entry or not
 *         0 means that the entry has not yet been configured or has been
 *           deleted.
 *         1 indicates that the entry is configured and valid.
 */
typedef enum {
  TIME_EVENT_ENABLED    = 0x01,
  TIME_EVENT_ENTRY_USED = 0x02
} time_event_status_t;

typedef struct {
  time_event_status_t status;
  char name[TIME_EVENT_NAME_LEN+1]; /* Allow for a 0 terminator */
  u8_t hrs;
  u8_t min;
  u8_t weekday;
} time_spec_t;

typedef struct {
  struct pt pt;
  char sd;
  time_spec_t *time_spec;
  event_prv_t event;
} time_event_t;

void init_time_event(time_event_t *time_event) __reentrant __banked;
time_spec_t *get_first_free_time_event_entry(void) __reentrant __banked;
PT_THREAD(handle_time_event(time_event_t *time_event) __reentrant __banked);

#endif // TIME_EVENT_H_INCLUDED
