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

#include <system.h>
#include <flash.h>
#include <iet_debug.h>
#include <event_switch.h>

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
 * returned. This
 */
void *rule_get_action_dptr (event_prv_t *ep)
{
  char i;

  if ((i = rule_lookup_event(ep)) != -1) {
    if (sys_cfg.rules[i].status == RULE_STATUS_ENABLED)
      return (void*)sys_cfg.rules[i].action_data;
  }
  return NULL;
}

/* This function will walk through all registered rules searching for an
 * event provider which has been triggered. If found it will return the
 * associated rule.
 */
rule_t *query_events(void)
{
  char i;

  for (i=0; i<MAX_NR_RULES; i++) {
    if ((sys_cfg.rules[i].status == RULE_STATUS_ENABLED) &&
        (sys_cfg.rules[i].event->signal)) {        // Check if signal has been triggered
      A_(printf (__FILE__ " Event Provider %d, Signal Content %d\n",
              i, sys_cfg.rules[i].event->signal);)
      /* Reset signal */
      sys_cfg.rules[i].event->signal = 0;
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
PT_THREAD(handle_event_switch(event_thread_t *et) __reentrant)
{
  PT_BEGIN(&et->pt);

  A_(printf(__FILE__ " Started the event switch !\n");)

  while (1) {
    PT_WAIT_UNTIL(&et->pt, (et->triggered_rule = query_events()) != NULL);
    A_(printf(__FILE__ " Received an event !\n");)
    et->new_action = et->triggered_rule->action;
    if (et->new_action != NULL) {
      /* Stop any ongoing action in the action manager */
      if (et->new_action->vt.stop_action)
        et->new_action->vt.stop_action();
      /* And execute the new trigger with data from the event provider */
      if (et->new_action->vt.trigger_action)
        et->new_action->vt.trigger_action((void*)et->triggered_rule->action_data);
    } else {
      A_(printf(__FILE__ " Warning: No action mapped to event %d\n", et->current_event);)
    }
  }
  PT_END(&et->pt);
}
