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
#ifndef REMOTE_H_INCLUDED
#define REMOTE_H_INCLUDED

enum remote_keys {
  DIGIT_0 = 0,
  DIGIT_1,
  DIGIT_2,
  DIGIT_3,
  DIGIT_4,
  DIGIT_5,
  DIGIT_6,
  DIGIT_7,
  DIGIT_8,
  DIGIT_9,
  DIGIT_RED,
  DIGIT_GREEN,
  DIGIT_YELLOW,
  DIGIT_BLUE,
  DIGIT_UP,
  DIGIT_DOWN,
  DIGIT_PWR
};

enum REPEAT {
  RPT_NO_REPEAT = 0x00,
  RPT_REPEAT_ON,
  RPT_LONG_PRESS_REACHED
};
struct cremote {
  struct pt pt;
  u8_t length;
  u8_t detect_timer;
  u8_t repeat_timer;
  u8_t repeat_status;
};

struct remote_trainer {
  struct pt pt;
  u8_t key;
  u8_t length;
  u8_t timer;
  int i;
  char ledstr[3];
};

void remote_init(void) banked;
unsigned char remote_get_packet_length(void) banked;
unsigned int remote_get_array(u8_t n) banked;
char get_remote_key(void) banked;
void copy_rc_to_flash(void) banked;
void clear_rc_flash(void) banked;

extern bit SIG_REMOTE_PROCESS;
extern bit ENABLE_REMOTE_TRAINER_MODE;
extern struct cremote cremote;
extern struct remote_trainer rtm;

#define REMOTE_DORMANT      1
#define REMOTE_STARTBIT     2
#define REMOTE_MEASURING    3

#define RUN                 1
#define STOP                0

/**
 * Interface declarations
 */
PT_THREAD(handle_rtm(struct remote_trainer *rtm) __reentrant banked);
PT_THREAD(handle_remote(struct cremote *Remote) __reentrant banked);

#endif // REMOTE_H_INCLUDED
