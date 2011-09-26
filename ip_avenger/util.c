#ifdef CONFIG_BANKED
#pragma codeseg  UIP_BANK
#endif
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

#include "util.h"
#include "Httpd.h"

/* convert two hex nibbles to 8-bit value */
u8_t hexstr2value(u8_t  *str) banked
{
   u8_t val;

   /* first char */
   if(('a'<= *str) && (*str <='z'))
   	*str -= 0x20;

   if(('0'<= *str) && (*str <='9'))
   	val = *str - '0';
   else
   	val = *str - 'A' + 10;

   val <<= 4;

   str++; /* second char */
   if(('a'<= *str) && (*str <='z'))
   	*str -= 0x20;

   if(('0'<= *str) && (*str <='9'))
   	val += (*str - '0');
   else
   	val += (*str - 'A' + 10);

   return val;
}

/* convert a decimal string to one-byte value.
return the number of digitals. */
u8_t decimal2byte(u8_t  *str, u8_t *val) banked
{
   u8_t i;
   u8_t negative=0;

   if(*str == ISO_minus)
   {
      negative=1;
      str++;
   }
   *val = *str - '0';
   str++;
   for(i=0; i<2; i++)  /* 3 digitals at most */
   {
      if(('0' <= *str) && (*str <= '9'))
      {
         (*val) *= 10;
	  (*val) += (*str - '0');
	  str++;
      }
      else
	  break;
   }
   if(negative)
   	*val = 0- (*val);;

   return (i+1+negative);
}

/* convert a decimal string to 16-bit positive integer */
void decimal2word(u8_t  *str, u16_t *val) banked
{
   u8_t i;

   *val = *str - '0';
   str++;
   for(i=0; i<4; i++)   /* 5 digitals at most */
   {
      if(('0'<=*str) && (*str <= '9'))
      {
         (*val)*=10;
         (*val) += (*str - '0');
         str++;
      	}
	else
	   break;
   }
}

/* search a value in an array */
u8_t search_value(u8_t *str, u8_t value, u8_t len) banked
{
   u8_t i;
   for(i=0; i<len; i++, str++)
   {
      if(*str == value)
	  return i;
   }
   return len;  /* return the next position behind the tail of the string */
}

/* parse mac address */
void parse_mac(u8_t * buf, u8_t *str) banked
{
   u8_t i;
   for(i=0; i<6; i++)
   {
      buf[i]=hexstr2value(str);
      str+=5;
   }
}

/* URL string copy, additional \0 will be added to the end of the destination string */
void url_str_cpy(u8_t *dest, u8_t *src, u16_t len) banked
{
   u16_t i;
   for(i=0; i<len; i++)
   {
      if(*src == ISO_plus )   /* '+' used to join to strings , convert it to space */
         *dest = ISO_space; /* convert '+' to space */
      else if(*src == ISO_percent)
      {
         *dest = hexstr2value(src+1); /* convert '%xx' to one byte */
         src+=2;  /* eat two more chars */
	  len-=2;
      }
      else
	  *dest = *src;

      dest++;
      src++;
   }
   *dest = 0;  /* add \0 to the end of the string*/
}

//-----------------------------------------------------------------------------
// Non-interrupt delay approx 1mS to 255mS
//-----------------------------------------------------------------------------
void wait_ms(u8_t count) banked
{
  u16_t i;

  for ( ; count > 0; count--) {
		for ( i = 10000; i > 0; i--);
	}
}

void wait_us(u8_t count) banked
{
   u8_t i;
    for ( ; count > 0; count--) {
		for ( i = 10; i>0; i--);
    	}

}

