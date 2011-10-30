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
#ifndef RTC_H_INCLUDED
#define RTC_H_INCLUDED

#include "rtc_i2c.h"
/**
 * PMD time structure.
 * This structure represents the time internally in the PMD.
 */
struct pmdtime {
  u8_t sec;     /* Time, seconds */
  u8_t min;     /* Time, minutes */
  u8_t hrs;     /* Time, hours */
  u8_t day;     /* Date, Day in month */
  u8_t dow;     /* Day of week, since sunday */
  u8_t month;   /* Month */
  u16_t year;   /* Year */
  /* Control bits
   *   Bit0 - 24 or 12 hour format, 0=24 hour, 1=12 hour
   */
  u8_t ctrl;
  /*
   * Status bits
   *   Bit0 - Am or Pm indicator, 0=Am, 1=Pm
   *          This bit is don't care if 24 hour format is selected
   */
  u8_t status;
};

struct time_param {
  unsigned long b_time;
  struct pmdtime time;
};

enum RTC_CTRL {
  RTC_12_24_HOUR_FORMAT = 0x01,
};

enum RTC_STATUS_BITS {
  RTC_AM_PM_INDICATOR = 0x01,
};


struct time_client {
  struct pt pt;
  struct i2c rtc_i2c;
  rtc_data_t hw_rtc;
  unsigned long update_time;
  struct time_param tp;
  u8_t timer;
  u8_t do_update;
  u8_t retries;
};

extern struct rtc rtc;
extern struct time_client tc;

/*
 * Interface declarations
 */
void init_rtc(void) banked;
void start_rtc(void) banked;
void stop_rtc(void) banked;
void print_datetime_formated(char *buf) __reentrant banked;
void print_time_formated(char *buf) __reentrant banked;
void print_date_formated(char *buf) __reentrant banked;
u8_t day_of_week(int y, int m, int d) __reentrant;
char *day_of_week_str (u8_t day) __reentrant;


void time_appcall(void) banked;
unsigned long get_g_time(void) banked;
void set_g_time(struct time_param *tp) __reentrant banked;

void binary_to_dat(struct time_param *tp) __reentrant banked;
void dat_to_binary(struct time_param *tp) __reentrant banked;
// void translate_system_rtc (struct time_param *tp, rtc_data_t *hw_rtc) __reentrant banked;
extern PT_THREAD(handle_time_client(struct time_client *tc) __reentrant banked);
extern PT_THREAD(handle_rtc(struct rtc *rtc) __reentrant banked);

extern bit RTC_GET_TIME_EVENT;
extern struct time_param *RTC_SET_HW_RTC;

#endif // RTC_H_INCLUDED
