/*
 * Copyright (c) 2012, Pontus Oldberg.
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
#ifndef DIG_EVENT_H_INCLUDED
#define DIG_EVENT_H_INCLUDED

/* Some usefull macros */
#define NUMBER_OF_DIG_INPUTS    2
#define ITERATE_BUTTONS(x)      for (x=0;x<NUMBER_OF_DIG_INPUTS;x++)

#define ALL_BUTTONS_MASK        0x60
#define BUTTON_PORT             P1

typedef struct {
  u8_t mode;                  /* Select the operation mode of the input pin */
  u8_t inverted;              /* Selects if the pin is inverted or not */
} dig_data_t;

typedef struct {
  struct pt pt;
  u8_t old_state;             /* Last state for all buttons */
  u8_t state;                 /* Current button state */
  u8_t mask;                  /* Temporary container for the button mask */
  u8_t tmr;                   /* Local Timer */
  u8_t i;
  dig_data_t *dptr;           /* Pointer to flash memory data */
  struct rule_data_s *rdata;  /* Pointer to dynamic rule data */
} dig_event_t;

void init_dig_event(dig_event_t *dig_event) __reentrant __banked;
PT_THREAD(handle_dig_event(dig_event_t *dig_event) __reentrant __banked);

#endif // DIG_EVENT_H_INCLUDED
