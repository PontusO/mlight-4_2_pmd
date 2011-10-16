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
 * Change History:
 *    2006-03-20 PO Introduced the possibility to build for different
 *                  targets.
 */

/*
 * This file contains system defintions used together with the uIP
 * stack. These defininitions are for the platform and application
 * specifics and do not affect uIP. uIP specific definitions are as
 * usual found in uipopt.h.
 *
 */

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

// #include "bank.h"
#include "uipopt.h"
#include "uip_arp.h"
#include "uip-split.h"

/*
 * Some os-light signal macros
 */
#define SEND(x)   x = 1
/*
 * Definitions of FLASH memory banks
 */
#define CONST_BANK  1
#define UIP_BANK    2
#define APP_BANK    3

/*
 * BUILD_TARGET defines what target system the software will be built
 * for. Currently two systems are valid, IET902X and IET912X. They
 * represent the corresponding Invector systems.
 */

#define IET902X   1   // IP Avenger
#define IET912X   2   // IP Avenger II
// Change below to alter target system
#define BUILD_TARGET  IET912X

/*
 * The IP Avenger targets can use both the internal oscillator and the
 * external 20MHz clock signal fed from the ethernet controller.
 * Make sure you set USE_EXTERNAL_CLOCK to use the external 20 MHz oscillator.
 */
#define USE_EXTERNAL_CLOCK

/*
 * Make sure the correct include file is included in the build.
 */
#if BUILD_TARGET == IET902X
#include <c8051f020.h>
#elif BUILD_TARGET == IET912X
#include <c8051f120.h>
#else
#error Incorrect Build Target specified
#endif

#define   SYSCLK          20000000

#define   UIP_ARP_TIMER   1000    // Gives 0,01 * 1000 = 10 sec interval
#define   UIP_TX_TIMER    50      // Gives 0,01 * 50 = 0,5 sec interval
#define   LED_TIMER       50     // (2 Hz blink)

#define   BUTTON_QUICK_PRESS_TIMER   20 // 20 *10ms = 0.2 s

/*
 * Macros used for controlling hardware
 */

#define ENTER_CRITICAL_SECTION  EA = 0
#define EXIT_CRITICAL_SECTION EA = 1

#define INTERRUPT_OFF   0
#define INTERRUPT_ON    1

/* Define the UART to be used for system prints */
#define PUTCHAR_UART    1
#define CONFIG_ENABLE_UART_1
#undef CONFIG_ENABLE_UART_0
void putchar(char n);

/*
 * Enable this flag if the system needs I2C functionality without
 * pthread support.
 */
#undef CONFIG_ENABLE_NOS_I2C

/*
 * Some generic macros.
 */
#ifndef NULL
#define NULL (void*)0
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#define IE0_VECTOR		0	  // 0x03 external interrupt 0
#define TF0_VECTOR		1	  // 0x0b timer 0 overflow
#define IE1_VECTOR		2	  // 0x13 external interrupt 1
#define TF1_VECTOR		3	  // 0x1b timer 1 overflow
#define SI0_VECTOR		4	  // 0x23 serial port 0
#define TF2_VECTOR		5	  // 0x2B timer 2 overflow
#define SPIO_VECTOR		6	  // 0x33 SPIO Hardware
#define SMB0_VECTOR		7	  // 0x3b SPIO Hardware
#define AD0WINT_VECT	8	  // 0x43 ADC0 Window Compare
#define PCA_VECTOR		9	  // 0x4b Programmable Counter Array
#define CP0FIF_VECTOR	10  // 0x53 Comparator 0 Falling Edge
#define CP0RIF_VECTOR	11  // 0x5b Comparator 0 Rising Edge
#define CP1FIF_VECTOR	12  // 0x63 Comparator 1 Falling Edge
#define CP1RIF_VECTOR	13  // 0x6b Comparator 1 Rising Edge
#define TF3_VECTOR    14  // 0x73 T3 overflow
#define AD0INT_VECTOR 15  // 0x7b ADC0 Conversion Complete
#define TF4_VECTOR    16  // 0x83 T4 overflow
#define AD1INT_VECTOR 17  // 0x8b ADC1 End of Conversion
#define IE6_VECTOR    18  // 0x93 External Interrupt 6
#define IE7_VECTOR		19  // 0x9B External Interrupt 7
#define UART1_VECTOR	20  // 0xA3 serial port 1
#define XTLVLD_VECTOR	21  // 0xAB External Crystal OSC Ready
#define TF3H 0x80			    // Timer 3 high byte overflow flag

#endif // SYSTEM_DEF_GUARD
