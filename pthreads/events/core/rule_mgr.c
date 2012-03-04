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
//#define PRINT_A

#include <string.h>

#include "system.h"
#include "flash.h"
#include "iet_debug.h"
#include "event_switch.h"

/* The volatile data table for the rule entries */
rule_data_t rule_data[MAX_NR_RULES];

/* ***************************** Event machine ****************************/
/*
 * Go through the list of event signals and return as soon as the
 * first event is found. Currently this function supports up to 128 events.
 * Currently a return value of -1 indicates that no event was detected.
 *
 * As it is implemented at the moment it will always process the event
 * provider with the highest priority (= registration order). This could
 * possibly be a problem in a system with very high loads on certain
 * event providers. Some kind of round robin should be considered.
 */

/*
 * Setup the volatile rule data table pointers in the rule table.
 */
void rule_setup_v_data_pointers (void)
{
  u8_t i;

  for (i=0; i<MAX_NR_RULES; i++) {
    sys_cfg.rules[i].r_data = &rule_data[i];
    memset (&rule_data[i], 0, sizeof rule_data[i]);
  }
}

/*
 * Look for the first free rule entry in the table and return a pointer
 * to it to the caller.
 */
rule_t *rule_find_free_entry(void) __reentrant __banked
{
  u8_t i;

  for (i=0; i<MAX_NR_RULES; i++) {
    if (sys_cfg.rules[i].status == RULE_STATUS_FREE) {
      return &sys_cfg.rules[i];
    }
  }
  return NULL;
}

/*
 * Generic function for looking up a specific event provider
 */
static char rule_lookup_event (event_prv_t *ep)
{
  char i;

  for (i=0; i<MAX_NR_RULES; i++) {
    if (sys_cfg.rules[i].event == ep) {
      return i;
    }
  }
  return -1;
}

/*
 * This function will lookup the action connected to a specific event provider
 * in the rule table. If the event provider was not found, NULL will be
 * returned.
 */
union rule_action_data *rule_get_action_dptr (event_prv_t *ep) __reentrant
{
  char i;

  if ((i = rule_lookup_event(ep)) != -1) {
    if (sys_cfg.rules[i].status != RULE_STATUS_FREE)
      return (union rule_action_data *)sys_cfg.rules[i].action_data;
  }
  return NULL;
}

/*
 * Set the trigger signal in all rules that have the specified event.
 */
void rule_send_event_signal (event_prv_t *ep)
{
  u8_t i;

  /* Go through the entire rule table, looking for the event */
  for (i=0; i<MAX_NR_RULES; i++) {
    /* Make sure the event is enabled and the the event pointer matches
     * the argument */
    if (sys_cfg.rules[i].status == RULE_STATUS_ENABLED &&
        sys_cfg.rules[i].event == ep) {
      /* Trigger the event */
      sys_cfg.rules[i].r_data->event_signal = 1;
    }
  }
}


/* This function will walk through all registered rules searching for an
 * event provider which has been triggered. If found it will return the
 * associated rule.
 */
static rule_t *query_events(void)
{
  char i;

  for (i=0; i<MAX_NR_RULES; i++) {
    if ((sys_cfg.rules[i].status == RULE_STATUS_ENABLED) &&
        (sys_cfg.rules[i].r_data->event_signal)) {        // Check if signal has been triggered
      A_(printf (__FILE__ " Event Provider %d, Signal Content %d\n",
              i, sys_cfg.rules[i].r_data->event_signal);)
      return (rule_t *)&sys_cfg.rules[i];
    }
  }
  /* Indicate that no event has occured */
  return (rule_t *)NULL;
}

/**
 * This is the actual event_switch.
 * This thread will wait for a signal to come in from any of the registered
 * event providers. It will then look in the rules table to see if the
 * incoming event has an action manager connected to it and if so signal
 * the action manager that an event has occured.
 */
#pragma save
#pragma nogcse
PT_THREAD(handle_event_switch(event_thread_t *et) __reentrant)
{
  PT_BEGIN(&et->pt);

  A_(printf(__FILE__ " Started the event switch !\n");)

  while (1) {
    PT_WAIT_UNTIL(&et->pt, query_events() != NULL);
    A_(printf(__FILE__ " Received an event !\n");)
    /* Walk through the rule table and execute all triggered action managers */
    for (et->i=0; et->i<MAX_NR_RULES; et->i++) {
      if ((sys_cfg.rules[et->i].status == RULE_STATUS_ENABLED) &&
          (sys_cfg.rules[et->i].r_data->event_signal)) {        // Check if signal has been triggered
        /* Found an active, triggered rule so first clear the signal again. */
        sys_cfg.rules[et->i].r_data->event_signal = 0;
        et->new_action = sys_cfg.rules[et->i].action;
        if (et->new_action != NULL) {
          /* Stop any ongoing action in the action manager */
          if (et->new_action->vt.stop_action)
            et->new_action->vt.stop_action();
          /* And execute the new trigger with data from the rule action data */
          if (et->new_action->vt.trigger_action) {
            A_(printf (__FILE__ " Executing action trigger function.\n");)
            et->new_action->vt.trigger_action((void*)sys_cfg.rules[et->i].action_data);
          } else {
            A_(printf(__FILE__ " Error: No action trigger defined !\n");)
          }
        } else {
          A_(printf(__FILE__ " Error: No action mapped to event %d\n", et->current_event);)
        }
      }
    }
  }
  PT_END(&et->pt);
}
#pragma restore

/*
 * Configure an iterator for iterating through the rule table.
 */
char rule_iter_create (evnt_iter_t *iter) __reentrant __banked
{
  iter->cur = 0;
  iter->num = 0;
  iter->tptr = NULL;

  return 0;
}

/*
 * Get the first entry from the specified table.
 * It will return a NULL result if something went wrong or
 * if there is no first entry = empty.
 */
rule_t *rule_iter_get_first_entry(evnt_iter_t *iter) __reentrant __banked
{
  u8_t i;

  /* Look for the first non empty entry */
  for (i=0; i<MAX_NR_RULES; i++) {
    if (sys_cfg.rules[i].status != RULE_STATUS_FREE) {
      iter->cur = i;
      return &sys_cfg.rules[i];
    }
  }
  return NULL;
}

/*
 * Get the next active entry from the specified table
 * The result must be cast to the proper type by the caller.
 * It will return a NULL result when there are no more entries in the table.
 */
rule_t *rule_iter_get_next_entry(evnt_iter_t *iter) __reentrant __banked
{
  while (++iter->cur < MAX_NR_RULES) {
    if (sys_cfg.rules[iter->cur].status != RULE_STATUS_FREE) {
      A_(printf (__FILE__ " %p - Returning item %d\n", iter, (int)iter->cur);)
      return &sys_cfg.rules[iter->cur];
    }
  }
  return NULL;
}

/*
 * Clear all rule entries in the sys_cfg area.
 */
void clear_all_rules(void) __reentrant __banked
{
  u8_t i;

  for (i=0; i<MAX_NR_RULES; i++) {
    memset (&sys_cfg.rules[i], 0, sizeof (rule_t));
  }
}
