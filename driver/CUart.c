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

const u8_t baudrates[] = {0xf5};
static char tx_busy; /* TX Busy flag */
static char cnt;
char sonar_str[6];
bit RX_sonar;

/*********************************************************************************
*
* Function: CUart_init(void)
*
*********************************************************************************/
void CUart_init(u8_t baud) __reentrant
{
#if BUILD_TARGET == IET912X
  SFRPAGE = TIMER01_PAGE; // Set the correct SFR page
#endif
  if ((baud == BAUD_UNDEFINED) || (baud > BAUD_END))
    return; // Return if the wanted baudrate is out of bounds
  TMOD |= 0x20; // TMOD: timer 1, mode 2, 8-bit reload
  TH1 = baudrates[baud-1]; // set Timer1 reload value for baudrate
  TR1 = 1; // start Timer1
  CKCON |= 0x10; // Timer1 uses SYSCLK as time base

  SCON0 = 0x50; // SCON0: mode 1, 8-bit UART, enable RX
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
void putchar(char a)
{
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

#ifdef CONFIG_ENABLE_UART_1
/*********************************************************************************
*
* Function: CUart1_init(void)
*
* Initializes UART1 to do 9600 baud, The first section code only works for C8051F02X
* and uses Timer 4 as a baudrate generator.
*
* The second section of the code is for C8051F12x and uses timer1 as a baudrate
* generator. It assumes an operating frequency (SYSCLK) of 24.5 MHz.
*
*********************************************************************************/
void UART1_init(void) __reentrant
{
#if BUILD_TARGET == IET902X
  /* Mode 1, Check Stop Bit */
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

  tx_busy = 0;
  cnt = 0;
#else
  /* Setup UART1 as an 8 bit UART */
  SFRPAGE = UART1_PAGE;
  SCON1 = 0x50;
  TI1 = 1;

  /* Setup timer one to do 115200 baud */
  SFRPAGE = TIMER01_PAGE;
  TMOD = 0x20;
  CKCON = 0x10;
  TH1 = 0x96;
  TR1 = 1;

/* Set interrupts below this point */
#endif
}

/*********************************************************************************
*
* Function: UART1_ISR
*
* UART1 interrupt handler
*
* FIXME: Rewrite to a more generic ISR
*
*********************************************************************************/
void UART1_ISR (void) __interrupt UART1_VECTOR
{
  char sd;

  if (SCON1 & RI1) {
    SCON1 &= ~RI1;
    sd = SBUF1;
    /* For syncronization */
    if (sd == 'R')
      cnt = 0;
    /* translate 0x0d */
    if (sd == '\n')
      sd = 0;
    /* Store in array */
    sonar_str[cnt] = sd;
    /* Check for wrap */
    if (++cnt == 5) {
      cnt = 0;
      RX_sonar = 1;
    }
  } else if (SCON1 & TI1) {
    tx_busy = 0;
    SCON1 &= ~TI1;
  }
}

/*********************************************************************************
*
* Function: putchar_uart1
*
* Prints a character on uart 1
*
*********************************************************************************/
void putchar_uart1(char chr)
{
  /* First wait for the tx_busy flag to be cleared by the interrupt */
  while (tx_busy);
  /* Send the data */
  SBUF1 = chr;
  /* Indicate that a tx is in progress */
  tx_busy = 1;
  /* Do we need to do something more ? */
}

/*********************************************************************************
*
* Function: string1
*
* Prints a given string to uart 1
*
*********************************************************************************/
u8_t put_string1(char *ptr)
{
  u8_t n = 0;

  while (*ptr != '\0') {
    n++;
    putchar_uart1(*ptr++);
  }
  return n;
}

#endif
// EOF
