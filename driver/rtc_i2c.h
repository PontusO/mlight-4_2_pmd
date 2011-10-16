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
#ifndef RTC_I2C_H_INCLUDED
#define RTC_I2C_H_INCLUDED

#include "i2c.h"

/* This is the slave address of the I2C Rtc */
#define I2C_RTC      0xA2
#define HWRTC_CENTURY_BIT   0x80
#define HWRTC_VOLTAGE_LOW   0x80
/**
 * A data tructure mapping directly to the internal register structure
 * of the PCF8563 RTC chip
 */
typedef struct {
  u8_t  ctrl_stat_1;    /* Register 0 */
  u8_t  ctrl_stat_2;    /* Register 1 */
  u8_t  vl_seconds;     /* Register 2 */
  u8_t  minutes;        /* Register 3 */
  u8_t  hours;          /* Register 4 */
  u8_t  days;           /* Register 5 */
  u8_t  weekdays;       /* Register 6 */
  u8_t  century_months; /* Register 7 */
  u8_t  years;          /* Register 8 */
  /* Alarm registers */
  u8_t  minute_alarm;   /* Register 9 */
  u8_t  hour_alarm;     /* Register A */
  u8_t  day_alarm;      /* Register B */
  u8_t  weekday_alarm;  /* Register C */
  /* Clock Control register */
  u8_t  clkout_control; /* Register D */
  /* Timer registers */
  u8_t  timer_control;  /* Register E */
  u8_t  timer;          /* Register F */
  /* This is data that is separated out from the information above */
  u8_t  century;
  u8_t  low_voltage;
} rtc_data_t;

#endif // RTC_I2C_H_INCLUDED
