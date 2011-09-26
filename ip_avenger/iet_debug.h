/*
 * Copyright (c) 2006, Invector Embedded Technologies
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
 * Author:    Pontus Oldberg
 *
 */

/*
 *
 * iet_debug.h
 *
 * Debug macros
 *
 * This file contains usefull debug macros for debuging via printf statements.
 * Two levels of debug prints are available: A_ and B_. A_ should be used for
 * information to the user and B_ should be used when reporting an error.
 *
 * To enable the macro's define either the PRINT_A flag, the PRINT_B flag or
 * both to enable both levels of debug output.
 *
 */

#ifndef IET_DEBUG_GUARD
#define IET_DEBUG_GUARD

#include <stdio.h>

#define dprintf(bank, str, ...)             \
  do {                                      \
    u8_t xxxp_temp;                         \
    xxxp_temp = PSBANK;                     \
    PSBANK = (PSBANK & 0xcf) | (bank << 4); \
    printf(str, ##__VA_ARGS__);             \
    PSBANK = xxxp_temp;                     \
  } while (0)

// Some combinations of debug prints.
#ifdef PRINT_ALL
  #define PRINT_A
  #define PRINT_B
  #define PRINT_C
#endif

#ifdef PRINT_BC
  #define PRINT_B
  #define PRINT_C
#endif

#ifdef PRINT_AB
  #define PRINT_A
  #define PRINT_B
#endif

#ifdef PRINT_A
    #define A_(n) n
#else
    #define A_(n)
#endif

#ifdef PRINT_B
    #define B_(n) n
#else
    #define B_(n)
#endif

#ifdef PRINT_C
    #define C_(n) n
#else
    #define C_(n)
#endif

#endif // IET_DEBUG_GUARD
// EOF
