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

//#include "system.h"
#include "pt.h"
#include "iet_debug.h"
#include "event_switch.h"
#include "string.h"

static  char nr_registered_actions;
static  action_mgr_t *action_table[MAX_NR_ACTION_MGRS];
static  char nr_registered_events;
static  event_prv_t *event_table[MAX_NR_EVENT_PROVDERS];
static  char nr_rules;
static  rule_t rule_table[MAX_NR_RULES];

void init_event_switch(void)
{
  nr_registered_actions = 0;
  memset (action_table, 0, sizeof *action_table);
  memset (event_table, 0, sizeof *event_table);
  memset (&rule_table, 0, sizeof rule_table);
}

static char find_first_free_entry(void)
{
  int i;

  for (i=0; i<MAX_NR_ACTION_MGRS; i++)
    if (!action_table[i])
      return i;
  return -1;
}

char register_action_mgr(struct action_mgr_s *mgr) __reentrant
{
  char tmp;
  /* Check that the manager table isn't full */
  if (nr_registered_actions == MAX_NR_ACTION_MGRS)
    return -1;

  tmp = find_first_free_entry();
  if (tmp == -1)
    return -1;

  action_table[tmp] = mgr;
  nr_registered_actions++;

  /* Return the associated entry */
  return tmp;
}

char unregister_action_mgr(char entry) __reentrant
{
  /* Some sanity checking at first */
  if (!nr_registered_actions)
    return -1;
  if (!action_table[entry])
    return -2;

  action_table[entry] = 0;
  nr_registered_actions--;

  return 0;
}

/**
 * Go through the list of event signals and return as soon as the
 * first event is found
 */
static char query_events()
{
  char i;

  for (i=0; i<MAX_NR_EVENT_PROVDERS; i++)
    if (event_table[i]->signal)
      return i;
  return 0;
}

/**
 * Map an action from a given event
 */
static char get_action_from_event(char event)
{
  char i;

  for (i=0; i<MAX_NR_RULES; i++)
    if (rule_table[i].event == event)
      return i;
  return -1;
}

/* Here's the event thread */
PT_THREAD(handle_event_thread(event_thread_t *et) __reentrant)
{
  PT_BEGIN(&et->pt);

  while (1) {
    PT_WAIT_UNTIL(&et->pt, (et->current_event = query_events()));
    et->new_action = get_action_from_event(et->current_event);
    if (et->new_action != -1) {
      /* Stop any ongoing action in the action manager */
      action_table[et->new_action]->vt.stop_action();
      /* And execute the new trigger */
      action_table[et->new_action]->vt.trigger_action(rule_table[et->new_action].action_data);
    } else {
      A_(printf("Warning: No action mapped to event %d\n", et->current_event);)
    }
  }

  PT_END(&et->pt);
}
