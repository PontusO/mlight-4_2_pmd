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
//#define PRINT_B     // Spam prints

#include <system.h>
#include "iet_debug.h"
#include <string.h>
#include <stdlib.h>
#include <pca.h>
#include <lightlib.h>

/*
 * This table holds the io pins used for on off switches.
 */
static char io_pin_tab[] = {3,4};
struct light_driver light_drivers[CFG_NUM_LIGHT_DRIVERS];

static const int logvec[] = { 0x0000,
0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x0010, 0x0011, 0x0013,
0x0014, 0x0016, 0x0019, 0x001B, 0x001E, 0x0020, 0x0024, 0x0027, 0x002B, 0x002F,
0x0033, 0x0038, 0x003E, 0x0044, 0x004A, 0x0051, 0x0059, 0x0061, 0x006A, 0x0074,
0x0080, 0x008C, 0x0099, 0x00A7, 0x00B7, 0x00C9, 0x00DC, 0x00F1, 0x0107, 0x0120,
0x013C, 0x015A, 0x017A, 0x019E, 0x01C6, 0x01F0, 0x021F, 0x0253, 0x028B, 0x02C9,
0x030C, 0x0356, 0x03A7, 0x0400, 0x0461, 0x04CB, 0x053F, 0x05BE, 0x0649, 0x06E1,
0x0788, 0x083F, 0x0906, 0x09E1, 0x0AD0, 0x0BD6, 0x0CF5, 0x0E2F, 0x0F87, 0x10FF,
0x129A, 0x145D, 0x164A, 0x1866, 0x1AB5, 0x1D3C, 0x2001, 0x2308, 0x2658, 0x29F9,
0x2DF2, 0x324B, 0x370D, 0x3C43, 0x41F6, 0x4834, 0x4F09, 0x5683, 0x5EB3, 0x67A9,
0x7177, 0x7C34, 0x87F4, 0x94D1, 0xA2E6, 0xB250, 0xC32F, 0xD5A7, 0xE9DE, 0xFFFF,
};
/*
 * Returns the absolute pwm value from the supplied natural input value.
 */
static u16_t calc_pwm (pwm_perc_t value) __reentrant
{
  pwm_perc_t step, ret, index;

  if (value > MAX_PERCENTAGE_VALUE) value = 1000;

  /* Calculate an index in the logvec table */
  index = value / 10;
  /* Check if we are exactly on one point in the table */
  if (value % (MAX_PERCENTAGE_VALUE / 100) == 0) {
    ret = logvec[index];
  } else {
    /* Calculate the step per integrated step */
    step = (logvec[index+1] - logvec[index]) / 10;
    /* Interpolate the correct value */
    ret = logvec[index] + step * (value % (MAX_PERCENTAGE_VALUE / 100));
  }
  return ret;
}

/*
 * Poor mans version of invers exp(x) i.e. ln(x), takes time but saves space.
 */
static pwm_perc_t get_percent (u16_t value) __reentrant
{
  pwm_perc_t i;
  for (i=0;i<LOGVEC_LENGTH-1;i++) {
    if (value > logvec[i] && value < logvec[i+1])
      return i*10;
  }
  /* Just to keep the compiler happy */
  return 0;
}

/*
 * Initialize the light driver
 */
void init_ledlib(void) __reentrant __banked
{
  u8_t i, j=0;

  memset (light_drivers, 0, sizeof light_drivers);
  for (i=0;i<CFG_NUM_LIGHT_DRIVERS;i++) {
    if (i<CFG_NUM_PWM_DRIVERS) {
      /* A pwm source, so setup for pwm */
      light_drivers[i].driver_type = LIGHT_PWM;
    } else {
      /* An on off source, so setup for that */
      light_drivers[i].driver_type = LIGH_ON_OFF;
      light_drivers[i].io_pin = io_pin_tab[j++];
    }
  }
}

/*
 * assign a value to he appropriate gpio __bit on port 1
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
char ledlib_set_light_abs (ld_param_t *params) __reentrant __banked
{
  u8_t channel = params->channel;
  u16_t value = params->level_absolute;

  /* Make sure parameters have sensible values */
  if (channel >= CFG_NUM_LIGHT_DRIVERS)
    return -1;

  /* the percentage value is 1/655 of max_int. */
  light_drivers[channel].pwm_percent = get_percent (value);
  light_drivers[channel].pwm_ratio = value;

  if (light_drivers[channel].driver_type == LIGHT_PWM) {
    set_pca_duty (channel, value);
  } else
    set_ledbit (light_drivers[channel].io_pin,
                light_drivers[channel].pwm_percent ? 1 : 0);
  return 0;
}

/*
 * Request a light driver update with a percentage value
 */
char ledlib_set_light_percentage_log (ld_param_t *params) __reentrant __banked
{
  u8_t channel = params->channel;
  pwm_perc_t value = params->level_percent;

  /* Make sure parameters have sensible values */
  if (channel >= CFG_NUM_LIGHT_DRIVERS || value > MAX_PERCENTAGE_VALUE)
    return -1;

  light_drivers[channel].pwm_percent = value;
  if (light_drivers[channel].driver_type == LIGHT_PWM) {
    light_drivers[channel].pwm_ratio = calc_pwm (value);
    set_pca_duty (channel, light_drivers[channel].pwm_ratio);
  }  else
    set_ledbit (light_drivers[channel].io_pin,
                light_drivers[channel].pwm_percent ? 1 : 0);
  return 0;
}

u16_t ledlib_get_light_abs (u8_t channel) __reentrant __banked
{
  if (light_drivers[channel].driver_type == LIGHT_PWM)
    return light_drivers[channel].pwm_ratio;
  else
    return light_drivers[channel].pwm_ratio ? 1 : 0;
}

pwm_perc_t ledlib_get_light_percentage (u8_t channel) __reentrant __banked
{
  if (light_drivers[channel].driver_type == LIGHT_PWM)
    return light_drivers[channel].pwm_percent;
  else
    return light_drivers[channel].pwm_percent ? MAX_PERCENTAGE_VALUE : 0;
}

u8_t ledlib_get_type (u8_t channel) __reentrant __banked
{
  return light_drivers[channel].driver_type;
}
/* EOF */
