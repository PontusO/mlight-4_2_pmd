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
#ifndef LIGHTLIB_H_INCLUDED
#define LIGHTLIB_H_INCLUDED

/* Configuration constants for the driver block */
#define CFG_NUM_LIGHT_DRIVERS   6
#define CFG_NUM_PWM_DRIVERS     4

#define MAX_INTENSITY           0xffff
/*
 * Data types used by the button monitor
 */
/* Light driver types */
enum light_driver_type {
  LIGHT_NONE,
  LIGHT_PWM,
  LIGH_ON_OFF
};

/* Declaration of the light driver object */
struct light_driver {
  u8_t driver_type;
  u8_t pwm_percent;
  u16_t pwm_ratio;
  u8_t io_pin;
};

/* Prototypes for the lib */
void init_ledlib(void) __reentrant;
char ledlib_set_light_percentage_log (u8_t channel, u8_t value) __reentrant;
u8_t ledlib_get_light_percentage (u8_t channel) __reentrant;
char ledlib_set_light_abs (u8_t channel, u16_t value) __reentrant;
u16_t ledlib_get_light_abs (u8_t channel)  __reentrant;
u8_t ledlib_get_type (u8_t channel) __reentrant;
char ledlib_decrement_light_abs (u8_t channel, u16_t value) __reentrant;
char ledlib_increment_light_abs (u8_t channel, u16_t value) __reentrant;

#endif // LIGHTLIB_H_INCLUDED
