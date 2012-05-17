/*
* Copyright (c) 2006, Invector Embedded Technologies
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* 3. The name of the author may not be used to endorse or promote
* products derived from this software without specific prior
* written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
* OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
* GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* Author: Pontus Oldberg
*
*/

/*
* Notes on debug usage in this module.
*
* The include file iet_debug.h defines a number of (A-C) inclusive macros
* that can be used to enter debug printouts into the code. When no PRINT_X
* macros are defined during compile time no debug prints are present in the
* resulting binary.
*
* In this module I have classified the three levels of debug prints in the
* following classes:
* - A = System level printouts, important stuff that is always needed
* when debugging the system.
* - B = Informative prints, used to trace program flow etc during execution.
* - C = Informative prints. Should be used on repetetive code that spams
* the output but is sometimes useful.
*/
#include <string.h>

#include "system.h"
#include "iet_debug.h"
#include "uip.h"
#include "CUart.h"

const u8_t baudrates0[] = {0xf5};
const u8_t baudrates1[] = {0xa9};

#ifdef CONFIG_ENABLE_UART_0
/*********************************************************************************
*
* Function: uart0_init(void)
*
*********************************************************************************/
#if PUTCHAR_UART==0
void sys_uart_init(u8_t baud) __reentrant
#else
void uart0_init(u8_t baud) __reentrant
#endif
{
#if BUILD_TARGET == IET912X
  SFRPAGE = TIMER01_PAGE; // Set the correct SFR page
#endif
  if ((baud == BAUD_UNDEFINED) || (baud >= BAUD_END))
    return; // Return if the wanted baudrate is out of bounds
  TMOD |= 0x20; // TMOD: timer 1, mode 2, 8-__bit reload
  TH1 = baudrates0[baud-1]; // set Timer1 reload value for baudrate
  TR1 = 1; // start Timer1
  CKCON |= 0x10; // Timer1 uses SYSCLK as time base

  SCON0 = 0x50; // SCON0: mode 1, 8-__bit UART, enable RX
#if BUILD_TARGET == IET912X
  SSTA0 = 0x10; // Set baud rate doubler.
#else
  PCON |= 0x80; // Set baud rate doubler.
#endif
  TI0 = 1; // Indicate TX0 ready
}

/*********************************************************************************
*
* Print a character on the serial port.
* Will override dummy putchar in SDCC libraries.
*
*********************************************************************************/
void putchar0(char a) __reentrant
{
  if (a = '\n')
    putchar (0x0a);

#if BUILD_TARGET == IET912X
  SFRPAGE = UART0_PAGE; // Set the correct SFR page
#endif
  while (TI0 == 0);
  SBUF = a;
  TI0 = 0;
#if BUILD_TARGET == IET912X
  SFRPAGE = LEGACY_PAGE; // Reset to legacy SFR page
#endif
}

#if PUTCHAR_UART==0
void putchar(char a)
{
  if (a == '\n')
    putchar0 (0x0d);
  putchar0 (a);
}
#endif
#endif

#ifdef CONFIG_ENABLE_UART_1
/*********************************************************************************
*
* Function: CUart1_init(void)
*
* Initializes UART1 to do 9600 baud, The first section code only works for C8051F02X
* and uses Timer 4 as a baudrate generator.
*
* The second section of the code is for C8051F12x and uses timer1 as a baudrate
* generator. It assumes an operating frequency (SYSCLK) of 20 MHz.
*
*********************************************************************************/
#if PUTCHAR_UART==1
void sys_uart_init(u8_t baud) __reentrant
#else
void uart1_init(u8_t baud) __reentrant
#endif
{
  if ((baud == BAUD_UNDEFINED) || (baud >= BAUD_END))
    return; // Return if the wanted baudrate is out of bounds
#if BUILD_TARGET == IET902X
  /* Mode 1, Check Stop __bit */
  SCON1 = 0x50;

  /* Set timer4 to do 9600 baud for UART1 */
  T4CON = 0x30;
  RCAP4 = -(SYSCLK / 9600 / 32);
  TMR4 = RCAP4;

  CKCON |= 0x40;
  T4CON |= 0x04;
  PCON |= 0x10;

  /* Enable UART1 interrupts */
  EIE2 |= 0x40;
#else
  /* Setup timer one */
  SFRPAGE = TIMER01_PAGE;
  /* Timer 1 mode2 */
  TMOD    |= 0x20;
  /* set Timer1 reload value for baudrate */
  TH1     = baudrates1[baud-1];
  /* Timer 1 uses sysclk */
  CKCON   |= 0x10;
  /* Start Timer 1 */
  TR1     = 1;

  /* Setup UART1 as an 8 __bit UART */
  SFRPAGE = UART1_PAGE;
  SCON1   = 0x50;
  TI1     = 1;

/* Set interrupts below this point */
#endif

}

/*********************************************************************************
*
* Function: putchar_uart1
*
* Prints a character on uart 1
*
*********************************************************************************/
void putchar1(char chr)
{
  SFRPAGE = UART1_PAGE;
  /* First wait for the uart to be done */
  while (!TI1);
  TI1   = 0;
  SBUF1 = chr;
}

#if PUTCHAR_UART==1
void putchar(char a)
{
  if (a == '\n')
    putchar1 (0x0d);
  putchar1 (a);
}
#endif

#ifdef IET_CONFIG_UART1_INTERRUPT_ENABLE
/*********************************************************************************
*
* Function: UART1_ISR
*
* UART1 __interrupt handler
*
* FIXME: Rewrite to a more generic ISR
*
*********************************************************************************/
void UART1_ISR (void) __interrupt UART1_VECTOR
{

}
#endif
#endif

// EOF
