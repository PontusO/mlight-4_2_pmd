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

#if 0
long ilog(long x)
{
  long t,y;

  y=0xa65af;
  if(x<0x00008000) x<<=16,              y-=0xb1721;
  if(x<0x00800000) x<<= 8,              y-=0x58b91;
  if(x<0x08000000) x<<= 4,              y-=0x2c5c8;
  if(x<0x20000000) x<<= 2,              y-=0x162e4;
  if(x<0x40000000) x<<= 1,              y-=0x0b172;
  t=x+(x>>1); if((t&0x80000000)==0) x=t,y-=0x067cd;
  t=x+(x>>2); if((t&0x80000000)==0) x=t,y-=0x03920;
  t=x+(x>>3); if((t&0x80000000)==0) x=t,y-=0x01e27;
  t=x+(x>>4); if((t&0x80000000)==0) x=t,y-=0x00f85;
  t=x+(x>>5); if((t&0x80000000)==0) x=t,y-=0x007e1;
  t=x+(x>>6); if((t&0x80000000)==0) x=t,y-=0x003f8;
  t=x+(x>>7); if((t&0x80000000)==0) x=t,y-=0x001fe;
  x=0x80000000-x;
  y-=x>>15;
  return y;
}
#endif

static long iexp(long x)
{
  __xdata static long t, y;

  y=0x00010000;
  t=x-0x58b91; if(t>=0) x=t,y<<=8;
  t=x-0x2c5c8; if(t>=0) x=t,y<<=4;
  t=x-0x162e4; if(t>=0) x=t,y<<=2;
  t=x-0x0b172; if(t>=0) x=t,y<<=1;
  t=x-0x067cd; if(t>=0) x=t,y+=y>>1;
  t=x-0x03920; if(t>=0) x=t,y+=y>>2;
  t=x-0x01e27; if(t>=0) x=t,y+=y>>3;
  t=x-0x00f85; if(t>=0) x=t,y+=y>>4;
  t=x-0x007e1; if(t>=0) x=t,y+=y>>5;
  t=x-0x003f8; if(t>=0) x=t,y+=y>>6;
  t=x-0x001fe; if(t>=0) x=t,y+=y>>7;
  if(x&0x100)               y+=y>>8;
  if(x&0x080)               y+=y>>9;
  if(x&0x040)               y+=y>>10;
  if(x&0x020)               y+=y>>11;
  if(x&0x010)               y+=y>>12;
  if(x&0x008)               y+=y>>13;
  if(x&0x004)               y+=y>>14;
  if(x&0x002)               y+=y>>15;
  if(x&0x001)               y+=y>>16;

  return y;
}

#define int_to_fp(x)  (long)((long)x << 16)
#define fp_to_int(x)  (int)((long)x >> 16)

struct s64_2x32 {
  unsigned long lo;
  unsigned long hi;
};

struct s64_4x16 {
  unsigned int w0;
  unsigned int w1;
  unsigned int w2;
  unsigned int w3;
};

union s64_t {
  struct s64_2x32 x32;
  struct s64_4x16 x16;
};

typedef union s64_t s64;

/* Local math variables */
__xdata static long a, b, c, d;
__xdata static long x, y;
__xdata static s64 r;
__xdata static long acca;
__xdata static long accb;

static void mmul32 (void) __reentrant
{
  a = (acca >> 16) & 0xffff;
  b = acca & 0xffff;
  c = (accb >> 16) & 0xffff;
  d = accb & 0xffff;

  r.x32.lo = b * d;           /* BD */
  x = a * d + c * b;    /* AD + CB */
  y = ((r.x32.lo >> 16) & 0xffff) + x;
  r.x32.lo = (r.x32.lo & 0xffff) | ((y & 0xffff) << 16);
  r.x32.hi = (y >> 16) & 0xffff;

  r.x32.hi += a * c;          /* AC */
}

static u16_t calc_pwm (u8_t value) __reentrant
{
  /* Convert value to fp format and scale to our range */
  acca = int_to_fp(value) / 25;
  /* Take e^test and subtract 1.0000 to remove offset */
  acca = iexp (acca) - 0x10000;
  accb = 0x4C6B5CAL;
  mmul32 ();
  /* Return the normalized 16-__bit pwm value */
  return (u16_t)r.x16.w2;
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
  light_drivers[channel].pwm_percent = value / 655;
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
  u8_t value = params->level_percent;
  /* Make sure parameters have sensible values */
  if (channel >= CFG_NUM_LIGHT_DRIVERS || value > 100)
    return -1;
  else
    B_(printf (__FILE__ " Channel: %d, value %d\n", channel, value);)

  light_drivers[channel].pwm_percent = value;
  if (light_drivers[channel].driver_type == LIGHT_PWM) {
//    light_drivers[channel].pwm_ratio = (u16_t)value * 655;
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

u8_t ledlib_get_light_percentage (u8_t channel) __reentrant __banked
{
  if (light_drivers[channel].driver_type == LIGHT_PWM)
    return light_drivers[channel].pwm_percent;
  else
    return light_drivers[channel].pwm_percent ? 100 : 0;
}

u8_t ledlib_get_type (u8_t channel) __reentrant __banked
{
  return light_drivers[channel].driver_type;
}
/* EOF */
