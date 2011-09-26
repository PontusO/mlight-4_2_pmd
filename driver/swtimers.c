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
#pragma codeseg APP_BANK

#include "system.h"
#include "swtimers.h"
#include "led.h"
#include "iet_debug.h"

#include <stdio.h>

u16_t swtimer[NUMBER_OF_SWTIMERS];
u8_t  timer_table[NUMBER_OF_SWTIMERS];
timer_cb timer_cbs[NUMBER_OF_SWTIMERS];

struct kicker kicker;

extern bit callback_kicker;
static u8_t count = 0;


/*
 * Initialize software timers
 * This function must be executed before timer 0 interrupts
 * are enabled.
 */
void init_swtimers(void) banked
{
  u8_t i;

  for (i=0 ; i<NUMBER_OF_SWTIMERS ; i++)
  {
    /* Setting the timer value to 0 indicates that it is inactive and
     * not counting */
    timer_table[i] = TMR_FREE;
    swtimer[i]   = 0x0000;
    timer_cbs[i] = 0x0000;
  }

  PT_INIT(&kicker.pt);
}

/*
 * Allocate a timer, can range from 0-127.
 * Negative number are error codes.
 */
char alloc_timer(void) banked
{
  u8_t i;
#ifdef DEBUG_SWTIMERS
  u8_t j;
#endif

  ET0 = INTERRUPT_OFF;
  for (i=0 ; i<NUMBER_OF_SWTIMERS ; i++)
  {
    if (timer_table[i] == TMR_FREE) {
      /* Timer is now taken */
      timer_table[i] = TMR_ALLOCATED;
      swtimer[i]   = 0x0000;
      timer_cbs[i] = 0x0000;
      ET0 = INTERRUPT_ON;
      count++;
      A_(printf("Allocating Timer %d, Timers used = %d '", i, count);)
#ifdef DEBUG_SWTIMERS
      for (j=0 ; j<NUMBER_OF_SWTIMERS ; j++) {
        switch (timer_table[j])
        {
          case TMR_RUNNING:
            putchar('R');
            break;
          case TMR_FREE:
            putchar('F');
            break;
          case TMR_ALLOCATED:
            putchar('A');
            break;
          case TMR_STOPPED:
            putchar('S');
            break;
          case TMR_KICK:
            putchar('K');
            break;
          case TMR_ENDED:
            putchar('E');
            break;
          default:
            putchar('U');
            break;
        }
      }
#endif
      A_(printf("\r\n");)
      return i;
    }
  }
  ET0 = INTERRUPT_ON;
  return -1;
}

/*
 * Free a timer
 */
u8_t free_timer(u8_t timer) banked
{
#ifdef DEBUG_SWTIMERS
  u8_t j;
#endif
  ET0 = INTERRUPT_OFF;
  if (timer_table[timer] != TMR_FREE) {
    timer_table[timer] = TMR_FREE;
    count--;
    A_(printf("Freeing Timer %d, Timers used = %d ", timer, count);)
#ifdef DEBUG_SWTIMERS
    for (j=0 ; j<NUMBER_OF_SWTIMERS ; j++) {
      switch (timer_table[j])
      {
        case TMR_RUNNING:
          putchar('R');
          break;
        case TMR_FREE:
          putchar('F');
          break;
        case TMR_ALLOCATED:
          putchar('A');
          break;
        case TMR_STOPPED:
          putchar('S');
          break;
        case TMR_KICK:
          putchar('K');
          break;
        case TMR_ENDED:
          putchar('E');
          break;
        default:
          putchar('U');
          break;
      }
    }
#endif
    A_(printf("\r\n");)
  }
  ET0 = INTERRUPT_ON;
  return 0;
}

/*
 * Set a timer to a defined value with callback.
 * If callbacks are not to be used, this must be set to NULL
 * The timer is started immediatly before returning
 */
void set_timer(u8_t timer, u16_t time, timer_cb cb) banked
{
  ET0 = INTERRUPT_OFF;
  swtimer[timer] = time;
  timer_cbs[timer] = cb;
  timer_table[timer] = TMR_RUNNING;
  ET0 = INTERRUPT_ON;
}

/*
 * Renews the timer counter
 */
void set_timer_cnt(u8_t timer, u16_t time) banked
{
  ET0 = INTERRUPT_OFF;
  swtimer[timer] = time;
  ET0 = INTERRUPT_ON;
}

/*
 * Get the value of a specific timer
 */
u16_t get_timer(u8_t timer) banked
{
  u16_t value;

  ET0 = INTERRUPT_OFF;
  value = swtimer[timer];
  ET0 = INTERRUPT_ON;

  return value;
}

/*
 * Get the status of a specified timer
 */
u8_t get_timer_status(u8_t timer) banked
{
  return timer_table[timer];
}

/*
 * Stop the timer from counting down
 */
void stop_timer(u8_t timer) banked
{
  ET0 = INTERRUPT_OFF;
  timer_table[timer] = TMR_STOPPED;
  ET0 = INTERRUPT_ON;
}

/*
 * Start the timer counting
 */
void start_timer(u8_t timer) banked
{
  ET0 = INTERRUPT_OFF;
  timer_table[timer] = TMR_RUNNING;
  ET0 = INTERRUPT_ON;
}

/**
 * This thread listens to the <callback_kicker> signal and when it is
 * triggered it goes through all software timers and executes the
 * indicated call back methods.
 * This functionality was broken out of the timer 0 interrupt routine
 * as it was severely instable.
 */
PT_THREAD(handle_kicker(struct kicker *Kicker) banked)
{
  PT_BEGIN(&Kicker->pt);

  while (1)
  {
    /* Wait to be kicked by the timer system */
    PT_WAIT_UNTIL(&Kicker->pt, (callback_kicker));

    ET0 = INTERRUPT_OFF;
    callback_kicker = 0;
    {
      near u8_t i;

      for (i=0 ; i<NUMBER_OF_SWTIMERS ; i++)
      {
        if (timer_table[i] == TMR_KICK)
        {
          /* Allow timer 0 interrupts while executing callback */
          // ET0 = INTERRUPT_ON;
          timer_cbs[i](i);
          // ET0 = INTERRUPT_OFF;
        }
      }
    }
    ET0 = INTERRUPT_ON;
  }
  PT_END(&Kicker->pt);
}

/* End of file */
