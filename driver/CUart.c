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
 * Notes on debug usage in this module.
 *
 *  The include file iet_debug.h defines a number of (A-C) inclusive macros
 *  that can be used to enter debug printouts into the code. When no PRINT_X
 *  macros are defined during compile time no debug prints are present in the
 *  resulting binary.
 *
 *  In this module I have classified the three levels of debug prints in the
 *  following classes:
 *    - A = System level printouts, important stuff that is always needed
 *          when debugging the system.
 *    - B = Informative prints, used to trace program flow etc during execution.
 *    - C = Informative prints. Should be used on repetetive code that spams
 *          the output but is sometimes useful.
 */

#include "system.h"
#include "CUart.h"

/*********************************************************************************
 *
 * Function: CUart_init(void)
 *
 *********************************************************************************/
void init_uart(void) __reentrant
{
#ifdef USE_UART_INSTEAD_OF_SMB
  SFRPAGE   = TMR2_PAGE;
  TMR2CN    = 0x04;
  TMR2CF    = 0x08;
  RCAP2L    = 0xF5;
  RCAP2H    = 0xFF;
  TR2 = 1;

  SFRPAGE   = UART0_PAGE;
  SCON0     = 0x50;
  SSTA0     = 0x05;

  TI0 = 1;
  SFRPAGE = LEGACY_PAGE;                // Reset to legacy SFR page
#endif
}

/*********************************************************************************
 *
 * Print a character on the serial port.
 *    Will override dummy putchar in SDCC libraries.
 *
 *********************************************************************************/
void putchar(char a)
{
#ifdef USE_UART_INSTEAD_OF_SMB
  SFRPAGE = UART0_PAGE;                 // Set the correct SFR page
  while (TI0 == 0);
  SBUF = a;
  TI0 = 0;
  SFRPAGE = LEGACY_PAGE;                // Reset to legacy SFR page
#else
  IDENTIFIER_NOT_USED(a);
#endif
}


// EOF
