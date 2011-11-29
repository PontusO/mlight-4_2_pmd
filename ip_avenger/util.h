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

#ifndef _UTIL_H
#define _UTIL_H
#include "Uipopt.h"

/* some utilities used by the system */

/* convert two hex nibbles to 8-__bit value */
u8_t hexstr2value(u8_t  *str) __banked;
/* convert a decimal string to one-byte value */
u8_t decimal2byte(u8_t  *str, u8_t *val) __banked;
/* convert a decimal string to 16-__bit positive integer */
void decimal2word(u8_t  *str, u16_t *val) __banked;
/* search a value in an array */
u8_t search_value(u8_t *str, u8_t value, u8_t len) __banked;
/* parse ip address */
void parse_ip(u8_t *buf, u8_t *str) __banked;
/* parse mac address */
void parse_mac(u8_t * buf, u8_t * str) __banked;
/* URL string copy, additional \0 will be added to the destination string */
void url_str_cpy(u8_t *dest, u8_t *src, u16_t len) __banked;
/* delay ms*/
void wait_ms(u8_t count) __banked;
/* delay us */
void wait_us(u8_t count) __banked;

#define ISO_minus   '-'
#define ISO_plus    '+'
#define ISO_space   ' '
#define ISO_percent '%'

#endif
