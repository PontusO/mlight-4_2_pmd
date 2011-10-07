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

#ifndef EVENT_SWITCH_H_INCLUDED
#define EVENT_SWITCH_H_INCLUDED

#define MAX_NR_ACTION_MGRS      20
#define MAX_NR_EVENT_PROVDERS   20
#define MAX_NR_RULES            20

enum action_properties_e {
  ACT_PRP_HAS_VALUE = 0x01,
};

struct action_vt_s {
  void (*stop_action)() __reentrant;
  void (*trigger_action)(void* action_data) __reentrant;
};

struct action_mgr_s {
  void *priv;
  enum action_properties_e props;
  struct action_vt_s vt;
};
typedef struct action_mgr_s action_mgr_t;

struct event_prv_s {
  char signal;
};
typedef struct event_prv_s event_prv_t;

struct rule_s {
  char event;
  char action;
  void *action_data;
};
typedef struct rule_s rule_t;

/* Protothread structure */
struct event_thread_p {
  struct pt pt;
  char current_event;
  char new_action;
};
typedef struct event_thread_p event_thread_t;

char register_action_mgr(struct action_mgr_s *mgr) __reentrant;
void init_event_switch(void);
char register_action_mgr(struct action_mgr_s *mgr) __reentrant;
char unregister_action_mgr(char entry) __reentrant;

#endif // EVENT_SWITCH_H_INCLUDED
