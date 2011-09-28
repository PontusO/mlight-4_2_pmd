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

#ifndef LED_MANAGER_H_INCLUDED
#define LED_MANAGER_H_INCLUDED

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
} light_driver_type;

/* Declaration of the light driver object */
struct light_driver {
  enum light_driver_type driver_type;
  u8_t pwm_percent;
  u16_t pwm_ratio;
  u8_t io_pin;
};

struct led_lights {
  struct light_driver light_drivers[CFG_NUM_LIGHT_DRIVERS];
};

void ledlib_create(struct led_lights *led_lights) __reentrant banked;
struct led_lights *ledlib_get_current(void);
char ledlib_set_light_abs (struct led_lights *led_lights, u8_t channel,
    u16_t value) __reentrant banked;
char ledlib_set_light_percentage (struct led_lights *led_lights,
    u8_t channel, u8_t value) __reentrant banked;
u16_t ledlib_get_light_abs (struct led_lights *led_lights, u8_t channel)
    __reentrant banked;
u8_t ledlib_get_light_percentage (struct led_lights *led_lights, u8_t channel)
    __reentrant banked;
u8_t ledlib_get_type (struct led_lights *led_lights, u8_t channel)
    __reentrant banked;

#endif // LED_MANAGER_H_INCLUDED

#endif // LIGHTLIB_H_INCLUDED
