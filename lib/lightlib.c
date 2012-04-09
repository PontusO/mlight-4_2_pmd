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
	0x0002, 0x0003, 0x0003, 0x0004, 0x0004, 0x0004, 0x0005, 0x0006, 0x0006, 0x0007,
	0x0008, 0x0009, 0x0009, 0x000B, 0x000C, 0x000D, 0x000E, 0x0010, 0x0012, 0x0014,
	0x0016, 0x0018, 0x001B, 0x001E, 0x0021, 0x0025, 0x0029, 0x002D, 0x0032, 0x0037,
	0x003D, 0x0044, 0x004B, 0x0053, 0x005C, 0x0066, 0x0070, 0x007C, 0x008A, 0x0098,
	0x00A9, 0x00BB, 0x00CF, 0x00E5, 0x00FD, 0x0118, 0x0136, 0x0157, 0x017B, 0x01A3,
	0x01D0, 0x0201, 0x0238, 0x0274, 0x02B7, 0x0301, 0x0353, 0x03AD, 0x0412, 0x0480,
	0x04FB, 0x0582, 0x0618, 0x06BE, 0x0776, 0x0841, 0x0921, 0x0A1A, 0x0B2D, 0x0C5D,
	0x0DAD, 0x0F22, 0x10BD, 0x1285, 0x147D, 0x16AA, 0x1913, 0x1BBD, 0x1EB0, 0x21F3,
	0x258F, 0x298D, 0x2DF8, 0x32DB, 0x3842, 0x3E3D, 0x44DB, 0x4C2C, 0x5445, 0x5D3A,
	0x6723, 0x7219, 0x7E3A, 0x8BA5, 0x9A7C, 0xAAE8, 0xBD12, 0xD12B, 0xE766, 0xFFFF,
};
/*
 * Returns the absolute pwm value from the supplied natural input value.
 * Valid values ranges from 0 - (currently) 1000. 0 Is treated specially and
 * just returns a 0.
 */
static u16_t calc_pwm (pwm_perc_t value) __reentrant
{
  pwm_perc_t ret, index;

  /* Handle special case */
  if (value > MAX_PERCENTAGE_VALUE) value = MAX_PERCENTAGE_VALUE;

  /* Calculate an index in the logvec table */
  index = value / 10;
  /* Check if we are exactly on one point in the table */
  if (value % (MAX_PERCENTAGE_VALUE / 100) == 0) {
    ret = logvec[index];
  } else {
    /* Calculate next value in the interpolated series */
    ret = logvec[index] + ((logvec[index+1] - logvec[index]) *
                           (value % (MAX_PERCENTAGE_VALUE / 100)) / 10);
  }
  /* Failsafe mechanism, since we only want one zero point */
  if (!ret && value != 0)
    ret = 1;

  A_(printf (__AT__ "Value=%d, Ret=%04x\n", value, ret);)
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
  /* Invert the value since we have an inverter as a mosfet driver */
  value = value ? 0 : 1;

  switch (ledbit)
  {
#if 0
    case 0:
      P1_0 = value;
      break;
    case 1:
      P1_1 = value;
      break;
    case 2:
      P1_2 = value;
      break;
#endif
    case 3:
      P1_3 = value;
      break;
    case 4:
      P1_4 = value;
      break;
#if 0
    case 5:
      P1_5 = value;
      break;
    case 6:
      P1_6 = value;
      break;
    case 7:
      P1_7 = value;
      break;
#endif
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

  if (light_drivers[channel].driver_type == LIGHT_PWM) {
    light_drivers[channel].pwm_ratio = value;
    set_pca_duty (channel, value);
  } else {
    light_drivers[channel].pwm_ratio = value ? 1 : 0;
    set_ledbit (light_drivers[channel].io_pin, value ? 1 : 0);
  }
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

  if (light_drivers[channel].driver_type == LIGHT_PWM) {
    light_drivers[channel].pwm_percent = value;
    light_drivers[channel].pwm_ratio = calc_pwm (value);
    set_pca_duty (channel, light_drivers[channel].pwm_ratio);
  }  else {
    light_drivers[channel].pwm_percent = value ? 1000 : 0;
    light_drivers[channel].pwm_ratio = value ? 1 : 0;
    set_ledbit (light_drivers[channel].io_pin, value ? 1 : 0);
  }
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
