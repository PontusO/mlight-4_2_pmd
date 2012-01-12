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
#define PRINT_B

#include "system.h"
#include "pt.h"
#include "iet_debug.h"
#include "event_switch.h"
#include "string.h"

static  char nr_registered_actions;
static  action_mgr_t *action_table[MAX_NR_ACTION_MGRS];
static  char nr_registered_events;
static  event_prv_t *event_table[MAX_NR_EVENT_PROVIDERS];
static  char nr_registered_rules;
static  rule_t *rule_table[MAX_NR_RULES];

void init_event_switch(event_thread_t *et)
{
  PT_INIT (&et->pt);

  nr_registered_actions = 0;
  nr_registered_events = 0;
  nr_registered_rules = 0;
  memset (action_table, 0, sizeof *action_table);
  memset (event_table, 0, sizeof *event_table);
  memset (rule_table, 0, sizeof *rule_table);

  B_(printf(__FILE__ " Action table ptr %p\n", action_table);)
  B_(printf(__FILE__ " Event table ptr %p\n", event_table);)
  B_(printf(__FILE__ " Rule table ptr %p\n", rule_table);)
}

/* **************************** Events ***********************************/
/**
 * Lookup the ID number from a supplied pointer. If the provider was
 * found the corresponding ID is returned, otherwise -1 is returned.
 */
/* TODO: Rewrite to handle all event types */
char get_num_from_event (event_prv_t *ptr) __reentrant __banked
{
  u8_t i;

  for (i=0; i<MAX_NR_EVENT_PROVIDERS; i++) {
    if (event_table[i] && event_table[i] == ptr)
      return i;
  }
  return -1;
}

/**
 * Lookup the ID number from a supplied pointer. If the provider was
 * found the corresponding ID is returned, otherwise -1 is returned.
 */
char get_num_from_action (action_mgr_t *ptr) __reentrant __banked
{
  u8_t i;

  for (i=0; i<MAX_NR_EVENT_PROVIDERS; i++) {
    if (action_table[i] && action_table[i] == ptr)
      return i;
  }
  return -1;
}

event_prv_t *get_event_from_num (u8_t entry) __reentrant __banked
{
  if (entry < MAX_NR_EVENT_PROVIDERS)
    return event_table[entry];
  else
    return NULL;
}

action_mgr_t *get_action_from_num (u8_t entry) __reentrant __banked
{
  if (entry < MAX_NR_ACTION_MGRS)
    return action_table[entry];
  else
    return NULL;
}

static char find_first_free_entry(void **table, char max)
{
  char i;

  B_(printf(__FILE__ " table pointer %p\n", table);)
  for (i=0; i<max; i++) {
    if (!table[i])
      return i;
  }
  return -1;
}

/*
 * This function will register both event providers and action managers
 * as well as rules in the event switch. The supplied handle need to be
 * initialized with the correct handle type before calling this function.
 * Example of how to register an action manager:
 *
 * static action_mgr_t  absvalmgr;
 *
 * absvalmgr.base.type = EVENT_ACTION_MANAGER;
 * absvalmgr.base.name = "A cool action manager";  // This is for the GUI
 * absvalmgr.props = ACT_PRP_ABSOLUTE_VALUE;
 * absvalmgr.vt.stop_action = absval_stop;
 * absvalmgr.vt.trigger_action = absval_trigger;
 *
 * evnt_register_handle (&absvalmgr);
 *
 */
char evnt_register_handle(void *handle) __reentrant
{
  char tmp;
  void **ptr;
  char *pctr;
  event_base_t *peb = (event_base_t*)handle;

  A_(printf (__FILE__ " Registering %s\n", peb->name);)
  switch (peb->type) {
    case EVENT_EVENT_PROVIDER:
      tmp = find_first_free_entry((void*)event_table, MAX_NR_EVENT_PROVIDERS);
      ptr = (void*)event_table;
      pctr = &nr_registered_events;
      break;
    case EVENT_ACTION_MANAGER:
      tmp = find_first_free_entry((void*)action_table, MAX_NR_ACTION_MGRS);
      ptr = (void*)action_table;
      pctr = &nr_registered_actions;
      break;
    case EVENT_RULE:
      tmp = find_first_free_entry((void*)rule_table, MAX_NR_RULES);
      ptr = (void*)rule_table;
      pctr = &nr_registered_rules;
      break;
    default:
      A_(printf(__FILE__ " Error: Incorrect event type entered !\n");)
      return -1;
  }
  if (tmp == -1)
    return -1;
  B_(printf(__FILE__ " Using pointer %p, setting element %p\n", ptr, &ptr[tmp]);)
  ptr[tmp] = (void*)handle;
  peb->enabled = 1;
  /* This gets optimized out */
  // **pctr++;
  /* This does not */
  *pctr += 1;

  return tmp;
}

/*
 * This function will unregister the specified handle from the event switch.
 * Probably most needed for rules configuration but can be used for dynamic
 * event providers and action managers as well.
 *
 * FIXME: Does not work at all yet
 */
