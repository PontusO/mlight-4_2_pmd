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
#include "button.h"
#include "sound.h"
#include "swtimers.h"
#include "remote.h"
#include "flash.h"

#define RESET_TIMEOUT   5
#define SHORT_PUSH      2
/*
 * Local data
 */
struct button button;

/*
 * Initialize the button handler and any hardware not initialized
 * previously. The button is connected to comparator 1 on the positive
 * input terminal. The negative terminal is connected to the VREF output
 * pin from the module.
 */
void init_button(void) banked
{
  u8_t i;

  /* First we need to set up the voltage reference to do 2.4 Volt */
  SFRPAGE = LEGACY_PAGE;
  REF0CN = 0x07;

  /* And we need to initialize all comparator registers */
  SFRPAGE = CPT1_PAGE;
  /* Comp1 enabled */
  CPT1CN = 0x80;
  for (i = 0; i < 100; i++);  // Wait 20us for initialization
  CPT1CN    &= ~0x30;
  /* No interrupts, lowest power consumption */
  CPT1MD = 0x03;

  /* And finally initialize the button thread. */
  PT_INIT(&button.button_pt);
}

/*
 * This method is used to create an atomic getter for my protothread
 */
static u8_t get_button(void)
{
  u8_t temp;

  /* Clear any indicator bits */
  SFRPAGE = CPT1_PAGE;
  CPT1CN    &= ~0x30;

  /* Get value from comparator */
  temp = CPT1CN & 0x40;

  return temp;
}

/*
 * This is our protothread for the button handler
 * perhaps this is overkill but it really is simple to do it like this.
 */
PT_THREAD(handle_button(struct button *btn) banked)
{
  PT_BEGIN(&btn->button_pt);

  /* First thing to do is to check if the button is already pressed
   * This is for the power on reset sequence
   */
  if (!get_button())
  {
    btn->timer = alloc_timer();
    set_timer(btn->timer, RESET_TIMEOUT * 100, NULL);
    PT_WAIT_UNTIL(&btn->button_pt, (get_timer(btn->timer) == 0x0000));
    free_timer(btn->timer);
    /* Check if button is still pressed */
    if (!get_button())
    {
      /* If we end up here, the user pressed the button for 2 seconds
       * at reset.
       */
      /* First do general settings */
      load_default_config();
      write_config_to_flash();

      /* Now flash a new remote control table */
      clear_rc_flash();
      copy_rc_to_flash();
      load_network_params();

      beep(500, 10);
      /* Wait for the button to be released again */
      PT_WAIT_UNTIL(&btn->button_pt, get_button());
    }
  }

  while (1)
  {
    /* Wait for the button to be pressed again */
    PT_WAIT_UNTIL(&btn->button_pt, !get_button());

    /* Do another short timeout to validate the press */
    btn->timer = alloc_timer();
    set_timer(btn->timer, RESET_TIMEOUT * 100, NULL);
    PT_WAIT_UNTIL(&btn->button_pt, (get_timer(btn->timer) == 0x0000));
    free_timer(btn->timer);

    if (!get_button())
    {
      beep(3000, 15);
      /* Wait for the button to be released again */
      PT_WAIT_UNTIL(&btn->button_pt, get_button());
      /* Time to set the unit in remote learnging mode */
      ENABLE_REMOTE_TRAINER_MODE = 1;
    }
  }

  PT_END(&btn->button_pt);
}
