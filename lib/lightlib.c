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

//#define PRINT_A     // Enable A prints
#include <system.h>
#include <lightlib.h>
#include "iet_debug.h"
#include <string.h>
#include <stdlib.h>
#include <pca.h>

/*
 * This table holds the io pins used for on off switches.
 */
static char io_pin_tab[] = {3,4};

/*
 * A pointer to the current led_lights structure
 */
static struct led_lights *current_led_lights;

/*
 * Initialize the light driver
 */
void ledlib_create(struct led_lights *led_lights) __reentrant banked
{
  u8_t i, j=0;

  memset (led_lights, 0, sizeof *led_lights);
  for (i=0;i<CFG_NUM_LIGHT_DRIVERS;i++) {
    if (i<CFG_NUM_PWM_DRIVERS) {
      /* A pwm source, so setup for pwm */
      led_lights->light_drivers[i].driver_type = LIGHT_PWM;
    } else {
      /* An on off source, so setup for that */
      led_lights->light_drivers[i].driver_type = LIGH_ON_OFF;
      led_lights->light_drivers[i].io_pin = io_pin_tab[j++];
    }
  }
  current_led_lights = led_lights;
}

/*
 * Return the current set led_lights instance
 */
struct led_lights *ledlib_get_current(void)
{
  return current_led_lights;
}
/*
 * assign a value to he appropriate gpio bit on port 1
 */
static void set_ledbit (u8_t ledbit, u8_t value) __reentrant
{
  switch (ledbit)
  {
    case 0:
      P1_0 = value;
      break;
    case 1:
      P1_1 = value;
      break;
    case 2:
      P1_2 = value;
      break;
    case 3:
      P1_3 = value;
      break;
    case 4:
      P1_4 = value;
      break;
    case 5:
      P1_5 = value;
      break;
    case 6:
      P1_6 = value;
      break;
    case 7:
      P1_7 = value;
      break;
  }
}
/*
 * Request a light driver update with absolute value
 */
char ledlib_set_light_abs (struct led_lights *led_lights,
                               u8_t channel, u16_t value) __reentrant banked
{
  long int tmp;

  /* Make sure parameters have sensible values */
  if (channel >= CFG_NUM_LIGHT_DRIVERS)
    return -1;

  /* the percentage value is 1/655 of max_int. */
  tmp = value * 100 / 65535;
  led_lights->light_drivers[channel].pwm_percent = value * 100 / 65535;
  led_lights->light_drivers[channel].pwm_ratio = value;

  if (led_lights->light_drivers[channel].driver_type == LIGHT_PWM)
    set_pca_duty (channel, led_lights->light_drivers[channel].pwm_ratio);
  else
    set_ledbit (led_lights->light_drivers[channel].io_pin,
                led_lights->light_drivers[channel].pwm_percent ? 1 : 0);
  return 0;
}

/*
 * Request a light driver update with a percentage value
 */
char ledlib_set_light_percentage (struct led_lights *led_lights,
                                      u8_t channel, u8_t value) __reentrant banked
{
  /* Make sure parameters have sensible values */
  if (channel >= CFG_NUM_LIGHT_DRIVERS || value > 100)
    return -1;

  led_lights->light_drivers[channel].pwm_percent = value;
  if (value == 100) {
    led_lights->light_drivers[channel].pwm_ratio = MAX_INTENSITY;
  } else {
    led_lights->light_drivers[channel].pwm_ratio = value / 655;
  }
  if (led_lights->light_drivers[channel].driver_type == LIGHT_PWM)
    set_pca_duty (channel, led_lights->light_drivers[channel].pwm_ratio);
  else
    set_ledbit (led_lights->light_drivers[channel].io_pin,
                led_lights->light_drivers[channel].pwm_percent ? 1 : 0);
  return 0;
}

u16_t ledlib_get_light_abs (struct led_lights *led_lights, u8_t channel) __reentrant banked
{
  if (led_lights->light_drivers[channel].driver_type == LIGHT_PWM)
    return led_lights->light_drivers[channel].pwm_ratio;
  else
    return led_lights->light_drivers[channel].pwm_ratio ? 1 : 0;
}

u8_t ledlib_get_light_percentage (struct led_lights *led_lights, u8_t channel) __reentrant banked
{
  if (led_lights->light_drivers[channel].driver_type == LIGHT_PWM)
    return led_lights->light_drivers[channel].pwm_percent;
  else
    return led_lights->light_drivers[channel].pwm_percent ? 100 : 0;
}

u8_t ledlib_get_type (struct led_lights *led_lights, u8_t channel) __reentrant banked
{
  return led_lights->light_drivers[channel].driver_type;
}
/* EOF */