char unregister_event_pvdr(char entry) __reentrant
{
  if (entry >= MAX_NR_EVENT_PROVIDERS)
    return -1;
  if (!nr_registered_events)
    return -2;
  if (!event_table[entry])
    return -3;

  event_table[entry] = 0;
  nr_registered_events--;

  return 0;
}

/*
 * Configure an iterator for iterating through a specified table.
 */
char evnt_iter_create (evnt_iter_t *iter) __reentrant __banked
{
  iter->cur = 0;
  iter->num = 0;
  iter->tptr = NULL;

  return 0;
}

/*
 * Get the first entry from the specified table.
 * The result must be cast to the proper type by the caller.
 * It will return a NULL result if something went wrong or
 * if there is no first entry.
 */
void *evnt_iter_get_first_entry(evnt_iter_t *iter) __reentrant __banked
{
  void *ptr;
  char *pctr;

  B_(printf (__FILE__ " Getting first entry, type=%d\n", (int)iter->type);)
  switch (iter->type) {
    case EVENT_EVENT_PROVIDER:
      ptr = (void*)event_table;
      pctr = &nr_registered_events;
      break;
    case EVENT_ACTION_MANAGER:
      ptr = (void*)action_table;
      pctr = &nr_registered_actions;
      break;
    case EVENT_RULE:
      ptr = (void*)rule_table;
      pctr = &nr_registered_rules;
      break;
    default:
      A_(printf(__FILE__ " Error: Incorrect event type entered !\n");)
      return NULL;
  }

  /* Update the iterator with information about the table */
  iter->cur = 0;
  iter->tptr = ptr;
  iter->num = *pctr;
  B_(printf (__FILE__ " %p - cur: %d, tot: %d, ptr: %p",
             iter, (int)iter->cur, (int)iter->num, iter->tptr);)
  B_(printf (" retptr %p\n", iter->tptr[iter->cur]);)

  return iter->tptr[iter->cur];
}

/*
 * Get the next active entry from the specified table
 * The result must be cast to the proper type by the caller.
 * It will return a NULL result when there are no more entries in the table.
 */
void *evnt_iter_get_next_entry(evnt_iter_t *iter) __reentrant __banked
{
  B_(printf (__FILE__ " cur: %d, tot: %d\n", (int)iter->cur, (int)iter->num);)
  while (++iter->cur < iter->num) {
    if (iter->tptr[iter->cur]) {
      B_(printf (__FILE__ " %p - Returning item %d\n", iter, (int)iter->cur);)
      return iter->tptr[iter->cur];
    } else {
      B_(printf (__FILE__ " %p - No entry at %d\n", iter, (int)iter->cur);)
    }
  }
  return NULL;
}

/* ***************************** Event machine ****************************/
/*
 * Go through the list of event signals and return as soon as the
 * first event is found. Currently this function supports up to 128 events.
 * Currently a return value of -1 indicates that no event was detected.
 *
 * As it is implemented at the moment it will always process the event
 * provider with the highest priority (= registration order). This could
 * possibly be a problem in a system with very high loads on certain
 * event providers.
 */
static event_prv_t *query_events()
{
  char i;

  for (i=0; i<MAX_NR_EVENT_PROVIDERS; i++) {
    if ((event_table[i]->base.enabled) &&   // Make sure entry is enabled
        (event_table[i]->signal)) {         // Check if signal has been triggered
      /* Reset signal */
      A_(printf (__FILE__ " Event Provider %d, Signal Content %d\n",
              i, event_table[i]->signal);)
      event_table[i]->signal = 0;
      return event_table[i];
    }
  }
  /* Indicate that no event has occured */
  return NULL;
}

/*
 * Map an action from a given event
 *
 * At the moment multiple action managers for one event provider is not
 * supported. Is this a requirement ????
 */
static action_mgr_t *get_action_from_event(void *event)
{
  char i;

  for (i=0; i<MAX_NR_RULES; i++)
    if ((rule_table[i]->base.enabled) &&     // Make sure entry is enabled
        (rule_table[i]->event == event))     // and that the event match
      return rule_table[i]->action;
  return NULL;
}

/**
 * This is the actual event_switch.
 * This thread will wait for a signal to come in from any of the registered
 * event providers. It will then look in the rules table to see if the
 * incoming event has an action manager connected to it and if so signal
 * the action manager that an event has occured.
 */
PT_THREAD(handle_event_switch(event_thread_t *et) __reentrant)
{
  PT_BEGIN(&et->pt);

  A_(printf(__FILE__ " Started the event switch !\n");)

  while (1) {
    PT_WAIT_UNTIL(&et->pt, (et->current_event = query_events()) != NULL);
    A_(printf(__FILE__ " Received an event !\n");)
    et->new_action = get_action_from_event(et->current_event);
    if (et->new_action != NULL) {
      /* Stop any ongoing action in the action manager */
      et->new_action->vt.stop_action();
      /* And execute the new trigger with data from the event provider */
      et->new_action->vt.trigger_action(et->current_event->act_data);
    } else {
      A_(printf(__FILE__ " Warning: No action mapped to event %d\n", et->current_event);)
    }
  }
  PT_END(&et->pt);
}
