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

#include "dm9000.h"
#include "pt.h"
#include "led.h"
#include "swtimers.h"
#include "sound.h"
#include "iet_debug.h"

#define INDICATOR_LED   0x02

struct led led;

static const char valid_chars[] = " 0123456789AbCdEFlnPrUY-G\0";
static const unsigned char display_pattern[] = {
  0x7f, 0x40, 0x79, 0x24, 0x30, 0x19,  /* Space, 0-4 */
  0x12, 0x02, 0x78, 0x00, 0x10,        /* 5-9 */
  0x08, 0x03, 0x46, 0x21, 0x06, 0x0e,  /* A-F */
  0x4f, 0x2b, 0x0c, 0x2f, 0x41, 0x0d, 0x3f, 0x03, /* l-G */
/*   l     n     P     r     U     Y     -     G   */
  };

extern char digit[2];

static unsigned char validate_char(char chr, unsigned char *num)
{
  char *ptr = valid_chars;
  unsigned char i = 0;

  /* Look for valid character */
  while (*ptr != '\0') {
    if (*ptr++ == chr) {
      /* Valid character found,m indicate and return */
      *num = i;
      return 1;
    }
    i++;
  }
  /* Character invalid, indicate and return */
  return 0;
}

/**
 * Initialize LED settings
 */
void init_led(void) banked
{
  /* Initialized to the same value as in the DM9000 driver (ugly... you bet) */
  led.gpr_shadow = 0x06;

  PT_INIT(&led.led_pt);
}

/**
 * This function translates the given string into something that displays
 * well on the system LED display.
 */
void led_out(char *str) banked
{
  unsigned char i;
  unsigned char chr;

  for (i=0 ; i<2 ; i++) {
    if ((validate_char(*str++, &chr)))
      digit[i] = display_pattern[chr];
    else
      digit[i] = 0xff;
  }
}

/**
 * This function will set the indicator led according to the supplied
 * status.
 */
void set_led(u8_t status, u8_t option) banked
{
  A_(printf("Setting LED status to ");)
  switch (status)
  {
    case ON:
      A_(printf("ON\r\n");)
      led.command = 0;
      led.gpr_shadow &= ~INDICATOR_LED;
      WriteNic(DM9000_GPR, led.gpr_shadow);
      break;

    case OFF:
      A_(printf("OFF\r\n");)
      led.command = 0;
      led.gpr_shadow |= INDICATOR_LED;
      WriteNic(DM9000_GPR, led.gpr_shadow);
      break;

    case BLINK:
      A_(printf("BLINK\r\n");)
      led.command = BLINK;
      led.blink_rate = option;
      break;
  }
}

/**
 * Here's the led super duper task.
 * It will take care of blinking the high quality super duper front
 * indicator LED
 */
PT_THREAD(handle_led(struct led *ld) banked)
{
  PT_BEGIN(&ld->led_pt);

  ld->timer = alloc_timer();

  while (1)
  {
    PT_WAIT_WHILE(&ld->led_pt, (ld->command == 0));
    B_(printf("LED command 0x%02x\r\n", ld->command);)

    ld->gpr_shadow &= ~INDICATOR_LED;
    WriteNic(DM9000_GPR, ld->gpr_shadow);
    set_timer(ld->timer, ld->blink_rate, NULL);
    PT_WAIT_UNTIL(&ld->led_pt, (get_timer(ld->timer) == 0x00));

    if (ld->command != 0)
    {
      ld->gpr_shadow |= INDICATOR_LED;
      WriteNic(DM9000_GPR, ld->gpr_shadow);
      set_timer(ld->timer, ld->blink_rate, NULL);
      PT_WAIT_UNTIL(&ld->led_pt, (get_timer(ld->timer) == 0x00));
    }
  }

  PT_END(&ld->led_pt);
}

/* End of file */
