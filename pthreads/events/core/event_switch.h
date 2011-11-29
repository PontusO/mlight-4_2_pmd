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

#ifndef EVENT_SWITCH_H_INCLUDED
#define EVENT_SWITCH_H_INCLUDED

#define MAX_NR_ACTION_MGRS      20
#define MAX_NR_EVENT_PROVIDERS  20
#define MAX_NR_RULES            20

enum event_type_e {
  EVENT_EVENT_PROVIDER = 1,
  EVENT_ACTION_MANAGER,
  EVENT_RULE
};
typedef enum event_type_e event_type_t;

/** Event base data structure
 * All event providers, action managers and rules inherit
 * these properties.
 */
typedef struct event_base_s {
  event_type_t type;
  char *name;
  char enabled;
} event_base_t;

/**
 * A data type that is used to specify the properties of an
 * action manager. This shall be used to ensure that a event provider
 * is connected to a compatible action manager.
 */
typedef enum action_properties_e {
  ACT_PRP_ABSOLUTE_VALUE = 0x01,
  ACT_PRP_RAMP_VALUE = 0x02,
} action_properties_t;

/**
 * Every action manager need to implement these function.
 * A stateless manager can simply return without any action from the
 * stop method.
 */
typedef struct action_vt_s {
  void (*stop_action)() __reentrant;
  void (*trigger_action)(void* action_data) __reentrant;
} action_vt_t;

/**
 * The action manager handle.
 * A manager need to fill out these fields before registering with
 * the event switch.
 */
typedef struct action_mgr_s {
  event_base_t base;
  enum action_properties_e props;
  struct action_vt_s vt;
} action_mgr_t;

/**
 * The event provider handle.
 * All event providers need to create its own instance of this data
 * structure and fill out the fields before registering the handle
 * with the event switch.
 */
typedef struct event_prv_s {
  event_base_t base;
  void *priv;
  char signal;
} event_prv_t;

/**
 * The rule handle.
 * All rules need to have its own copy of this data structure.
 */
typedef struct rule_s {
  event_base_t base;
  char event;
  char action;
} rule_t;

/* Protothread structure */
typedef struct event_thread_p {
  struct pt pt;
  char current_event;
  char new_action;
} event_thread_t;

char evnt_register_handle(void *handler) __reentrant;
void init_event_switch(event_thread_t *et);
PT_THREAD(handle_event_switch(event_thread_t *et) __reentrant);

#endif // EVENT_SWITCH_H_INCLUDED
